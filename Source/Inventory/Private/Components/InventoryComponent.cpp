// Copyright Telephone Studios. All Rights Reserved.


#include "Components/InventoryComponent.h"
#include "Components/InventoryManipulatorComponent.h"
#include "Blueprint/UserWidget.h"
#include "WorldItem.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Log.h"

static UDataTable* ItemDataTable = nullptr;
static TMap<FName, FItemData> RegisteredItemData;
static TMap<FName, FName> RegisteredItemTemplateRow;
static bool bPhase1ItemsSeeded = false;
static bool bPhase1ItemsLinkedToDataTable = false;

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);

	DisplayName = TEXT("Inventory");

	SlotCount = 24;
	LoadedFromSave = false;

	if (GetWorld())
	{
		static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableBlueprint(TEXT("/Game/Inventory/DataTables/DT_Items"));
		if (ItemDataTableBlueprint.Succeeded())
		{
			ItemDataTable = ItemDataTableBlueprint.Object;
		}
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultWidgetBlueprint(TEXT("/Game/Deployables/UI/WBP_ContainerBase"));
	if (DefaultWidgetBlueprint.Succeeded())
	{
		WidgetClass = DefaultWidgetBlueprint.Class;
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UInventoryComponent, ServerState, COND_OwnerOnly);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority() || ServerState.Slots.Num() == SlotCount)
	{
		return;
	}

	// Initialize slots
	ServerState.Slots.SetNum(SlotCount, EAllowShrinking::No);
	for (int32 i = 0; i < SlotCount; ++i)
	{
		ServerState.Slots[i].Index = i;
	}

	LoadedFromSave = false;

	OnRep_ServerState();
}

void UInventoryComponent::AddItem(const FInventoryItem& ItemToAdd, bool FromHarvesting, AActor* ActorToSpawnDropedItems, bool ForceClientUpdate)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return;
	}

	const bool UseServerState = OwningActor->HasAuthority();

	if (UseServerState && ServerState.Slots.Num() < SlotCount)
	{
		ServerState.Slots.SetNum(SlotCount, EAllowShrinking::No);
		for (int32 i = 0; i < SlotCount; ++i)
		{
			ServerState.Slots[i].Index = i;
		}
	}

	// Create the Item Update
	if (UseServerState)
	{
		FInventoryItemUpdate AdditionItemUpdate;
		AdditionItemUpdate.Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
		AdditionItemUpdate.Addition = true;
		AdditionItemUpdate.Item = ItemToAdd;
		ServerState.Updates.Add(AdditionItemUpdate);
	}
	
	// The amount of items we were unable to add
	int32 Remaining;
	
	// The amount of items we successfully added
	int32 AmountAdded;
	
	bool AddSuccess = AddItemToSlots(ItemToAdd, Remaining);

	// Calculate how many were added and add that amount to our contents
	AmountAdded = ItemToAdd.Quantity - Remaining;
	UseServerState ? ServerState.Contents.AddItem(ItemToAdd.Name, AmountAdded) : Contents.AddItem(ItemToAdd.Name, AmountAdded);

	if (FromHarvesting && OnItemHarvested.IsBound())
	{
		FInventoryItem ItemHarvested;
		ItemHarvested.Name = ItemToAdd.Name;
		ItemHarvested.Quantity = AmountAdded;
		OnItemHarvested.Broadcast(ItemHarvested);
	}

	if (UseServerState == true && AddSuccess == false)
	{
		FInventoryItem DroppedItem = ItemToAdd;
		DroppedItem.Quantity = Remaining;

		FInventoryItemUpdate DroppedItemUpdate;
		DroppedItemUpdate.Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
		DroppedItemUpdate.Addition = false;
		DroppedItemUpdate.Item = DroppedItem;
		ServerState.Updates.Add(DroppedItemUpdate);

		AActor* DropperActor = nullptr;
		if (ActorToSpawnDropedItems == nullptr)
		{
			DropperActor = GetOwner();
		}
		else
		{
			DropperActor = ActorToSpawnDropedItems;
		}
		SpawnWorldItem(GetWorld(), DroppedItem, DropperActor);
	}
	
	if (ForceClientUpdate)
	{
		GetOwner()->ForceNetUpdate();
	}

	OnRep_ServerState();
}

void UInventoryComponent::RemoveItem(const FInventoryItem& ItemToRemove)
{
	int32 Remaining;
	int32 AmountSuccessfullyRemoved;
	
	RemoveItemFromSlots(ItemToRemove.Name, ItemToRemove.Quantity, Remaining);
	AmountSuccessfullyRemoved = ItemToRemove.Quantity - Remaining;

	// Remove from contents
	ServerState.Contents.RemoveItem(ItemToRemove.Name, AmountSuccessfullyRemoved);

	// Call inventory change
	OnRep_ServerState();
}

void UInventoryComponent::SlotInteraction(const int32& SlotIndex, UInventoryManipulatorComponent* Manipulator, bool QuickMove, bool Primary)
{
	// Create an interaction
	FInventorySlotInteraction CurrentInteraction;
	CurrentInteraction.SlotIndex = SlotIndex;
	CurrentInteraction.Manipulator = Manipulator;
	CurrentInteraction.QuickMove = QuickMove;
	CurrentInteraction.Primary = Primary;
	CurrentInteraction.Time = GetWorld()->GetRealTimeSeconds();

	// Send it to the server
	Server_SlotInteraction(CurrentInteraction);

	// Add it to our unacknowlaged interactions
	UnacknowalgedInteractions.Add(CurrentInteraction);

	if (GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentInteraction.QuickMove ? HandleItemQuickMove(CurrentInteraction) : HandleItemSelection(CurrentInteraction);

	BroadcastInventoryUpdate();
}

void UInventoryComponent::EnsureDefaultPhase1ItemsRegistered()
{
	if (!bPhase1ItemsSeeded)
	{
		bPhase1ItemsSeeded = true;

		auto AddItemDef = [](const FName& ID, const TCHAR* Display, EItemCategory Cat, int32 Stack, const TCHAR* Desc, const FName& TemplateRow, const TArray<TPair<FName, int32>>& DefaultStats = TArray<TPair<FName, int32>>())
		{
			FItemData Data;
			Data.DisplayName = Display;
			Data.Category = Cat;
			Data.StackSize = Stack;
			Data.Description = Desc;
			for (const TPair<FName, int32>& Pair : DefaultStats)
			{
				FItemStat Stat;
				Stat.Name = Pair.Key;
				Stat.Value = Pair.Value;
				Data.Stats.Add(Stat);
			}
			RegisteredItemData.Add(ID, Data);
			RegisteredItemTemplateRow.Add(ID, TemplateRow);
		};

		// Raw & refined resources
		AddItemDef(TEXT("Wood"), TEXT("Wood"), EItemCategory::Resource, 1000, TEXT("Raw timber gathered from trees and log piles."), TEXT("wood"));
		AddItemDef(TEXT("Stone"), TEXT("Stone"), EItemCategory::Resource, 1000, TEXT("Raw stone gathered from rock nodes."), TEXT("stone"));
		AddItemDef(TEXT("MetalOre"), TEXT("Metal Ore"), EItemCategory::Resource, 1000, TEXT("Unrefined iron-rich ore; smelt in a furnace for Metal Fragments."), TEXT("ore.metal"));
		AddItemDef(TEXT("MetalFragments"), TEXT("Metal Fragments"), EItemCategory::Resource, 1000, TEXT("Smelted metal fragments used for crafting and Sheet Metal upgrades."), TEXT("metal"));
		AddItemDef(TEXT("HighQualityMetalOre"), TEXT("High-Quality Metal Ore"), EItemCategory::Resource, 100, TEXT("Rare high-grade ore; smelt in a furnace for High-Quality Metal."), TEXT("ore.metal"));
		AddItemDef(TEXT("HighQualityMetal"), TEXT("High-Quality Metal"), EItemCategory::Resource, 100, TEXT("Refined high-grade alloy used for Armored structures and advanced weapons."), TEXT("metal.refined"));
		AddItemDef(TEXT("SulfurOre"), TEXT("Sulfur Ore"), EItemCategory::Resource, 1000, TEXT("Crystalline yellow ore; smelt in a furnace for Sulfur."), TEXT("ore.sulfur"));
		AddItemDef(TEXT("Sulfur"), TEXT("Sulfur"), EItemCategory::Resource, 1000, TEXT("Refined sulfur; combine with Charcoal to craft Gunpowder."), TEXT("sulfur"));
		AddItemDef(TEXT("Charcoal"), TEXT("Charcoal"), EItemCategory::Resource, 1000, TEXT("Byproduct of burning wood in campfires and furnaces."), TEXT("charcoal"));
		AddItemDef(TEXT("AnimalFat"), TEXT("Animal Fat"), EItemCategory::Resource, 500, TEXT("Rendered fat harvested from animals; combine with Cloth for Low Grade Fuel."), TEXT("cloth"));
		AddItemDef(TEXT("BoneFragments"), TEXT("Bone Fragments"), EItemCategory::Resource, 1000, TEXT("Harvested from animal carcasses; used for Bone Arrows and primitive tools."), TEXT("stone"));
		AddItemDef(TEXT("Cloth"), TEXT("Cloth"), EItemCategory::Resource, 1000, TEXT("Woven plant or animal fiber used in bows, clothing, and fuel."), TEXT("cloth"));
		AddItemDef(TEXT("LowGradeFuel"), TEXT("Low Grade Fuel"), EItemCategory::Resource, 500, TEXT("Combustible fuel crafted from Animal Fat and Cloth."), TEXT("water.bottle"));
		AddItemDef(TEXT("Gunpowder"), TEXT("Gunpowder"), EItemCategory::Resource, 1000, TEXT("Propellant crafted from Sulfur and Charcoal."), TEXT("gunpowder"));

		// Tiered Tools
		AddItemDef(TEXT("StoneHatchet"), TEXT("Stone Hatchet"), EItemCategory::Tool, 1, TEXT("Tier 1 stone hatchet for chopping wood."), TEXT("hatchet.stone"), { { TEXT("Durability"), 100 } });
		AddItemDef(TEXT("StonePickaxe"), TEXT("Stone Pickaxe"), EItemCategory::Tool, 1, TEXT("Tier 1 stone pickaxe for mining stone, metal, and sulfur nodes."), TEXT("pickaxe.stone"), { { TEXT("Durability"), 100 } });
		AddItemDef(TEXT("MetalHatchet"), TEXT("Metal Hatchet"), EItemCategory::Tool, 1, TEXT("Tier 2 metal hatchet with 1.5x wood harvesting yield."), TEXT("hatchet"), { { TEXT("Durability"), 200 } });
		AddItemDef(TEXT("MetalPickaxe"), TEXT("Metal Pickaxe"), EItemCategory::Tool, 1, TEXT("Tier 2 metal pickaxe with 1.5x ore yield and HQM extraction capability."), TEXT("pickaxe"), { { TEXT("Durability"), 200 } });

		// Primitive & Military Weapons
		AddItemDef(TEXT("WoodenSpear"), TEXT("Wooden Spear"), EItemCategory::Weapon, 1, TEXT("Sharpened long-reach wooden spear."), TEXT("sword.wooden"), { { TEXT("Durability"), 60 } });
		AddItemDef(TEXT("StoneSpear"), TEXT("Stone Spear"), EItemCategory::Weapon, 1, TEXT("Stone-tipped spear with increased melee damage and reach."), TEXT("machete"), { { TEXT("Durability"), 90 } });
		AddItemDef(TEXT("HuntingBow"), TEXT("Hunting Bow"), EItemCategory::Weapon, 1, TEXT("Primitive hunting bow firing Wooden or Bone Arrows."), TEXT("bow.recurve"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 1 }, { TEXT("Durability"), 100 } });
		AddItemDef(TEXT("Crossbow"), TEXT("Crossbow"), EItemCategory::Weapon, 1, TEXT("High-velocity primitive crossbow."), TEXT("bow.recurve"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 1 }, { TEXT("Durability"), 150 } });
		AddItemDef(TEXT("HandmadeRevolver"), TEXT("Handmade Revolver"), EItemCategory::Weapon, 1, TEXT("Six-shot primitive revolver chambered in Pistol Cartridges."), TEXT("pistol.semiauto"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 6 }, { TEXT("Durability"), 150 } });
		AddItemDef(TEXT("CustomSMG"), TEXT("Custom SMG"), EItemCategory::Weapon, 1, TEXT("High-rate-of-fire 24-round submachine gun chambered in Pistol Cartridges."), TEXT("pistol.semiauto"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 24 }, { TEXT("Durability"), 250 } });
		AddItemDef(TEXT("CombatShotgun"), TEXT("Spas-12 Combat Shotgun"), EItemCategory::Weapon, 1, TEXT("Semi-automatic 8-round tactical shotgun firing 12-Gauge Buckshot and Slugs."), TEXT("shotgun.pump"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 8 }, { TEXT("Durability"), 280 } });
		AddItemDef(TEXT("BoltActionRifle"), TEXT("Bolt-Action Rifle"), EItemCategory::Weapon, 1, TEXT("High-velocity long-range precision rifle chambered in 5.56 Rifle Cartridges."), TEXT("rifle.semiauto"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 4 }, { TEXT("Durability"), 300 } });
		AddItemDef(TEXT("AssaultRifle"), TEXT("Assault Rifle (AK-47)"), EItemCategory::Weapon, 1, TEXT("Automatic 5.56 rifle with 30-round magazine and S-curve recoil profile."), TEXT("rifle.assult"), { { TEXT("CurrentAmmo"), 0 }, { TEXT("MaxAmmo"), 30 }, { TEXT("Durability"), 300 } });
		AddItemDef(TEXT("SatchelCharge"), TEXT("Satchel Charge"), EItemCategory::Weapon, 10, TEXT("Improvised bundle of 4 Bean Can Grenades dealing 220 structural raid damage."), TEXT("ammo.rocket.basic"));
		AddItemDef(TEXT("TimedExplosiveCharge"), TEXT("Timed Explosive Charge (C4)"), EItemCategory::Weapon, 10, TEXT("10-second timed demolition charge dealing 550 structural damage."), TEXT("ammo.rocket.basic"));
		AddItemDef(TEXT("FragGrenade"), TEXT("F1 Fragmentation Grenade"), EItemCategory::Weapon, 5, TEXT("Timed fragmentation grenade used by Scientist squads and players to flush targets from cover."), TEXT("can.tuna"));

		// Ammo Types
		AddItemDef(TEXT("WoodenArrow"), TEXT("Wooden Arrow"), EItemCategory::Ammo, 64, TEXT("Standard wooden arrow for bows and crossbows."), TEXT("arrow.wooden"), { { TEXT("Damage"), 35 }, { TEXT("Velocity"), 4500 } });
		AddItemDef(TEXT("BoneArrow"), TEXT("Bone Arrow"), EItemCategory::Ammo, 64, TEXT("Heavy bone-tipped arrow with wider hit radius and higher impact damage."), TEXT("arrow.wooden"), { { TEXT("Damage"), 42 }, { TEXT("Velocity"), 3900 } });
		AddItemDef(TEXT("HighVelocityArrow"), TEXT("High-Velocity Arrow"), EItemCategory::Ammo, 64, TEXT("Lightweight aerodynamic shaft with flatter trajectory and +35% flight velocity."), TEXT("arrow.wooden"), { { TEXT("Damage"), 32 }, { TEXT("Velocity"), 6200 } });
		AddItemDef(TEXT("PistolBullet"), TEXT("Pistol Cartridge (9mm)"), EItemCategory::Ammo, 128, TEXT("Tier 1 pistol cartridge for revolvers and SMGs."), TEXT("ammo.pistol"), { { TEXT("Damage"), 38 }, { TEXT("Velocity"), 12000 } });
		AddItemDef(TEXT("HighVelocityPistolBullet"), TEXT("HV Pistol Cartridge"), EItemCategory::Ammo, 128, TEXT("Overpressured 9mm cartridge with higher muzzle velocity and extended effective range."), TEXT("ammo.pistol"), { { TEXT("Damage"), 42 }, { TEXT("Velocity"), 16000 } });
		AddItemDef(TEXT("RifleBullet"), TEXT("5.56 Rifle Cartridge"), EItemCategory::Ammo, 128, TEXT("Tier 2 high-velocity rifle cartridge."), TEXT("ammo.rifle"), { { TEXT("Damage"), 55 }, { TEXT("Velocity"), 22000 } });
		AddItemDef(TEXT("HighVelocityRifleBullet"), TEXT("HV 5.56 Rifle Cartridge"), EItemCategory::Ammo, 128, TEXT("Match-grade high-velocity 5.56 sniper round for long-range engagements."), TEXT("ammo.rifle"), { { TEXT("Damage"), 62 }, { TEXT("Velocity"), 28500 } });
		AddItemDef(TEXT("ExplosiveRifleBullet"), TEXT("Explosive 5.56 Ammo"), EItemCategory::Ammo, 128, TEXT("Tier 3 anti-materiel 5.56 round that detonates on impact, damaging structures and doors."), TEXT("ammo.rifle"), { { TEXT("Damage"), 68 }, { TEXT("Velocity"), 19500 } });
		AddItemDef(TEXT("TwelveGaugeBuckshot"), TEXT("12-Gauge Buckshot"), EItemCategory::Ammo, 64, TEXT("Standard 12-gauge shell loaded with lethal close-quarters lead pellets."), TEXT("ammo.shotgun"), { { TEXT("Damage"), 90 }, { TEXT("Velocity"), 9500 } });
		AddItemDef(TEXT("TwelveGaugeSlug"), TEXT("12-Gauge Rifled Slug"), EItemCategory::Ammo, 64, TEXT("Heavy single-projectile 12-gauge slug for medium-range precision impact."), TEXT("ammo.shotgun"), { { TEXT("Damage"), 85 }, { TEXT("Velocity"), 14500 } });
		AddItemDef(TEXT("Rocket"), TEXT("High-Velocity Raid Rocket"), EItemCategory::Ammo, 6, TEXT("Rocket warhead dealing 275 splash damage to fortified structures."), TEXT("ammo.rocket.basic"));
		AddItemDef(TEXT("IncendiaryRocket"), TEXT("Incendiary Rocket"), EItemCategory::Ammo, 6, TEXT("Napalm-filled rocket warhead that blankets impact zones in sustained fire."), TEXT("ammo.rocket.basic"));

		// Phase 2: Industrial Resources, Storage Barrels, Workbenches, Deployables, Electricity, Lighting & Traps
		AddItemDef(TEXT("Scrap"), TEXT("Scrap"), EItemCategory::Resource, 1000, TEXT("Salvaged industrial scrap used to build Workbenches and unlock Blueprints at a Research Table."), TEXT("metal"));
		AddItemDef(TEXT("CrudeOil"), TEXT("Crude Oil"), EItemCategory::Resource, 500, TEXT("Unrefined petroleum found in barrels; refine in an Oil Refinery into Low Grade Fuel."), TEXT("can.beans"));
		AddItemDef(TEXT("Explosives"), TEXT("Military Explosives"), EItemCategory::Resource, 100, TEXT("Refined military-grade explosive compound used in Timed Explosive Charges and Rockets."), TEXT("gunpowder"));

		// Storage Barrels & Containers
		AddItemDef(TEXT("StorageBarrel"), TEXT("Rust Storage Barrel (24 Slots)"), EItemCategory::Building, 1, TEXT("Vertical salvaged steel drum barrel for compact 24-slot base storage."), TEXT("storage.box.small"));
		AddItemDef(TEXT("FuelBarrel"), TEXT("Oil & Fuel Storage Drum (30 Slots)"), EItemCategory::Building, 1, TEXT("Reinforced industrial drum barrel for storing Crude Oil, Low Grade Fuel, and ores."), TEXT("storage.box.large"));
		AddItemDef(TEXT("LargeStorageBox"), TEXT("Large Wooden Box (30 Slots)"), EItemCategory::Building, 1, TEXT("Reinforced 30-slot wooden storage crate."), TEXT("storage.box.large"));
		AddItemDef(TEXT("ArmoredLocker"), TEXT("Armored Gear Locker (42 Slots)"), EItemCategory::Building, 1, TEXT("High-capacity sheet-metal locker for storing weapon and armor loadouts."), TEXT("fridge"));

		// Workbenches, Locks & Furnaces
		AddItemDef(TEXT("WorkbenchT1"), TEXT("Workbench Level 1"), EItemCategory::Building, 1, TEXT("Tier 1 Workbench enabling early metal tools, primitive firearms, and industrial deployables."), TEXT("table"));
		AddItemDef(TEXT("WorkbenchT2"), TEXT("Workbench Level 2"), EItemCategory::Building, 1, TEXT("Tier 2 Workbench enabling military-grade ammunition, Large Furnaces, and electrical systems."), TEXT("table"));
		AddItemDef(TEXT("WorkbenchT3"), TEXT("Workbench Level 3"), EItemCategory::Building, 1, TEXT("Tier 3 Workbench enabling end-game weapons, Automated Turrets, and raiding explosives."), TEXT("table"));
		AddItemDef(TEXT("ResearchTable"), TEXT("Research Table"), EItemCategory::Building, 1, TEXT("Industrial drafting table for spending Scrap to permanently learn crafting Blueprints."), TEXT("table"));
		AddItemDef(TEXT("KeyLock"), TEXT("Key Lock"), EItemCategory::Building, 1, TEXT("Primitive wooden/iron key lock for securing early-game doors, boxes, and Tool Cupboards."), TEXT("lock.code"));
		AddItemDef(TEXT("CodeLock"), TEXT("Code Lock"), EItemCategory::Building, 1, TEXT("Electronic 4-digit keypad lock for multi-player base authorization."), TEXT("lock.code"));
		AddItemDef(TEXT("SmallFurnace"), TEXT("Small Furnace"), EItemCategory::Building, 1, TEXT("Compact clay and stone furnace for smelting Metal and Sulfur Ore."), TEXT("furnace"));
		AddItemDef(TEXT("LargeFurnace"), TEXT("Large Industrial Furnace"), EItemCategory::Building, 1, TEXT("High-capacity industrial smelter with 2x smelting speed and 18 slots."), TEXT("furnace"));
		AddItemDef(TEXT("OilRefinery"), TEXT("Small Oil Refinery"), EItemCategory::Building, 1, TEXT("Distillation refinery that converts Crude Oil into Low Grade Fuel."), TEXT("furnace"));

		// Electrical Systems & Traps
		AddItemDef(TEXT("SolarPanel"), TEXT("Large Solar Panel (20W)"), EItemCategory::Building, 1, TEXT("Photovoltaic generator producing up to 20 electrical power units during daylight."), TEXT("sign.wall"));
		AddItemDef(TEXT("WindTurbine"), TEXT("Wind Turbine Generator (50W)"), EItemCategory::Building, 1, TEXT("High-altitude rotary wind generator producing continuous day/night electrical power."), TEXT("lamp.street"));
		AddItemDef(TEXT("SmallBattery"), TEXT("Small Rechargeable Battery (10W)"), EItemCategory::Building, 1, TEXT("Stores electrical charge and outputs up to 10 continuous power units."), TEXT("storage.box.small"));
		AddItemDef(TEXT("LargeBattery"), TEXT("Large Base Battery Bank (100W)"), EItemCategory::Building, 1, TEXT("Heavy-duty industrial battery bank storing 24,000W-seconds and outputting 100W."), TEXT("storage.box.large"));
		AddItemDef(TEXT("ElectricSwitch"), TEXT("Electrical Switch"), EItemCategory::Building, 1, TEXT("Inline power switch for toggling base circuits on and off."), TEXT("lock.code"));
		AddItemDef(TEXT("ElectricSplitter"), TEXT("Electrical Splitter"), EItemCategory::Building, 1, TEXT("Divides incoming electrical power equally across up to 3 connected outputs."), TEXT("lock.code"));
		AddItemDef(TEXT("AutoTurret"), TEXT("Automated Sentry Turret"), EItemCategory::Building, 1, TEXT("Powered automated sentry turret (requires 10W) that engages unauthorized intruders."), TEXT("airraidsiren"));
		AddItemDef(TEXT("ShotgunTrap"), TEXT("Flame & Shotgun Trap"), EItemCategory::Building, 1, TEXT("Mechanical tripwire shotgun trap that fires at unauthorized players entering its cone."), TEXT("storage.box.small"));

		// Lighting Deployables
		AddItemDef(TEXT("TunaCanLamp"), TEXT("Tuna Can Wall Lamp"), EItemCategory::Building, 1, TEXT("Primitive wall-mounted oil lamp burning Low Grade Fuel for warm indoor light."), TEXT("wall.light"));
		AddItemDef(TEXT("IndustrialWallLight"), TEXT("Industrial Wall Light"), EItemCategory::Building, 1, TEXT("Caged bulkhead wall fixture providing bright low-power base illumination."), TEXT("wall.light"));
		AddItemDef(TEXT("CeilingFloodLight"), TEXT("Ceiling Grow & Flood Light"), EItemCategory::Building, 1, TEXT("Overhead high-lumen ceiling lamp that illuminates rooms and accelerates indoor crop growth."), TEXT("ceiling.light"));
		AddItemDef(TEXT("StandingWorkLamp"), TEXT("Standing Halogen Work Lamp"), EItemCategory::Building, 1, TEXT("Portable tripod halogen floor light for night construction and smelting areas."), TEXT("lamp.floor"));
		AddItemDef(TEXT("PoweredSearchLight"), TEXT("Perimeter Security Searchlight"), EItemCategory::Building, 1, TEXT("High-powered directional floodlight for spotting night raiders and helicopters."), TEXT("lamp.street"));

		// More Building Materials (Twig, Armored Tier 4, and Compound Perimeter Walls)
		AddItemDef(TEXT("TwigFoundation"), TEXT("Twig Foundation (Tier 0)"), EItemCategory::Building, 10, TEXT("Lightweight twig planning foundation; upgrade with a Hammer to Wood, Stone, Metal, or Armored."), TEXT("foundation.wood"));
		AddItemDef(TEXT("TwigWall"), TEXT("Twig Wall (Tier 0)"), EItemCategory::Building, 10, TEXT("Lightweight twig wall frame for rapid base layout."), TEXT("wall.wood"));
		AddItemDef(TEXT("TwigFloor"), TEXT("Twig Floor / Ceiling (Tier 0)"), EItemCategory::Building, 10, TEXT("Lightweight twig ceiling/floor tile."), TEXT("floor.wood"));
		AddItemDef(TEXT("TwigDoorway"), TEXT("Twig Doorway (Tier 0)"), EItemCategory::Building, 10, TEXT("Lightweight twig doorway frame."), TEXT("wall.doorway.wood"));
		AddItemDef(TEXT("TwigStairs"), TEXT("Twig U-Stairs (Tier 0)"), EItemCategory::Building, 10, TEXT("Lightweight twig staircase."), TEXT("ustairs.wood"));

		AddItemDef(TEXT("ArmoredFoundation"), TEXT("Armored Foundation (Tier 4 - 2000 HP)"), EItemCategory::Building, 10, TEXT("High-Quality Metal blast-resistant foundation capable of withstanding multiple C4 charges."), TEXT("foundation.metal"));
		AddItemDef(TEXT("ArmoredTriangleFoundation"), TEXT("Armored Triangle Foundation (Tier 4)"), EItemCategory::Building, 10, TEXT("High-Quality Metal blast-resistant triangle foundation for honeycomb base designs."), TEXT("foundation.triangle.metal"));
		AddItemDef(TEXT("ArmoredWall"), TEXT("Armored Wall (Tier 4 - 2000 HP)"), EItemCategory::Building, 10, TEXT("Heavy High-Quality Metal vault wall offering maximum raid protection."), TEXT("wall.metal"));
		AddItemDef(TEXT("ArmoredDoorway"), TEXT("Armored Doorway (Tier 4 - 2000 HP)"), EItemCategory::Building, 10, TEXT("Reinforced High-Quality Metal doorway frame."), TEXT("wall.doorway.metal"));
		AddItemDef(TEXT("ArmoredWindowWall"), TEXT("Armored Window Wall (Tier 4)"), EItemCategory::Building, 10, TEXT("Reinforced High-Quality Metal embrasure window wall."), TEXT("wall.window.metal"));
		AddItemDef(TEXT("ArmoredFloor"), TEXT("Armored Floor / Roof (Tier 4 - 2000 HP)"), EItemCategory::Building, 10, TEXT("Blast-resistant High-Quality Metal ceiling/floor slab against top-down rocket raids."), TEXT("floor.metal"));
		AddItemDef(TEXT("ArmoredStairs"), TEXT("Armored U-Stairs (Tier 4)"), EItemCategory::Building, 10, TEXT("Heavy High-Quality Metal interior staircase."), TEXT("ustairs.metal"));
		AddItemDef(TEXT("ArmoredDoor"), TEXT("Armored Vault Door (800 HP)"), EItemCategory::Building, 1, TEXT("Heavy High-Quality Metal bunker door with peephole hatch."), TEXT("door.metal"));
		AddItemDef(TEXT("HighExternalWoodWall"), TEXT("High External Wooden Compound Wall"), EItemCategory::Building, 5, TEXT("Tall log palisade wall topped with barbed wire for compound perimeters."), TEXT("highwall.stone"));
		AddItemDef(TEXT("HighExternalStoneWall"), TEXT("High External Stone Compound Wall"), EItemCategory::Building, 5, TEXT("Massive 6-meter reinforced stone perimeter wall for securing large compounds and refineries."), TEXT("highwall.stone"));

		// Medical & Clothing
		AddItemDef(TEXT("Bandage"), TEXT("Cloth Bandage"), EItemCategory::Medical, 3, TEXT("Stops bleeding immediately and restores 5 Health."), TEXT("bandage"));
		AddItemDef(TEXT("MedicalSyringe"), TEXT("Medical Syringe"), EItemCategory::Medical, 2, TEXT("Injects adrenaline and coagulants: +15 instant HP, +20 healing over time, and -10 Radiation."), TEXT("syringe.medical"));
		AddItemDef(TEXT("AntiRadPills"), TEXT("Anti-Radiation Pills"), EItemCategory::Medical, 10, TEXT("Potassium iodide tablets that purge 75 Radiation at the cost of 5 Thirst."), TEXT("can.tuna"));
		AddItemDef(TEXT("LargeMedkit"), TEXT("Large First-Aid Medkit"), EItemCategory::Medical, 2, TEXT("Field trauma kit that immediately stops all bleeding and restores 100 Health over time."), TEXT("bandage"));

		AddItemDef(TEXT("BurlapShirt"), TEXT("Burlap Shirt"), EItemCategory::Clothing, 1, TEXT("Early-game woven cloth shirt providing basic cold and melee protection."), TEXT("cloth"));
		AddItemDef(TEXT("BurlapTrousers"), TEXT("Burlap Trousers"), EItemCategory::Clothing, 1, TEXT("Early-game woven cloth trousers providing basic cold protection."), TEXT("cloth"));
		AddItemDef(TEXT("HidePoncho"), TEXT("Leather Hide Poncho"), EItemCategory::Clothing, 1, TEXT("Cured animal leather poncho providing strong cold and rain insulation."), TEXT("leather"));
		AddItemDef(TEXT("HazmatSuit"), TEXT("Hazmat Suit"), EItemCategory::Clothing, 1, TEXT("Sealed NBC protective suit providing 50% Radiation resistance and moderate projectile protection."), TEXT("cloth"));
		AddItemDef(TEXT("RoadSignJacket"), TEXT("Road Sign Jacket"), EItemCategory::Clothing, 1, TEXT("Tier 2 salvaged road-sign chest armor providing 35% projectile protection."), TEXT("metal"));
		AddItemDef(TEXT("RoadSignKilt"), TEXT("Road Sign Kilt"), EItemCategory::Clothing, 1, TEXT("Tier 2 armored leg guards crafted from salvaged street signs."), TEXT("metal"));
		AddItemDef(TEXT("MetalFacemask"), TEXT("Metal Facemask"), EItemCategory::Clothing, 1, TEXT("Tier 3 High-Quality Metal ballistic faceplate providing 50% head protection."), TEXT("metal.refined"));
		AddItemDef(TEXT("MetalChestplate"), TEXT("Metal Chestplate"), EItemCategory::Clothing, 1, TEXT("Tier 3 High-Quality Metal torso armor providing 50% projectile protection."), TEXT("metal.refined"));

		// Seeds & Crops
		AddItemDef(TEXT("HempSeed"), TEXT("Hemp Seed"), EItemCategory::Food, 50, TEXT("Plantable Hemp seed carrying a 6-gene allele sequence; harvests into Cloth."), TEXT("seed.cotton"));
		AddItemDef(TEXT("CornSeed"), TEXT("Corn Seed"), EItemCategory::Food, 50, TEXT("Plantable Corn seed carrying a 6-gene allele sequence; harvests into edible Corn."), TEXT("seed.berry"));
		AddItemDef(TEXT("PumpkinSeed"), TEXT("Pumpkin Seed"), EItemCategory::Food, 50, TEXT("Plantable Pumpkin seed carrying a 6-gene allele sequence; harvests into filling Pumpkins."), TEXT("seed.pumpkin"));
		AddItemDef(TEXT("Corn"), TEXT("Ear of Corn"), EItemCategory::Food, 20, TEXT("Freshly harvested corn restoring Hunger and Hydration."), TEXT("apple"));
		AddItemDef(TEXT("Pumpkin"), TEXT("Pumpkin"), EItemCategory::Food, 20, TEXT("Nutrient-dense pumpkin restoring substantial Hunger and Hydration."), TEXT("pumpkin"));

		// Phase 4: Weather Adaptation Deployables
		AddItemDef(TEXT("RainCollector"), TEXT("Rain Catcher"), EItemCategory::Building, 1, TEXT("Outdoor funnel and cistern that passively collects fresh drinking water during rainstorms."), TEXT("water.bottle"));
		AddItemDef(TEXT("WeatherRadar"), TEXT("Weather Radar Station"), EItemCategory::Building, 1, TEXT("Powered meteorological radar (requires 5W) that detects incoming Tornadoes, Acid Rain, and Blizzards."), TEXT("airraidsiren"));
	}

	if (ItemDataTable != nullptr && !bPhase1ItemsLinkedToDataTable)
	{
		bPhase1ItemsLinkedToDataTable = true;
		static const FString ContextString(TEXT("Phase1TemplateLink"));
		FItemData* FallbackRow = ItemDataTable->FindRow<FItemData>(TEXT("wood"), ContextString, false);

		for (TPair<FName, FItemData>& Pair : RegisteredItemData)
		{
			const FName* TemplateRowName = RegisteredItemTemplateRow.Find(Pair.Key);
			FItemData* SourceRow = (TemplateRowName != nullptr)
				? ItemDataTable->FindRow<FItemData>(*TemplateRowName, ContextString, false)
				: nullptr;
			if (SourceRow == nullptr)
			{
				SourceRow = FallbackRow;
			}
			if (SourceRow != nullptr)
			{
				if (Pair.Value.Thumbnail == nullptr)
				{
					Pair.Value.Thumbnail = SourceRow->Thumbnail;
				}
				if (Pair.Value.Mesh == nullptr)
				{
					Pair.Value.Mesh = SourceRow->Mesh;
				}
				if (Pair.Value.EquipItemClass == nullptr)
				{
					Pair.Value.EquipItemClass = SourceRow->EquipItemClass;
				}
			}
		}
	}
}

void UInventoryComponent::RegisterItemData(const FName& ItemID, const FItemData& Data)
{
	EnsureDefaultPhase1ItemsRegistered();
	RegisteredItemData.Add(ItemID, Data);
}

FItemData* UInventoryComponent::GetItemData(const FName& ItemID)
{
	EnsureDefaultPhase1ItemsRegistered();
	if (ItemDataTable != nullptr)
	{
		static const FString ContextString(TEXT("Item Data Context"));
		if (FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, ContextString, false))
		{
			return Row;
		}
	}

	return RegisteredItemData.Find(ItemID);
}

void UInventoryComponent::SpawnWorldItem(UWorld* WorldContextObject, const FInventoryItem& ItemToSpawn, AActor* SpawningActor)
{
	if (WorldContextObject == nullptr || SpawningActor == nullptr)
	{
		return;
	}

	FVector SpawningActorOrigin;
	FVector SpawningActorBounds;
	SpawningActor->GetActorBounds(true, SpawningActorOrigin, SpawningActorBounds);

	// Spawn a world item actor
	const FVector SpawnLocation = SpawningActor->GetActorLocation() + FVector(0.0f, 0.0f, SpawningActorBounds.Z * 0.5f);
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SpawningActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AWorldItem* WorldItem = WorldContextObject->SpawnActor<AWorldItem>(SpawnLocation, SpawnRotation, SpawnParams);
	if (WorldItem == nullptr)
	{
		return;
	}

	// Update world items properties
	WorldItem->SetItem(ItemToSpawn);

	const FVector PhysicsImpulse = SpawningActor->GetActorForwardVector() * 5000.0f;
	
	WorldItem->AddImpulse(PhysicsImpulse);
}

TArray<FName> UInventoryComponent::GetAllItems()
{
	EnsureDefaultPhase1ItemsRegistered();
	TArray<FName> Names;
	if (ItemDataTable != nullptr)
	{
		Names = ItemDataTable->GetRowNames();
	}
	for (const TPair<FName, FItemData>& Pair : RegisteredItemData)
	{
		Names.AddUnique(Pair.Key);
	}
	return Names;
}

void UInventoryComponent::OnLoadComplete_Implementation()
{
	if (ServerState.Slots.Num() < SlotCount)
	{
		UE_LOG(LogInventory, Warning, TEXT("Container Slot Count Invalid Resizing, SlotsNum: %i, SlotCount: %i"), ServerState.Slots.Num(), SlotCount);
		for (int32 i = ServerState.Slots.Num(); i < SlotCount; ++i)
		{
			FInventorySlot Slot;
			Slot.Index = i;
			ServerState.Slots.Add(Slot);
		}
	}
	else if (ServerState.Slots.Num() > SlotCount)
	{
		UE_LOG(LogInventory, Warning, TEXT("Container Slot Count Invalid Resizing, SlotsNum: %i, SlotCount: %i"), ServerState.Slots.Num(), SlotCount);
		for (int32 i = SlotCount; i < ServerState.Slots.Num(); i++)
		{
			if (ServerState.Slots[i].IsEmpty())
			{
				ServerState.Slots.RemoveAt(ServerState.Slots.Num() - 1);
				continue;
			}
			FInventoryItem SlotItem = ServerState.Slots[i].Item;
			ServerState.Contents.RemoveItem(SlotItem.Name, SlotItem.Quantity);
			UInventoryComponent::SpawnWorldItem(GetWorld(), SlotItem, nullptr);
			ServerState.Slots.RemoveAt(ServerState.Slots.Num() - 1);
		}
		ServerState.Slots.Shrink();
	}

	ServerState.Contents = Contents;
	ServerState.Slots = Slots;

	LoadedFromSave = true;
}

void UInventoryComponent::HandleItemSelection(const FInventorySlotInteraction& Interaction)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return;
	}
	
	bool UseServerState = OwningActor->HasAuthority();

	if (!Interaction.Manipulator->IsDragging())
	{
		if (Interaction.Primary)
		{
			DragAll(Interaction.SlotIndex, Interaction.Manipulator, UseServerState ? ServerState.Slots : Slots, UseServerState ? ServerState.Contents : Contents);
		}
		else
		{
			DragSplit(Interaction.SlotIndex, Interaction.Manipulator, UseServerState ? ServerState.Slots : Slots, UseServerState ? ServerState.Contents : Contents);
		}
	}
	else
	{
		if (Interaction.Primary)
		{
			DropAll(Interaction.SlotIndex, Interaction.Manipulator, UseServerState ? ServerState.Slots : Slots, UseServerState ? ServerState.Contents : Contents);
		}
		else
		{
			DropSingle(Interaction.SlotIndex, Interaction.Manipulator, UseServerState ? ServerState.Slots : Slots, UseServerState ? ServerState.Contents : Contents);
		}
	}
}

void UInventoryComponent::HandleItemQuickMove(const FInventorySlotInteraction& Interaction)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return;
	}

	bool UseServerState = OwningActor->HasAuthority();
	
	bool WithinPlayerInventory = Interaction.Manipulator->GetOwnersInventory() == this;
	bool ContainerOpen = Interaction.Manipulator->GetOpenContainer() != nullptr;
	
	FInventorySlot& InteractionSlot = UseServerState ? ServerState.Slots[Interaction.SlotIndex] : Slots[Interaction.SlotIndex];
	if (InteractionSlot.IsEmpty())
	{
		return;
	}

	if (WithinPlayerInventory) // Interaction from within the players inventory
	{
		if (ContainerOpen) // Move from the player's inventory to the open container
		{
			// Store the slot's item information
			FInventoryItem MovingItem = UseServerState ? ServerState.Slots[Interaction.SlotIndex].Item : Slots[Interaction.SlotIndex].Item;

			// Remove the item from the player's inventory
			UseServerState ? ServerState.Contents.RemoveItem(MovingItem.Name, MovingItem.Quantity) : Contents.RemoveItem(MovingItem.Name, MovingItem.Quantity);
			UseServerState ? ServerState.Slots[Interaction.SlotIndex].ClearItem() : Slots[Interaction.SlotIndex].ClearItem();

			// Add item to the container's inventory
			Interaction.Manipulator->GetOpenContainer()->AddItem(MovingItem, false, GetOwner());
		}
		else // Move within the player's inventory
		{
			// Store the slot's item information
			FInventoryItem MovingItem = UseServerState ? ServerState.Slots[Interaction.SlotIndex].Item : Slots[Interaction.SlotIndex].Item;
			
			// Clear this slot before performing quick move
			UseServerState ? ServerState.Slots[Interaction.SlotIndex].ClearItem() : Slots[Interaction.SlotIndex].ClearItem();

			// Find the best availible slot and add the item to it
			const bool FromToolbar = Interaction.SlotIndex < 6;
			int32 Remaining = 0;
			const bool AddedAll = AddItemToSlots(MovingItem, Remaining, FromToolbar);

			// This was added so items don't disapear if the inventory is full 6/14/2024 at like 6:43 AM
			if (AddedAll == false && FromToolbar == true)
			{
				MovingItem.Quantity = Remaining;
				(void*)AddItemToSlots(MovingItem, Remaining);
			}
		}
	}
	else if (ContainerOpen && !WithinPlayerInventory) // Interaction from within an open container to the player's inventory
	{
		// Store the slot's item information
		FInventoryItem MovingItem = UseServerState ? ServerState.Slots[Interaction.SlotIndex].Item : Slots[Interaction.SlotIndex].Item;

		// Remove the item from this container
		UseServerState ? ServerState.Contents.RemoveItem(MovingItem.Name, MovingItem.Quantity) : Contents.RemoveItem(MovingItem.Name, MovingItem.Quantity);
		UseServerState ? ServerState.Slots[Interaction.SlotIndex].ClearItem() : Slots[Interaction.SlotIndex].ClearItem();

		// Add the item to the player's inventory
		Interaction.Manipulator->GetOwnersInventory()->AddItem(MovingItem);
	}
}

void UInventoryComponent::Server_SlotInteraction_Implementation(const FInventorySlotInteraction& Interaction)
{
	// Update our server state
	ServerState.LastInteraction = Interaction;

	Interaction.QuickMove ? HandleItemQuickMove(Interaction) : HandleItemSelection(Interaction);

	OnRep_ServerState();
}

FInventoryItem* UInventoryComponent::FindItemWithUniqueID(const uint32& UniqueID)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return nullptr;
	}

	const bool UseServerState = OwningActor->HasAuthority();

	FInventoryItem* FoundItem = nullptr;
	TArray<FInventorySlot>& SlotArray = UseServerState ? ServerState.Slots : Slots;
	for (FInventorySlot& Slot : SlotArray)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}
		if (Slot.Item.UniqueID != UniqueID)
		{
			continue;
		}
		
		FoundItem = &Slot.Item;
		break;
	}

	return FoundItem;
}

FInventorySlot* UInventoryComponent::FindSlotContainingItem(const FName& ItemToFind)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return nullptr;
	}

	const bool UseServerState = OwningActor->HasAuthority();

	FInventorySlot* FoundSlot = nullptr;
	TArray<FInventorySlot>& SlotArray = UseServerState ? ServerState.Slots : Slots;
	for (FInventorySlot& Slot : SlotArray)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}
		if (!FInventoryItem::CompareNames(Slot.Item, ItemToFind))
		{
			continue;
		}

		FoundSlot = &Slot;
		break;
	}

	return FoundSlot;
}

FString UInventoryComponent::GetDisplayName() const
{
	return DisplayName;
}

UClass* UInventoryComponent::GetWidgetClass() const
{
	return WidgetClass.Get();
}

FInventoryContents* UInventoryComponent::GetContents()
{
	return &Contents;
}

FInventorySlot* UInventoryComponent::GetSlot(const int32& SlotIndex)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return nullptr;
	}
	
	const bool UseServerState = OwningActor->HasAuthority();
	
	if (UseServerState ? SlotIndex >= ServerState.Slots.Num() : SlotIndex > Slots.Num())
	{
		return nullptr;
	}
	else if (SlotIndex < 0)
	{
		return nullptr;
	}

	return UseServerState? &ServerState.Slots[SlotIndex] :  &Slots[SlotIndex];
}

uint8 UInventoryComponent::GetSlotCount() const
{
	return SlotCount;
}

TArray<FInventorySlot>& UInventoryComponent::GetSlots()
{
	return Slots;
}

void UInventoryComponent::RequestInventoryRefresh()
{
	OnRep_ServerState();
}

void UInventoryComponent::SetDisplayName(const FString& NewDisplayName)
{
	DisplayName = NewDisplayName;
}

void UInventoryComponent::SetSlotCount(const uint8& InSlotCount)
{
	SlotCount = InSlotCount;
}

bool UInventoryComponent::WasLoadedFromSave() const
{
	return LoadedFromSave;
}

TArray<uint8> UInventoryComponent::Save()
{
	TArray<uint8> Bytes;

	FMemoryWriter MemoryWriter(Bytes);
	FObjectAndNameAsStringProxyArchive InventoryArchive(MemoryWriter, true);
	InventoryArchive.ArIsSaveGame = true;
	this->Serialize(InventoryArchive);

	return Bytes;
}

void UInventoryComponent::Load(const TArray<uint8>& InSave)
{
	FMemoryReader MemoryReader(InSave);
	FObjectAndNameAsStringProxyArchive InventoryArchive(MemoryReader, true);
	InventoryArchive.ArIsSaveGame = true;
	this->Serialize(InventoryArchive);

	OnLoadComplete_Implementation();
	OnRep_ServerState();
}

void UInventoryComponent::OnRep_ServerState()
{
	ClearAcknowlagedInteractions(ServerState.LastInteraction);
	
	TArray<FInventoryItemUpdate> UnacknowlagedUpdates;
	GetUnacknowlagedUpdates(UnacknowlagedUpdates);
	BroadcastAllUpdateNotifications(UnacknowlagedUpdates);

	if (GetOwner()->HasAuthority() || UnacknowalgedInteractions.Num() == 0 || ServerState.LastInteraction.Time == 0)
	{
		Slots = ServerState.Slots;
		Contents = ServerState.Contents;
	}

	BroadcastInventoryUpdate();
}

void UInventoryComponent::ClearAcknowlagedInteractions(const FInventorySlotInteraction& LastInteraction)
{
	TArray<FInventorySlotInteraction> NewInteractionsList;

	for (const FInventorySlotInteraction& Interaction : UnacknowalgedInteractions)
	{
		if (Interaction.Time > LastInteraction.Time)
		{
			NewInteractionsList.Add(Interaction);
		}
	}

	UnacknowalgedInteractions = NewInteractionsList;
}

void UInventoryComponent::GetUnacknowlagedUpdates(TArray<FInventoryItemUpdate>& OutUpdatesList)
{
	for (const FInventoryItemUpdate& Update : ServerState.Updates)
	{
		if (Update.Time <= LastClientAcknowlagedItemUpdate.Time)
		{
			continue;
		}
		OutUpdatesList.Add(Update);
	}

	if (ServerState.Updates.IsEmpty())
	{
		return;
	}
	
	LastClientAcknowlagedItemUpdate = ServerState.Updates[ServerState.Updates.Num() - 1];
}

void UInventoryComponent::BroadcastAllUpdateNotifications(const TArray<FInventoryItemUpdate>& UpdatesList)
{
	for (const FInventoryItemUpdate& ItemUpdate : UpdatesList)
	{
		BroadcastItemUpdate(ItemUpdate);
	}
}

void UInventoryComponent::BroadcastInventoryUpdate()
{
	if (!OnUpdate.IsBound())
	{
		return;
	}

	OnUpdate.Broadcast();
}

void UInventoryComponent::BroadcastItemUpdate(const FInventoryItemUpdate& ItemUpdate)
{
	if (!OnItemUpdate.IsBound())
	{
		return;
	}

	OnItemUpdate.Broadcast(ItemUpdate);
}

bool UInventoryComponent::AddItemToSlots(const FInventoryItem& ItemToAdd, int32& Remaining, const int32& RowsToSkip)
{
	Remaining = ItemToAdd.Quantity;

	FItemData* ItemData = GetItemData(ItemToAdd.Name);
	if (ItemData == nullptr)
	{
		return false;
	}

	uint32 UniqueID = FMath::RandRange(1, 999999);
	
	// if the item stats are empty populate with defaults
	TArray<FItemStat> ItemStats;
	if (ItemData->Stats.Num() > 0 && ItemToAdd.Stats.Num() == 0)
	{
		ItemStats = ItemData->Stats;
	}
	else
	{
		ItemStats = ItemToAdd.Stats;
	}

	if (!FindAndAddToPopulatedSlot(ItemToAdd.Name, ItemData->StackSize, Remaining, RowsToSkip))
	{
		FindAndAddToEmptySlot(ItemToAdd.Name, ItemData->StackSize, ItemStats, Remaining, RowsToSkip);
	}

	return Remaining == 0;
}

bool UInventoryComponent::FindAndAddToPopulatedSlot(const FName& ItemName, const int32& ItemStackSize, int32& QuantityToAdd, const int32& RowsToSkip)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return false;
	}

	const bool UseServerState = OwningActor->HasAuthority();

	TArray<FInventorySlot>& SlotArray = UseServerState ? ServerState.Slots : Slots;
	const int32 StartIndex = RowsToSkip * 6;
	for (int32 i = StartIndex; i < SlotArray.Num(); i++)
	{
		FInventorySlot& Slot = SlotArray[i];
		if (QuantityToAdd == 0)
		{
			break;
		}

		if (!FInventoryItem::CompareNames(Slot.Item, ItemName) || Slot.Item.Quantity == 0 || Slot.Item.Quantity >= GetItemData(ItemName)->StackSize)
		{
			continue;
		}

		if (Slot.Item.Quantity + QuantityToAdd > ItemStackSize)
		{
			QuantityToAdd -= ItemStackSize - Slot.Item.Quantity;
			FInventoryItem NewSlotItem;
			NewSlotItem.Name = Slot.Item.Name;
			NewSlotItem.Quantity = ItemStackSize;
			Slot.Item = NewSlotItem;
		}
		else
		{
			FInventoryItem NewSlotItem;
			NewSlotItem.Name = Slot.Item.Name;
			NewSlotItem.Quantity = QuantityToAdd + Slot.Item.Quantity;
			Slot.Item = NewSlotItem;
			QuantityToAdd = 0;
		}
	}
	return QuantityToAdd == 0;
}

bool UInventoryComponent::FindAndAddToEmptySlot(const FName& ItemName, const int32& ItemStackSize, const TArray<FItemStat>& Stats, int32& QuantityToAdd, const int32& RowsToSkip)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return false;
	}

	const bool UseServerState = OwningActor->HasAuthority();
	TArray<FInventorySlot>& SlotArray = UseServerState ? ServerState.Slots : Slots;
	const int32 StartIndex = RowsToSkip * 6;

	for (int32 i = StartIndex; i < SlotArray.Num(); i++)
	{
		FInventorySlot& Slot = SlotArray[i];

		if (QuantityToAdd == 0)
		{
			break;
		}

		if (Slot.Item.Quantity > 0)
		{
			continue;
		}

		uint32 ItemUniqueID = FMath::RandRange(0, 99999);

		if (QuantityToAdd > ItemStackSize)
		{
			QuantityToAdd -= ItemStackSize;
			FInventoryItem NewSlotItem;
			NewSlotItem.Name = ItemName;
			NewSlotItem.Quantity = ItemStackSize;
			NewSlotItem.UniqueID = ItemUniqueID;
			Slot.Item = NewSlotItem;
		}
		else
		{
			FInventoryItem NewSlotItem;
			NewSlotItem.Name = ItemName;
			NewSlotItem.Quantity = QuantityToAdd;
			NewSlotItem.Stats = Stats;
			NewSlotItem.UniqueID = ItemUniqueID;
			Slot.Item = NewSlotItem;
			QuantityToAdd = 0;
		}
	}
	return QuantityToAdd == 0;
}

bool UInventoryComponent::RemoveItemFromSlots(const FName& ItemName, const int32& Quantity, int32& Remaining)
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return false;
	}
	
	bool UseServerState = OwningActor->HasAuthority();

	Remaining = Quantity;
	TArray<FInventorySlot>& SlotArray = UseServerState ? ServerState.Slots : Slots;
	for (FInventorySlot& Slot : SlotArray)
	{
		if (!FInventoryItem::CompareNames(Slot.Item, ItemName))
		{
			continue;
		}

		if (Slot.Item.Quantity <= Remaining)
		{
			Remaining -= Slot.Item.Quantity;
			Slot.ClearItem();
			continue;
		}
		else
		{
			Slot.Item.Quantity -= Remaining;
			Remaining = 0;
		}
		
		if (Slot.Item.IsZero())
		{
			Slot.ClearItem();
		}

		if (Remaining == 0)
		{
			break;
		}
	}

	return Remaining == 0;
}

void UInventoryComponent::DragAll(const int32& FromSlotIndex, UInventoryManipulatorComponent* Manipulator, TArray<FInventorySlot>& SlotsToModify, FInventoryContents& ContentsToModify)
{
	FInventorySlot& FromSlot = SlotsToModify[FromSlotIndex];

	if (FromSlot.IsEmpty())
	{
		return;
	}

	Manipulator->StartDragging(FromSlot.Item);

	ContentsToModify.RemoveItem(FromSlot.Item.Name, FromSlot.Item.Quantity);

	FromSlot.ClearItem();
}

void UInventoryComponent::DragSplit(const int32& FromSlotIndex, UInventoryManipulatorComponent* Manipulator, TArray<FInventorySlot>& SlotsToModify, FInventoryContents& ContentsToModify)
{
	FInventorySlot& FromSlot = SlotsToModify[FromSlotIndex];

	if (FromSlot.IsEmpty())
	{
		return;
	}

	// Just take all if there is only one item
	if (FromSlot.Item.Quantity == 1)
	{
		Manipulator->StartDragging(FromSlot.Item);
		FromSlot.ClearItem();

		return;
	}

	// Calculate the half
	int32 HalfQuantity = FromSlot.Item.Quantity / 2;

	// Start Dragging half
	FInventoryItem NewSelection = FromSlot.Item;
	NewSelection.Quantity -= HalfQuantity;


	Manipulator->StartDragging(NewSelection);
	ContentsToModify.RemoveItem(NewSelection.Name, NewSelection.Quantity);

	FInventoryItem NewSlotItem = FromSlot.Item;
	NewSlotItem.Quantity = HalfQuantity;
	FromSlot.Item = NewSlotItem;
}

void UInventoryComponent::DropAll(const int32& ToSlotIndex, UInventoryManipulatorComponent* Manipulator, TArray<FInventorySlot>& SlotsToModify, FInventoryContents& ContentsToModify)
{
	FInventorySlot& ToSlot = SlotsToModify[ToSlotIndex];
	FInventoryItem SelectedItem = Manipulator->GetSelectedItem();

	if (ToSlot.IsEmpty())
	{
		ToSlot.Item = SelectedItem;

		ContentsToModify.AddItem(SelectedItem.Name, SelectedItem.Quantity);

		Manipulator->StopDragging();
	}
	else if (ToSlot.SameItemNameAs(SelectedItem) && GetItemData(SelectedItem.Name)->StackSize != 1) // Same item and the stack size for this item is not 1
	{
		if (WithinStackSize(ToSlot.Item, SelectedItem.Quantity))
		{
			// Update Inventory
			ToSlot.Item.Quantity += SelectedItem.Quantity;
			ContentsToModify.AddItem(Manipulator->GetSelectedItem().Name, Manipulator->GetSelectedItem().Quantity);
			
			// Update Selection
			Manipulator->StopDragging();
		}
		else
		{
			int32 OldSlotItemQuantity = ToSlot.Item.Quantity;

			// Update Inventory
			int32 AmountAdded = GetItemData(ToSlot.Item.Name)->StackSize - ToSlot.Item.Quantity;
			ToSlot.Item.Quantity = GetItemData(SelectedItem.Name)->StackSize;
			ContentsToModify.AddItem(SelectedItem.Name, AmountAdded);
			
			// Update Selection
			SelectedItem.Quantity = (OldSlotItemQuantity + SelectedItem.Quantity) - GetItemData(SelectedItem.Name)->StackSize;
			Manipulator->StartDragging(SelectedItem);
		}
	}
	else
	{
		// Store the old item for swap
		FInventoryItem OldSlotItem = ToSlot.Item;

		// Update Inventory
		ContentsToModify.RemoveItem(OldSlotItem.Name, OldSlotItem.Quantity);
		ContentsToModify.AddItem(SelectedItem.Name, SelectedItem.Quantity);
		ToSlot.Item = SelectedItem;

		// Update Selection
		Manipulator->StartDragging(OldSlotItem);
	}

	if (Manipulator->GetSelectedItem().IsZero())
	{
		Manipulator->StopDragging();
	}
}

void UInventoryComponent::DropSingle(const int32& ToSlotIndex, UInventoryManipulatorComponent* Manipulator, TArray<FInventorySlot>& SlotsToModify, FInventoryContents& ContentsToModify)
{
	if (!Manipulator->IsDragging())
	{
		return;
	}

	FInventorySlot& ToSlot = SlotsToModify[ToSlotIndex];
	FInventoryItem SelectedItem = Manipulator->GetSelectedItem();

	if (ToSlot.IsEmpty())
	{
		// Update Inventory
		ToSlot.Item.Name = SelectedItem.Name;
		ToSlot.Item.Quantity = 1;
		ContentsToModify.AddItem(SelectedItem.Name, 1);

		// Update Selection
		FInventoryItem NewSelection = SelectedItem;
		NewSelection.Quantity -= 1;
		Manipulator->StartDragging(NewSelection);
	}
	else if (ToSlot.SameItemNameAs(SelectedItem) && WithinStackSize(ToSlot.Item, 1))
	{
		// Update Inventory
		ToSlot.Item.Quantity += 1;
		ContentsToModify.AddItem(SelectedItem.Name, 1);

		// Update Selection
		FInventoryItem NewSelection = SelectedItem;
		NewSelection.Quantity -= 1;
		Manipulator->StartDragging(NewSelection);
	}

	if (Manipulator->GetSelectedItem().Quantity <= 0)
	{
		Manipulator->StopDragging();
	}
}

bool UInventoryComponent::WithinStackSize(const FInventoryItem& BaseItem, const int32& AmountToAdd) const
{
	return (BaseItem.Quantity + AmountToAdd) <= GetItemData(BaseItem.Name)->StackSize;
}
