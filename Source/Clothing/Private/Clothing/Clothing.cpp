// Copyright Telephone Studios. All Rights Reserved.


#include "Clothing/Clothing.h"

// Sets default values for this component's properties
UClothing::UClothing()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UClothing::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UClothing::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

TEnumAsByte<EClothingType> UClothing::GetType() const
{
	return Type;
}

void UClothing::ApplyItemPreset(const FName& ItemID)
{
	ProtectionStats = FClothingProtectionStats::GetPresetForItem(ItemID);
}

FClothingProtectionStats FClothingProtectionStats::GetPresetForItem(const FName& ItemID)
{
	FClothingProtectionStats Stats;
	if (ItemID == TEXT("HazmatSuit"))
	{
		Stats.ProjectileProtection = 0.30f;
		Stats.MeleeProtection = 0.30f;
		Stats.RadiationProtection = 0.50f; // Blocks up to 25 rads/s completely and halves extreme rads
		Stats.ColdInsulationCelsius = 8.0f;
	}
	else if (ItemID == TEXT("RoadSignJacket"))
	{
		Stats.ProjectileProtection = 0.35f;
		Stats.MeleeProtection = 0.45f;
		Stats.RadiationProtection = 0.10f;
		Stats.ColdInsulationCelsius = 2.0f;
	}
	else if (ItemID == TEXT("RoadSignKilt"))
	{
		Stats.ProjectileProtection = 0.30f;
		Stats.MeleeProtection = 0.40f;
		Stats.RadiationProtection = 0.08f;
		Stats.ColdInsulationCelsius = 2.0f;
	}
	else if (ItemID == TEXT("MetalFacemask"))
	{
		Stats.ProjectileProtection = 0.50f;
		Stats.MeleeProtection = 0.70f;
		Stats.RadiationProtection = 0.05f;
		Stats.ColdInsulationCelsius = -2.0f;
	}
	else if (ItemID == TEXT("MetalChestplate"))
	{
		Stats.ProjectileProtection = 0.50f;
		Stats.MeleeProtection = 0.65f;
		Stats.RadiationProtection = 0.05f;
		Stats.ColdInsulationCelsius = -4.0f;
	}
	return Stats;
}

