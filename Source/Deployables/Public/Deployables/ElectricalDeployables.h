// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/Deployable.h"
#include "Deployables/ItemContainerBase.h"
#include "Components/ElectricNodeComponent.h"
#include "Interfaces/Interactable.h"
#include "ElectricalDeployables.generated.h"

/**
 * Large Solar Panel deployable generating up to 20 electrical power units.
 */
UCLASS()
class DEPLOYABLES_API ASolarPanelDeployable : public ADeployable
{
	GENERATED_BODY()

public:
	ASolarPanelDeployable();

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }
	void SetSunlightEfficiency(float InEfficiency01);

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	int32 MaxSolarOutput;
};

/**
 * Small Rechargeable Battery deployable storing charge and outputting 10 power units.
 */
UCLASS()
class DEPLOYABLES_API ASmallBatteryDeployable : public ADeployable
{
	GENERATED_BODY()

public:
	ASmallBatteryDeployable();

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;
};

/**
 * Toggleable Electrical Switch deployable.
 */
UCLASS()
class DEPLOYABLES_API AElectricSwitchDeployable : public ADeployable, public IInteractable
{
	GENERATED_BODY()

public:
	AElectricSwitchDeployable();

	virtual void Interact(AActor* Interactor) override;
	virtual FString PromptText() override;

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;
};

/**
 * 3-Way Electrical Splitter deployable.
 */
UCLASS()
class DEPLOYABLES_API AElectricSplitterDeployable : public ADeployable
{
	GENERATED_BODY()

public:
	AElectricSplitterDeployable();

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;
};

/**
 * Powered Automated Defense Turret (requires 10 electrical power).
 * Engages targets not authorized on the turret or nearby Tool Cupboard.
 */
UCLASS()
class DEPLOYABLES_API AAutoTurretDeployable : public AItemContainerBase
{
	GENERATED_BODY()

public:
	AAutoTurretDeployable();

	UElectricNodeComponent* GetElectricNode() const { return ElectricNode; }

	bool IsTurretActive() const;
	void AuthorizePlayer(const FString& PlayerUniqueID);
	bool IsPlayerAuthorizedOnTurret(const FString& PlayerUniqueID) const;

	/** Evaluates and engages TargetActor if powered, in range, and unauthorized. */
	bool TryEngageTarget(AActor* TargetActor, const FString& TargetPlayerUniqueID = TEXT(""));

private:
	UPROPERTY(VisibleAnywhere)
	UElectricNodeComponent* ElectricNode;

	UPROPERTY(EditDefaultsOnly, Category = "Defense")
	float SightRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Defense")
	float ShotDamage;

	UPROPERTY(SaveGame)
	TArray<FString> AuthorizedPlayers;
};

/**
 * Mechanical Shotgun Trap (requires 0 electrical power).
 * Fires a high-damage buckshot blast when an unauthorized target enters its trigger zone.
 */
UCLASS()
class DEPLOYABLES_API AShotgunTrapDeployable : public AItemContainerBase
{
	GENERATED_BODY()

public:
	AShotgunTrapDeployable();

	void AuthorizePlayer(const FString& PlayerUniqueID);
	bool IsPlayerAuthorizedOnTrap(const FString& PlayerUniqueID) const;

	/** Evaluates and triggers buckshot damage if IntruderActor is in range and unauthorized. */
	bool TryTriggerTrap(AActor* IntruderActor, const FString& IntruderPlayerUniqueID = TEXT(""));

private:
	UPROPERTY(EditDefaultsOnly, Category = "Defense")
	float TriggerRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Defense")
	float BuckshotDamage;

	UPROPERTY(SaveGame)
	TArray<FString> AuthorizedPlayers;
};
