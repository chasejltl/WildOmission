// Copyright Telephone Studios. All Rights Reserved.


#include "Deployables/BuildingBlock.h"
#include "Items/SwingableItem.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ABuildingBlock::ABuildingBlock()
{
	MaterialType = EToolType::WOOD;
	UpgradedTier = nullptr;
	BuildingTier = EBuildingTier::Twig;
	SupportHopsFromFoundation = 0;
	StructuralStability = 1.0f;

	ApplyTierStats(BuildingTier);
}

void ABuildingBlock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABuildingBlock, BuildingTier);
	DOREPLIFETIME(ABuildingBlock, SupportHopsFromFoundation);
	DOREPLIFETIME(ABuildingBlock, StructuralStability);
}

void ABuildingBlock::BeginPlay()
{
	Super::BeginPlay();

	StructuralStability = FBuildingTierRules::CalculateStability(SupportHopsFromFoundation);

	if (HasAuthority())
	{
		if (StructuralStability < 0.05f)
		{
			OnDeployableDestroyed();
			return;
		}

		if (FBuildingTierRules::IsDecayProneTwig(BuildingTier) && GetWorld() != nullptr)
		{
			GetWorld()->GetTimerManager().SetTimer(TwigDecayTimerHandle, this, &ABuildingBlock::TickTwigDecay, 60.0f, true);
		}
	}
}

void ABuildingBlock::ApplyTierStats(EBuildingTier InTier)
{
	BuildingTier = InTier;
	MaxDurability = FBuildingTierRules::GetTierMaxDurability(InTier);
	CurrentDurability = MaxDurability;
	NormalizedDurability = 1.0f;

	switch (InTier)
	{
	case EBuildingTier::Twig:
	case EBuildingTier::Wood:
		MaterialType = EToolType::WOOD;
		break;
	case EBuildingTier::Stone:
		MaterialType = EToolType::STONE;
		break;
	case EBuildingTier::SheetMetal:
	case EBuildingTier::Armored:
		MaterialType = EToolType::METAL;
		break;
	}
}

void ABuildingBlock::TickTwigDecay()
{
	if (!HasAuthority() || !FBuildingTierRules::IsDecayProneTwig(BuildingTier))
	{
		if (GetWorld() != nullptr)
		{
			GetWorld()->GetTimerManager().ClearTimer(TwigDecayTimerHandle);
		}
		return;
	}

	FDamageEvent DecayDamageEvent;
	TakeDamage(2.0f, DecayDamageEvent, nullptr, this);
}

float ABuildingBlock::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Positive damage from hand/melee tools cannot break Stone, SheetMetal, or Armored tiers (repairs use negative DamageAmount)
	if (DamageAmount > 0.0f && !FBuildingTierRules::IsBreakableByHandTools(BuildingTier))
	{
		if (DamageCauser != nullptr && DamageCauser->IsA<ASwingableItem>())
		{
			return 0.0f;
		}
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ABuildingBlock::Upgrade()
{
	UWorld* World = GetWorld();
	if (UpgradedTier != nullptr && World != nullptr)
	{
		ABuildingBlock* SpawnedBuildingBlock = World->SpawnActor<ABuildingBlock>(UpgradedTier, this->GetActorLocation(), this->GetActorRotation());
		if (SpawnedBuildingBlock != nullptr)
		{
			SpawnedBuildingBlock->SetSupportHopsFromFoundation(SupportHopsFromFoundation);
			SpawnedBuildingBlock->OnSpawn();
			this->Destroy();
			return;
		}
	}

	if (IsUpgradable())
	{
		UpgradeToTier(GetNextBuildingTier());
	}
}

bool ABuildingBlock::IsUpgradable() const
{
	return UpgradedTier != nullptr || static_cast<uint8>(BuildingTier) < static_cast<uint8>(EBuildingTier::Armored);
}

bool ABuildingBlock::UpgradeToTier(EBuildingTier TargetTier)
{
	if (static_cast<uint8>(TargetTier) <= static_cast<uint8>(BuildingTier))
	{
		return false;
	}

	ApplyTierStats(TargetTier);

	if (!FBuildingTierRules::IsDecayProneTwig(BuildingTier) && GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(TwigDecayTimerHandle);
	}

	Multi_PlayPlacementEffects();
	return true;
}

EBuildingTier ABuildingBlock::GetBuildingTier() const
{
	return BuildingTier;
}

EBuildingTier ABuildingBlock::GetNextBuildingTier() const
{
	const uint8 CurrentIndex = static_cast<uint8>(BuildingTier);
	const uint8 MaxIndex = static_cast<uint8>(EBuildingTier::Armored);
	return static_cast<EBuildingTier>(FMath::Min<uint8>(CurrentIndex + 1, MaxIndex));
}

bool ABuildingBlock::GetNextUpgradeCost(TArray<FInventoryItem>& OutCost) const
{
	if (!IsUpgradable())
	{
		return false;
	}

	FInventoryItem TierCost = FBuildingTierRules::GetTierUpgradeCost(GetNextBuildingTier());
	if (TierCost.IsZero())
	{
		return false;
	}

	OutCost.Reset();
	OutCost.Add(TierCost);
	return true;
}

void ABuildingBlock::SetSupportHopsFromFoundation(int32 InSupportHops)
{
	SupportHopsFromFoundation = FMath::Max(0, InSupportHops);
	StructuralStability = FBuildingTierRules::CalculateStability(SupportHopsFromFoundation);
}

int32 ABuildingBlock::GetSupportHopsFromFoundation() const
{
	return SupportHopsFromFoundation;
}

float ABuildingBlock::GetStructuralStability() const
{
	return StructuralStability;
}

TEnumAsByte<EToolType> ABuildingBlock::GetMaterialType()
{
	return MaterialType;
}

ABuildingBlock* ABuildingBlock::GetUpgradeDefaultClass() const
{
	if (UpgradedTier == nullptr)
	{
		return nullptr;
	}

	return UpgradedTier.GetDefaultObject();
}
