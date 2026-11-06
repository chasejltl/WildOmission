// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Deployable.h"
#include "Interfaces/Interactable.h"
#include "ToolCupboard.generated.h"

/**
 * 
 */
UCLASS()
class DEPLOYABLES_API AToolCupboard : public ADeployable, public IInteractable
{
	GENERATED_BODY()
	
public:
	AToolCupboard();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnSpawn();

	// Begin IInteractable Interface
	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;
	// End IInteractable Interface

	void AuthorizePlayer(const FString& PlayerUniqueID);
	void DeauthorizePlayer(const FString& PlayerUniqueID);
	bool IsPlayerAuthorized(const FString& PlayerUniqueID) const;
	void ClearAuthorizedPlayers();

	bool IsWithinRange(const FVector& LocationToTest) const;

	static TArray<AToolCupboard*> GetAllToolCupboards();
	static bool IsPlayerAuthorizedAtLocation(const FString& PlayerUniqueID, const FVector& LocationToTest);

	class UInventoryComponent* GetUpkeepInventory() const { return UpkeepInventory; }

	void RegisterManagedBuildingBlock(class ABuildingBlock* BuildingBlock);
	TArray<class ABuildingBlock*> GetConnectedBuildingBlocks() const;

	/** Calculates the 24-hour upkeep resource tax across all connected non-Twig building blocks. */
	TMap<FName, int32> CalculateDailyUpkeepCost() const;

	/**
	 * Processes an upkeep & decay cycle for HoursElapsed (default 1.0 hour).
	 * Consumes required resources from UpkeepInventory; applies decay damage to starving building tiers.
	 * Returns true if all required upkeep resources were paid without starvation.
	 */
	bool ProcessUpkeepCycle(float HoursElapsed = 1.0f);

	/** Returns how many hours of upkeep protection remain in UpkeepInventory based on current daily cost. */
	float GetProtectedHoursRemaining() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere)
	class ULockComponent* LockComponent;

	UPROPERTY(VisibleAnywhere)
	class UInventoryComponent* UpkeepInventory;

	UPROPERTY(EditDefaultsOnly)
	float Range;
	
	UPROPERTY(VisibleAnywhere, Replicated, SaveGame)
	TArray<FString> AuthorizedPlayers;

	UPROPERTY()
	TArray<class ABuildingBlock*> ManagedBuildingBlocks;

	FTimerHandle UpkeepTimerHandle;

	void TickHourlyUpkeep();
	void OnLoadComplete_Implementation();

};
