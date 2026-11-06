// Copyright Telephone Studios. All Rights Reserved.

#include "Components/ElectricNodeComponent.h"
#include "ServerRules.h"

UElectricNodeComponent::UElectricNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	NodeType = EElectricNodeType::Consumer;
	GeneratedPower = 0;
	RequiredPower = 1;
	MaxOutputs = 1;
	MaxStoredCharge = 150.0f;
	CurrentStoredCharge = 50.0f;
	bSwitchOn = true;
	CurrentInputPower = 0;
}

void UElectricNodeComponent::ConfigureNode(EElectricNodeType InType, int32 InGeneratedOrDischargePower, int32 InRequiredPower)
{
	NodeType = InType;
	GeneratedPower = FMath::Max(0, InGeneratedOrDischargePower);
	RequiredPower = FMath::Max(0, InRequiredPower);
	MaxOutputs = (InType == EElectricNodeType::Splitter) ? 3 : 1;
}

bool UElectricNodeComponent::ConnectOutputTo(UElectricNodeComponent* TargetNode, float MaxWireDistance)
{
	if (TargetNode == nullptr || TargetNode == this)
	{
		return false;
	}

	if (ConnectedOutputs.Contains(TargetNode))
	{
		return true;
	}

	if (ConnectedOutputs.Num() >= MaxOutputs)
	{
		return false;
	}

	if (GetOwner() != nullptr && TargetNode->GetOwner() != nullptr && MaxWireDistance > 0.0f)
	{
		const float Dist = FVector::Distance(GetOwner()->GetActorLocation(), TargetNode->GetOwner()->GetActorLocation());
		if (Dist > MaxWireDistance)
		{
			return false;
		}
	}

	ConnectedOutputs.Add(TargetNode);
	TargetNode->ConnectedInputs.AddUnique(this);

	RecalculateNetworkPower();
	return true;
}

void UElectricNodeComponent::DisconnectOutput(UElectricNodeComponent* TargetNode)
{
	if (TargetNode == nullptr)
	{
		return;
	}

	ConnectedOutputs.Remove(TargetNode);
	TargetNode->ConnectedInputs.Remove(this);

	RecalculateNetworkPower();
	TargetNode->RecalculateNetworkPower();
}

void UElectricNodeComponent::DisconnectAll()
{
	TArray<UElectricNodeComponent*> OldOutputs = ConnectedOutputs;
	for (UElectricNodeComponent* OutNode : OldOutputs)
	{
		if (IsValid(OutNode))
		{
			OutNode->ConnectedInputs.Remove(this);
		}
	}
	ConnectedOutputs.Empty();

	TArray<UElectricNodeComponent*> OldInputs = ConnectedInputs;
	for (UElectricNodeComponent* InNode : OldInputs)
	{
		if (IsValid(InNode))
		{
			InNode->ConnectedOutputs.Remove(this);
		}
	}
	ConnectedInputs.Empty();

	RecalculateNetworkPower();
}

void UElectricNodeComponent::SetSwitchState(bool bInOn)
{
	if (bSwitchOn != bInOn)
	{
		bSwitchOn = bInOn;
		RecalculateNetworkPower();
	}
}

void UElectricNodeComponent::SetGeneratedPower(int32 InPower)
{
	GeneratedPower = FMath::Max(0, InPower);
	RecalculateNetworkPower();
}

void UElectricNodeComponent::SetStoredCharge(float InCharge)
{
	CurrentStoredCharge = FMath::Clamp(InCharge, 0.0f, MaxStoredCharge);
	RecalculateNetworkPower();
}

void UElectricNodeComponent::StepBatterySimulation(float DeltaSeconds)
{
	if (NodeType != EElectricNodeType::Battery || DeltaSeconds <= 0.0f)
	{
		return;
	}

	// Charge from input power at 80% efficiency; discharge when driving active outputs
	const float ChargeDelta = (static_cast<float>(CurrentInputPower) * 0.8f - (ConnectedOutputs.Num() > 0 ? static_cast<float>(GeneratedPower) : 0.0f)) * (DeltaSeconds / 60.0f);
	SetStoredCharge(CurrentStoredCharge + ChargeDelta);
}

bool UElectricNodeComponent::IsPowered() const
{
	const UServerRules* Rules = UServerRules::Get();
	if (Rules != nullptr && !Rules->bElectricityEnabled)
	{
		return true;
	}

	switch (NodeType)
	{
	case EElectricNodeType::Generator:
		return GeneratedPower > 0;
	case EElectricNodeType::Battery:
		return CurrentStoredCharge > 0.0f || CurrentInputPower >= GeneratedPower;
	case EElectricNodeType::Switch:
		return bSwitchOn && CurrentInputPower > 0;
	case EElectricNodeType::Splitter:
		return CurrentInputPower > 0;
	case EElectricNodeType::Consumer:
	default:
		return CurrentInputPower >= RequiredPower;
	}
}

int32 UElectricNodeComponent::GetOutputPowerForConnection(int32 OutputIndex) const
{
	if (OutputIndex < 0 || OutputIndex >= ConnectedOutputs.Num())
	{
		return 0;
	}

	switch (NodeType)
	{
	case EElectricNodeType::Generator:
		return GeneratedPower;
	case EElectricNodeType::Battery:
		return (CurrentStoredCharge > 0.0f || CurrentInputPower > 0) ? GeneratedPower : 0;
	case EElectricNodeType::Switch:
		return bSwitchOn ? CurrentInputPower : 0;
	case EElectricNodeType::Splitter:
	{
		const int32 ActiveCount = ConnectedOutputs.Num();
		return (ActiveCount > 0) ? (CurrentInputPower / ActiveCount) : 0;
	}
	case EElectricNodeType::Consumer:
	default:
		return FMath::Max(0, CurrentInputPower - RequiredPower);
	}
}

void UElectricNodeComponent::RecalculateNetworkPower()
{
	TSet<UElectricNodeComponent*> Visited;
	PropagatePowerRecursive(Visited);
}

void UElectricNodeComponent::PropagatePowerRecursive(TSet<UElectricNodeComponent*>& VisitedNodes)
{
	if (VisitedNodes.Contains(this))
	{
		return;
	}
	VisitedNodes.Add(this);

	const bool bWasPowered = IsPowered();

	int32 SumInput = 0;
	for (UElectricNodeComponent* InputNode : ConnectedInputs)
	{
		if (!IsValid(InputNode))
		{
			continue;
		}
		const int32 OutputIdx = InputNode->ConnectedOutputs.IndexOfByKey(this);
		if (OutputIdx != INDEX_NONE)
		{
			SumInput += InputNode->GetOutputPowerForConnection(OutputIdx);
		}
	}
	CurrentInputPower = SumInput;

	const bool bNowPowered = IsPowered();
	if (bWasPowered != bNowPowered && OnPowerStateChanged.IsBound())
	{
		OnPowerStateChanged.Broadcast(bNowPowered);
	}

	for (UElectricNodeComponent* OutNode : ConnectedOutputs)
	{
		if (IsValid(OutNode))
		{
			OutNode->PropagatePowerRecursive(VisitedNodes);
		}
	}
}
