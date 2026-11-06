// Copyright Telephone Studios. All Rights Reserved.


#include "Components/CraftingComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/InventoryManipulatorComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Log.h"

static UDataTable* RecipeDataTable = nullptr;
static TMap<FName, FCraftingRecipe> RegisteredRecipes;
static bool bPhase1RecipesSeeded = false;

// Sets default values for this component's properties
UCraftingComponent::UCraftingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	HighestAvailableWorkbenchTier = 0;

	static ConstructorHelpers::FObjectFinder<UDataTable> RecipeDataTableBlueprint(TEXT("/Game/Crafting/DataTables/DT_CraftingRecipes"));
	if (RecipeDataTableBlueprint.Succeeded())
	{
		RecipeDataTable = RecipeDataTableBlueprint.Object;
	}

	EnsureDefaultPhase1RecipesRegistered();
}

void UCraftingComponent::EnsureDefaultPhase1RecipesRegistered()
{
	if (bPhase1RecipesSeeded)
	{
		return;
	}
	bPhase1RecipesSeeded = true;

	auto AddRecipe = [](const FName& Name, int32 YieldQty, int32 CraftSeconds, uint8 WorkbenchTier, const TArray<TPair<FName, int32>>& Ings)
	{
		FCraftingRecipe Recipe;
		Recipe.YieldQuantity = YieldQty;
		Recipe.CraftTime = CraftSeconds;
		Recipe.RequiredWorkbenchTier = WorkbenchTier;
		for (const TPair<FName, int32>& Pair : Ings)
		{
			FInventoryItem Ing;
			Ing.Name = Pair.Key;
			Ing.Quantity = Pair.Value;
			Recipe.Ingredients.Add(Ing);
		}
		RegisteredRecipes.Add(Name, Recipe);
	};

	// Refined derivatives & fuels
	AddRecipe(TEXT("LowGradeFuel"), 4, 2, 0, { { TEXT("AnimalFat"), 3 }, { TEXT("Cloth"), 1 } });
	AddRecipe(TEXT("Gunpowder"), 10, 3, 1, { { TEXT("Charcoal"), 30 }, { TEXT("Sulfur"), 20 } });

	// Tiered Tools
	AddRecipe(TEXT("StoneHatchet"), 1, 5, 0, { { TEXT("Wood"), 200 }, { TEXT("Stone"), 100 } });
	AddRecipe(TEXT("StonePickaxe"), 1, 5, 0, { { TEXT("Wood"), 200 }, { TEXT("Stone"), 100 } });
	AddRecipe(TEXT("MetalHatchet"), 1, 10, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 75 } });
	AddRecipe(TEXT("MetalPickaxe"), 1, 10, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 125 } });

	// Primitive Weapons
	AddRecipe(TEXT("WoodenSpear"), 1, 5, 0, { { TEXT("Wood"), 300 } });
	AddRecipe(TEXT("StoneSpear"), 1, 8, 0, { { TEXT("WoodenSpear"), 1 }, { TEXT("Stone"), 100 }, { TEXT("Cloth"), 5 } });
	AddRecipe(TEXT("HuntingBow"), 1, 8, 0, { { TEXT("Wood"), 200 }, { TEXT("Cloth"), 50 } });
	AddRecipe(TEXT("Crossbow"), 1, 15, 1, { { TEXT("Wood"), 200 }, { TEXT("MetalFragments"), 75 }, { TEXT("Cloth"), 20 } });
	AddRecipe(TEXT("HandmadeRevolver"), 1, 20, 1, { { TEXT("Wood"), 100 }, { TEXT("MetalFragments"), 125 }, { TEXT("Cloth"), 25 } });

	// Ammo Types tied to Workbench crafting tiers
	AddRecipe(TEXT("WoodenArrow"), 2, 3, 0, { { TEXT("Wood"), 25 }, { TEXT("Stone"), 10 } });
	AddRecipe(TEXT("BoneArrow"), 3, 3, 0, { { TEXT("Wood"), 25 }, { TEXT("BoneFragments"), 10 } });
	AddRecipe(TEXT("PistolBullet"), 4, 5, 1, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 5 } });
	AddRecipe(TEXT("RifleBullet"), 3, 8, 2, { { TEXT("MetalFragments"), 10 }, { TEXT("Gunpowder"), 10 } });
}

void UCraftingComponent::RegisterRecipe(const FName& RecipeName, const FCraftingRecipe& RecipeData)
{
	EnsureDefaultPhase1RecipesRegistered();
	RegisteredRecipes.Add(RecipeName, RecipeData);
}

void UCraftingComponent::SetHighestAvailableWorkbenchTier(uint8 InTier)
{
	HighestAvailableWorkbenchTier = InTier;
}

uint8 UCraftingComponent::GetHighestAvailableWorkbenchTier() const
{
	return HighestAvailableWorkbenchTier;
}

void UCraftingComponent::Server_CraftItem_Implementation(const FName& ItemToCraft)
{
	AActor* OwnerActor = GetOwner();
	FCraftingRecipe* RecipeData = GetRecipe(ItemToCraft);
	if (OwnerActor == nullptr || RecipeData == nullptr)
	{
		return;
	}

	if (RecipeData->RequiredWorkbenchTier > HighestAvailableWorkbenchTier)
	{
		UE_LOG(LogCrafting, Display, TEXT("Cannot craft %s: requires Workbench Tier %d (have Tier %d)."),
			*ItemToCraft.ToString(), RecipeData->RequiredWorkbenchTier, HighestAvailableWorkbenchTier);
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = OwnerActor->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}
	
	FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
	{
		const int32 IngredientAmountInInventory = InventoryContents->GetItemQuantity(Ingredient.Name);
		
		if (IngredientAmountInInventory < Ingredient.Quantity)
		{
			return;
		}
	}

	for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
	{
		OwnerInventoryComponent->RemoveItem(Ingredient);
	}

	FInventoryItem Yield;
	Yield.Name = ItemToCraft;
	Yield.Quantity = RecipeData->YieldQuantity;

	OwnerInventoryComponent->AddItem(Yield);

	if (OnItemCrafted.IsBound())
	{
		OnItemCrafted.Broadcast(ItemToCraft);
	}
}

TArray<FName> UCraftingComponent::GetAllRecipes()
{
	EnsureDefaultPhase1RecipesRegistered();
	TArray<FName> Names;
	if (RecipeDataTable != nullptr)
	{
		Names = RecipeDataTable->GetRowNames();
	}
	for (const TPair<FName, FCraftingRecipe>& Pair : RegisteredRecipes)
	{
		Names.AddUnique(Pair.Key);
	}
	return Names;
}

FCraftingRecipe* UCraftingComponent::GetRecipe(const FName& RecipeName)
{
	EnsureDefaultPhase1RecipesRegistered();
	if (RecipeDataTable != nullptr)
	{
		static const FString ContextString(TEXT("Recipe Context"));
		if (FCraftingRecipe* TableRow = RecipeDataTable->FindRow<FCraftingRecipe>(RecipeName, ContextString, false))
		{
			return TableRow;
		}
	}

	return RegisteredRecipes.Find(RecipeName);
}
