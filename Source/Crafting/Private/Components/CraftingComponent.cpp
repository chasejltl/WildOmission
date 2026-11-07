// Copyright Telephone Studios. All Rights Reserved.


#include "Components/CraftingComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InventoryManipulatorComponent.h"
#include "ServerRules.h"
#include "UObject/ConstructorHelpers.h"
#include "Log.h"

static UDataTable* RecipeDataTable = nullptr;
static TMap<FName, FCraftingRecipe> RegisteredRecipes;
static bool bPhase1RecipesSeeded = false;
static TFunction<uint8(const FVector&)> ProximityWorkbenchQueryFunc;

// Sets default values for this component's properties
UCraftingComponent::UCraftingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	HighestAvailableWorkbenchTier = 0;

	static ConstructorHelpers::FObjectFinder<UDataTable> RecipeDataTableBlueprint(TEXT("/Game/Crafting/DataTables/DT_CraftingRecipes"));
	if (RecipeDataTableBlueprint.Succeeded())
	{
		RecipeDataTable = RecipeDataTableBlueprint.Object;
	}

	EnsureDefaultPhase1RecipesRegistered();
}

void UCraftingComponent::EnsureDefaultPhase1RecipesRegistered()
{
	if (!bPhase1RecipesSeeded)
	{
		bPhase1RecipesSeeded = true;

		auto AddRecipe = [](const FName& Name, int32 YieldQty, int32 CraftSeconds, uint8 WorkbenchTier, const TArray<TPair<FName, int32>>& Ings, int32 ScrapCost = 0)
		{
			FCraftingRecipe Recipe;
			Recipe.YieldQuantity = YieldQty;
			Recipe.CraftTime = CraftSeconds;
			Recipe.RequiredWorkbenchTier = WorkbenchTier;
			Recipe.ResearchScrapCost = ScrapCost;
			for (const TPair<FName, int32>& Pair : Ings)
			{
				FInventoryItem Ing;
				Ing.Name = Pair.Key;
				Ing.Quantity = Pair.Value;
				Recipe.Ingredients.Add(Ing);
			}
			RegisteredRecipes.Add(Name, Recipe);
		};

		// Refined derivatives & fuels
		AddRecipe(TEXT("LowGradeFuel"), 4, 2, 0, { { TEXT("AnimalFat"), 3 }, { TEXT("Cloth"), 1 } });
		AddRecipe(TEXT("Gunpowder"), 10, 3, 1, { { TEXT("Charcoal"), 30 }, { TEXT("Sulfur"), 20 } });

		// Tiered Tools
		AddRecipe(TEXT("StoneHatchet"), 1, 5, 0, { { TEXT("Wood"), 200 }, { TEXT("Stone"), 100 } });
		AddRecipe(TEXT("StonePickaxe"), 1, 5, 0, { { TEXT("Wood"), 200 }, { TEXT("Stone"), 100 } });
		AddRecipe(TEXT("MetalHatchet"), 1, 10, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 75 } });
		AddRecipe(TEXT("MetalPickaxe"), 1, 10, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 125 } });

		// Primitive & Military Weapons
		AddRecipe(TEXT("WoodenSpear"), 1, 5, 0, { { TEXT("Wood"), 300 } });
		AddRecipe(TEXT("StoneSpear"), 1, 8, 0, { { TEXT("WoodenSpear"), 1 }, { TEXT("Stone"), 100 }, { TEXT("Cloth"), 5 } });
		AddRecipe(TEXT("HuntingBow"), 1, 8, 0, { { TEXT("Wood"), 200 }, { TEXT("Cloth"), 50 } });
		AddRecipe(TEXT("Crossbow"), 1, 15, 1, { { TEXT("Wood"), 200 }, { TEXT("MetalFragments"), 75 }, { TEXT("Cloth"), 20 } });
		AddRecipe(TEXT("HandmadeRevolver"), 1, 20, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 125 }, { TEXT("Cloth"), 25 } });
		AddRecipe(TEXT("CustomSMG"), 1, 25, 2, { { TEXT("HighQualityMetal"), 15 }, { TEXT("MetalFragments"), 200 } }, 250);
		AddRecipe(TEXT("CombatShotgun"), 1, 30, 2, { { TEXT("HighQualityMetal"), 20 }, { TEXT("MetalFragments"), 250 } }, 250);
		AddRecipe(TEXT("BoltActionRifle"), 1, 35, 3, { { TEXT("HighQualityMetal"), 30 }, { TEXT("MetalFragments"), 300 } }, 500);
		AddRecipe(TEXT("AssaultRifle"), 1, 40, 3, { { TEXT("HighQualityMetal"), 50 }, { TEXT("Wood"), 200 }, { TEXT("MetalFragments"), 400 } }, 500);
		AddRecipe(TEXT("SatchelCharge"), 1, 15, 1, { { TEXT("Gunpowder"), 240 }, { TEXT("MetalFragments"), 80 }, { TEXT("Cloth"), 10 } }, 125);
		AddRecipe(TEXT("Explosives"), 1, 5, 3, { { TEXT("Gunpowder"), 50 }, { TEXT("LowGradeFuel"), 3 }, { TEXT("Sulfur"), 10 }, { TEXT("MetalFragments"), 10 } }, 250);
		AddRecipe(TEXT("TimedExplosiveCharge"), 1, 30, 3, { { TEXT("Explosives"), 20 }, { TEXT("Cloth"), 5 } }, 500);
		AddRecipe(TEXT("FragGrenade"), 1, 10, 2, { { TEXT("Gunpowder"), 30 }, { TEXT("MetalFragments"), 40 } }, 75);

		// Ammo Types tied to Workbench crafting tiers
		AddRecipe(TEXT("WoodenArrow"), 2, 3, 0, { { TEXT("Wood"), 25 }, { TEXT("Stone"), 10 } });
		AddRecipe(TEXT("BoneArrow"), 3, 3, 0, { { TEXT("Wood"), 25 }, { TEXT("BoneFragments"), 10 } });
		AddRecipe(TEXT("HighVelocityArrow"), 2, 4, 1, { { TEXT("Wood"), 20 }, { TEXT("MetalFragments"), 5 } });
		AddRecipe(TEXT("PistolBullet"), 4, 5, 1, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 5 } });
		AddRecipe(TEXT("HighVelocityPistolBullet"), 4, 6, 2, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 10 } }, 75);
		AddRecipe(TEXT("RifleBullet"), 3, 8, 2, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 10 } });
		AddRecipe(TEXT("HighVelocityRifleBullet"), 2, 10, 2, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 20 } }, 125);
		AddRecipe(TEXT("ExplosiveRifleBullet"), 2, 12, 3, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 20 }, { TEXT("Sulfur"), 10 } }, 250);
		AddRecipe(TEXT("TwelveGaugeBuckshot"), 2, 5, 1, { { TEXT("MetalFragments"), 5 }, { TEXT("Gunpowder"), 10 } });
		AddRecipe(TEXT("TwelveGaugeSlug"), 2, 6, 2, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 10 } }, 75);
		AddRecipe(TEXT("Rocket"), 1, 15, 3, { { TEXT("Explosives"), 10 }, { TEXT("Gunpowder"), 150 }, { TEXT("MetalFragments"), 100 } }, 500);
		AddRecipe(TEXT("IncendiaryRocket"), 1, 15, 2, { { TEXT("Gunpowder"), 120 }, { TEXT("LowGradeFuel"), 75 }, { TEXT("MetalFragments"), 80 } }, 250);

		// Phase 2: Workbenches & Research Table
		AddRecipe(TEXT("WorkbenchT1"), 1, 15, 0, { { TEXT("Wood"), 500 }, { TEXT("MetalFragments"), 100 }, { TEXT("Scrap"), 50 } });
		AddRecipe(TEXT("WorkbenchT2"), 1, 30, 1, { { TEXT("MetalFragments"), 500 }, { TEXT("HighQualityMetal"), 20 }, { TEXT("Scrap"), 500 } });
		AddRecipe(TEXT("WorkbenchT3"), 1, 45, 2, { { TEXT("MetalFragments"), 1000 }, { TEXT("HighQualityMetal"), 100 }, { TEXT("Scrap"), 1250 } });
		AddRecipe(TEXT("ResearchTable"), 1, 20, 1, { { TEXT("MetalFragments"), 200 }, { TEXT("Scrap"), 75 } });

		// Phase 2: Locks, Storage Barrels, Boxes, Smelters & Refineries
		AddRecipe(TEXT("KeyLock"), 1, 5, 0, { { TEXT("Wood"), 100 } });
		AddRecipe(TEXT("CodeLock"), 1, 10, 1, { { TEXT("MetalFragments"), 100 } });
		AddRecipe(TEXT("StorageBarrel"), 1, 8, 0, { { TEXT("MetalFragments"), 75 }, { TEXT("Wood"), 50 } });
		AddRecipe(TEXT("FuelBarrel"), 1, 10, 1, { { TEXT("MetalFragments"), 125 }, { TEXT("LowGradeFuel"), 25 } });
		AddRecipe(TEXT("LargeStorageBox"), 1, 10, 1, { { TEXT("Wood"), 250 }, { TEXT("MetalFragments"), 50 } });
		AddRecipe(TEXT("ArmoredLocker"), 1, 20, 2, { { TEXT("MetalFragments"), 300 }, { TEXT("HighQualityMetal"), 10 } }, 125);
		AddRecipe(TEXT("SmallFurnace"), 1, 10, 0, { { TEXT("Stone"), 200 }, { TEXT("Wood"), 100 }, { TEXT("LowGradeFuel"), 50 } });
		AddRecipe(TEXT("LargeFurnace"), 1, 25, 2, { { TEXT("Stone"), 500 }, { TEXT("Wood"), 200 }, { TEXT("LowGradeFuel"), 75 } }, 125);
		AddRecipe(TEXT("OilRefinery"), 1, 20, 1, { { TEXT("Wood"), 200 }, { TEXT("MetalFragments"), 500 }, { TEXT("LowGradeFuel"), 50 } }, 125);

		// Phase 2: Electricity, Lighting & Automated Defenses
		AddRecipe(TEXT("SolarPanel"), 1, 15, 1, { { TEXT("HighQualityMetal"), 5 }, { TEXT("MetalFragments"), 100 } }, 75);
		AddRecipe(TEXT("WindTurbine"), 1, 25, 2, { { TEXT("HighQualityMetal"), 15 }, { TEXT("MetalFragments"), 350 }, { TEXT("Wood"), 200 } }, 125);
		AddRecipe(TEXT("SmallBattery"), 1, 15, 1, { { TEXT("HighQualityMetal"), 5 } }, 75);
		AddRecipe(TEXT("LargeBattery"), 1, 25, 2, { { TEXT("HighQualityMetal"), 15 }, { TEXT("MetalFragments"), 200 } }, 125);
		AddRecipe(TEXT("ElectricSwitch"), 1, 5, 1, { { TEXT("HighQualityMetal"), 1 }, { TEXT("MetalFragments"), 25 } }, 20);
		AddRecipe(TEXT("ElectricSplitter"), 1, 5, 1, { { TEXT("HighQualityMetal"), 1 }, { TEXT("MetalFragments"), 50 } }, 20);
		AddRecipe(TEXT("TunaCanLamp"), 1, 5, 0, { { TEXT("MetalFragments"), 25 }, { TEXT("LowGradeFuel"), 10 } });
		AddRecipe(TEXT("IndustrialWallLight"), 1, 5, 1, { { TEXT("MetalFragments"), 40 } });
		AddRecipe(TEXT("CeilingFloodLight"), 1, 8, 1, { { TEXT("MetalFragments"), 60 }, { TEXT("HighQualityMetal"), 2 } });
		AddRecipe(TEXT("StandingWorkLamp"), 1, 8, 1, { { TEXT("MetalFragments"), 75 }, { TEXT("Wood"), 50 } });
		AddRecipe(TEXT("PoweredSearchLight"), 1, 15, 2, { { TEXT("MetalFragments"), 150 }, { TEXT("HighQualityMetal"), 5 } }, 75);
		AddRecipe(TEXT("ShotgunTrap"), 1, 20, 1, { { TEXT("Wood"), 300 }, { TEXT("MetalFragments"), 150 } }, 75);
		AddRecipe(TEXT("AutoTurret"), 1, 30, 2, { { TEXT("HighQualityMetal"), 40 }, { TEXT("MetalFragments"), 500 } }, 500);

		// Building Materials (Twig, Armored Tier 4, and High External Compound Walls)
		AddRecipe(TEXT("TwigFoundation"), 1, 2, 0, { { TEXT("Wood"), 50 } });
		AddRecipe(TEXT("TwigWall"), 1, 2, 0, { { TEXT("Wood"), 50 } });
		AddRecipe(TEXT("TwigFloor"), 1, 2, 0, { { TEXT("Wood"), 35 } });
		AddRecipe(TEXT("TwigDoorway"), 1, 2, 0, { { TEXT("Wood"), 40 } });
		AddRecipe(TEXT("TwigStairs"), 1, 2, 0, { { TEXT("Wood"), 50 } });
		AddRecipe(TEXT("ArmoredFoundation"), 1, 10, 2, { { TEXT("HighQualityMetal"), 25 }, { TEXT("MetalFragments"), 100 } });
		AddRecipe(TEXT("ArmoredTriangleFoundation"), 1, 8, 2, { { TEXT("HighQualityMetal"), 13 }, { TEXT("MetalFragments"), 50 } });
		AddRecipe(TEXT("ArmoredWall"), 1, 10, 2, { { TEXT("HighQualityMetal"), 25 }, { TEXT("MetalFragments"), 100 } });
		AddRecipe(TEXT("ArmoredDoorway"), 1, 10, 2, { { TEXT("HighQualityMetal"), 20 }, { TEXT("MetalFragments"), 75 } });
		AddRecipe(TEXT("ArmoredWindowWall"), 1, 10, 2, { { TEXT("HighQualityMetal"), 20 }, { TEXT("MetalFragments"), 75 } });
		AddRecipe(TEXT("ArmoredFloor"), 1, 8, 2, { { TEXT("HighQualityMetal"), 15 }, { TEXT("MetalFragments"), 75 } });
		AddRecipe(TEXT("ArmoredStairs"), 1, 10, 2, { { TEXT("HighQualityMetal"), 20 }, { TEXT("MetalFragments"), 100 } });
		AddRecipe(TEXT("ArmoredDoor"), 1, 15, 2, { { TEXT("HighQualityMetal"), 20 }, { TEXT("MetalFragments"), 150 } }, 125);
		AddRecipe(TEXT("HighExternalWoodWall"), 1, 15, 1, { { TEXT("Wood"), 500 } });
		AddRecipe(TEXT("HighExternalStoneWall"), 1, 20, 2, { { TEXT("Stone"), 500 } }, 125);

		// Phase 3: Medicals & Armor/Hazmat
		AddRecipe(TEXT("Bandage"), 1, 3, 0, { { TEXT("Cloth"), 4 } });
		AddRecipe(TEXT("MedicalSyringe"), 1, 8, 2, { { TEXT("Cloth"), 15 }, { TEXT("MetalFragments"), 10 }, { TEXT("LowGradeFuel"), 10 } }, 125);
		AddRecipe(TEXT("AntiRadPills"), 2, 5, 1, { { TEXT("Cloth"), 10 }, { TEXT("Charcoal"), 20 } }, 75);
		AddRecipe(TEXT("LargeMedkit"), 1, 10, 2, { { TEXT("MedicalSyringe"), 2 }, { TEXT("Bandage"), 2 }, { TEXT("LowGradeFuel"), 10 } }, 125);
		AddRecipe(TEXT("BurlapShirt"), 1, 5, 0, { { TEXT("Cloth"), 20 } });
		AddRecipe(TEXT("BurlapTrousers"), 1, 5, 0, { { TEXT("Cloth"), 20 } });
		AddRecipe(TEXT("HidePoncho"), 1, 8, 0, { { TEXT("Cloth"), 30 }, { TEXT("BoneFragments"), 20 } });
		AddRecipe(TEXT("HazmatSuit"), 1, 20, 2, { { TEXT("Cloth"), 50 }, { TEXT("HighQualityMetal"), 8 }, { TEXT("LowGradeFuel"), 20 } }, 125);
		AddRecipe(TEXT("RoadSignJacket"), 1, 15, 2, { { TEXT("Cloth"), 30 }, { TEXT("HighQualityMetal"), 4 }, { TEXT("MetalFragments"), 150 } }, 125);
		AddRecipe(TEXT("RoadSignKilt"), 1, 15, 2, { { TEXT("Cloth"), 25 }, { TEXT("HighQualityMetal"), 3 }, { TEXT("MetalFragments"), 100 } }, 125);
		AddRecipe(TEXT("MetalFacemask"), 1, 20, 3, { { TEXT("Cloth"), 15 }, { TEXT("HighQualityMetal"), 15 } }, 500);
		AddRecipe(TEXT("MetalChestplate"), 1, 25, 3, { { TEXT("Cloth"), 25 }, { TEXT("HighQualityMetal"), 25 } }, 500);

		// Phase 4: Weather Adaptation Deployables
		AddRecipe(TEXT("RainCollector"), 1, 15, 1, { { TEXT("Wood"), 250 }, { TEXT("Cloth"), 30 }, { TEXT("MetalFragments"), 50 } });
		AddRecipe(TEXT("WeatherRadar"), 1, 20, 2, { { TEXT("HighQualityMetal"), 10 }, { TEXT("MetalFragments"), 200 }, { TEXT("Scrap"), 100 } }, 125);
	}

	if (RecipeDataTable != nullptr)
	{
		// Already loaded
	}
}

void UCraftingComponent::RegisterRecipe(const FName& RecipeName, const FCraftingRecipe& RecipeData)
{
	EnsureDefaultPhase1RecipesRegistered();
	RegisteredRecipes.Add(RecipeName, RecipeData);
}

void UCraftingComponent::SetHighestAvailableWorkbenchTier(uint8 InTier)
{
	HighestAvailableWorkbenchTier = InTier;
}

uint8 UCraftingComponent::GetHighestAvailableWorkbenchTier() const
{
	return HighestAvailableWorkbenchTier;
}

void UCraftingComponent::SetProximityWorkbenchQuery(TFunction<uint8(const FVector&)> InQueryFunc)
{
	ProximityWorkbenchQueryFunc = MoveTemp(InQueryFunc);
}

uint8 UCraftingComponent::QueryProximityWorkbenchTier(const FVector& Location)
{
	if (ProximityWorkbenchQueryFunc)
	{
		return ProximityWorkbenchQueryFunc(Location);
	}
	return 0;
}

uint8 UCraftingComponent::GetEffectiveWorkbenchTier() const
{
	uint8 EffectiveTier = HighestAvailableWorkbenchTier;
	if (const AActor* OwnerActor = GetOwner())
	{
		EffectiveTier = FMath::Max(EffectiveTier, QueryProximityWorkbenchTier(OwnerActor->GetActorLocation()));
	}
	return EffectiveTier;
}

void UCraftingComponent::UnlockBlueprint(const FName& RecipeName)
{
	UnlockedBlueprints.Add(RecipeName);
}

bool UCraftingComponent::IsBlueprintUnlocked(const FName& RecipeName) const
{
	const FCraftingRecipe* RecipeData = GetRecipe(RecipeName);
	if (RecipeData == nullptr)
	{
		return false;
	}

	if (RecipeData->ResearchScrapCost <= 0)
	{
		return true;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && !Rules->bWorkbenchTierGatingEnabled)
	{
		return true;
	}

	return UnlockedBlueprints.Contains(RecipeName);
}

bool UCraftingComponent::ResearchRecipeWithScrap(const FName& RecipeName)
{
	const FCraftingRecipe* RecipeData = GetRecipe(RecipeName);
	if (RecipeData == nullptr)
	{
		return false;
	}

	if (IsBlueprintUnlocked(RecipeName))
	{
		return true;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	UInventoryComponent* OwnerInventory = OwnerActor->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventory == nullptr || OwnerInventory->GetContents() == nullptr)
	{
		return false;
	}

	const int32 ScrapRequired = RecipeData->ResearchScrapCost;
	if (OwnerInventory->GetContents()->GetItemQuantity(TEXT("Scrap")) < ScrapRequired)
	{
		return false;
	}

	FInventoryItem ScrapCostItem;
	ScrapCostItem.Name = TEXT("Scrap");
	ScrapCostItem.Quantity = ScrapRequired;
	OwnerInventory->RemoveItem(ScrapCostItem);

	UnlockBlueprint(RecipeName);
	return true;
}

bool UCraftingComponent::CanCraftRecipe(const FName& RecipeName) const
{
	const FCraftingRecipe* RecipeData = GetRecipe(RecipeName);
	if (RecipeData == nullptr)
	{
		return false;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && Rules->bCreativeFreeBuildEnabled)
	{
		return true;
	}

	if (RecipeData->RequiredWorkbenchTier > GetEffectiveWorkbenchTier())
	{
		return false;
	}

	if (!IsBlueprintUnlocked(RecipeName))
	{
		return false;
	}

	return true;
}

void UCraftingComponent::Server_CraftItem_Implementation(const FName& ItemToCraft)
{
	AActor* OwnerActor = GetOwner();
	FCraftingRecipe* RecipeData = GetRecipe(ItemToCraft);
	if (OwnerActor == nullptr || RecipeData == nullptr)
	{
		return;
	}

	if (!CanCraftRecipe(ItemToCraft))
	{
		UE_LOG(LogCrafting, Display, TEXT("Cannot craft %s: requires Workbench Tier %d (have Tier %d) and unlocked blueprint."),
			*ItemToCraft.ToString(), RecipeData->RequiredWorkbenchTier, GetEffectiveWorkbenchTier());
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = OwnerActor->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}
	
	const UServerRules* Rules = UServerRules::Get(this);
	const bool bFreeBuild = (Rules != nullptr && Rules->bCreativeFreeBuildEnabled);
	if (!bFreeBuild)
	{
		FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
		if (InventoryContents == nullptr)
		{
			return;
		}

		for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
		{
			const int32 IngredientAmountInInventory = InventoryContents->GetItemQuantity(Ingredient.Name);
			
			if (IngredientAmountInInventory < Ingredient.Quantity)
			{
				return;
			}
		}

		for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
		{
			OwnerInventoryComponent->RemoveItem(Ingredient);
		}
	}

	FInventoryItem Yield;
	Yield.Name = ItemToCraft;
	Yield.Quantity = RecipeData->YieldQuantity;

	OwnerInventoryComponent->AddItem(Yield);

	if (OnItemCrafted.IsBound())
	{
		OnItemCrafted.Broadcast(ItemToCraft);
	}
}

TArray<FName> UCraftingComponent::GetAllRecipes()
{
	EnsureDefaultPhase1RecipesRegistered();
	TArray<FName> Names;
	if (RecipeDataTable != nullptr)
	{
		Names = RecipeDataTable->GetRowNames();
	}
	for (const TPair<FName, FCraftingRecipe>& Pair : RegisteredRecipes)
	{
		Names.AddUnique(Pair.Key);
	}
	return Names;
}

FCraftingRecipe* UCraftingComponent::GetRecipe(const FName& RecipeName)
{
	EnsureDefaultPhase1RecipesRegistered();
	if (RecipeDataTable != nullptr)
	{
		static const FString ContextString(TEXT("Recipe Context"));
		if (FCraftingRecipe* TableRow = RecipeDataTable->FindRow<FCraftingRecipe>(RecipeName, ContextString, false))
		{
			return TableRow;
		}
	}

	return RegisteredRecipes.Find(RecipeName);
}
