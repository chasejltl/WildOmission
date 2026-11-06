// Copyright Telephone Studios. All Rights Reserved.

#include "Deployables/ElectricalDeployables.h"
#include "Deployables/ToolCupboard.h"
#include "Components/InventoryComponent.h"
#include "Engine/DamageEvents.h"

ASolarPanelDeployable::ASolarPanelDeployable()
{
	MaxSolarOutput = 20;
	MaxDurability = 300.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("SolarPanel");
	Identifier = TEXT("SolarPanel");

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Generator, MaxSolarOutput, 0);
}

void ASolarPanelDeployable::SetSunlightEfficiency(float InEfficiency01)
{
	if (ElectricNode != nullptr)
	{
		const int32 Output = FMath::RoundToInt(static_cast<float>(MaxSolarOutput) * FMath::Clamp(InEfficiency01, 0.0f, 1.0f));
		ElectricNode->SetGeneratedPower(Output);
	}
}

ASmallBatteryDeployable::ASmallBatteryDeployable()
{
	MaxDurability = 250.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("SmallBattery");
	Identifier = TEXT("SmallBattery");

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Battery, 10, 0);
}

AElectricSwitchDeployable::AElectricSwitchDeployable()
{
	MaxDurability = 150.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("ElectricSwitch");
	Identifier = TEXT("ElectricSwitch");

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Switch, 0, 0);
}

void AElectricSwitchDeployable::Interact(AActor* Interactor)
{
	if (ElectricNode != nullptr)
	{
		ElectricNode->SetSwitchState(!ElectricNode->GetSwitchState());
	}
}

FString AElectricSwitchDeployable::PromptText()
{
	const bool bIsOn = (ElectricNode != nullptr) && ElectricNode->GetSwitchState();
	return bIsOn ? TEXT("turn switch off") : TEXT("turn switch on");
}

AElectricSplitterDeployable::AElectricSplitterDeployable()
{
	MaxDurability = 150.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("ElectricSplitter");
	Identifier = TEXT("ElectricSplitter");

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Splitter, 0, 0);
}

AAutoTurretDeployable::AAutoTurretDeployable()
{
	ContainerName = TEXT("Auto Turret");
	MaxDurability = 1000.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("AutoTurret");
	Identifier = TEXT("AutoTurret");

	SightRadius = 3000.0f;
	ShotDamage = 35.0f;

	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->SetDisplayName(TEXT("Auto Turret Ammo"));
		Inv->SetSlotCount(6);
	}

	ElectricNode = CreateDefaultSubobject<UElectricNodeComponent>(TEXT("ElectricNode"));
	ElectricNode->ConfigureNode(EElectricNodeType::Consumer, 0, 10);
}

bool AAutoTurretDeployable::IsTurretActive() const
{
	return ElectricNode != nullptr && ElectricNode->IsPowered();
}

void AAutoTurretDeployable::AuthorizePlayer(const FString& PlayerUniqueID)
{
	if (!PlayerUniqueID.IsEmpty())
	{
		AuthorizedPlayers.AddUnique(PlayerUniqueID);
	}
}

bool AAutoTurretDeployable::IsPlayerAuthorizedOnTurret(const FString& PlayerUniqueID) const
{
	if (PlayerUniqueID.IsEmpty())
	{
		return false;
	}

	if (AuthorizedPlayers.Contains(PlayerUniqueID))
	{
		return true;
	}

	for (const AToolCupboard* TC : AToolCupboard::GetAllToolCupboards())
	{
		if (IsValid(TC) && TC->IsWithinRange(GetActorLocation()) && TC->IsPlayerAuthorized(PlayerUniqueID))
		{
			return true;
		}
	}

	return false;
}

bool AAutoTurretDeployable::TryEngageTarget(AActor* TargetActor, const FString& TargetPlayerUniqueID)
{
	if (TargetActor == nullptr || TargetActor == this || !IsTurretActive())
	{
		return false;
	}

	if (FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(SightRadius))
	{
		return false;
	}

	if (IsPlayerAuthorizedOnTurret(TargetPlayerUniqueID))
	{
		return false;
	}

	// If ammo is stocked in the turret container, consume 1 round per shot
	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		if (FInventoryContents* Contents = Inv->GetContents())
		{
			if (Contents->GetItemQuantity(TEXT("RifleBullet")) > 0)
			{
				FInventoryItem Bullet;
				Bullet.Name = TEXT("RifleBullet");
				Bullet.Quantity = 1;
				Inv->RemoveItem(Bullet);
			}
			else if (Contents->GetItemQuantity(TEXT("PistolBullet")) > 0)
			{
				FInventoryItem Bullet;
				Bullet.Name = TEXT("PistolBullet");
				Bullet.Quantity = 1;
				Inv->RemoveItem(Bullet);
			}
		}
	}

	FDamageEvent TurretShotEvent;
	TargetActor->TakeDamage(ShotDamage, TurretShotEvent, nullptr, this);
	return true;
}

AShotgunTrapDeployable::AShotgunTrapDeployable()
{
	ContainerName = TEXT("Shotgun Trap");
	MaxDurability = 400.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("ShotgunTrap");
	Identifier = TEXT("ShotgunTrap");

	TriggerRadius = 500.0f;
	BuckshotDamage = 120.0f;

	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->SetDisplayName(TEXT("Shotgun Trap Ammo"));
		Inv->SetSlotCount(6);
	}
}

void AShotgunTrapDeployable::AuthorizePlayer(const FString& PlayerUniqueID)
{
	if (!PlayerUniqueID.IsEmpty())
	{
		AuthorizedPlayers.AddUnique(PlayerUniqueID);
	}
}

bool AShotgunTrapDeployable::IsPlayerAuthorizedOnTrap(const FString& PlayerUniqueID) const
{
	if (PlayerUniqueID.IsEmpty())
	{
		return false;
	}

	if (AuthorizedPlayers.Contains(PlayerUniqueID))
	{
		return true;
	}

	for (const AToolCupboard* TC : AToolCupboard::GetAllToolCupboards())
	{
		if (IsValid(TC) && TC->IsWithinRange(GetActorLocation()) && TC->IsPlayerAuthorized(PlayerUniqueID))
		{
			return true;
		}
	}

	return false;
}

bool AShotgunTrapDeployable::TryTriggerTrap(AActor* IntruderActor, const FString& IntruderPlayerUniqueID)
{
	if (IntruderActor == nullptr || IntruderActor == this)
	{
		return false;
	}

	if (FVector::DistSquared(GetActorLocation(), IntruderActor->GetActorLocation()) > FMath::Square(TriggerRadius))
	{
		return false;
	}

	if (IsPlayerAuthorizedOnTrap(IntruderPlayerUniqueID))
	{
		return false;
	}

	FDamageEvent BuckshotEvent;
	IntruderActor->TakeDamage(BuckshotDamage, BuckshotEvent, nullptr, this);
	return true;
}
