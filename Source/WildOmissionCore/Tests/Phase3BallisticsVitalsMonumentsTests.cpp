// Copyright Telephone Studios. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Items/MilitaryFirearms.h"
#include "Deployables/BuildingBlock.h"
#include "Deployables/CropGenetics.h"
#include "Components/VitalsComponent.h"
#include "Clothing/Clothing.h"
#include "Structures/MonumentStructure.h"
#include "ServerRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase3BallisticsAndRaidingTest,
	"WildOmission.Phase3.BallisticsAndRaiding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase3BallisticsAndRaidingTest::RunTest(const FString& Parameters)
{
	AAssaultRifleItem* AK = NewObject<AAssaultRifleItem>();
	ABoltActionRifleItem* Bolt = NewObject<ABoltActionRifleItem>();
	ACustomSMGItem* SMG = NewObject<ACustomSMGItem>();

	TestEqual(TEXT("AssaultRifle RPM is 450"), AK->GetRateOfFire(), 450);
	TestEqual(TEXT("BoltActionRifle RPM is 35"), Bolt->GetRateOfFire(), 35);
	TestEqual(TEXT("CustomSMG RPM is 600"), SMG->GetRateOfFire(), 600);
	TestTrue(TEXT("BoltActionRifle has 1.6x muzzle velocity multiplier"), Bolt->GetMuzzleVelocityMultiplier() > 1.5f);

	// Verify S-curve recoil climbs vertically and oscillates horizontally
	const FVector2D Shot0 = AK->AdvanceBurstRecoil();
	const FVector2D Shot5 = FBallisticRecoilProfile::GetAssaultRifleRecoilForShot(5);
	const FVector2D Shot20 = FBallisticRecoilProfile::GetAssaultRifleRecoilForShot(20);
	TestTrue(TEXT("AssaultRifle vertical recoil is positive"), Shot0.X > 1.0f);
	TestTrue(TEXT("AssaultRifle S-curve sways right then left across magazine"), Shot5.Y > 0.0f && Shot20.Y < 0.0f);

	// Verify bullet drop decreases with higher muzzle velocity
	const float PistolDropAt100m = FBallisticRecoilProfile::CalculateBulletDropCm(10000.0f, 12000.0f);
	const float BoltDropAt100m = FBallisticRecoilProfile::CalculateBulletDropCm(10000.0f, 22000.0f * Bolt->GetMuzzleVelocityMultiplier());
	TestTrue(TEXT("High-velocity Bolt-Action rifle has significantly less drop at 100m than pistol cartridge"), BoltDropAt100m < PistolDropAt100m);

	// Verify C4 raiding math against Stone (500 HP) and SheetMetal (1000 HP)
	ATimedExplosiveChargeProjectile* C4 = NewObject<ATimedExplosiveChargeProjectile>();
	ABuildingBlock* StoneWall = NewObject<ABuildingBlock>();
	ABuildingBlock* MetalWall = NewObject<ABuildingBlock>();
	StoneWall->UpgradeToTier(EBuildingTier::Stone);
	MetalWall->UpgradeToTier(EBuildingTier::SheetMetal);

	C4->DetonateOnTarget(StoneWall, false);
	TestEqual(TEXT("1 C4 (550 dmg) destroys a Stone wall (500 HP)"), StoneWall->GetCurrentDurability(), 0.0f);

	C4->DetonateOnTarget(MetalWall, false);
	TestEqual(TEXT("1 C4 leaves SheetMetal wall (1000 HP) at 450 HP"), MetalWall->GetCurrentDurability(), 450.0f);
	C4->DetonateOnTarget(MetalWall, false);
	TestEqual(TEXT("2 C4 charges destroy a SheetMetal wall"), MetalWall->GetCurrentDurability(), 0.0f);

	// Verify Offline Raid Protection halves raid damage when enabled
	UServerRules* Rules = UServerRules::Get();
	const bool bPrevORP = Rules->bOfflineRaidProtection;
	Rules->bOfflineRaidProtection = true;
	const float OfflineDamage = FRaidExplosiveDamage::CalculateEffectiveRaidDamage(FRaidExplosiveDamage::C4StructuralDamage, true);
	TestEqual(TEXT("Offline Raid Protection halves 550 C4 damage to 275"), OfflineDamage, 275.0f);
	Rules->bOfflineRaidProtection = bPrevORP;

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase3VitalsMedicalAndClothingTest,
	"WildOmission.Phase3.VitalsMedicalAndClothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase3VitalsMedicalAndClothingTest::RunTest(const FString& Parameters)
{
	UVitalsComponent* Vitals = NewObject<UVitalsComponent>();
	Vitals->SetHealth(50.0f);

	// Unprotected player takes full radiation exposure
	Vitals->SetRadiationProtection(0.0f);
	Vitals->AddRadiationExposure(20.0f);
	TestEqual(TEXT("Unprotected player accumulates 20 rads"), Vitals->GetRadiation(), 20.0f);

	// Hazmat Suit (0.50 rad protection) completely blocks up to 25 ambient monument rads
	const FClothingProtectionStats HazmatStats = FClothingProtectionStats::GetPresetForItem(TEXT("HazmatSuit"));
	TestEqual(TEXT("HazmatSuit provides 0.50 radiation protection"), HazmatStats.RadiationProtection, 0.50f);
	Vitals->SetRadiationProtection(HazmatStats.RadiationProtection);
	Vitals->AddRadiationExposure(20.0f);
	TestEqual(TEXT("HazmatSuit blocks 20 rad monument exposure completely"), Vitals->GetRadiation(), 20.0f);

	// Bleeding & Bandage / MedicalSyringe / AntiRadPills
	Vitals->AddBleeding(25.0f);
	TestEqual(TEXT("Player has 25 bleeding"), Vitals->GetBleeding(), 25.0f);

	Vitals->ApplyMedicalItem(TEXT("Bandage"));
	TestEqual(TEXT("Bandage clears bleeding to 0"), Vitals->GetBleeding(), 0.0f);
	TestEqual(TEXT("Bandage heals +5 HP (50 -> 55)"), Vitals->GetHealth(), 55.0f);

	Vitals->ApplyMedicalItem(TEXT("MedicalSyringe"));
	TestEqual(TEXT("MedicalSyringe heals +35 HP (55 -> 90)"), Vitals->GetHealth(), 90.0f);
	TestEqual(TEXT("MedicalSyringe reduces radiation by 10 (20 -> 10)"), Vitals->GetRadiation(), 10.0f);

	Vitals->ApplyMedicalItem(TEXT("AntiRadPills"));
	TestEqual(TEXT("AntiRadPills purges remaining radiation to 0"), Vitals->GetRadiation(), 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWildOmissionPhase3CropGeneticsAndMonumentsTest,
	"WildOmission.Phase3.CropGeneticsAndMonuments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter
)

bool FWildOmissionPhase3CropGeneticsAndMonumentsTest::RunTest(const FString& Parameters)
{
	// Verify 6-gene parsing & crossbreeding dominance (2 positive G/Y alleles at 0.6+0.6=1.2 beat 1 negative W/X allele at 1.0)
	const FCropGeneSequence CenterPlant = FCropGeneSequence::FromString(TEXT("WXWXWX"));
	const TArray<FCropGeneSequence> Neighbors = {
		FCropGeneSequence::FromString(TEXT("GGGYYY")),
		FCropGeneSequence::FromString(TEXT("GGGYYY"))
	};

	const FCropGeneSequence BredPlant = CenterPlant.CrossbreedWithNeighbors(Neighbors);
	TestEqual(TEXT("Two GGGYYY neighbors out-compete WXWXWX to breed GGGYYY"), BredPlant.ToString(), FString(TEXT("GGGYYY")));

	AHempCropSeedling* Hemp = NewObject<AHempCropSeedling>();
	Hemp->SetGeneSequence(BredPlant); // 3 Y genes -> 1.0 + 3*0.25 = 1.75x yield (40 * 1.75 = 70 Cloth)
	const FInventoryItem HempYield = Hemp->CalculateHarvestYield();
	TestEqual(TEXT("Hemp harvests Cloth"), HempYield.Name, FName(TEXT("Cloth")));
	TestEqual(TEXT("GGGYYY Hemp yields 70 Cloth (1.75x of 40)"), HempYield.Quantity, 70);

	// Verify Monuments & 15-minute Hackable Locked Crate
	AAirfieldMonument* Airfield = NewObject<AAirfieldMonument>();
	APowerPlantMonument* PowerPlant = NewObject<APowerPlantMonument>();
	AMilitaryTunnelsMonument* MilTunnels = NewObject<AMilitaryTunnelsMonument>();

	TestTrue(TEXT("MilitaryTunnels has higher radiation than PowerPlant and Airfield"),
		MilTunnels->GetRadiationRadsPerSecond() > PowerPlant->GetRadiationRadsPerSecond() &&
		PowerPlant->GetRadiationRadsPerSecond() > Airfield->GetRadiationRadsPerSecond());

	FHackableLockedCrateState& Crate = MilTunnels->GetLockedCrateState();
	TestFalse(TEXT("Locked crate starts locked"), Crate.IsUnlocked());
	Crate.StartHacking();
	Crate.AdvanceHackTimer(450.0f);
	TestFalse(TEXT("Locked crate still locked after 7.5 minutes"), Crate.IsUnlocked());
	Crate.AdvanceHackTimer(450.0f);
	TestTrue(TEXT("Locked crate unlocks after full 15 minutes (900s)"), Crate.IsUnlocked());

	const TArray<FInventoryItem> Tier3Loot = MilTunnels->GenerateLockedCrateLoot();
	TestTrue(TEXT("MilitaryTunnels locked crate spawns high-tier loot"), Tier3Loot.Num() >= 5);

	return true;
}
