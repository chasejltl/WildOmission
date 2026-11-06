// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Deployable.h"
#include "Deployables/ItemContainerBase.h"
#include "Interfaces/Interactable.h"
#include "WorkbenchDeployable.generated.h"

/**
 * Rust-style tiered Workbench deployable (Tier 1, Tier 2, Tier 3).
 * Grants nearby players access to recipes requiring up to WorkbenchTier.
 */
UCLASS()
class DEPLOYABLES_API AWorkbenchDeployable : public ADeployable, public IInteractable
{
	GENERATED_BODY()

public:
	AWorkbenchDeployable();

	virtual void OnSpawn() override;

	// Begin IInteractable Interface
	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;
	// End IInteractable Interface

	uint8 GetWorkbenchTier() const { return WorkbenchTier; }
	float GetProximityRadius() const { return ProximityRadius; }
	bool IsLocationWithinRange(const FVector& Location) const;

	static uint8 GetHighestWorkbenchTierAtLocation(const FVector& Location);
	static const TArray<AWorkbenchDeployable*>& GetAllWorkbenches();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Workbench")
	uint8 WorkbenchTier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Workbench")
	float ProximityRadius;

private:
	void RegisterActiveWorkbench();
	void UnregisterActiveWorkbench();
};

UCLASS()
class DEPLOYABLES_API AWorkbenchTier1 : public AWorkbenchDeployable
{
	GENERATED_BODY()

public:
	AWorkbenchTier1();
};

UCLASS()
class DEPLOYABLES_API AWorkbenchTier2 : public AWorkbenchDeployable
{
	GENERATED_BODY()

public:
	AWorkbenchTier2();
};

UCLASS()
class DEPLOYABLES_API AWorkbenchTier3 : public AWorkbenchDeployable
{
	GENERATED_BODY()

public:
	AWorkbenchTier3();
};

/**
 * Rust-style Research Table deployable.
 * Allows players to spend Scrap (either inside the table container or their own inventory)
 * to permanently unlock crafting blueprints on their UCraftingComponent.
 */
UCLASS()
class DEPLOYABLES_API AResearchTableDeployable : public AItemContainerBase
{
	GENERATED_BODY()

public:
	AResearchTableDeployable();

	/**
	 * Attempts to unlock the blueprint for RecipeName on Researcher's UCraftingComponent,
	 * consuming the required Scrap from the Research Table container or Researcher's inventory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Research")
	bool ResearchItemForActor(AActor* Researcher, const FName& RecipeName);
};
