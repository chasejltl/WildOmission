// Copyright Telephone Studios. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Actors/ExtremeWeatherEvents.h"
#include "Characters/ScientistAICharacter.h"
#include "Components/VitalsComponent.h"
#include "Deployables/BuildingBlock.h"
#include "Deployables/WeatherDeployables.h"
#include "ServerRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhase4WeatherAndTemperatureTest,
	"WildOmission.Phase4.WeatherAndTemperature",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPhase4WeatherAndTemperatureTest::RunTest(const FString& Parameters)
{
	UServerRules* Rules = UServerRules::Get(nullptr);
	if (Rules)
	{
		Rules->ResetToDefaults();
	}

	UVitalsComponent* Vitals = NewObject<UVitalsComponent>();
	TestNotNull(TEXT("VitalsComponent created"), Vitals);
	TestEqual(TEXT("Default body temperature is 37.0C"), Vitals->GetBodyTemperature(), 37.0f);
	TestFalse(TEXT("Not hypothermic at 37C"), Vitals->IsHypothermic());
	TestFalse(TEXT("Not hyperthermic at 37C"), Vitals->IsHyperthermic());

	// Expose naked survivor to Blizzard (-18C) for 20 seconds -> drops temperature into hypothermia (< 35C) and drains health
	const FExtremeWeatherProfile BlizzardProfile = FExtremeWeatherProfile::GetProfileForType(EExtremeWeatherType::Blizzard);
	TestEqual(TEXT("Blizzard ambient temperature is -18C"), BlizzardProfile.AmbientTemperatureCelsius, -18.0f);

	Vitals->ApplyAmbientTemperature(BlizzardProfile.AmbientTemperatureCelsius, 20.0f);
	TestTrue(TEXT("Naked survivor in Blizzard becomes hypothermic (< 35C)"), Vitals->IsHypothermic());
	TestTrue(TEXT("Hypothermia drains health below 100"), Vitals->GetHealth() < 100.0f);

	// Enter shelter / stand near campfire -> warms back to 37C
	const float ColdTemp = Vitals->GetBodyTemperature();
	Vitals->SetShelteredOrNearFire(true);
	Vitals->ApplyAmbientTemperature(BlizzardProfile.AmbientTemperatureCelsius, 20.0f);
	TestTrue(TEXT("Shelter/Campfire warms survivor back toward 37C"), Vitals->GetBodyTemperature() > ColdTemp);
	TestEqual(TEXT("20s by campfire restores 37C homeostasis"), Vitals->GetBodyTemperature(), 37.0f);

	// Acid Rain hazard zone corrodes unsheltered players & irradiates them
	Vitals->SetShelteredOrNearFire(false);
	Vitals->SetHealth(100.0f);
	AExtremeWeatherHazardZone* AcidZone = NewObject<AExtremeWeatherHazardZone>();
	AcidZone->SetWeatherType(EExtremeWeatherType::AcidRain);
	AcidZone->ApplyWeatherHazardToVitals(Vitals, 4.0f);
	TestEqual(TEXT("4s of Acid Rain (2.5 HP/s) deals 10.0 corrosion damage unsheltered"), Vitals->GetHealth(), 90.0f);
	TestTrue(TEXT("Acid Rain adds ambient radiation"), Vitals->GetRadiation() > 0.0f);

	// Rain Collector accumulates water during storm and restores Thirst
	ARainCollectorDeployable* RainCollector = NewObject<ARainCollectorDeployable>();
	TestEqual(TEXT("Rain Collector starts empty"), RainCollector->GetStoredWater(), 0.0f);
	RainCollector->AccumulateRainwater(1.0f, 10.0f);
	TestTrue(TEXT("Rain Collector accumulates water in storm"), RainCollector->GetStoredWater() > 0.0f);

	Vitals->SetThirst(40.0f);
	const bool bDrank = RainCollector->DrinkWater(Vitals, 50.0f);
	TestTrue(TEXT("DrinkWater succeeds when water is stored"), bDrank);
	TestTrue(TEXT("Drinking collected rainwater restores player Thirst"), Vitals->GetThirst() > 40.0f);

	// Weather Radar requires 5W electrical power
	AWeatherRadarDeployable* Radar = NewObject<AWeatherRadarDeployable>();
	TestFalse(TEXT("Unpowered Weather Radar is offline"), Radar->IsRadarOnline());
	UElectricNodeComponent* GeneratorNode = NewObject<UElectricNodeComponent>();
	GeneratorNode->ConfigureNode(EElectricNodeType::Generator, 20, 0);
	GeneratorNode->ConnectOutputTo(Radar->GetElectricNode());
	TestTrue(TEXT("Weather Radar comes online with 5W power"), Radar->IsRadarOnline());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhase4TornadoStructuralWindStrippingTest,
	"WildOmission.Phase4.TornadoStructuralWindStripping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPhase4TornadoStructuralWindStrippingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Twig wind damage multiplier is 3.0x"), FBuildingTierRules::GetWindDamageMultiplierForTier(EBuildingTier::Twig), 3.0f);
	TestEqual(TEXT("Wood wind damage multiplier is 1.5x"), FBuildingTierRules::GetWindDamageMultiplierForTier(EBuildingTier::Wood), 1.5f);
	TestEqual(TEXT("Stone wind damage multiplier is 0.25x"), FBuildingTierRules::GetWindDamageMultiplierForTier(EBuildingTier::Stone), 0.25f);
	TestEqual(TEXT("SheetMetal wind damage multiplier is 0.05x"), FBuildingTierRules::GetWindDamageMultiplierForTier(EBuildingTier::SheetMetal), 0.05f);
	TestEqual(TEXT("Armored wind damage multiplier is 0.0x (immune to Tornado)"), FBuildingTierRules::GetWindDamageMultiplierForTier(EBuildingTier::Armored), 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhase4ScientistAndPatrolHelicopterAITest,
	"WildOmission.Phase4.ScientistAndPatrolHelicopterAI",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPhase4ScientistAndPatrolHelicopterAITest::RunTest(const FString& Parameters)
{
	UServerRules* Rules = UServerRules::Get(nullptr);
	if (Rules)
	{
		Rules->ResetToDefaults();
	}

	AScientistAICharacter* Scientist = NewObject<AScientistAICharacter>();
	TestNotNull(TEXT("ScientistAICharacter created"), Scientist);

	// 1. Direct LOS within range -> EngageRanged
	TestEqual(
		TEXT("Scientist engages ranged when target in sight"),
		Scientist->EvaluateTacticalState(1.0f, true, 1800.0f, 0.0f),
		EScientistTacticalState::EngageRanged);

	// 2. Target ducks behind cover within 25m -> ThrowGrenade and decrement grenade count
	const int32 InitialGrenades = Scientist->RemainingGrenades;
	TestEqual(
		TEXT("Scientist throws grenade when target ducks behind cover"),
		Scientist->EvaluateTacticalState(0.85f, false, 1600.0f, 3.0f),
		EScientistTacticalState::ThrowGrenade);
	TestEqual(TEXT("Scientist grenade count decremented"), Scientist->RemainingGrenades, InitialGrenades - 1);

	// 3. Critical health (<= 35%) -> HealWithSyringe and decrement syringe count
	const int32 InitialSyringes = Scientist->RemainingSyringes;
	TestEqual(
		TEXT("Scientist heals with Medical Syringe when critically wounded"),
		Scientist->EvaluateTacticalState(0.25f, false, 2000.0f, 4.0f),
		EScientistTacticalState::HealWithSyringe);
	TestEqual(TEXT("Scientist syringe count decremented"), Scientist->RemainingSyringes, InitialSyringes - 1);

	// 4. Target hidden >= 8s -> FlankTarget
	TestEqual(
		TEXT("Scientist flanks target after 10s out of sight"),
		Scientist->EvaluateTacticalState(0.9f, false, 2200.0f, 10.0f),
		EScientistTacticalState::FlankTarget);

	// 5. Sound perception within HearingRadiusCm
	TestTrue(TEXT("Scientist hears nearby gunshot"), Scientist->ReportHeardSound(FVector(1000.0f, 0.0f, 0.0f), 1.0f));
	TestFalse(TEXT("Scientist ignores distant quiet footstep"), Scientist->ReportHeardSound(FVector(5000.0f, 0.0f, 0.0f), 0.2f));

	// 6. Patrol Helicopter threat rules & weakspot destruction
	APatrolHelicopterAI* Heli = NewObject<APatrolHelicopterAI>();
	TestFalse(TEXT("Patrol Helicopter ignores naked/primitive player (0 ranged, 2 clothing)"), Heli->IsPlayerHostileThreat(0, 2));
	TestTrue(TEXT("Patrol Helicopter engages player with ranged weapon"), Heli->IsPlayerHostileThreat(1, 0));
	TestTrue(TEXT("Patrol Helicopter engages geared player (>2 clothing pieces)"), Heli->IsPlayerHostileThreat(0, 3));

	// Destroy Main Rotor (1000 HP) and Tail Rotor (500 HP) weakspots -> crashes helicopter
	Heli->ApplyHelicopterDamage(1000.0f, true, false);
	TestFalse(TEXT("Helicopter still airborne with only main rotor destroyed"), Heli->bIsCrashed);
	Heli->ApplyHelicopterDamage(500.0f, false, true);
	TestTrue(TEXT("Helicopter crashes when both rotors are destroyed"), Heli->bIsCrashed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
