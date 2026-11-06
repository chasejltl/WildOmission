// Copyright Telephone Studios. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Enums/ToolType.h"
#include "Actors/OreResourceNodes.h"
#include "Deployables/BuildingBlock.h"
#include "Components/InventoryComponent.h"
#include "Components/CraftingComponent.h"
#include "ServerRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionHarvestTierScalingTest,
	"WildOmission.Phase1.HarvestTierScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionHarvestTierScalingTest::RunTest(const FString& Parameters)
{
	// Verify tool tier multipliers (Hand=0.5x, Stone=1.0x, Metal=1.5x, Salvaged=2.0x)
	const int32 BaseYield = 100;
	const float HatchetBaseMult = 0.2f; // 20 base yield at 1.0x

	const int32 HandYield = FHarvestTierMath::CalculateScaledYield(BaseYield, HatchetBaseMult, EToolTier::Hand, EToolTier::Hand);
	const int32 StoneYield = FHarvestTierMath::CalculateScaledYield(BaseYield, HatchetBaseMult, EToolTier::Stone, EToolTier::Hand);
	const int32 MetalYield = FHarvestTierMath::CalculateScaledYield(BaseYield, HatchetBaseMult, EToolTier::Metal, EToolTier::Hand);
	const int32 SalvagedYield = FHarvestTierMath::CalculateScaledYield(BaseYield, HatchetBaseMult, EToolTier::Salvaged, EToolTier::Hand);

	TestEqual(TEXT("Hand tool tier yields 0.5x (10)"), HandYield, 10);
	TestEqual(TEXT("Stone tool tier yields 1.0x (20)"), StoneYield, 20);
	TestEqual(TEXT("Metal tool tier yields 1.5x (30)"), MetalYield, 30);
	TestEqual(TEXT("Salvaged tool tier yields 2.0x (40)"), SalvagedYield, 40);

	// Verify minimum tool tier gating (e.g. HQM node requires Metal pickaxe)
	const int32 BlockedHQM = FHarvestTierMath::CalculateScaledYield(5, 1.0f, EToolTier::Stone, EToolTier::Metal);
	const int32 AllowedHQM = FHarvestTierMath::CalculateScaledYield(5, 1.0f, EToolTier::Metal, EToolTier::Metal);

	TestEqual(TEXT("Stone tool cannot harvest Metal-gated HQM node"), BlockedHQM, 0);
	TestEqual(TEXT("Metal tool harvests Metal-gated HQM node with 1.5x scaling"), AllowedHQM, 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionBuildingTierProgressionTest,
	"WildOmission.Phase1.BuildingTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionBuildingTierProgressionTest::RunTest(const FString& Parameters)
{
	// Verify durability scaling across Twig -> Wood -> Stone -> SheetMetal -> Armored
	TestEqual(TEXT("Twig HP is 50"), FBuildingTierRules::GetTierMaxDurability(EBuildingTier::Twig), 50.0f);
	TestEqual(TEXT("Wood HP is 250"), FBuildingTierRules::GetTierMaxDurability(EBuildingTier::Wood), 250.0f);
	TestEqual(TEXT("Stone HP is 500"), FBuildingTierRules::GetTierMaxDurability(EBuildingTier::Stone), 500.0f);
	TestEqual(TEXT("SheetMetal HP is 1000"), FBuildingTierRules::GetTierMaxDurability(EBuildingTier::SheetMetal), 1000.0f);
	TestEqual(TEXT("Armored HP is 2000"), FBuildingTierRules::GetTierMaxDurability(EBuildingTier::Armored), 2000.0f);

	// Verify hand-tool breakability rules
	TestTrue(TEXT("Twig is breakable by hand tools"), FBuildingTierRules::IsBreakableByHandTools(EBuildingTier::Twig));
	TestTrue(TEXT("Wood is breakable by hand tools"), FBuildingTierRules::IsBreakableByHandTools(EBuildingTier::Wood));
	TestFalse(TEXT("Stone is immune to hand tools"), FBuildingTierRules::IsBreakableByHandTools(EBuildingTier::Stone));
	TestFalse(TEXT("SheetMetal is immune to hand tools"), FBuildingTierRules::IsBreakableByHandTools(EBuildingTier::SheetMetal));
	TestFalse(TEXT("Armored is immune to hand tools"), FBuildingTierRules::IsBreakableByHandTools(EBuildingTier::Armored));

	// Verify upgrade resource types
	TestEqual(TEXT("Wood tier costs Wood"), FBuildingTierRules::GetTierUpgradeCost(EBuildingTier::Wood).Name, FName(TEXT("Wood")));
	TestEqual(TEXT("Stone tier costs Stone"), FBuildingTierRules::GetTierUpgradeCost(EBuildingTier::Stone).Name, FName(TEXT("Stone")));
	TestEqual(TEXT("SheetMetal tier costs MetalFragments"), FBuildingTierRules::GetTierUpgradeCost(EBuildingTier::SheetMetal).Name, FName(TEXT("MetalFragments")));
	TestEqual(TEXT("Armored tier costs HighQualityMetal"), FBuildingTierRules::GetTierUpgradeCost(EBuildingTier::Armored).Name, FName(TEXT("HighQualityMetal")));

	// Verify structural stability calculation
	TestEqual(TEXT("Foundation (0 hops) has 1.0 stability"), FBuildingTierRules::CalculateStability(0), 1.0f);
	TestTrue(TEXT("7 unsupported hops collapses (< 0.05 stability)"), FBuildingTierRules::CalculateStability(7) < 0.05f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase1ItemAndRecipeRegistryTest,
	"WildOmission.Phase1.ItemsAndRecipes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase1ItemAndRecipeRegistryTest::RunTest(const FString& Parameters)
{
	const TArray<FName> RequiredItems = {
		TEXT("Wood"), TEXT("Stone"), TEXT("MetalOre"), TEXT("MetalFragments"),
		TEXT("HighQualityMetalOre"), TEXT("HighQualityMetal"), TEXT("SulfurOre"), TEXT("Sulfur"),
		TEXT("Charcoal"), TEXT("AnimalFat"), TEXT("BoneFragments"), TEXT("LowGradeFuel"), TEXT("Gunpowder"),
		TEXT("StoneHatchet"), TEXT("StonePickaxe"), TEXT("MetalHatchet"), TEXT("MetalPickaxe"),
		TEXT("WoodenSpear"), TEXT("StoneSpear"), TEXT("HuntingBow"), TEXT("Crossbow"), TEXT("HandmadeRevolver"),
		TEXT("WoodenArrow"), TEXT("BoneArrow"), TEXT("PistolBullet"), TEXT("RifleBullet")
	};

	for (const FName& ItemID : RequiredItems)
	{
		FItemData* Data = UInventoryComponent::GetItemData(ItemID);
		TestNotNull(FString::Printf(TEXT("Item %s is registered"), *ItemID.ToString()), Data);
	}

	FCraftingRecipe* WoodenArrowRecipe = UCraftingComponent::GetRecipe(TEXT("WoodenArrow"));
	FCraftingRecipe* PistolBulletRecipe = UCraftingComponent::GetRecipe(TEXT("PistolBullet"));
	FCraftingRecipe* RifleBulletRecipe = UCraftingComponent::GetRecipe(TEXT("RifleBullet"));

	TestNotNull(TEXT("WoodenArrow recipe exists"), WoodenArrowRecipe);
	TestNotNull(TEXT("PistolBullet recipe exists"), PistolBulletRecipe);
	TestNotNull(TEXT("RifleBullet recipe exists"), RifleBulletRecipe);

	if (WoodenArrowRecipe && PistolBulletRecipe && RifleBulletRecipe)
	{
		TestEqual(TEXT("WoodenArrow is hand-craftable (Tier 0)"), WoodenArrowRecipe->RequiredWorkbenchTier, static_cast<uint8>(0));
		TestEqual(TEXT("PistolBullet requires Workbench Tier 1"), PistolBulletRecipe->RequiredWorkbenchTier, static_cast<uint8>(1));
		TestEqual(TEXT("RifleBullet requires Workbench Tier 2"), RifleBulletRecipe->RequiredWorkbenchTier, static_cast<uint8>(2));
	}

	return true;
}
