// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Seedling.h"
#include "Structs/InventoryItem.h"
#include "CropGenetics.generated.h"

UENUM(BlueprintType)
enum class ECropGene : uint8
{
	G = 0 UMETA(DisplayName = "G - Growth Speed (+15%)"),
	Y = 1 UMETA(DisplayName = "Y - Yield (+25%)"),
	H = 2 UMETA(DisplayName = "H - Hardiness (+20%)"),
	W = 3 UMETA(DisplayName = "W - Increased Water Need (Negative)"),
	X = 4 UMETA(DisplayName = "X - Empty / Null Allele (Negative)")
};

UENUM(BlueprintType)
enum class ECropSpecies : uint8
{
	Hemp = 0	UMETA(DisplayName = "Hemp"),
	Corn = 1	UMETA(DisplayName = "Corn"),
	Pumpkin = 2	UMETA(DisplayName = "Pumpkin")
};

/**
 * Rust-style 6-gene plant allele sequence (e.g. "GGGYYY", "WXGHYY").
 * Positive genes (G, Y, H) have dominance weight 0.6; negative genes (W, X) have dominance weight 1.0.
 */
USTRUCT(BlueprintType)
struct DEPLOYABLES_API FCropGeneSequence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetics")
	TArray<ECropGene> Genes;

	FCropGeneSequence();
	static FCropGeneSequence FromString(const FString& GeneCode);
	FString ToString() const;

	int32 CountGene(ECropGene TargetGene) const;
	float GetGrowthSpeedMultiplier() const;
	float GetYieldMultiplier() const;
	float GetHardinessMultiplier() const;
	float GetWaterNeedMultiplier() const;

	static float GetAlleleDominanceWeight(ECropGene Gene);

	/**
	 * Performs Rust-style crossbreeding against NeighborSequences across all 6 gene slots.
	 */
	FCropGeneSequence CrossbreedWithNeighbors(const TArray<FCropGeneSequence>& NeighborSequences) const;
};

/**
 * Genetically-driven agricultural crop seedling (Hemp, Corn, Pumpkin).
 */
UCLASS()
class DEPLOYABLES_API ACropSeedling : public ASeedling
{
	GENERATED_BODY()

public:
	ACropSeedling();

	void SetGeneSequence(const FCropGeneSequence& InSequence);
	const FCropGeneSequence& GetGeneSequence() const { return GeneSequence; }

	ECropSpecies GetCropSpecies() const { return Species; }
	FInventoryItem CalculateHarvestYield() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crop")
	ECropSpecies Species;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crop")
	FName HarvestItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crop")
	int32 BaseHarvestQuantity;

	UPROPERTY(VisibleInstanceOnly, SaveGame, BlueprintReadOnly, Category = "Crop")
	FCropGeneSequence GeneSequence;
};

UCLASS()
class DEPLOYABLES_API AHempCropSeedling : public ACropSeedling
{
	GENERATED_BODY()

public:
	AHempCropSeedling();
};

UCLASS()
class DEPLOYABLES_API ACornCropSeedling : public ACropSeedling
{
	GENERATED_BODY()

public:
	ACornCropSeedling();
};

UCLASS()
class DEPLOYABLES_API APumpkinCropSeedling : public ACropSeedling
{
	GENERATED_BODY()

public:
	APumpkinCropSeedling();
};
