// Copyright Telephone Studios. All Rights Reserved.

#include "Items/MilitaryFirearms.h"
#include "ServerRules.h"
#include "Engine/DamageEvents.h"

float FRaidExplosiveDamage::CalculateEffectiveRaidDamage(float BaseExplosiveDamage, bool bTargetOwnerOffline, const UObject* WorldContextObject)
{
	float FinalDamage = FMath::Max(0.0f, BaseExplosiveDamage);
	const UServerRules* Rules = UServerRules::Get(WorldContextObject);
	if (Rules != nullptr && Rules->bOfflineRaidProtection && bTargetOwnerOffline)
	{
		FinalDamage *= 0.5f;
	}
	return FinalDamage;
}

AAssaultRifleItem::AAssaultRifleItem()
{
	RateOfFireRPM = 450;
	AmmoItem = TEXT("RifleBullet");
	CurrentBurstShotCount = 0;
}

FVector2D AAssaultRifleItem::AdvanceBurstRecoil()
{
	const FVector2D Offset = FBallisticRecoilProfile::GetAssaultRifleRecoilForShot(CurrentBurstShotCount);
	++CurrentBurstShotCount;
	return Offset;
}

void AAssaultRifleItem::ResetBurstRecoil()
{
	CurrentBurstShotCount = 0;
}

ABoltActionRifleItem::ABoltActionRifleItem()
{
	RateOfFireRPM = 35;
	AmmoItem = TEXT("RifleBullet");
	MuzzleVelocityMultiplier = 1.6f;
}

ACustomSMGItem::ACustomSMGItem()
{
	RateOfFireRPM = 600;
	AmmoItem = TEXT("PistolBullet");
}

ATimedExplosiveChargeProjectile::ATimedExplosiveChargeProjectile()
{
	FuseTimeSeconds = 10.0f;
	BaseStructuralDamage = FRaidExplosiveDamage::C4StructuralDamage;
}

float ATimedExplosiveChargeProjectile::DetonateOnTarget(AActor* TargetActor, bool bTargetOwnerOffline)
{
	if (TargetActor == nullptr)
	{
		return 0.0f;
	}

	const float EffectiveDamage = FRaidExplosiveDamage::CalculateEffectiveRaidDamage(BaseStructuralDamage, bTargetOwnerOffline, this);
	FDamageEvent ExplosiveDamageEvent;
	TargetActor->TakeDamage(EffectiveDamage, ExplosiveDamageEvent, nullptr, this);
	return EffectiveDamage;
}
