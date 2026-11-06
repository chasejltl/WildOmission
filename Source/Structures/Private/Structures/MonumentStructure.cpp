// Copyright Telephone Studios. All Rights Reserved.

#include "Structures/MonumentStructure.h"
#include "Components/VitalsComponent.h"
#include "ServerRules.h"

AMonumentStructure::AMonumentStructure()
{
	MonumentTier = EMonumentTier::Tier1_Airfield;
	RadiationRadius = 6000.0f;
	RadiationRadsPerSecond = 5.0f;
	BuildExclusionRadius = 7500.0f;
}

bool AMonumentStructure::IsLocationInRadiationZone(const FVector& Location) const
{
	return FVector::DistSquared(GetActorLocation(), Location) <= FMath::Square(RadiationRadius);
}

bool AMonumentStructure::IsLocationInBuildExclusionZone(const FVector& Location) const
{
	return FVector::DistSquared(GetActorLocation(), Location) <= FMath::Square(BuildExclusionRadius);
}

void AMonumentStructure::ApplyMonumentRadiationToVitals(UVitalsComponent* TargetVitals, float DeltaSeconds) const
{
	if (TargetVitals == nullptr || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && !Rules->bRadiationEnabled)
	{
		return;
	}

	TargetVitals->AddRadiationExposure(RadiationRadsPerSecond * DeltaSeconds);
}

TArray<FInventoryItem> AMonumentStructure::GenerateLockedCrateLoot() const
{
	TArray<FInventoryItem> Loot;
	auto AddLoot = [&Loot](const FName& ItemName, int32 Qty)
	{
		FInventoryItem Item;
		Item.Name = ItemName;
		Item.Quantity = Qty;
		Loot.Add(Item);
	};

	switch (MonumentTier)
	{
	case EMonumentTier::Tier1_Airfield:
		AddLoot(TEXT("Scrap"), 150);
		AddLoot(TEXT("HighQualityMetal"), 15);
		AddLoot(TEXT("CustomSMG"), 1);
		AddLoot(TEXT("PistolBullet"), 64);
		AddLoot(TEXT("MedicalSyringe"), 2);
		break;

	case EMonumentTier::Tier2_PowerPlant:
		AddLoot(TEXT("Scrap"), 300);
		AddLoot(TEXT("HighQualityMetal"), 35);
		AddLoot(TEXT("BoltActionRifle"), 1);
		AddLoot(TEXT("RifleBullet"), 64);
		AddLoot(TEXT("HazmatSuit"), 1);
		AddLoot(TEXT("MedicalSyringe"), 4);
		break;

	case EMonumentTier::Tier3_MilitaryTunnels:
	default:
		AddLoot(TEXT("Scrap"), 500);
		AddLoot(TEXT("HighQualityMetal"), 60);
		AddLoot(TEXT("AssaultRifle"), 1);
		AddLoot(TEXT("TimedExplosiveCharge"), 2);
		AddLoot(TEXT("RifleBullet"), 128);
		AddLoot(TEXT("MetalFacemask"), 1);
		AddLoot(TEXT("MetalChestplate"), 1);
		break;
	}

	return Loot;
}

AAirfieldMonument::AAirfieldMonument()
{
	MonumentTier = EMonumentTier::Tier1_Airfield;
	RadiationRadius = 6000.0f;
	RadiationRadsPerSecond = 5.0f;
	BuildExclusionRadius = 7500.0f;
}

APowerPlantMonument::APowerPlantMonument()
{
	MonumentTier = EMonumentTier::Tier2_PowerPlant;
	RadiationRadius = 8000.0f;
	RadiationRadsPerSecond = 15.0f;
	BuildExclusionRadius = 10000.0f;
}

AMilitaryTunnelsMonument::AMilitaryTunnelsMonument()
{
	MonumentTier = EMonumentTier::Tier3_MilitaryTunnels;
	RadiationRadius = 9000.0f;
	RadiationRadsPerSecond = 25.0f;
	BuildExclusionRadius = 11500.0f;
}
