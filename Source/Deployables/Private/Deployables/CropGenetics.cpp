// Copyright Telephone Studios. All Rights Reserved.

#include "Deployables/CropGenetics.h"

FCropGeneSequence::FCropGeneSequence()
{
	Genes = { ECropGene::G, ECropGene::Y, ECropGene::H, ECropGene::G, ECropGene::X, ECropGene::W };
}

FCropGeneSequence FCropGeneSequence::FromString(const FString& GeneCode)
{
	FCropGeneSequence Seq;
	Seq.Genes.Reset();
	for (int32 i = 0; i < 6; ++i)
	{
		const TCHAR Ch = (i < GeneCode.Len()) ? FChar::ToUpper(GeneCode[i]) : TEXT('X');
		switch (Ch)
		{
		case TEXT('G'): Seq.Genes.Add(ECropGene::G); break;
		case TEXT('Y'): Seq.Genes.Add(ECropGene::Y); break;
		case TEXT('H'): Seq.Genes.Add(ECropGene::H); break;
		case TEXT('W'): Seq.Genes.Add(ECropGene::W); break;
		default:        Seq.Genes.Add(ECropGene::X); break;
		}
	}
	return Seq;
}

FString FCropGeneSequence::ToString() const
{
	FString Out;
	for (int32 i = 0; i < Genes.Num(); ++i)
	{
		switch (Genes[i])
		{
		case ECropGene::G: Out.AppendChar(TEXT('G')); break;
		case ECropGene::Y: Out.AppendChar(TEXT('Y')); break;
		case ECropGene::H: Out.AppendChar(TEXT('H')); break;
		case ECropGene::W: Out.AppendChar(TEXT('W')); break;
		default:           Out.AppendChar(TEXT('X')); break;
		}
	}
	return Out;
}

int32 FCropGeneSequence::CountGene(ECropGene TargetGene) const
{
	int32 Count = 0;
	for (ECropGene Gene : Genes)
	{
		if (Gene == TargetGene)
		{
			++Count;
		}
	}
	return Count;
}

float FCropGeneSequence::GetGrowthSpeedMultiplier() const
{
	return 1.0f + (0.15f * static_cast<float>(CountGene(ECropGene::G)));
}

float FCropGeneSequence::GetYieldMultiplier() const
{
	return 1.0f + (0.25f * static_cast<float>(CountGene(ECropGene::Y)));
}

float FCropGeneSequence::GetHardinessMultiplier() const
{
	return 1.0f + (0.20f * static_cast<float>(CountGene(ECropGene::H)));
}

float FCropGeneSequence::GetWaterNeedMultiplier() const
{
	return 1.0f + (0.25f * static_cast<float>(CountGene(ECropGene::W)));
}

float FCropGeneSequence::GetAlleleDominanceWeight(ECropGene Gene)
{
	return (Gene == ECropGene::W || Gene == ECropGene::X) ? 1.0f : 0.6f;
}

FCropGeneSequence FCropGeneSequence::CrossbreedWithNeighbors(const TArray<FCropGeneSequence>& NeighborSequences) const
{
	FCropGeneSequence Result = *this;
	if (Result.Genes.Num() < 6)
	{
		Result = FCropGeneSequence();
	}

	if (NeighborSequences.IsEmpty())
	{
		return Result;
	}

	for (int32 SlotIdx = 0; SlotIdx < 6; ++SlotIdx)
	{
		TMap<ECropGene, float> AlleleScores;
		for (const FCropGeneSequence& Neighbor : NeighborSequences)
		{
			if (SlotIdx < Neighbor.Genes.Num())
			{
				const ECropGene NeighborGene = Neighbor.Genes[SlotIdx];
				AlleleScores.FindOrAdd(NeighborGene) += GetAlleleDominanceWeight(NeighborGene);
			}
		}

		ECropGene WinningGene = Result.Genes[SlotIdx];
		float BestScore = GetAlleleDominanceWeight(WinningGene);

		for (const TPair<ECropGene, float>& Pair : AlleleScores)
		{
			if (Pair.Value > BestScore)
			{
				BestScore = Pair.Value;
				WinningGene = Pair.Key;
			}
		}

		Result.Genes[SlotIdx] = WinningGene;
	}

	return Result;
}

ACropSeedling::ACropSeedling()
{
	Species = ECropSpecies::Hemp;
	HarvestItemName = TEXT("Cloth");
	BaseHarvestQuantity = 40;
	GeneSequence = FCropGeneSequence();
}

void ACropSeedling::SetGeneSequence(const FCropGeneSequence& InSequence)
{
	GeneSequence = InSequence;
}

FInventoryItem ACropSeedling::CalculateHarvestYield() const
{
	FInventoryItem Yield;
	Yield.Name = HarvestItemName;
	Yield.Quantity = FMath::Max(1, FMath::RoundToInt(static_cast<float>(BaseHarvestQuantity) * GeneSequence.GetYieldMultiplier()));
	return Yield;
}

AHempCropSeedling::AHempCropSeedling()
{
	Species = ECropSpecies::Hemp;
	HarvestItemName = TEXT("Cloth");
	BaseHarvestQuantity = 40;
	ItemID = TEXT("HempSeed");
	Identifier = TEXT("HempSeed");
}

ACornCropSeedling::ACornCropSeedling()
{
	Species = ECropSpecies::Corn;
	HarvestItemName = TEXT("Corn");
	BaseHarvestQuantity = 4;
	ItemID = TEXT("CornSeed");
	Identifier = TEXT("CornSeed");
}

APumpkinCropSeedling::APumpkinCropSeedling()
{
	Species = ECropSpecies::Pumpkin;
	HarvestItemName = TEXT("Pumpkin");
	BaseHarvestQuantity = 4;
	ItemID = TEXT("PumpkinSeed");
	Identifier = TEXT("PumpkinSeed");
}
