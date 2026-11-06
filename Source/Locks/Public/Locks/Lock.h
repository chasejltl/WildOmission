// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Lock.generated.h"

UCLASS()
class LOCKS_API ALock : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	ALock();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnPlacement(class ULockComponent* InOwnerLockComponent);

	// Begin IInteractable Implementation
	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;
	// End IInteractable Implementation

	void SetCode(const FString& NewCode, const FString& CodeSetterUniqueID = TEXT(""));
	void ClearCode();
	void SetLockedState(bool bInLocked);

	bool IsLocked() const;
	
	void AuthorizePlayer(const FString& PlayerUniqueID);
	
	bool IsAuthorized(const FString& PlayerUniqueID) const;
	bool IsAuthorized(class ULockModifierComponent* LockModifier) const;
	bool IsAuthorized(APawn* PlayerPawn) const;

	TArray<FString> GetAuthorizedPlayers() const;
	void SetAuthorizedPlayers(const TArray<FString>& InAuthorizedPlayers);
	FString GetCode() const;

	UStaticMesh* GetStaticMesh() const;

	class ULockComponent* GetOwnerLockComponent() const;

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlaySuccessSound();

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayFailureSound();

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayPlacementSound();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* SuccessSound;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* FailureSound;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* PlacementSound;

	UPROPERTY()
	class ULockComponent* OwnerLockComponent;

	UPROPERTY()
	TArray<FString> AuthorizedPlayers;

	UPROPERTY()
	FString Code;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Locked)
	bool Locked;

	UFUNCTION()
	void OnRep_Locked();

};
