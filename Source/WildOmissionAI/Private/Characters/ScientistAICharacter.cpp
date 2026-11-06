// Copyright Telephone Studios. All Rights Reserved.

#include "Characters/ScientistAICharacter.h"
#include "Components/VitalsComponent.h"
#include "ServerRules.h"

AScientistAICharacter::AScientistAICharacter()
{
	SightRadiusCm = 3500.0f;
	HearingRadiusCm = 2500.0f;
	RangedBurstDamage = 18.0f;
	RemainingSyringes = 3;
	RemainingGrenades = 2;
	CurrentTacticalState = EScientistTacticalState::Patrol;
	LastHeardSoundLocation = FVector::ZeroVector;
	bHasHeardSoundAlert = false;
}

EScientistTacticalState AScientistAICharacter::EvaluateTacticalState(float CurrentHealthPct, bool bHasLineOfSight, float TargetDistanceCm, float SecondsSinceTargetSeen)
{
	const UServerRules* Rules = UServerRules::Get(this);
	const bool bAdvancedEnabled = (Rules == nullptr) || Rules->bAdvancedAIEnabled;

	if (!bAdvancedEnabled)
	{
		CurrentTacticalState = (bHasLineOfSight && TargetDistanceCm <= SightRadiusCm)
			? EScientistTacticalState::EngageRanged
			: EScientistTacticalState::Patrol;
		return CurrentTacticalState;
	}

	// 1. Critical health self-heal behind cover using Medical Syringe
	if (CurrentHealthPct <= 0.35f)
	{
		if (RemainingSyringes > 0)
		{
			--RemainingSyringes;
			if (VitalsComponent)
			{
				VitalsComponent->AddHealth(35.0f);
			}
			CurrentTacticalState = EScientistTacticalState::HealWithSyringe;
			return CurrentTacticalState;
		}

		CurrentTacticalState = EScientistTacticalState::SeekCover;
		return CurrentTacticalState;
	}

	// 2. Direct line-of-sight engagement within range
	if (bHasLineOfSight && TargetDistanceCm <= SightRadiusCm)
	{
		CurrentTacticalState = EScientistTacticalState::EngageRanged;
		return CurrentTacticalState;
	}

	// 3. Target recently ducked behind cover within throwing distance -> flush with Frag Grenade
	if (!bHasLineOfSight && TargetDistanceCm <= 2500.0f && SecondsSinceTargetSeen > 0.0f && SecondsSinceTargetSeen < 8.0f)
	{
		if (RemainingGrenades > 0)
		{
			--RemainingGrenades;
			CurrentTacticalState = EScientistTacticalState::ThrowGrenade;
			return CurrentTacticalState;
		}
		CurrentTacticalState = EScientistTacticalState::SeekCover;
		return CurrentTacticalState;
	}

	// 4. Target hidden for extended period -> flank last known position
	if (!bHasLineOfSight && SecondsSinceTargetSeen >= 8.0f && SecondsSinceTargetSeen < 30.0f)
	{
		CurrentTacticalState = EScientistTacticalState::FlankTarget;
		return CurrentTacticalState;
	}

	CurrentTacticalState = EScientistTacticalState::Patrol;
	return CurrentTacticalState;
}

bool AScientistAICharacter::ReportHeardSound(const FVector& SoundLocation, float Loudness01)
{
	const float ClampedLoudness = FMath::Clamp(Loudness01, 0.0f, 1.0f);
	if (ClampedLoudness <= 0.0f)
	{
		return false;
	}

	const float EffectiveRadius = HearingRadiusCm * ClampedLoudness;
	const float DistanceSq = FVector::DistSquared(GetActorLocation(), SoundLocation);
	if (DistanceSq <= FMath::Square(EffectiveRadius))
	{
		LastHeardSoundLocation = SoundLocation;
		bHasHeardSoundAlert = true;
		return true;
	}

	return false;
}

APatrolHelicopterAI::APatrolHelicopterAI()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	HullHealth = 10000.0f;
	MainRotorHealth = 1000.0f;
	TailRotorHealth = 500.0f;
	CratesToSpawnOnCrash = 4;
	bIsCrashed = false;
}

bool APatrolHelicopterAI::IsPlayerHostileThreat(int32 CarriedRangedWeaponsCount, int32 EquippedClothingPiecesCount) const
{
	return CarriedRangedWeaponsCount >= 1 || EquippedClothingPiecesCount > 2;
}

void APatrolHelicopterAI::ApplyHelicopterDamage(float DamageAmount, bool bHitMainRotor, bool bHitTailRotor)
{
	if (DamageAmount <= 0.0f || bIsCrashed)
	{
		return;
	}

	if (bHitMainRotor && MainRotorHealth > 0.0f)
	{
		MainRotorHealth = FMath::Max(0.0f, MainRotorHealth - DamageAmount);
		HullHealth = FMath::Max(0.0f, HullHealth - (DamageAmount * 2.5f));
	}
	else if (bHitTailRotor && TailRotorHealth > 0.0f)
	{
		TailRotorHealth = FMath::Max(0.0f, TailRotorHealth - DamageAmount);
		HullHealth = FMath::Max(0.0f, HullHealth - (DamageAmount * 2.5f));
	}
	else
	{
		HullHealth = FMath::Max(0.0f, HullHealth - DamageAmount);
	}

	if (HullHealth <= 0.0f || (MainRotorHealth <= 0.0f && TailRotorHealth <= 0.0f))
	{
		HullHealth = 0.0f;
		bIsCrashed = true;
	}
}
