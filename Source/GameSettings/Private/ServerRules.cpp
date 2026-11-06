// Copyright Telephone Studios. All Rights Reserved.

#include "ServerRules.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#include "Log.h"

UServerRules::UServerRules()
{
	ResetToDefaults();
}

void UServerRules::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();
}

UServerRules* UServerRules::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject != nullptr && WorldContextObject->GetWorld() != nullptr)
	{
		if (UGameInstance* GI = WorldContextObject->GetWorld()->GetGameInstance())
		{
			if (UServerRules* Subsystem = GI->GetSubsystem<UServerRules>())
			{
				return Subsystem;
			}
		}
	}

	return GetMutableDefault<UServerRules>();
}

const UServerRules* UServerRules::GetDefault()
{
	return ::GetDefault<UServerRules>();
}

void UServerRules::ResetToDefaults()
{
	ResourceGatherMultiplier = 1.0f;
	bEnforceToolTierGates = true;

	bTwigDecayEnabled = true;
	TwigDecayDamagePerMinute = 2.0f;
	bUpkeepEnabled = true;
	UpkeepIntervalHours = 24.0f;
	UpkeepCostMultiplier = 1.0f;
	bBuildingDecayEnabled = true;

	bPvPEnabled = true;
	bOfflineRaidProtection = false;
	bWorkbenchTierGatingEnabled = true;
	bElectricityEnabled = true;

	DayNightSpeedMultiplier = 1.0f;
	TorchBurnTickSeconds = 3.0f;
	bRadiationEnabled = true;
	bTemperatureEnabled = true;
	bStructuralWindDestructionEnabled = true;
	bAdvancedAIEnabled = true;
}

bool UServerRules::SetRuleByString(const FString& Key, const FString& Value)
{
	FProperty* Prop = GetClass()->FindPropertyByName(FName(*Key));
	if (Prop == nullptr)
	{
		return false;
	}

	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
	{
		const bool bVal = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
		BoolProp->SetPropertyValue_InContainer(this, bVal);
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		FloatProp->SetPropertyValue_InContainer(this, FCString::Atof(*Value));
	}
	else
	{
		return false;
	}

	SaveConfig();
	OnServerRuleChanged.Broadcast(Prop->GetFName());
	return true;
}

TArray<FString> UServerRules::GetRulesSummary() const
{
	TArray<FString> OutLines;
	for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Config))
		{
			continue;
		}

		FString ValueStr;
		Prop->ExportTextItem_Direct(ValueStr, Prop->ContainerPtrToValuePtr<void>(this), nullptr, nullptr, PPF_None);
		OutLines.Add(FString::Printf(TEXT("%s = %s"), *Prop->GetName(), *ValueStr));
	}
	return OutLines;
}

static FAutoConsoleCommandWithWorldAndArgs GServerRulesSetCmd(
	TEXT("wo.rules.set"),
	TEXT("Set a Wild Omission server rule at runtime: wo.rules.set <PropertyName> <Value>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGameSettings, Warning, TEXT("Usage: wo.rules.set <RuleName> <Value>"));
			return;
		}
		UServerRules* Rules = UServerRules::Get(World);
		if (Rules && Rules->SetRuleByString(Args[0], Args[1]))
		{
			UE_LOG(LogGameSettings, Display, TEXT("Updated server rule %s = %s"), *Args[0], *Args[1]);
		}
		else
		{
			UE_LOG(LogGameSettings, Warning, TEXT("Unknown or unsupported server rule: %s"), *Args[0]);
		}
	})
);

static FAutoConsoleCommandWithWorld GServerRulesListCmd(
	TEXT("wo.rules.list"),
	TEXT("List all active Wild Omission server rules and multipliers"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		const UServerRules* Rules = UServerRules::Get(World);
		if (Rules == nullptr)
		{
			return;
		}
		for (const FString& Line : Rules->GetRulesSummary())
		{
			UE_LOG(LogGameSettings, Display, TEXT("[ServerRules] %s"), *Line);
		}
	})
);
