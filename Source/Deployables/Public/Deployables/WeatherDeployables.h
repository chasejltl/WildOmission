// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Deployable.h"
#include "Components/ElectricNodeComponent.h"
#include "Interfaces/Interactable.h"
#include "WeatherDeployables.generated.h"

/**
 * Outdoor Rain Collector deployable (Community Dev Must-Have item).
 * Passively collects fresh drinking water during rainstorms and restores player Thirst.
 */
UCLASS()
class DEPLOYABLES_API ARainCollectorDeployable : public ADeployable, public IInteractable
{
	GENERATED_BODY()

public:
	ARainCollectorDeployable();

	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;

	void AccumulateRainwater(float RainDensity01, float DeltaSeconds);
	bool DrinkWater(class UVitalsComponent* TargetVitals, float DesiredDrinkAmount = 100.0f);

	float GetStoredWater() const { return StoredWaterUnits; }
	float GetMaxWaterCapacity() const { return MaxWaterCapacity; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Rain Collector")
	float MaxWaterCapacity;

	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Rain Collector")
	float StoredWaterUnits;
};

/**
 * Powered Weather Radar Station deployable (requires 5W electrical power).
 * Scans for active Storms, Tornadoes, and Extreme Weather Hazard Zones.
 */
UCLASS()
class DEPLOYABLES_API AWeatherRadarDeployable : public ADeployable
{
	GENERATED_BODY()

public:
	AWeatherRadarDeployable();

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }
	bool IsRadarOnline() const;

	int32 GetDetectedWeatherHazardCount() const;

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;
};
