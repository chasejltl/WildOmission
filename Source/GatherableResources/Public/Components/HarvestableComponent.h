// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/InventoryItem.h"
#include "Enums/ToolType.h"
#include "Interfaces/SavableObject.h"
#include "Components/ActorComponent.h"
#include "HarvestableComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GATHERABLERESOURCES_API UHarvestableComponent : public UActorComponent, public ISavableObject
{
	GENERATED_BODY()

public:	
	UHarvestableComponent();
	
	virtual void OnHarvest(AActor* HarvestingActor, float GatherMultiplier, bool IsQualityTool);
	virtual void OnHarvestWithTier(AActor* HarvestingActor, float GatherMultiplier, bool IsQualityTool, EToolTier ToolTier);

	TEnumAsByte<EToolType> GetRequiredToolType() const;
	EToolTier GetMinimumToolTier() const;
	void SetMinimumToolTier(EToolTier InTier);

	void ConfigureDefaultDrops(const TArray<FInventoryItem>& InCommonDrops, const TArray<FInventoryItem>& InRareDrops = TArray<FInventoryItem>(), const TArray<FInventoryItem>& InQualityDrops = TArray<FInventoryItem>());
	
	int32 GetDurability() const;

	void SetDurability(const int32& InDurability);
	
private:
	UPROPERTY(EditDefaultsOnly)
	TArray<FInventoryItem> CommonDrops;
	UPROPERTY(EditDefaultsOnly)
	TArray<FInventoryItem> RareDrops;
	UPROPERTY(EditDefaultsOnly)
	TArray<FInventoryItem> QualityToolDrops;

	// 1 = 100%, 0.5 = 50%, 0 = 0%
	UPROPERTY(EditDefaultsOnly)
	float NormalizedRareDropChance;

	// 1 = 100%, 0.5 = 50%, 0 = 0%
	UPROPERTY(EditDefaultsOnly)
	float NormalizedQualityToolDropChance;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<EToolType> RequiredToolType;

	UPROPERTY(EditDefaultsOnly)
	EToolTier MinimumToolTier;
	
	UPROPERTY(EditDefaultsOnly, SaveGame)
	int32 Durability;

	FInventoryItem HandleYieldFromList(const TArray<FInventoryItem>& DropList, float GatherMultiplier, EToolTier ToolTier = EToolTier::Stone);

};
