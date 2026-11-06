// Copyright Telephone Studios. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/InventoryComponent.h"
#include "Components/CraftingComponent.h"
#include "Components/ElectricNodeComponent.h"
#include "Deployables/WorkbenchDeployable.h"
#include "Deployables/IndustrialDeployables.h"
#include "Deployables/ElectricalDeployables.h"
#include "Deployables/ToolCupboard.h"
#include "Deployables/BuildingBlock.h"
#include "Locks/KeyLock.h"
#include "ServerRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase2WorkbenchesAndResearchTest,
	"WildOmission.Phase2.WorkbenchesAndResearch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase2WorkbenchesAndResearchTest::RunTest(const FString& Parameters)
{
	const TArray<FName> Phase2Items = {
		TEXT("Scrap"), TEXT("CrudeOil"),
		TEXT("WorkbenchT1"), TEXT("WorkbenchT2"), TEXT("WorkbenchT3"), TEXT("ResearchTable"),
		TEXT("KeyLock"), TEXT("CodeLock"), TEXT("LargeStorageBox"),
		TEXT("SmallFurnace"), TEXT("LargeFurnace"), TEXT("OilRefinery"),
		TEXT("SolarPanel"), TEXT("SmallBattery"), TEXT("ElectricSwitch"), TEXT("ElectricSplitter"),
		TEXT("AutoTurret"), TEXT("ShotgunTrap")
	};

	for (const FName& ItemID : Phase2Items)
	{
		TestNotNull(FString::Printf(TEXT("Phase 2 item %s registered"), *ItemID.ToString()), UInventoryComponent::GetItemData(ItemID));
	}

	UCraftingComponent* CraftingComp = NewObject<UCraftingComponent>();
	TestNotNull(TEXT("Created UCraftingComponent"), CraftingComp);

	// AutoTurret requires Workbench Tier 2 and a 500-Scrap blueprint unlock
	CraftingComp->SetHighestAvailableWorkbenchTier(1);
	TestFalse(TEXT("Cannot craft AutoTurret at Workbench Tier 1"), CraftingComp->CanCraftRecipe(TEXT("AutoTurret")));

	CraftingComp->SetHighestAvailableWorkbenchTier(2);
	TestFalse(TEXT("Cannot craft AutoTurret at Tier 2 before unlocking blueprint"), CraftingComp->CanCraftRecipe(TEXT("AutoTurret")));

	CraftingComp->UnlockBlueprint(TEXT("AutoTurret"));
	TestTrue(TEXT("Can craft AutoTurret at Tier 2 after unlocking blueprint"), CraftingComp->CanCraftRecipe(TEXT("AutoTurret")));

	// Verify Industrial deployable defaults
	ALargeStorageBox* LargeBox = NewObject<ALargeStorageBox>();
	ASmallFurnaceDeployable* SmallFurnace = NewObject<ASmallFurnaceDeployable>();
	ALargeFurnaceDeployable* LargeFurnace = NewObject<ALargeFurnaceDeployable>();
	AOilRefineryDeployable* Refinery = NewObject<AOilRefineryDeployable>();

	TestEqual(TEXT("LargeStorageBox has 30 slots"), LargeBox->GetInventoryComponent()->GetSlotCount(), static_cast<uint8>(30));
	TestEqual(TEXT("LargeFurnace has 18 slots"), LargeFurnace->GetInventoryComponent()->GetSlotCount(), static_cast<uint8>(18));
	TestTrue(TEXT("LargeFurnace smelts 2x faster than SmallFurnace"), LargeFurnace->GetSmeltTimeSeconds() < SmallFurnace->GetSmeltTimeSeconds());
	TestEqual(TEXT("OilRefinery has 1 smelt rule (CrudeOil -> LowGradeFuel)"), Refinery->GetSmeltables().Num(), 1);
	if (Refinery->GetSmeltables().Num() == 1)
	{
		TestEqual(TEXT("OilRefinery input is CrudeOil"), Refinery->GetSmeltables()[0].RawItem.Name, FName(TEXT("CrudeOil")));
		TestEqual(TEXT("OilRefinery output is LowGradeFuel"), Refinery->GetSmeltables()[0].SmeltedItem.Name, FName(TEXT("LowGradeFuel")));
		TestEqual(TEXT("OilRefinery yields 3 LowGradeFuel per CrudeOil"), Refinery->GetSmeltables()[0].SmeltedItem.Quantity, 3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase2ToolCupboardUpkeepTest,
	"WildOmission.Phase2.ToolCupboardUpkeep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase2ToolCupboardUpkeepTest::RunTest(const FString& Parameters)
{
	AToolCupboard* TC = NewObject<AToolCupboard>();
	ABuildingBlock* WoodBlock = NewObject<ABuildingBlock>();
	ABuildingBlock* StoneBlock = NewObject<ABuildingBlock>();
	ABuildingBlock* MetalBlock = NewObject<ABuildingBlock>();
	ABuildingBlock* ArmoredBlock = NewObject<ABuildingBlock>();

	WoodBlock->UpgradeToTier(EBuildingTier::Wood);
	StoneBlock->UpgradeToTier(EBuildingTier::Stone);
	MetalBlock->UpgradeToTier(EBuildingTier::SheetMetal);
	ArmoredBlock->UpgradeToTier(EBuildingTier::Armored);

	TC->RegisterManagedBuildingBlock(WoodBlock);
	TC->RegisterManagedBuildingBlock(StoneBlock);
	TC->RegisterManagedBuildingBlock(MetalBlock);
	TC->RegisterManagedBuildingBlock(ArmoredBlock);

	// Authorization checks
	TC->AuthorizePlayer(TEXT("Player_Owner"));
	TestTrue(TEXT("Player_Owner is authorized on TC"), TC->IsPlayerAuthorized(TEXT("Player_Owner")));
	TestFalse(TEXT("Player_Raider is not authorized on TC"), TC->IsPlayerAuthorized(TEXT("Player_Raider")));

	// 4 blocks -> 10% bracket rate:
	// Wood (150 * 0.10 = 15), Stone (200 * 0.10 = 20), SheetMetal (150 * 0.10 = 15), Armored (20 * 0.10 = 2)
	const TMap<FName, int32> DailyUpkeep = TC->CalculateDailyUpkeepCost();
	TestEqual(TEXT("Daily Wood upkeep is 15"), DailyUpkeep.FindRef(TEXT("Wood")), 15);
	TestEqual(TEXT("Daily Stone upkeep is 20"), DailyUpkeep.FindRef(TEXT("Stone")), 20);
	TestEqual(TEXT("Daily MetalFragments upkeep is 15"), DailyUpkeep.FindRef(TEXT("MetalFragments")), 15);
	TestEqual(TEXT("Daily HighQualityMetal upkeep is 2"), DailyUpkeep.FindRef(TEXT("HighQualityMetal")), 2);

	// Stock exact 24h upkeep in TC inventory and run a 24h cycle
	UInventoryComponent* TCInv = TC->GetUpkeepInventory();
	FInventoryItem AddWood{ TEXT("Wood"), 15 };
	FInventoryItem AddStone{ TEXT("Stone"), 20 };
	FInventoryItem AddMetal{ TEXT("MetalFragments"), 15 };
	FInventoryItem AddHQM{ TEXT("HighQualityMetal"), 2 };
	TCInv->AddItem(AddWood);
	TCInv->AddItem(AddStone);
	TCInv->AddItem(AddMetal);
	TCInv->AddItem(AddHQM);

	TestTrue(TEXT("TC reports 24 hours of upkeep protection"), FMath::IsNearlyEqual(TC->GetProtectedHoursRemaining(), 24.0f, 0.1f));
	const bool bPaidFullDay = TC->ProcessUpkeepCycle(24.0f);
	TestTrue(TEXT("24h upkeep cycle paid without starvation"), bPaidFullDay);
	TestEqual(TEXT("WoodBlock remains at full 250 HP after paid upkeep"), WoodBlock->GetCurrentDurability(), 250.0f);
	TestEqual(TEXT("StoneBlock remains at full 500 HP after paid upkeep"), StoneBlock->GetCurrentDurability(), 500.0f);

	// Run 1 hour of starvation decay with empty TC inventory
	const bool bStarvingCycle = TC->ProcessUpkeepCycle(1.0f);
	TestFalse(TEXT("Empty TC reports starvation on upkeep cycle"), bStarvingCycle);
	TestTrue(TEXT("Starving WoodBlock took decay damage (< 250 HP)"), WoodBlock->GetCurrentDurability() < 250.0f);
	TestTrue(TEXT("Starving StoneBlock took decay damage (< 500 HP)"), StoneBlock->GetCurrentDurability() < 500.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase2ElectricityAndTrapsTest,
	"WildOmission.Phase2.ElectricityAndTraps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase2ElectricityAndTrapsTest::RunTest(const FString& Parameters)
{
	ASolarPanelDeployable* Solar = NewObject<ASolarPanelDeployable>();
	ASmallBatteryDeployable* Battery = NewObject<ASmallBatteryDeployable>();
	AElectricSwitchDeployable* Switch = NewObject<AElectricSwitchDeployable>();
	AElectricSplitterDeployable* Splitter = NewObject<AElectricSplitterDeployable>();
	AAutoTurretDeployable* Turret1 = NewObject<AAutoTurretDeployable>();
	AAutoTurretDeployable* Turret2 = NewObject<AAutoTurretDeployable>();
	AShotgunTrapDeployable* ShotgunTrap = NewObject<AShotgunTrapDeployable>();

	// Wire: Solar (20W) -> Battery (10W out) -> Switch -> Splitter -> Turret1 (needs 10W)
	Solar->GetElectricNode()->ConnectOutputTo(Battery->GetElectricNode(), 0.0f);
	Battery->GetElectricNode()->ConnectOutputTo(Switch->GetElectricNode(), 0.0f);
	Switch->GetElectricNode()->ConnectOutputTo(Splitter->GetElectricNode(), 0.0f);
	Splitter->GetElectricNode()->ConnectOutputTo(Turret1->GetElectricNode(), 0.0f);

	TestTrue(TEXT("Turret1 is powered when receiving 10W through Switch and 1-branch Splitter"), Turret1->IsTurretActive());

	// Toggle switch OFF -> Turret1 loses power
	Switch->GetElectricNode()->SetSwitchState(false);
	TestFalse(TEXT("Turret1 powers down when Switch is turned OFF"), Turret1->IsTurretActive());

	// Toggle switch back ON -> Turret1 regains power
	Switch->GetElectricNode()->SetSwitchState(true);
	TestTrue(TEXT("Turret1 regains power when Switch is turned ON"), Turret1->IsTurretActive());

	// Connect second turret to Splitter -> 10W Battery split 2 ways = 5W each (< 10W required)
	Splitter->GetElectricNode()->ConnectOutputTo(Turret2->GetElectricNode(), 0.0f);
	TestFalse(TEXT("Turret1 brown-outs when 10W battery is split across 2 turrets (5W each)"), Turret1->IsTurretActive());

	// Bypass 10W battery and feed 20W Solar Panel into Switch -> Splitter -> 10W to each turret!
	Battery->GetElectricNode()->DisconnectAll();
	Solar->GetElectricNode()->DisconnectAll();
	Solar->GetElectricNode()->ConnectOutputTo(Switch->GetElectricNode(), 0.0f);
	TestTrue(TEXT("Turret1 is powered when 20W Solar is split across 2 turrets (10W each)"), Turret1->IsTurretActive());
	TestTrue(TEXT("Turret2 is powered when 20W Solar is split across 2 turrets (10W each)"), Turret2->IsTurretActive());

	// Verify target authorization on AutoTurret and ShotgunTrap
	ABuildingBlock* DummyTarget = NewObject<ABuildingBlock>();
	DummyTarget->UpgradeToTier(EBuildingTier::Stone); // 500 HP

	Turret1->AuthorizePlayer(TEXT("Friend_01"));
	ShotgunTrap->AuthorizePlayer(TEXT("Friend_01"));

	TestFalse(TEXT("AutoTurret ignores authorized player"), Turret1->TryEngageTarget(DummyTarget, TEXT("Friend_01")));
	TestFalse(TEXT("ShotgunTrap ignores authorized player"), ShotgunTrap->TryTriggerTrap(DummyTarget, TEXT("Friend_01")));
	TestEqual(TEXT("Target took 0 damage from friendly traps"), DummyTarget->GetCurrentDurability(), 500.0f);

	TestTrue(TEXT("AutoTurret engages unauthorized intruder"), Turret1->TryEngageTarget(DummyTarget, TEXT("Raider_99")));
	TestEqual(TEXT("AutoTurret dealt 35 damage to intruder"), DummyTarget->GetCurrentDurability(), 465.0f);

	TestTrue(TEXT("ShotgunTrap triggers on unauthorized intruder"), ShotgunTrap->TryTriggerTrap(DummyTarget, TEXT("Raider_99")));
	TestEqual(TEXT("ShotgunTrap dealt 120 buckshot damage to intruder"), DummyTarget->GetCurrentDurability(), 345.0f);

	return true;
}
