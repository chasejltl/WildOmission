// Copyright Telephone Studios. All Rights Reserved.

#include "Deployables/WorkbenchDeployable.h"
#include "Components/CraftingComponent.h"
#include "Components/InventoryComponent.h"
#include "Structs/CraftingRecipe.h"

static TArray<AWorkbenchDeployable*> ActiveWorkbenches;
static bool bProximityHookInstalled = false;

AWorkbenchDeployable::AWorkbenchDeployable()
{
	WorkbenchTier = 1;
	ProximityRadius = 400.0f;
	MaxDurability = 500.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("WorkbenchT1");
	Identifier = TEXT("WorkbenchT1");

	if (!bProximityHookInstalled)
	{
		bProximityHookInstalled = true;
		UCraftingComponent::SetProximityWorkbenchQuery(&AWorkbenchDeployable::GetHighestWorkbenchTierAtLocation);
	}
}

void AWorkbenchDeployable::OnSpawn()
{
	Super::OnSpawn();
	RegisterActiveWorkbench();
}

void AWorkbenchDeployable::BeginPlay()
{
	Super::BeginPlay();
	RegisterActiveWorkbench();
}

void AWorkbenchDeployable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterActiveWorkbench();
	Super::EndPlay(EndPlayReason);
}

void AWorkbenchDeployable::RegisterActiveWorkbench()
{
	ActiveWorkbenches.AddUnique(this);
}

void AWorkbenchDeployable::UnregisterActiveWorkbench()
{
	ActiveWorkbenches.Remove(this);
}

void AWorkbenchDeployable::Interact(AActor* Interactor)
{
	if (Interactor == nullptr)
	{
		return;
	}

	if (UCraftingComponent* CraftingComp = Interactor->FindComponentByClass<UCraftingComponent>())
	{
		const uint8 BestTier = FMath::Max(WorkbenchTier, GetHighestWorkbenchTierAtLocation(Interactor->GetActorLocation()));
		CraftingComp->SetHighestAvailableWorkbenchTier(BestTier);
	}
}

FString AWorkbenchDeployable::PromptText()
{
	return FString::Printf(TEXT("use Workbench Tier %d"), WorkbenchTier);
}

bool AWorkbenchDeployable::IsLocationWithinRange(const FVector& Location) const
{
	return FVector::DistSquared(GetActorLocation(), Location) <= FMath::Square(ProximityRadius);
}

uint8 AWorkbenchDeployable::GetHighestWorkbenchTierAtLocation(const FVector& Location)
{
	uint8 BestTier = 0;
	for (int32 i = ActiveWorkbenches.Num() - 1; i >= 0; --i)
	{
		AWorkbenchDeployable* Workbench = ActiveWorkbenches[i];
		if (!IsValid(Workbench))
		{
			ActiveWorkbenches.RemoveAtSwap(i);
			continue;
		}

		if (Workbench->IsLocationWithinRange(Location))
		{
			BestTier = FMath::Max(BestTier, Workbench->GetWorkbenchTier());
		}
	}
	return BestTier;
}

const TArray<AWorkbenchDeployable*>& AWorkbenchDeployable::GetAllWorkbenches()
{
	return ActiveWorkbenches;
}

AWorkbenchTier1::AWorkbenchTier1()
{
	WorkbenchTier = 1;
	MaxDurability = 500.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("WorkbenchT1");
	Identifier = TEXT("WorkbenchT1");
}

AWorkbenchTier2::AWorkbenchTier2()
{
	WorkbenchTier = 2;
	MaxDurability = 750.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("WorkbenchT2");
	Identifier = TEXT("WorkbenchT2");
}

AWorkbenchTier3::AWorkbenchTier3()
{
	WorkbenchTier = 3;
	MaxDurability = 1000.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("WorkbenchT3");
	Identifier = TEXT("WorkbenchT3");
}

AResearchTableDeployable::AResearchTableDeployable()
{
	ContainerName = TEXT("Research Table");
	MaxDurability = 400.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("ResearchTable");
	Identifier = TEXT("ResearchTable");

	if (UInventoryComponent* TableInventory = GetInventoryComponent())
	{
		TableInventory->SetSlotCount(6);
		TableInventory->SetDisplayName(TEXT("Research Table"));
	}
}

bool AResearchTableDeployable::ResearchItemForActor(AActor* Researcher, const FName& RecipeName)
{
	if (Researcher == nullptr)
	{
		return false;
	}

	UCraftingComponent* CraftingComp = Researcher->FindComponentByClass<UCraftingComponent>();
	const FCraftingRecipe* RecipeData = UCraftingComponent::GetRecipe(RecipeName);
	if (CraftingComp == nullptr || RecipeData == nullptr)
	{
		return false;
	}

	if (CraftingComp->IsBlueprintUnlocked(RecipeName))
	{
		return true;
	}

	const int32 ScrapRequired = RecipeData->ResearchScrapCost;
	if (ScrapRequired <= 0)
	{
		CraftingComp->UnlockBlueprint(RecipeName);
		return true;
	}

	// First check Research Table container inventory for Scrap
	UInventoryComponent* TableInventory = GetInventoryComponent();
	if (TableInventory != nullptr && TableInventory->GetContents() != nullptr &&
		TableInventory->GetContents()->GetItemQuantity(TEXT("Scrap")) >= ScrapRequired)
	{
		FInventoryItem ScrapItem;
		ScrapItem.Name = TEXT("Scrap");
		ScrapItem.Quantity = ScrapRequired;
		TableInventory->RemoveItem(ScrapItem);
		CraftingComp->UnlockBlueprint(RecipeName);
		return true;
	}

	// Otherwise fall back to the Researcher's inventory
	return CraftingComp->ResearchRecipeWithScrap(RecipeName);
}
