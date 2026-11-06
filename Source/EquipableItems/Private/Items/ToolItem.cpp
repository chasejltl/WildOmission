// Copyright Telephone Studios. All Rights Reserved.


#include "Items/ToolItem.h"
#include "Components/EquipComponent.h"
#include "Actors/HarvestableResource.h"
#include "Components/HarvestableComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "Log.h"

AToolItem::AToolItem()
{
	ToolType = EToolType::WOOD;
	GatherMultiplier = 0.2f;
	QualityTool = false;
	ToolTier = EToolTier::Stone;
}

void AToolItem::OnPrimaryHeld()
{
	Super::OnPrimaryHeld();

	Swing();
}

float AToolItem::GetGatherMultiplier() const
{
	return GatherMultiplier;
}

bool AToolItem::IsQualityTool() const
{
	return QualityTool;
}

EToolTier AToolItem::GetToolTier() const
{
	if (QualityTool && ToolTier < EToolTier::Metal)
	{
		return EToolTier::Metal;
	}
	return ToolTier;
}

void AToolItem::OnSwingImpact(const FHitResult& HitResult, const FVector& OwnerCharacterLookVector, bool FromFirstPersonInstance)
{
	Super::OnSwingImpact(HitResult, OwnerCharacterLookVector, FromFirstPersonInstance);

	if (!HasAuthority())
	{
		return;
	}
	
	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr)
	{
		return;
	}

	const EToolTier EffectiveTier = GetToolTier();

	AHarvestableResource* HitHarvestable = Cast<AHarvestableResource>(HitActor);
	UHarvestableComponent* HitHarvestableComponent = HitActor->FindComponentByClass<UHarvestableComponent>();
	if (HitHarvestable != nullptr && (HitHarvestable->GetRequiredToolType() == ToolType || ToolType == EToolType::MULTI))
	{
		HitHarvestable->OnHarvestWithTier(GetOwner(), GatherMultiplier, QualityTool, EffectiveTier);
	}
	else if (HitHarvestableComponent != nullptr && (HitHarvestableComponent->GetRequiredToolType() == ToolType || ToolType == EToolType::MULTI))
	{
		HitHarvestableComponent->OnHarvestWithTier(GetOwner(), GatherMultiplier, QualityTool, EffectiveTier);
	}
}
