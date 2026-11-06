// Copyright Telephone Studios. All Rights Reserved.


#include "Components/VitalsComponent.h"
#include "ServerRules.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UVitalsComponent::UVitalsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	MaxHealth = 100.0f;
	MaxThirst = 300.0f;
	MaxHunger = 500.0f;

	StartHealth = MaxHealth * 0.9f;
	StartThirst = MaxThirst * 0.8f;
	StartHunger = MaxHunger * 0.8f;

	CurrentHealth = StartHealth;
	CurrentThirst = StartThirst;
	CurrentHunger = StartHunger;

	HealthDepletionRate = 1.0f;
	ThirstDepletionRate = 0.1f;
	HungerDepletionRate = 0.3f;

	ThirstThreshold = MaxThirst * 0.25f;
	HungerThreshold = MaxHunger * 0.25f;

	HealthCanDeplete = true;
	ThirstCanDeplete = true;
	HungerCanDeplete = true;

	BeginThirstBroadcasted = false;
	EndThirstBroadcasted = false;

	BeginStarvingBroadcasted = false;
	EndStarvingBroadcasted = false;

	CurrentRadiation = 0.0f;
	CurrentBleeding = 0.0f;
	CurrentBodyTemperatureCelsius = 37.0f;
	EquippedRadiationProtection = 0.0f;
	ColdInsulationCelsius = 0.0f;
	bIsShelteredOrNearFire = false;

	ConstructorHelpers::FObjectFinder<USoundBase> DefaultHurtSoundObject(TEXT("/Game/WildOmissionCore/Characters/Human/Audio/Damage/Human_HurtGeneric_Cue"));
	if (!DefaultHurtSoundObject.Succeeded())
	{
		return;
	}

	HurtSound = DefaultHurtSoundObject.Object;
}

// Called when the game starts
void UVitalsComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}
	
	OwnerActor->OnTakeAnyDamage.AddDynamic(this, &UVitalsComponent::OnOwnerTakeAnyDamage);
}

// Called every frame
void UVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CalculateDepletion();
}

void UVitalsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentHealth, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentThirst, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentHunger, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentRadiation, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentBleeding, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVitalsComponent, CurrentBodyTemperatureCelsius, COND_OwnerOnly);
}

void UVitalsComponent::CalculateDepletion()
{
	HandleDelegates();

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr || !OwnerActor->HasAuthority())
	{
		return;
	}

	// Remove from thirst and hunger, and clamp to min and max value
	if (ThirstCanDeplete)
	{
		CurrentThirst = FMath::Clamp(CurrentThirst - (ThirstDepletionRate * World->GetDeltaSeconds()), 0.0f, MaxThirst);
	}
	if (HungerCanDeplete)
	{
		CurrentHunger = FMath::Clamp(CurrentHunger - (HungerDepletionRate * World->GetDeltaSeconds()), 0.0f, MaxHunger);
	}
	
	// If Thirst or Hunger is below threshold start removing Health
	if ((IsThirsty() || IsStarving()) && HealthCanDeplete)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth - (HealthDepletionRate * World->GetDeltaSeconds()), 0.0f, MaxHealth);
	}

	StepEnvironmentalVitals(World->GetDeltaSeconds());

	// If Health is too low kill the player
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		OnHealthDepleted.Broadcast();
	}
}

void UVitalsComponent::HandleDelegates()
{
	if (!IsStarving())
	{
		BroadcastEndStarving();
	}
	else
	{
		BroadcastBeginStarving();
	}

	if (!IsThirsty())
	{
		BroadcastEndThirst();
	}
	else
	{
		BroadcastBeginThirst();
	}
}

void UVitalsComponent::BroadcastBeginThirst()
{
	UWorld* World = GetWorld();
	if (World == nullptr || BeginThirstBroadcasted || !OnBeginThirst.IsBound())
	{
		return;
	}

	OnBeginThirst.Broadcast(World->GetRealTimeSeconds());
	BeginThirstBroadcasted = true;
	EndThirstBroadcasted = false;
}

void UVitalsComponent::BroadcastEndThirst()
{
	UWorld* World = GetWorld();
	if (World == nullptr || EndThirstBroadcasted || !OnEndThirst.IsBound())
	{
		return;
	}

	OnEndThirst.Broadcast(World->GetRealTimeSeconds());
	BeginThirstBroadcasted = false;
	EndThirstBroadcasted = true;
}

void UVitalsComponent::BroadcastBeginStarving()
{
	UWorld* World = GetWorld();
	if (World == nullptr || BeginStarvingBroadcasted || !OnBeginStarving.IsBound())
	{
		return;
	}

	OnBeginStarving.Broadcast(World->GetRealTimeSeconds());
	BeginStarvingBroadcasted = true;
	EndStarvingBroadcasted = false;

}

void UVitalsComponent::BroadcastEndStarving()
{
	UWorld* World = GetWorld();
	if (World == nullptr || EndStarvingBroadcasted || !OnEndStarving.IsBound())
	{
		return;
	}

	OnEndStarving.Broadcast(World->GetRealTimeSeconds());
	BeginStarvingBroadcasted = false;
	EndStarvingBroadcasted = true;
}

void UVitalsComponent::SetHealth(float Value)
{
	CurrentHealth = FMath::Clamp(Value, 0.0f, MaxHealth);
}

void UVitalsComponent::SetThirst(float Value)
{
	CurrentThirst = FMath::Clamp(Value, 0.0f, MaxThirst);
}

void UVitalsComponent::SetHunger(float Value)
{
	CurrentHunger = FMath::Clamp(Value, 0.0f, MaxHunger);
}

void UVitalsComponent::AddHealth(float Value)
{
	if (Value < 0.0f && !HealthCanDeplete)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Value, 0.0f, MaxHealth);
	if (Value < 0.0f)
	{
		if (OnTakeAnyDamage.IsBound())
		{
			OnTakeAnyDamage.Broadcast();
		}
		Client_PlayHurtSound();
	}
}

void UVitalsComponent::AddThirst(float Value)
{
	if (Value < 0.0f && !ThirstCanDeplete)
	{
		return;
	}

	CurrentThirst = FMath::Clamp(CurrentThirst + Value, 0.0f, MaxThirst);
}

void UVitalsComponent::AddHunger(float Value)
{
	if (Value < 0.0f && !HungerCanDeplete)
	{
		return;
	}

	CurrentHunger = FMath::Clamp(CurrentHunger + Value, 0.0f, MaxHunger);
}

float UVitalsComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UVitalsComponent::GetMaxThirst() const
{
	return MaxThirst;
}

float UVitalsComponent::GetMaxHunger() const
{
	return MaxHunger;
}

float UVitalsComponent::GetHealth() const
{
	return CurrentHealth;
}

float UVitalsComponent::GetThirst() const
{
	return CurrentThirst;
}

float UVitalsComponent::GetHunger() const
{
	return CurrentHunger;
}

void UVitalsComponent::SetHealthCanDeplete(bool InCanDeplete)
{
	HealthCanDeplete = InCanDeplete;
}

void UVitalsComponent::SetThirstCanDeplete(bool InCanDeplete)
{
	ThirstCanDeplete = InCanDeplete;
}

void UVitalsComponent::SetHungerCanDeplete(bool InCanDeplete)
{
	HungerCanDeplete = InCanDeplete;
}

bool UVitalsComponent::IsThirsty() const
{
	return CurrentThirst < ThirstThreshold;
}

bool UVitalsComponent::IsStarving() const
{
	return CurrentHunger < HungerThreshold;
}

void UVitalsComponent::SetGodMode(bool InGodMode)
{
	HealthCanDeplete = !InGodMode;
	ThirstCanDeplete = !InGodMode;
	HungerCanDeplete = !InGodMode;
}

bool UVitalsComponent::IsGodMode() const
{
	return !HealthCanDeplete && !ThirstCanDeplete && !HungerCanDeplete;
}

void UVitalsComponent::SetHurtSound(USoundBase* InHurtSound)
{
	HurtSound = InHurtSound;
}

void UVitalsComponent::Client_PlayHurtSound_Implementation()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	USceneComponent* OwnerRootComponent = OwnerActor->GetRootComponent();
	if (OwnerRootComponent == nullptr || HurtSound == nullptr)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAttached(HurtSound, OwnerRootComponent);
}

void UVitalsComponent::OnOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority() || DamagedActor == nullptr)
	{
		return;
	}

	AddHealth(-Damage);
}

void UVitalsComponent::SetRadiationProtection(float InProtection01)
{
	EquippedRadiationProtection = FMath::Clamp(InProtection01, 0.0f, 1.0f);
}

void UVitalsComponent::AddRadiationExposure(float RawRads)
{
	if (RawRads <= 0.0f)
	{
		CurrentRadiation = FMath::Clamp(CurrentRadiation + RawRads, 0.0f, 500.0f);
		return;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && !Rules->bRadiationEnabled)
	{
		return;
	}

	// Hazmat suit (0.50 protection) completely blocks normal monument ambient rads (up to 25 rads) and scales the rest
	if (EquippedRadiationProtection >= 0.50f && RawRads <= 25.0f)
	{
		return;
	}

	const float EffectiveRads = RawRads * FMath::Max(0.0f, 1.0f - EquippedRadiationProtection);
	CurrentRadiation = FMath::Clamp(CurrentRadiation + EffectiveRads, 0.0f, 500.0f);
}

void UVitalsComponent::AddBleeding(float BleedAmount)
{
	CurrentBleeding = FMath::Clamp(CurrentBleeding + BleedAmount, 0.0f, 100.0f);
}

bool UVitalsComponent::ApplyMedicalItem(const FName& MedicalItemID)
{
	if (MedicalItemID == TEXT("Bandage"))
	{
		CurrentBleeding = 0.0f;
		AddHealth(5.0f);
		return true;
	}
	if (MedicalItemID == TEXT("MedicalSyringe"))
	{
		AddHealth(35.0f); // +15 instant + 20 HoT combined
		CurrentRadiation = FMath::Max(0.0f, CurrentRadiation - 10.0f);
		return true;
	}
	if (MedicalItemID == TEXT("AntiRadPills"))
	{
		CurrentRadiation = FMath::Max(0.0f, CurrentRadiation - 75.0f);
		AddThirst(-5.0f);
		return true;
	}
	return false;
}

void UVitalsComponent::StepEnvironmentalVitals(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || !HealthCanDeplete)
	{
		return;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	const bool bRadActive = (Rules == nullptr) || Rules->bRadiationEnabled;

	if (bRadActive && CurrentRadiation > 0.0f)
	{
		const float RadDamage = (CurrentRadiation * 0.15f) * DeltaSeconds;
		CurrentHealth = FMath::Clamp(CurrentHealth - RadDamage, 0.0f, MaxHealth);
		CurrentRadiation = FMath::Max(0.0f, CurrentRadiation - (0.5f * DeltaSeconds));
	}

	if (CurrentBleeding > 0.0f)
	{
		const float BleedDamage = (CurrentBleeding * 0.20f) * DeltaSeconds;
		CurrentHealth = FMath::Clamp(CurrentHealth - BleedDamage, 0.0f, MaxHealth);
		CurrentBleeding = FMath::Max(0.0f, CurrentBleeding - (1.0f * DeltaSeconds));
	}
}

void UVitalsComponent::SetColdInsulation(float InCelsiusBonus)
{
	ColdInsulationCelsius = InCelsiusBonus;
}

void UVitalsComponent::SetShelteredOrNearFire(bool bInSheltered)
{
	bIsShelteredOrNearFire = bInSheltered;
}

void UVitalsComponent::ApplyAmbientTemperature(float AmbientCelsius, float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && !Rules->bTemperatureEnabled)
	{
		CurrentBodyTemperatureCelsius = 37.0f;
		return;
	}

	const float ShelterWarmth = bIsShelteredOrNearFire ? 18.0f : 0.0f;
	const float EffectiveAmbient = AmbientCelsius + ColdInsulationCelsius + ShelterWarmth;

	if (EffectiveAmbient < 5.0f)
	{
		// Freezing environment lowers core body temperature below 35°C (Hypothermia)
		const float ColdDrop = ((5.0f - EffectiveAmbient) * 0.12f) * DeltaSeconds;
		CurrentBodyTemperatureCelsius = FMath::Clamp(CurrentBodyTemperatureCelsius - ColdDrop, 28.0f, 37.0f);
	}
	else if (EffectiveAmbient > 38.0f && !bIsShelteredOrNearFire)
	{
		// Extreme desert/sandstorm heat raises core body temperature above 39.5°C (Hyperthermia)
		const float HeatRise = ((EffectiveAmbient - 38.0f) * 0.10f) * DeltaSeconds;
		CurrentBodyTemperatureCelsius = FMath::Clamp(CurrentBodyTemperatureCelsius + HeatRise, 37.0f, 43.0f);
	}
	else
	{
		// Recover core temperature toward normal 37.0°C
		CurrentBodyTemperatureCelsius = FMath::FInterpConstantTo(CurrentBodyTemperatureCelsius, 37.0f, DeltaSeconds, 0.5f);
	}

	if (HealthCanDeplete)
	{
		if (IsHypothermic())
		{
			const float ColdDamage = (35.0f - CurrentBodyTemperatureCelsius) * 1.25f * DeltaSeconds;
			CurrentHealth = FMath::Clamp(CurrentHealth - ColdDamage, 0.0f, MaxHealth);
		}
		else if (IsHyperthermic())
		{
			const float HeatDamage = (CurrentBodyTemperatureCelsius - 39.5f) * 1.0f * DeltaSeconds;
			CurrentHealth = FMath::Clamp(CurrentHealth - HeatDamage, 0.0f, MaxHealth);
			AddThirst(-1.5f * DeltaSeconds);
		}
	}
}