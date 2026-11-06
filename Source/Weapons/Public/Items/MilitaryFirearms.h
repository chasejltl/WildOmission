// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/FullAutomaticFirearmItem.h"
#include "Items/ManualActionFirearmItem.h"
#include "Projectiles/WeaponProjectile.h"
#include "MilitaryFirearms.generated.h"

/**
 * Deterministic Rust-style S-curve recoil & projectile drop calculator for military firearms.
 */
struct WEAPONS_API FBallisticRecoilProfile
{
	/** Returns (PitchUpDegrees, YawHorizontalDegrees) for the Nth consecutive shot in a burst (0-indexed). */
	static FVector2D GetAssaultRifleRecoilForShot(int32 ShotIndex)
	{
		const int32 ClampedShot = FMath::Clamp(ShotIndex, 0, 29);
		const float PitchUp = 1.65f + (ClampedShot < 10 ? 0.15f * ClampedShot : 1.0f);
		// S-curve horizontal sway across the 30-round spray
		const float Phase = (static_cast<float>(ClampedShot) / 29.0f) * PI * 2.0f;
		const float Yaw = FMath::Sin(Phase) * 1.35f;
		return FVector2D(PitchUp, Yaw);
	}

	static FVector2D GetSMGRecoilForShot(int32 ShotIndex)
	{
		const int32 ClampedShot = FMath::Clamp(ShotIndex, 0, 23);
		const float PitchUp = 1.10f;
		const float Phase = (static_cast<float>(ClampedShot) / 23.0f) * PI * 2.0f;
		const float Yaw = FMath::Sin(Phase) * 0.85f;
		return FVector2D(PitchUp, Yaw);
	}

	/** Calculates vertical bullet drop in centimeters at DistanceCm for a given muzzle velocity (cm/s). */
	static float CalculateBulletDropCm(float DistanceCm, float MuzzleVelocityCmPerSec, float GravityCmPerSecSq = 980.0f)
	{
		if (DistanceCm <= 0.0f || MuzzleVelocityCmPerSec <= 1.0f)
		{
			return 0.0f;
		}
		const float FlightTimeSec = DistanceCm / MuzzleVelocityCmPerSec;
		return 0.5f * GravityCmPerSecSq * FlightTimeSec * FlightTimeSec;
	}
};

/**
 * Structural raiding damage calculator for C4 (Timed Explosive Charge) and Rockets.
 * Enforces UServerRules::bOfflineRaidProtection when enabled.
 */
struct WEAPONS_API FRaidExplosiveDamage
{
	static constexpr float C4StructuralDamage = 550.0f;
	static constexpr float RocketStructuralDamage = 275.0f;

	static float CalculateEffectiveRaidDamage(float BaseExplosiveDamage, bool bTargetOwnerOffline, const UObject* WorldContextObject = nullptr);
};

/**
 * Tier 3 Assault Rifle (450 RPM, 30-round 5.56 magazine, deterministic S-curve recoil).
 */
UCLASS()
class WEAPONS_API AAssaultRifleItem : public AFullAutomaticFirearmItem
{
	GENERATED_BODY()

public:
	AAssaultRifleItem();

	FVector2D AdvanceBurstRecoil();
	void ResetBurstRecoil();
	int32 GetCurrentBurstShotCount() const { return CurrentBurstShotCount; }

private:
	int32 CurrentBurstShotCount;
};

/**
 * Tier 3 Bolt-Action Sniper Rifle (35 RPM, 4-round 5.56 internal magazine, 1.6x muzzle velocity).
 */
UCLASS()
class WEAPONS_API ABoltActionRifleItem : public AManualActionFirearmItem
{
	GENERATED_BODY()

public:
	ABoltActionRifleItem();

	float GetMuzzleVelocityMultiplier() const { return MuzzleVelocityMultiplier; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ballistics")
	float MuzzleVelocityMultiplier;
};

/**
 * Tier 2 Custom Submachine Gun (600 RPM, 24-round Pistol Bullet magazine).
 */
UCLASS()
class WEAPONS_API ACustomSMGItem : public AFullAutomaticFirearmItem
{
	GENERATED_BODY()

public:
	ACustomSMGItem();
};

/**
 * Tier 3 Timed Explosive Charge (C4) projectile/deployable charge.
 * Detonates after FuseTimeSeconds (10.0s) dealing 550 structural damage.
 */
UCLASS()
class WEAPONS_API ATimedExplosiveChargeProjectile : public AWeaponProjectile
{
	GENERATED_BODY()

public:
	ATimedExplosiveChargeProjectile();

	float GetFuseTimeSeconds() const { return FuseTimeSeconds; }
	float DetonateOnTarget(AActor* TargetActor, bool bTargetOwnerOffline = false);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Raiding")
	float FuseTimeSeconds;

	UPROPERTY(EditDefaultsOnly, Category = "Raiding")
	float BaseStructuralDamage;
};
