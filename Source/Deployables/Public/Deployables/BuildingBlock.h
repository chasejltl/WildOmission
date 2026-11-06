// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Deployable.h"
#include "Structs/InventoryItem.h"
#include "BuildingBlock.generated.h"

/**
 * Rust-style 4+1 building block progression tiers:
 * Twig (decay-prone, breakable by hand tools) -> Wood -> Stone -> Sheet Metal -> Armored.
 */
UENUM(BlueprintType)
enum class EBuildingTier : uint8
{
	Twig = 0		UMETA(DisplayName = "Twig"),
	Wood = 1		UMETA(DisplayName = "Wood"),
	Stone = 2		UMETA(DisplayName = "Stone"),
	SheetMetal = 3	UMETA(DisplayName = "Sheet Metal"),
	Armored = 4		UMETA(DisplayName = "Armored")
};

struct DEPLOYABLES_API FBuildingTierRules
{
	static float GetTierMaxDurability(EBuildingTier Tier)
	{
		switch (Tier)
		{
		case EBuildingTier::Twig:
			return 50.0f;
		case EBuildingTier::Wood:
			return 250.0f;
		case EBuildingTier::Stone:
			return 500.0f;
		case EBuildingTier::SheetMetal:
			return 1000.0f;
		case EBuildingTier::Armored:
			return 2000.0f;
		default:
			return 250.0f;
		}
	}

	static FInventoryItem GetTierUpgradeCost(EBuildingTier TargetTier)
	{
		FInventoryItem Cost;
		switch (TargetTier)
		{
		case EBuildingTier::Twig:
			Cost.Name = TEXT("Wood");
			Cost.Quantity = 25;
			break;
		case EBuildingTier::Wood:
			Cost.Name = TEXT("Wood");
			Cost.Quantity = 150;
			break;
		case EBuildingTier::Stone:
			Cost.Name = TEXT("Stone");
			Cost.Quantity = 200;
			break;
		case EBuildingTier::SheetMetal:
			Cost.Name = TEXT("MetalFragments");
			Cost.Quantity = 150;
			break;
		case EBuildingTier::Armored:
			Cost.Name = TEXT("HighQualityMetal");
			Cost.Quantity = 20;
			break;
		default:
			break;
		}
		return Cost;
	}

	static bool IsBreakableByHandTools(EBuildingTier Tier)
	{
		return Tier == EBuildingTier::Twig || Tier == EBuildingTier::Wood;
	}

	static bool IsDecayProneTwig(EBuildingTier Tier)
	{
		return Tier == EBuildingTier::Twig;
	}

	static float CalculateStability(int32 SupportHopsFromFoundation, float LossPerHop = 0.15f)
	{
		if (SupportHopsFromFoundation <= 0)
		{
			return 1.0f;
		}
		return FMath::Max(0.0f, 1.0f - (static_cast<float>(SupportHopsFromFoundation) * LossPerHop));
	}

	static FName GetTierUpkeepResource(EBuildingTier Tier)
	{
		switch (Tier)
		{
		case EBuildingTier::Wood:
			return TEXT("Wood");
		case EBuildingTier::Stone:
			return TEXT("Stone");
		case EBuildingTier::SheetMetal:
			return TEXT("MetalFragments");
		case EBuildingTier::Armored:
			return TEXT("HighQualityMetal");
		default:
			return NAME_None;
		}
	}

	static float GetTierDecayDurationHours(EBuildingTier Tier)
	{
		switch (Tier)
		{
		case EBuildingTier::Twig:
			return 1.0f;
		case EBuildingTier::Wood:
			return 3.0f;
		case EBuildingTier::Stone:
			return 5.0f;
		case EBuildingTier::SheetMetal:
			return 8.0f;
		case EBuildingTier::Armored:
			return 12.0f;
		default:
			return 3.0f;
		}
	}

	static float GetUpkeepTaxBracketRate(int32 TotalBlockCount)
	{
		if (TotalBlockCount <= 0)
		{
			return 0.0f;
		}
		if (TotalBlockCount <= 15)
		{
			return 0.10f;
		}
		if (TotalBlockCount <= 50)
		{
			return 0.15f;
		}
		if (TotalBlockCount <= 100)
		{
			return 0.20f;
		}
		return 0.33f;
	}
};

UCLASS()
class DEPLOYABLES_API ABuildingBlock : public ADeployable
{
	GENERATED_BODY()
	
public:
	ABuildingBlock();

	virtual void OnSpawn() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	static const TArray<ABuildingBlock*>& GetAllBuildingBlocks();

	void Upgrade();
	bool IsUpgradable() const;

	UFUNCTION(BlueprintCallable, Category = "Building Tier")
	bool UpgradeToTier(EBuildingTier TargetTier);

	UFUNCTION(BlueprintPure, Category = "Building Tier")
	EBuildingTier GetBuildingTier() const;

	UFUNCTION(BlueprintPure, Category = "Building Tier")
	EBuildingTier GetNextBuildingTier() const;

	UFUNCTION(BlueprintPure, Category = "Building Tier")
	bool GetNextUpgradeCost(TArray<FInventoryItem>& OutCost) const;

	UFUNCTION(BlueprintCallable, Category = "Building Stability")
	void SetSupportHopsFromFoundation(int32 InSupportHops);

	UFUNCTION(BlueprintPure, Category = "Building Stability")
	int32 GetSupportHopsFromFoundation() const;

	UFUNCTION(BlueprintPure, Category = "Building Stability")
	float GetStructuralStability() const;

	TEnumAsByte<EToolType> GetMaterialType();

	ABuildingBlock* GetUpgradeDefaultClass() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Deployable")
	TEnumAsByte<EToolType> MaterialType;

	UPROPERTY(EditDefaultsOnly, Category = "Deployable")
	TSubclassOf<ABuildingBlock> UpgradedTier;

	UPROPERTY(EditDefaultsOnly, SaveGame, Replicated, Category = "Building Tier")
	EBuildingTier BuildingTier;

	UPROPERTY(EditDefaultsOnly, SaveGame, Replicated, Category = "Building Stability")
	int32 SupportHopsFromFoundation;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Building Stability")
	float StructuralStability;

	FTimerHandle TwigDecayTimerHandle;

	void ApplyTierStats(EBuildingTier InTier);
	void TickTwigDecay();
};
