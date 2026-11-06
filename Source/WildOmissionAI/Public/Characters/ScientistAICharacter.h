// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/WildOmissionAICharacter.h"
#include "ScientistAICharacter.generated.h"

/**
 * Tactical decision state for Monument Scientist AI and human NPC patrols.
 */
UENUM(BlueprintType)
enum class EScientistTacticalState : uint8
{
	Patrol = 0			UMETA(DisplayName = "Patrol"),
	EngageRanged = 1	UMETA(DisplayName = "Engage Ranged"),
	SeekCover = 2		UMETA(DisplayName = "Seek Cover"),
	ThrowGrenade = 3	UMETA(DisplayName = "Throw Grenade"),
	HealWithSyringe = 4	UMETA(DisplayName = "Heal With Syringe"),
	FlankTarget = 5		UMETA(DisplayName = "Flank Target")
};

/**
 * Armed Hazmat Scientist AI guarding Tier 2/3 Monuments.
 * Features sight/sound perception, cover-seeking, grenade flushing, and medical syringe self-healing.
 */
UCLASS()
class WILDOMISSIONAI_API AScientistAICharacter : public AWildOmissionAICharacter
{
	GENERATED_BODY()

public:
	AScientistAICharacter();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Scientist AI")
	float SightRadiusCm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Scientist AI")
	float HearingRadiusCm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Scientist AI")
	float RangedBurstDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Scientist AI")
	int32 RemainingSyringes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Scientist AI")
	int32 RemainingGrenades;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scientist AI")
	EScientistTacticalState CurrentTacticalState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scientist AI")
	FVector LastHeardSoundLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scientist AI")
	bool bHasHeardSoundAlert;

	/**
	 * Evaluates and transitions tactical combat state based on health, line of sight, target distance, and time hidden.
	 * Respects UServerRules::bAdvancedAIEnabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scientist AI")
	EScientistTacticalState EvaluateTacticalState(float CurrentHealthPct, bool bHasLineOfSight, float TargetDistanceCm, float SecondsSinceTargetSeen);

	/** Records a gunshot, footstep, or explosion heard within HearingRadiusCm. */
	UFUNCTION(BlueprintCallable, Category = "Scientist AI")
	bool ReportHeardSound(const FVector& SoundLocation, float Loudness01);
};

/**
 * Attack Helicopter world boss that patrols monuments and engages geared/armed players.
 * Features dual weakspot rotors (Main Rotor & Tail Rotor) and drops high-tier locked crates on crash.
 */
UCLASS()
class WILDOMISSIONAI_API APatrolHelicopterAI : public AActor
{
	GENERATED_BODY()

public:
	APatrolHelicopterAI();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Patrol Helicopter")
	float HullHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Patrol Helicopter")
	float MainRotorHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Patrol Helicopter")
	float TailRotorHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Patrol Helicopter")
	int32 CratesToSpawnOnCrash;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol Helicopter")
	bool bIsCrashed;

	/**
	 * Rust threat rules: Patrol Helicopter ignores naked/primitive survivors,
	 * engaging only if the player carries at least 1 ranged weapon or wears > 2 clothing/armor pieces.
	 */
	UFUNCTION(BlueprintPure, Category = "Patrol Helicopter")
	bool IsPlayerHostileThreat(int32 CarriedRangedWeaponsCount, int32 EquippedClothingPiecesCount) const;

	/**
	 * Applies ballistic/explosive damage to hull or rotor weakspots.
	 * Destroying both rotors or depleting HullHealth triggers a crash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Patrol Helicopter")
	void ApplyHelicopterDamage(float DamageAmount, bool bHitMainRotor, bool bHitTailRotor);
};
