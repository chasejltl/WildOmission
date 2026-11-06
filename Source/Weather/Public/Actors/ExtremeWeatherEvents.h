// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExtremeWeatherEvents.generated.h"

UENUM(BlueprintType)
enum class EExtremeWeatherType : uint8
{
	Clear = 0			UMETA(DisplayName = "Clear"),
	Thunderstorm = 1	UMETA(DisplayName = "Thunderstorm"),
	AcidRain = 2		UMETA(DisplayName = "Acid Rain"),
	Blizzard = 3		UMETA(DisplayName = "Blizzard"),
	Sandstorm = 4		UMETA(DisplayName = "Sandstorm")
};

USTRUCT(BlueprintType)
struct WEATHER_API FExtremeWeatherProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EExtremeWeatherType WeatherType = EExtremeWeatherType::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AmbientTemperatureCelsius = 21.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AcidCorrosionDamagePerSec = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AmbientRadiationPerSec = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float VisibilityMultiplier = 1.0f;

	static FExtremeWeatherProfile GetProfileForType(EExtremeWeatherType InType);
};

/**
 * Environmental weather hazard zone actor (Acid Rain, Blizzard, Sandstorm).
 */
UCLASS()
class WEATHER_API AExtremeWeatherHazardZone : public AActor
{
	GENERATED_BODY()

public:
	AExtremeWeatherHazardZone();

	void SetWeatherType(EExtremeWeatherType InType);
	EExtremeWeatherType GetWeatherType() const { return ActiveProfile.WeatherType; }
	const FExtremeWeatherProfile& GetWeatherProfile() const { return ActiveProfile; }

	/** Applies this weather event's temperature, acid corrosion, and radiation effects to TargetVitals. */
	void ApplyWeatherHazardToVitals(class UVitalsComponent* TargetVitals, float DeltaSeconds) const;

	static const TArray<AExtremeWeatherHazardZone*>& GetActiveWeatherHazardZones();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditAnywhere, Category = "Weather")
	FExtremeWeatherProfile ActiveProfile;
};
