// Copyright Telephone Studios. All Rights Reserved.


#include "Deployables/ToolCupboard.h"
#include "Deployables/BuildingBlock.h"
#include "Components/InventoryComponent.h"
#include "Components/LockComponent.h"
#include "Locks/Lock.h"
#include "Components/BuilderComponent.h"
#include "ServerRules.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static TArray<AToolCupboard*> AllToolCupboards;

AToolCupboard::AToolCupboard()
{
	SetNetDormancy(ENetDormancy::DORM_DormantAll);

	LockComponent = CreateDefaultSubobject<ULockComponent>(TEXT("LockComponent"));
	LockComponent->SetupAttachment(MeshComponent);

	UpkeepInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("UpkeepInventory"));
	if (UpkeepInventory != nullptr)
	{
		UpkeepInventory->SetDisplayName(TEXT("Tool Cupboard Upkeep"));
		UpkeepInventory->SetSlotCount(18);
	}

	Range = 5000.0f;
	AuthorizedPlayers = TArray<FString>();
}

void AToolCupboard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AToolCupboard, AuthorizedPlayers);
}

void AToolCupboard::OnSpawn()
{
	Super::OnSpawn();
	AllToolCupboards.Add(this);
}

void AToolCupboard::Interact(AActor* Interactor)
{
	if (Interactor == nullptr)
	{
		return;
	}

	if (IsValid(LockComponent->GetLock()) && !LockComponent->GetLock()->IsAuthorized(Cast<APawn>(Interactor)) && LockComponent->GetLock()->IsLocked())
	{
		return;
	}

	UBuilderComponent* InteractorBuilderComponent = Interactor->FindComponentByClass<UBuilderComponent>();
	if (InteractorBuilderComponent == nullptr)
	{
		return;
	}

	InteractorBuilderComponent->OpenToolCupboardMenu(this);
}

FString AToolCupboard::PromptText()
{
	return TEXT("open authorization menu");
}

void AToolCupboard::AuthorizePlayer(const FString& PlayerUniqueID)
{
	AuthorizedPlayers.Add(PlayerUniqueID);
	FlushNetDormancy();
}

void AToolCupboard::DeauthorizePlayer(const FString& PlayerUniqueID)
{
	AuthorizedPlayers.Remove(PlayerUniqueID);
	FlushNetDormancy();
}

bool AToolCupboard::IsPlayerAuthorized(const FString& PlayerUniqueID) const
{
	for (const FString& AuthorizedPlayer : AuthorizedPlayers)
	{
		if (AuthorizedPlayer != PlayerUniqueID)
		{
			continue;
		}

		return true;
	}

	return false;
}

void AToolCupboard::ClearAuthorizedPlayers()
{
	AuthorizedPlayers.Empty();
	FlushNetDormancy();
}

bool AToolCupboard::IsWithinRange(const FVector& LocationToTest) const
{
	const float Distance = FVector::Distance(this->GetActorLocation(), LocationToTest);
	if (Distance <= Range)
	{
		return true;
	}
	
	return false;
}

TArray<AToolCupboard*> AToolCupboard::GetAllToolCupboards()
{
	return AllToolCupboards;
}

bool AToolCupboard::IsPlayerAuthorizedAtLocation(const FString& PlayerUniqueID, const FVector& LocationToTest)
{
	bool bInAnyTCRange = false;
	for (const AToolCupboard* TC : AllToolCupboards)
	{
		if (!IsValid(TC) || !TC->IsWithinRange(LocationToTest))
		{
			continue;
		}
		bInAnyTCRange = true;
		if (!TC->IsPlayerAuthorized(PlayerUniqueID))
		{
			return false;
		}
	}
	return bInAnyTCRange ? true : true;
}

void AToolCupboard::RegisterManagedBuildingBlock(ABuildingBlock* BuildingBlock)
{
	if (IsValid(BuildingBlock))
	{
		ManagedBuildingBlocks.AddUnique(BuildingBlock);
	}
}

TArray<ABuildingBlock*> AToolCupboard::GetConnectedBuildingBlocks() const
{
	TArray<ABuildingBlock*> ConnectedBlocks;
	for (ABuildingBlock* Block : ManagedBuildingBlocks)
	{
		if (IsValid(Block))
		{
			ConnectedBlocks.AddUnique(Block);
		}
	}

	for (ABuildingBlock* WorldBlock : ABuildingBlock::GetAllBuildingBlocks())
	{
		if (IsValid(WorldBlock) && IsWithinRange(WorldBlock->GetActorLocation()))
		{
			ConnectedBlocks.AddUnique(WorldBlock);
		}
	}

	return ConnectedBlocks;
}

TMap<FName, int32> AToolCupboard::CalculateDailyUpkeepCost() const
{
	TMap<FName, int32> DailyCosts;

	const UServerRules* Rules = UServerRules::Get();
	if (Rules != nullptr && (!Rules->bUpkeepEnabled || Rules->UpkeepRateMultiplier <= 0.0f))
	{
		return DailyCosts;
	}

	const float ServerMultiplier = (Rules != nullptr) ? Rules->UpkeepRateMultiplier : 1.0f;
	const TArray<ABuildingBlock*> ConnectedBlocks = GetConnectedBuildingBlocks();

	int32 NonTwigCount = 0;
	for (const ABuildingBlock* Block : ConnectedBlocks)
	{
		if (IsValid(Block) && Block->GetBuildingTier() != EBuildingTier::Twig)
		{
			++NonTwigCount;
		}
	}

	if (NonTwigCount <= 0)
	{
		return DailyCosts;
	}

	const float TaxRate = FBuildingTierRules::GetUpkeepTaxBracketRate(NonTwigCount) * ServerMultiplier;
	for (const ABuildingBlock* Block : ConnectedBlocks)
	{
		if (!IsValid(Block))
		{
			continue;
		}

		const EBuildingTier Tier = Block->GetBuildingTier();
		const FName UpkeepResource = FBuildingTierRules::GetTierUpkeepResource(Tier);
		if (UpkeepResource.IsNone())
		{
			continue;
		}

		const FInventoryItem UpgradeCost = FBuildingTierRules::GetTierUpgradeCost(Tier);
		const int32 BlockDailyTax = FMath::Max(1, FMath::RoundToInt(static_cast<float>(UpgradeCost.Quantity) * TaxRate));
		DailyCosts.FindOrAdd(UpkeepResource) += BlockDailyTax;
	}

	return DailyCosts;
}

bool AToolCupboard::ProcessUpkeepCycle(float HoursElapsed)
{
	if (HoursElapsed <= 0.0f)
	{
		return true;
	}

	const UServerRules* Rules = UServerRules::Get();
	const bool bUpkeepActive = (Rules == nullptr) || Rules->bUpkeepEnabled;
	const bool bDecayActive = (Rules == nullptr) || Rules->bBuildingDecayEnabled;

	if (!bUpkeepActive)
	{
		return true;
	}

	const TMap<FName, int32> DailyCosts = CalculateDailyUpkeepCost();
	if (DailyCosts.IsEmpty())
	{
		return true;
	}

	const float DayFraction = FMath::Clamp(HoursElapsed / 24.0f, 0.0f, 1.0f);
	TSet<FName> StarvingResources;

	for (const TPair<FName, int32>& Pair : DailyCosts)
	{
		const FName& ResourceName = Pair.Key;
		const int32 RequiredForCycle = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Pair.Value) * DayFraction));

		int32 AvailableAmount = 0;
		if (UpkeepInventory != nullptr && UpkeepInventory->GetContents() != nullptr)
		{
			AvailableAmount = UpkeepInventory->GetContents()->GetItemQuantity(ResourceName);
		}

		if (AvailableAmount >= RequiredForCycle)
		{
			FInventoryItem TaxItem;
			TaxItem.Name = ResourceName;
			TaxItem.Quantity = RequiredForCycle;
			UpkeepInventory->RemoveItem(TaxItem);
		}
		else
		{
			if (AvailableAmount > 0 && UpkeepInventory != nullptr)
			{
				FInventoryItem RemainingItem;
				RemainingItem.Name = ResourceName;
				RemainingItem.Quantity = AvailableAmount;
				UpkeepInventory->RemoveItem(RemainingItem);
			}
			StarvingResources.Add(ResourceName);
		}
	}

	if (StarvingResources.Num() > 0 && bDecayActive)
	{
		TArray<ABuildingBlock*> ConnectedBlocks = GetConnectedBuildingBlocks();
		for (ABuildingBlock* Block : ConnectedBlocks)
		{
			if (!IsValid(Block))
			{
				continue;
			}

			const EBuildingTier Tier = Block->GetBuildingTier();
			const FName ResourceName = FBuildingTierRules::GetTierUpkeepResource(Tier);
			if (!StarvingResources.Contains(ResourceName))
			{
				continue;
			}

			const float DecayHours = FMath::Max(0.1f, FBuildingTierRules::GetTierDecayDurationHours(Tier));
			const float DecayDamage = (Block->GetMaxDurability() / DecayHours) * HoursElapsed;
			FDamageEvent DecayEvent;
			Block->TakeDamage(DecayDamage, DecayEvent, nullptr, this);
		}
	}

	return StarvingResources.IsEmpty();
}

float AToolCupboard::GetProtectedHoursRemaining() const
{
	const TMap<FName, int32> DailyCosts = CalculateDailyUpkeepCost();
	if (DailyCosts.IsEmpty())
	{
		return 999.0f;
	}

	if (UpkeepInventory == nullptr || UpkeepInventory->GetContents() == nullptr)
	{
		return 0.0f;
	}

	float MinHours = 999.0f;
	for (const TPair<FName, int32>& Pair : DailyCosts)
	{
		if (Pair.Value <= 0)
		{
			continue;
		}
		const int32 Available = UpkeepInventory->GetContents()->GetItemQuantity(Pair.Key);
		const float HoursForResource = (static_cast<float>(Available) / static_cast<float>(Pair.Value)) * 24.0f;
		MinHours = FMath::Min(MinHours, HoursForResource);
	}

	return FMath::Max(0.0f, MinHours);
}

void AToolCupboard::TickHourlyUpkeep()
{
	if (HasAuthority())
	{
		ProcessUpkeepCycle(1.0f);
	}
}

void AToolCupboard::BeginPlay()
{
	Super::BeginPlay();

	AllToolCupboards.AddUnique(this);

	if (HasAuthority() && GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(UpkeepTimerHandle, this, &AToolCupboard::TickHourlyUpkeep, 3600.0f, true);
	}
}

void AToolCupboard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(UpkeepTimerHandle);
	}
	AllToolCupboards.Remove(this);
	Super::EndPlay(EndPlayReason);
}

void AToolCupboard::OnLoadComplete_Implementation()
{
	AllToolCupboards.Add(this);
}
