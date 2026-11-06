// Copyright Telephone Studios. All Rights Reserved.

#include "Locks/KeyLock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

AKeyLock::AKeyLock()
{
	SetLockedState(true);
}

void AKeyLock::InitializeKeyOwner(const FString& OwnerUniqueID)
{
	if (!OwnerUniqueID.IsEmpty())
	{
		AuthorizePlayer(OwnerUniqueID);
	}
	SetLockedState(true);
}

void AKeyLock::Interact(AActor* Interactor)
{
	APawn* InteractorPawn = Cast<APawn>(Interactor);
	if (InteractorPawn == nullptr)
	{
		return;
	}

	// If no player has claimed the key lock yet, the first interactor claims and locks it
	if (GetAuthorizedPlayers().IsEmpty())
	{
		if (APlayerState* PS = InteractorPawn->GetPlayerState())
		{
			InitializeKeyOwner(PS->GetUniqueId().ToString());
			Multi_PlaySuccessSound();
			return;
		}
	}

	if (IsAuthorized(InteractorPawn))
	{
		SetLockedState(!IsLocked());
		Multi_PlaySuccessSound();
	}
	else
	{
		Multi_PlayFailureSound();
	}
}

FString AKeyLock::PromptText()
{
	return IsLocked() ? TEXT("unlock key lock") : TEXT("lock key lock");
}
