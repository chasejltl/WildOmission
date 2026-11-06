// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ElectricNodeComponent.generated.h"

UENUM(BlueprintType)
enum class EElectricNodeType : uint8
{
	Generator = 0	UMETA(DisplayName = "Generator"),
	Battery = 1		UMETA(DisplayName = "Battery"),
	Switch = 2		UMETA(DisplayName = "Switch"),
	Splitter = 3	UMETA(DisplayName = "Splitter"),
	Consumer = 4	UMETA(DisplayName = "Consumer")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElectricPowerChangedSignature, bool, bIsPowered);

/**
 * Modular electrical circuit node component for Rust-style base automation.
 * Supports Generators, Rechargeable Batteries, Switches, Splitters, and Consumers.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEPLOYABLES_API UElectricNodeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UElectricNodeComponent();

	void ConfigureNode(EElectricNodeType InType, int32 InGeneratedOrDischargePower, int32 InRequiredPower = 0);

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	bool ConnectOutputTo(UElectricNodeComponent* TargetNode, float MaxWireDistance = 3000.0f);

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void DisconnectOutput(UElectricNodeComponent* TargetNode);

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void DisconnectAll();

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void RecalculateNetworkPower();

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void SetSwitchState(bool bInOn);

	UFUNCTION(BlueprintPure, Category = "Electricity")
	bool GetSwitchState() const { return bSwitchOn; }

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void SetGeneratedPower(int32 InPower);

	UFUNCTION(BlueprintPure, Category = "Electricity")
	int32 GetGeneratedPower() const { return GeneratedPower; }

	UFUNCTION(BlueprintPure, Category = "Electricity")
	int32 GetCurrentInputPower() const { return CurrentInputPower; }

	UFUNCTION(BlueprintPure, Category = "Electricity")
	int32 GetRequiredPower() const { return RequiredPower; }

	UFUNCTION(BlueprintPure, Category = "Electricity")
	float GetStoredCharge() const { return CurrentStoredCharge; }

	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void SetStoredCharge(float InCharge);

	/** Advances battery charge/discharge state by DeltaSeconds. */
	UFUNCTION(BlueprintCallable, Category = "Electricity")
	void StepBatterySimulation(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Electricity")
	bool IsPowered() const;

	UFUNCTION(BlueprintPure, Category = "Electricity")
	int32 GetOutputPowerForConnection(int32 OutputIndex) const;

	UPROPERTY(BlueprintAssignable, Category = "Electricity")
	FOnElectricPowerChangedSignature OnPowerStateChanged;

private:
	void PropagatePowerRecursive(TSet<UElectricNodeComponent*>& VisitedNodes);

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	EElectricNodeType NodeType;

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	int32 GeneratedPower;

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	int32 RequiredPower;

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	int32 MaxOutputs;

	UPROPERTY(EditDefaultsOnly, Category = "Electricity")
	float MaxStoredCharge;

	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Electricity")
	float CurrentStoredCharge;

	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Electricity")
	bool bSwitchOn;

	UPROPERTY(VisibleInstanceOnly, Category = "Electricity")
	int32 CurrentInputPower;

	UPROPERTY()
	TArray<UElectricNodeComponent*> ConnectedInputs;

	UPROPERTY()
	TArray<UElectricNodeComponent*> ConnectedOutputs;
};
