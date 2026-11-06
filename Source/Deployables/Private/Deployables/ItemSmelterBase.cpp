// Copyright Telephone Studios. All Rights Reserved.


#include "Deployables/ItemSmelterBase.h"
#include "Components/InventoryComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"

AItemSmelterBase::AItemSmelterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bCanSpawnOnGround = true;
	bCanSpawnOnFloor = true;
	bCanSpawnOnWall = false;
	bFollowsSurfaceNormal = true;

	SmeltTimeSeconds = 5.0f;
	SmeltCounter = 0.0f;
	ContainerName = TEXT("Item Smelter");

	GetInventoryComponent()->SetDisplayName(TEXT("Item Smelter"));
	GetInventoryComponent()->SetSlotCount(6);

	LightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightComponent"));
	LightComponent->SetupAttachment(MeshComponent);

	ParticlesComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
	ParticlesComponent->SetupAttachment(MeshComponent);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(MeshComponent);

	EnsureDefaultSmeltingRules();
}

void AItemSmelterBase::EnsureDefaultSmeltingRules()
{
	if (FuelSource.Fuel.Name.IsNone())
	{
		FuelSource.Fuel.Name = TEXT("Wood");
		FuelSource.Fuel.Quantity = 1;
	}
	if (FuelSource.Byproduct.Name.IsNone())
	{
		FuelSource.Byproduct.Name = TEXT("Charcoal");
		FuelSource.Byproduct.Quantity = 1;
	}

	if (Smeltables.IsEmpty())
	{
		auto AddSmeltPair = [this](const FName& RawName, int32 RawQty, const FName& OutName, int32 OutQty)
		{
			FSmeltResult Entry;
			Entry.RawItem.Name = RawName;
			Entry.RawItem.Quantity = RawQty;
			Entry.SmeltedItem.Name = OutName;
			Entry.SmeltedItem.Quantity = OutQty;
			Smeltables.Add(Entry);
		};

		AddSmeltPair(TEXT("MetalOre"), 1, TEXT("MetalFragments"), 1);
		AddSmeltPair(TEXT("SulfurOre"), 1, TEXT("Sulfur"), 1);
		AddSmeltPair(TEXT("HighQualityMetalOre"), 1, TEXT("HighQualityMetal"), 1);
	}
}

void AItemSmelterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemSmelterBase, bTurnedOn);
}

void AItemSmelterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !IsTurnedOn())
	{
		return;
	}

	SmeltCounter += DeltaTime;

	if (SmeltCounter >= SmeltTimeSeconds)
	{
		OnSmelt();
		SmeltCounter = 0.0f;
	}
}

bool AItemSmelterBase::IsTurnedOn() const
{
	return bTurnedOn;
}

void AItemSmelterBase::BeginPlay()
{
	Super::BeginPlay();

	OnRep_TurnedOn();
}

void AItemSmelterBase::OnSmelt()
{
	UInventoryComponent* SmelterInventoryComponent = GetInventoryComponent();
	if (SmelterInventoryComponent == nullptr)
	{
		return;
	}

	FInventoryContents* InventoryContents = SmelterInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	const int32& AmountOfFuel = InventoryContents->GetItemQuantity(FuelSource.Fuel.Name);
	if (AmountOfFuel < FuelSource.Fuel.Quantity)
	{
		Server_ToggleState(false);
		return;
	}

	if (!BurnFuel())
	{
		return;
	}

	SmeltAllSmeltables();
	ProduceByproduct();

	FlushNetDormancy();
}

bool AItemSmelterBase::HasFuel() const
{
	UInventoryComponent* SmelterInventoryComponent = GetInventoryComponent();
	if (SmelterInventoryComponent == nullptr)
	{
		return false;
	}

	FInventoryContents* InventoryContents = SmelterInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return false;
	}

	const int32& AmountOfFuel = InventoryContents->GetItemQuantity(FuelSource.Fuel.Name);
	if (AmountOfFuel < FuelSource.Fuel.Quantity)
	{
		return false;
	}

	return true;
}

bool AItemSmelterBase::BurnFuel()
{
	if (!HasFuel())
	{
		Server_ToggleState(false);
		return false;
	}
	
	UInventoryComponent* SmelterInventoryComponent = GetInventoryComponent();
	if (SmelterInventoryComponent == nullptr)
	{
		return false;
	}

	SmelterInventoryComponent->RemoveItem(FuelSource.Fuel);
	return true;
}

void AItemSmelterBase::SmeltAllSmeltables()
{
	UInventoryComponent* SmelterInventoryComponent = GetInventoryComponent();
	if (SmelterInventoryComponent == nullptr)
	{
		return;
	}

	FInventoryContents* InventoryContents = SmelterInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	for (const FSmeltResult& Smeltable : Smeltables)
	{
		int32 AmountOfSmeltable = InventoryContents->GetItemQuantity(Smeltable.RawItem.Name);
		if (AmountOfSmeltable < Smeltable.RawItem.Quantity)
		{
			continue;
		}

		SmelterInventoryComponent->RemoveItem(Smeltable.RawItem);
		SmelterInventoryComponent->AddItem(Smeltable.SmeltedItem, false, this);

		if (OnItemSmelted.IsBound())
		{
			OnItemSmelted.Broadcast(Smeltable.RawItem, Smeltable.SmeltedItem);
		}
	}
}

void AItemSmelterBase::ProduceByproduct()
{
	if (FuelSource.Byproduct.IsZero())
	{
		return;
	}

	UInventoryComponent* SmelterInventoryComponent = GetInventoryComponent();
	if (SmelterInventoryComponent == nullptr)
	{
		return;
	}

	SmelterInventoryComponent->AddItem(FuelSource.Byproduct);
}

void AItemSmelterBase::OnTurnedOn()
{
	MeshComponent->SetCustomPrimitiveDataFloat(0, 1.0f);
	LightComponent->SetVisibility(true);
	ParticlesComponent->Activate();
	AudioComponent->Play();
}

void AItemSmelterBase::OnTurnedOff()
{
	MeshComponent->SetCustomPrimitiveDataFloat(0, 0.0f);
	LightComponent->SetVisibility(false);
	ParticlesComponent->Deactivate();
	AudioComponent->Stop();
}

void AItemSmelterBase::OnRep_TurnedOn()
{
	IsTurnedOn() ? OnTurnedOn() : OnTurnedOff();
}

void AItemSmelterBase::OnLoadComplete_Implementation()
{
	OnRep_TurnedOn();
}

void AItemSmelterBase::Server_ToggleState_Implementation(bool NewState)
{
	if (!HasFuel())
	{
		bTurnedOn = false;
		OnRep_TurnedOn();
		return;
	}

	bTurnedOn = NewState;

	OnRep_TurnedOn();
}