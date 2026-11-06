// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/Structure.h"
#include "Structs/InventoryItem.h"
#include "MonumentStructure.generated.h"

UENUM(BlueprintType)
enum class EMonumentTier : uint8
{
	Tier1_Airfield = 0			UMETA(DisplayName = "Tier 1 - Airfield"),
	Tier2_PowerPlant = 1		UMETA(DisplayName = "Tier 2 - Power Plant"),
	Tier3_MilitaryTunnels = 2	UMETA(DisplayName = "Tier 3 - Military Tunnels")
};

/**
 * 15-minute (900s) Hackable Locked Crate state found at high-tier monuments.
 */
USTRUCT(BlueprintType)
struct STRUCTURES_API FHackableLockedCrateState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monument|Crate")
	float TotalHackDurationSeconds = 900.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monument|Crate")
	float RemainingHackSeconds = 900.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monument|Crate")
	bool bHackingInProgress = false;

	void StartHacking()
	{
		if (!IsUnlocked())
		{
			bHackingInProgress = true;
		}
	}

	void AdvanceHackTimer(float DeltaSeconds)
	{
		if (bHackingInProgress && DeltaSeconds > 0.0f)
		{
			RemainingHackSeconds = FMath::Max(0.0f, RemainingHackSeconds - DeltaSeconds);
			if (RemainingHackSeconds <= 0.0f)
			{
				bHackingInProgress = false;
			}
		}
	}

	bool IsUnlocked() const
	{
		return RemainingHackSeconds <= 0.0f;
	}
};

/**
 * Rust-style procedural Monument & Radiation Zone (Airfield, Power Plant, Military Tunnels).
 */
UCLASS()
class STRUCTURES_API AMonumentStructure : public AStructure
{
	GENERATED_BODY()

public:
	AMonumentStructure();

	EMonumentTier GetMonumentTier() const { return MonumentTier; }
	float GetRadiationRadius() const { return RadiationRadius; }
	float GetRadiationRadsPerSecond() const { return RadiationRadsPerSecond; }
	float GetBuildExclusionRadius() const { return BuildExclusionRadius; }

	bool IsLocationInRadiationZone(const FVector& Location) const;
	bool IsLocationInBuildExclusionZone(const FVector& Location) const;

	/** Applies monument radiation to TargetVitals over DeltaSeconds (mitigated by Hazmat/clothing protection). */
	void ApplyMonumentRadiationToVitals(class UVitalsComponent* TargetVitals, float DeltaSeconds) const;

	FHackableLockedCrateState& GetLockedCrateState() { return LockedCrateState; }
	const FHackableLockedCrateState& GetLockedCrateState() const { return LockedCrateState; }

	/** Generates high-tier loot items appropriate for this monument's tier. */
	TArray<FInventoryItem> GenerateLockedCrateLoot() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monument")
	EMonumentTier MonumentTier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monument")
	float RadiationRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monument")
	float RadiationRadsPerSecond;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monument")
	float BuildExclusionRadius;

	UPROPERTY(VisibleInstanceOnly, SaveGame, BlueprintReadOnly, Category = "Monument")
	FHackableLockedCrateState LockedCrateState;
};

UCLASS()
class STRUCTURES_API AAirfieldMonument : public AMonumentStructure
{
	GENERATED_BODY()

public:
	AAirfieldMonument();
};

UCLASS()
class STRUCTURES_API APowerPlantMonument : public AMonumentStructure
{
	GENERATED_BODY()

public:
	APowerPlantMonument();
};

UCLASS()
class STRUCTURES_API AMilitaryTunnelsMonument : public AMonumentStructure
{
	GENERATED_BODY()

public:
	AMilitaryTunnelsMonument();
};
