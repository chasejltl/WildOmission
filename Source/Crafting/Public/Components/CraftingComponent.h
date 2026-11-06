// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structs/CraftingRecipe.h"
#include "CraftingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCraftedSignature, const FName&, ItemID);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRAFTING_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCraftingComponent();

	UFUNCTION(Server, Reliable)
	void Server_CraftItem(const FName& ItemToCraft);

	FOnItemCraftedSignature OnItemCrafted;

	static TArray<FName> GetAllRecipes();

	static FCraftingRecipe* GetRecipe(const FName& RecipeName);

	static void RegisterRecipe(const FName& RecipeName, const FCraftingRecipe& RecipeData);
	static void EnsureDefaultPhase1RecipesRegistered();

	void SetHighestAvailableWorkbenchTier(uint8 InTier);
	uint8 GetHighestAvailableWorkbenchTier() const;
	uint8 GetEffectiveWorkbenchTier() const;

	void UnlockBlueprint(const FName& RecipeName);
	bool IsBlueprintUnlocked(const FName& RecipeName) const;
	bool ResearchRecipeWithScrap(const FName& RecipeName);
	bool CanCraftRecipe(const FName& RecipeName) const;

	static void SetProximityWorkbenchQuery(TFunction<uint8(const FVector&)> InQueryFunc);
	static uint8 QueryProximityWorkbenchTier(const FVector& Location);

private:
	uint8 HighestAvailableWorkbenchTier;

	UPROPERTY(SaveGame)
	TSet<FName> UnlockedBlueprints;
};
