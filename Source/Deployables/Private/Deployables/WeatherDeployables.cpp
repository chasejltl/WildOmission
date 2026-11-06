// Copyright Telephone Studios. All Rights Reserved.

#include "Deployables/WeatherDeployables.h"
#include "Components/VitalsComponent.h"
#include "Actors/ExtremeWeatherEvents.h"

ARainCollectorDeployable::ARainCollectorDeployable()
{
	MaxDurability = 250.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("RainCollector");
	Identifier = TEXT("RainCollector");

	MaxWaterCapacity = 500.0f;
	StoredWaterUnits = 0.0f;
}

void ARainCollectorDeployable::AccumulateRainwater(float RainDensity01, float DeltaSeconds)
{
	if (RainDensity01 <= 0.0f || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const float AddedWater = (RainDensity01 * 2.5f) * DeltaSeconds;
	StoredWaterUnits = FMath::Clamp(StoredWaterUnits + AddedWater, 0.0f, MaxWaterCapacity);
}

bool ARainCollectorDeployable::DrinkWater(UVitalsComponent* TargetVitals, float DesiredDrinkAmount)
{
	if (TargetVitals == nullptr || StoredWaterUnits <= 0.0f || DesiredDrinkAmount <= 0.0f)
	{
		return false;
	}

	const float ConsumedWater = FMath::Min(StoredWaterUnits, DesiredDrinkAmount);
	StoredWaterUnits -= ConsumedWater;
	TargetVitals->AddThirst(ConsumedWater);
	return true;
}

void ARainCollectorDeployable::Interact(AActor* Interactor)
{
	if (Interactor == nullptr)
	{
		return;
	}

	if (UVitalsComponent* Vitals = Interactor->FindComponentByClass<UVitalsComponent>())
	{
		DrinkWater(Vitals, 100.0f);
	}
}

FString ARainCollectorDeployable::PromptText()
{
	return FString::Printf(TEXT("drink fresh water (%d / %d ml)"), FMath::RoundToInt(StoredWaterUnits), FMath::RoundToInt(MaxWaterCapacity));
}

AWeatherRadarDeployable::AWeatherRadarDeployable()
{
	MaxDurability = 400.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("WeatherRadar");
	Identifier = TEXT("WeatherRadar");

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Consumer, 0, 5);
}

bool AWeatherRadarDeployable::IsRadarOnline() const
{
	return ElectricNode != nullptr && ElectricNode->IsPowered();
}

int32 AWeatherRadarDeployable::GetDetectedWeatherHazardCount() const
{
	if (!IsRadarOnline())
	{
		return 0;
	}

	return AExtremeWeatherHazardZone::GetActiveWeatherHazardZones().Num();
}
