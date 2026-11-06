// Copyright Telephone Studios. All Rights Reserved.

#include "Actors/ExtremeWeatherEvents.h"
#include "Components/VitalsComponent.h"

static TArray<AExtremeWeatherHazardZone*> ActiveWeatherHazardZones;

FExtremeWeatherProfile FExtremeWeatherProfile::GetProfileForType(EExtremeWeatherType InType)
{
	FExtremeWeatherProfile Profile;
	Profile.WeatherType = InType;

	switch (InType)
	{
	case EExtremeWeatherType::AcidRain:
		Profile.AmbientTemperatureCelsius = 14.0f;
		Profile.AcidCorrosionDamagePerSec = 2.5f;
		Profile.AmbientRadiationPerSec = 1.5f;
		Profile.VisibilityMultiplier = 0.60f;
		break;

	case EExtremeWeatherType::Blizzard:
		Profile.AmbientTemperatureCelsius = -18.0f;
		Profile.AcidCorrosionDamagePerSec = 0.0f;
		Profile.AmbientRadiationPerSec = 0.0f;
		Profile.VisibilityMultiplier = 0.25f;
		break;

	case EExtremeWeatherType::Sandstorm:
		Profile.AmbientTemperatureCelsius = 44.0f;
		Profile.AcidCorrosionDamagePerSec = 0.0f;
		Profile.AmbientRadiationPerSec = 0.0f;
		Profile.VisibilityMultiplier = 0.20f;
		break;

	case EExtremeWeatherType::Thunderstorm:
		Profile.AmbientTemperatureCelsius = 12.0f;
		Profile.AcidCorrosionDamagePerSec = 0.0f;
		Profile.AmbientRadiationPerSec = 0.0f;
		Profile.VisibilityMultiplier = 0.70f;
		break;

	case EExtremeWeatherType::Clear:
	default:
		Profile.AmbientTemperatureCelsius = 21.0f;
		Profile.AcidCorrosionDamagePerSec = 0.0f;
		Profile.AmbientRadiationPerSec = 0.0f;
		Profile.VisibilityMultiplier = 1.0f;
		break;
	}

	return Profile;
}

AExtremeWeatherHazardZone::AExtremeWeatherHazardZone()
{
	ActiveProfile = FExtremeWeatherProfile::GetProfileForType(EExtremeWeatherType::Clear);
	ActiveWeatherHazardZones.AddUnique(this);
}

void AExtremeWeatherHazardZone::BeginPlay()
{
	Super::BeginPlay();
	ActiveWeatherHazardZones.AddUnique(this);
}

void AExtremeWeatherHazardZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveWeatherHazardZones.Remove(this);
	Super::EndPlay(EndPlayReason);
}

void AExtremeWeatherHazardZone::SetWeatherType(EExtremeWeatherType InType)
{
	ActiveProfile = FExtremeWeatherProfile::GetProfileForType(InType);
}

void AExtremeWeatherHazardZone::ApplyWeatherHazardToVitals(UVitalsComponent* TargetVitals, float DeltaSeconds) const
{
	if (TargetVitals == nullptr || DeltaSeconds <= 0.0f)
	{
		return;
	}

	TargetVitals->ApplyAmbientTemperature(ActiveProfile.AmbientTemperatureCelsius, DeltaSeconds);

	// Acid rain burns exposed players unless sheltered or wearing Hazmat protection
	if (ActiveProfile.AcidCorrosionDamagePerSec > 0.0f && !TargetVitals->IsShelteredOrNearFire())
	{
		const float HazmatMitigation = FMath::Clamp(1.0f - (TargetVitals->GetRadiationProtection() * 2.0f), 0.0f, 1.0f);
		const float AcidDamage = ActiveProfile.AcidCorrosionDamagePerSec * HazmatMitigation * DeltaSeconds;
		if (AcidDamage > 0.0f)
		{
			TargetVitals->AddHealth(-AcidDamage);
		}
		if (ActiveProfile.AmbientRadiationPerSec > 0.0f)
		{
			TargetVitals->AddRadiationExposure(ActiveProfile.AmbientRadiationPerSec * DeltaSeconds);
		}
	}
}

const TArray<AExtremeWeatherHazardZone*>& AExtremeWeatherHazardZone::GetActiveWeatherHazardZones()
{
	return ActiveWeatherHazardZones;
}
