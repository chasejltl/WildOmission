// Copyright Telephone Studios. All Rights Reserved.


#include "Components/HarvestableComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InventoryManipulatorComponent.h"
#include "ServerRules.h"
#include "Kismet/KismetMathLibrary.h"
#include "Log.h"

UHarvestableComponent::UHarvestableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	
	NormalizedRareDropChance = 0.05f;
	NormalizedQualityToolDropChance = 0.25f;
	
	RequiredToolType = EToolType::WOOD;
	MinimumToolTier = EToolTier::Hand;

	Durability = 5;
}

void UHarvestableComponent::OnHarvest(AActor* HarvestingActor, float GatherMultiplier, bool IsQualityTool)
{
	const EToolTier DerivedTier = IsQualityTool ? EToolTier::Metal : EToolTier::Stone;
	OnHarvestWithTier(HarvestingActor, GatherMultiplier, IsQualityTool, DerivedTier);
}

void UHarvestableComponent::OnHarvestWithTier(AActor* HarvestingActor, float GatherMultiplier, bool IsQualityTool, EToolTier ToolTier)
{
	if (HarvestingActor == nullptr)
	{
		UE_LOG(LogGatherableResources, Warning, TEXT("Could not harvest resource, HarvestingActor was nullptr."));
		return;
	}

	if (!FHarvestTierMath::CanTierHarvest(ToolTier, MinimumToolTier))
	{
		return;
	}

	UInventoryComponent* HarvestingInventoryComponent = HarvestingActor->FindComponentByClass<UInventoryComponent>();
	if (HarvestingInventoryComponent == nullptr)
	{
		UE_LOG(LogGatherableResources, Warning, TEXT("Could not harvest resource, HarvestingActor has no InventoryComponent."));
		return;
	}

	// Calculate Normal Drops
	const bool bQualifiesForQualityDrop = IsQualityTool || static_cast<uint8>(ToolTier) >= static_cast<uint8>(EToolTier::Metal);
	const bool GiveQualityToolDrop = !QualityToolDrops.IsEmpty() && bQualifiesForQualityDrop && UKismetMathLibrary::RandomBoolWithWeight(NormalizedQualityToolDropChance);
	TArray<FInventoryItem>& DropList = GiveQualityToolDrop ? QualityToolDrops : CommonDrops;

	FInventoryItem Drop = HandleYieldFromList(DropList, GatherMultiplier, ToolTier);
	if (!Drop.IsZero())
	{
		HarvestingInventoryComponent->AddItem(Drop, true);
	}

	// Calculate Rare Drops
	const bool GiveRareDrop = !RareDrops.IsEmpty() && UKismetMathLibrary::RandomBoolWithWeight(NormalizedRareDropChance);
	if (GiveRareDrop)
	{
		FInventoryItem RareDrop = HandleYieldFromList(RareDrops, GatherMultiplier, ToolTier);
		if (!RareDrop.IsZero())
		{
			HarvestingInventoryComponent->AddItem(RareDrop, true);
		}
	}

	Durability--;

	if (Durability <= 0)
	{
		AActor* OwnerActor = GetOwner();
		if (OwnerActor == nullptr)
		{
			return;
		}
		
		OwnerActor->Destroy();
	}
}

FInventoryItem UHarvestableComponent::HandleYieldFromList(const TArray<FInventoryItem>& DropList, float GatherMultiplier, EToolTier ToolTier)
{
	FInventoryItem Item;
	const int32 DropIndex = FMath::RandRange(0, DropList.Num() - 1);
	if (!DropList.IsEmpty() && DropList.IsValidIndex(DropIndex))
	{
		const UServerRules* Rules = UServerRules::Get(this);
		const float ServerGatherMult = (Rules != nullptr) ? Rules->ResourceGatherMultiplier : 1.0f;
		Item = DropList[DropIndex];
		Item.Quantity = FHarvestTierMath::CalculateScaledYield(Item.Quantity, GatherMultiplier, ToolTier, MinimumToolTier, ServerGatherMult);
	}

	return Item;
}

TEnumAsByte<EToolType> UHarvestableComponent::GetRequiredToolType() const
{
	return RequiredToolType;
}

EToolTier UHarvestableComponent::GetMinimumToolTier() const
{
	return MinimumToolTier;
}

void UHarvestableComponent::SetMinimumToolTier(EToolTier InTier)
{
	MinimumToolTier = InTier;
}

void UHarvestableComponent::ConfigureDefaultDrops(const TArray<FInventoryItem>& InCommonDrops, const TArray<FInventoryItem>& InRareDrops, const TArray<FInventoryItem>& InQualityDrops)
{
	CommonDrops = InCommonDrops;
	RareDrops = InRareDrops;
	QualityToolDrops = InQualityDrops;
}

int32 UHarvestableComponent::GetDurability() const
{
	return Durability;
}

void UHarvestableComponent::SetDurability(const int32& InDurability)
{
	Durability = InDurability;
}
