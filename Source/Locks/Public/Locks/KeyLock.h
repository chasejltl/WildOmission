// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Locks/Lock.h"
#include "KeyLock.generated.h"

/**
 * Early-game wooden/iron Key Lock (crafted with 100 Wood).
 * Authorizes its owner directly without requiring a 4-digit electronic keypad code.
 */
UCLASS()
class LOCKS_API AKeyLock : public ALock
{
	GENERATED_BODY()

public:
	AKeyLock();

	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;

	/** Initializes and locks the Key Lock for the placing player's UniqueID. */
	void InitializeKeyOwner(const FString& OwnerUniqueID);
};
