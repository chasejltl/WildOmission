// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ToolType.generated.h"

UENUM()
enum EToolType
{
	// Wood is for trees.
	WOOD	UMETA(DisplayName = "Wood"),
	// Stone is for nodes.
	STONE	UMETA(DisplayName = "Stone"),
	// Metal/Armor tier for building blocks and reinforced nodes.
	METAL	UMETA(DisplayName = "Metal"),
	// Multi is used by the rock to allow it to gather both node and wood resources.
	MULTI	UMETA(DisplayName = "Multi-Tool"),
	// Unassigned / Generic.
	NONE	UMETA(DisplayName = "None"),
	// Used to denote that it's for building purposes only
	// The building hammer uses this one.
	BUILD	UMETA(DisplayName = "Building Tool"),
	// Unused :3
	FABRIC	UMETA(DisplayName = "Fabric")
};

/**
 * Rust-style tool progression tier used for resource yield scaling and node minimum-tier gating.
 */
UENUM(BlueprintType)
enum class EToolTier : uint8
{
	Hand = 0		UMETA(DisplayName = "Hand / Rock (Tier 0)"),
	Stone = 1		UMETA(DisplayName = "Stone Tool (Tier 1)"),
	Metal = 2		UMETA(DisplayName = "Metal Tool (Tier 2)"),
	Salvaged = 3	UMETA(DisplayName = "Salvaged / High-Tier Tool (Tier 3)")
};

struct GATHERABLERESOURCES_API FHarvestTierMath
{
	static float GetTierYieldMultiplier(EToolTier Tier)
	{
		switch (Tier)
		{
		case EToolTier::Hand:
			return 0.5f;
		case EToolTier::Stone:
			return 1.0f;
		case EToolTier::Metal:
			return 1.5f;
		case EToolTier::Salvaged:
			return 2.0f;
		default:
			return 1.0f;
		}
	}

	static bool CanTierHarvest(EToolTier ToolTier, EToolTier MinimumRequiredTier)
	{
		return static_cast<uint8>(ToolTier) >= static_cast<uint8>(MinimumRequiredTier);
	}

	static int32 CalculateScaledYield(int32 BaseQuantity, float ToolGatherMultiplier, EToolTier ToolTier, EToolTier MinimumRequiredTier, float ServerGatherMultiplier = 1.0f)
	{
		if (BaseQuantity <= 0 || !CanTierHarvest(ToolTier, MinimumRequiredTier))
		{
			return 0;
		}

		const float EffectiveMultiplier = FMath::Max(0.01f, ToolGatherMultiplier) * GetTierYieldMultiplier(ToolTier) * FMath::Max(0.1f, ServerGatherMultiplier);
		return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(BaseQuantity) * EffectiveMultiplier));
	}
};
