// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VitalsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginThirstSignature, const float&, Time);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndThirstSignature, const float&, Time);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginStarvingSignature, const float&, Time);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndStarvingSignature, const float&, Time);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepletedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTakeAnyDamageSignature);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VITALS_API UVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVitalsComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable)
	void SetHealth(float Value);
	
	UFUNCTION(BlueprintCallable)
	void SetThirst(float Value);
	
	UFUNCTION(BlueprintCallable)
	void SetHunger(float Value);

	UFUNCTION(BlueprintCallable)
	void AddHealth(float Value);

	UFUNCTION(BlueprintCallable)
	void AddThirst(float Value);

	UFUNCTION(BlueprintCallable)
	void AddHunger(float Value);
	
	UFUNCTION(BlueprintCallable)
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable)
	float GetMaxThirst() const;
	
	UFUNCTION(BlueprintCallable)
	float GetMaxHunger() const;

	UFUNCTION(BlueprintCallable)
	float GetHealth() const;

	UFUNCTION(BlueprintCallable)
	float GetThirst() const;

	UFUNCTION(BlueprintCallable)
	float GetHunger() const;

	void SetHealthCanDeplete(bool InCanDeplete);
	void SetThirstCanDeplete(bool InCanDeplete);
	void SetHungerCanDeplete(bool InCanDeplete);

	UFUNCTION(BlueprintCallable)
	bool IsThirsty() const;

	UFUNCTION(BlueprintCallable)
	bool IsStarving() const;

	UFUNCTION(BlueprintCallable)
	void SetGodMode(bool InGodMode);

	UFUNCTION(BlueprintCallable)
	bool IsGodMode() const;

	UFUNCTION(BlueprintCallable)
	void SetHurtSound(USoundBase* InHurtSound);

	// --- Phase 3: Radiation, Bleeding & Medical Systems ---

	UFUNCTION(BlueprintCallable, Category = "Vitals|Radiation")
	void SetRadiationProtection(float InProtection01);

	UFUNCTION(BlueprintPure, Category = "Vitals|Radiation")
	float GetRadiationProtection() const { return EquippedRadiationProtection; }

	/** Applies environmental radiation exposure (rads), mitigated by EquippedRadiationProtection. */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Radiation")
	void AddRadiationExposure(float RawRads);

	UFUNCTION(BlueprintPure, Category = "Vitals|Radiation")
	float GetRadiation() const { return CurrentRadiation; }

	UFUNCTION(BlueprintCallable, Category = "Vitals|Bleeding")
	void AddBleeding(float BleedAmount);

	UFUNCTION(BlueprintPure, Category = "Vitals|Bleeding")
	float GetBleeding() const { return CurrentBleeding; }

	/** Applies Bandage, MedicalSyringe, or AntiRadPills effects to vitals. */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Medical")
	bool ApplyMedicalItem(const FName& MedicalItemID);

	/** Advances Radiation & Bleeding health drain over DeltaSeconds. */
	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void StepEnvironmentalVitals(float DeltaSeconds);

	FOnBeginThirstSignature OnBeginThirst;
	FOnEndThirstSignature OnEndThirst;

	FOnBeginStarvingSignature OnBeginStarving;
	FOnEndStarvingSignature OnEndStarving;

	FOnHealthDepletedSignature OnHealthDepleted;
	FOnTakeAnyDamageSignature OnTakeAnyDamage;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly)
	float MaxHealth;

	UPROPERTY(EditDefaultsOnly)
	float MaxThirst;
	
	UPROPERTY(EditDefaultsOnly)
	float MaxHunger;
	
	UPROPERTY(EditDefaultsOnly)
	float StartHealth;
	
	UPROPERTY(EditDefaultsOnly)
	float StartThirst;
	
	UPROPERTY(EditDefaultsOnly)
	float StartHunger;
	
	UPROPERTY(EditDefaultsOnly)
	float HealthDepletionRate;

	UPROPERTY(EditDefaultsOnly)
	float ThirstDepletionRate;
	
	UPROPERTY(EditDefaultsOnly)
	float HungerDepletionRate;

	UPROPERTY(EditDefaultsOnly)
	float ThirstThreshold;
	
	UPROPERTY(EditDefaultsOnly)
	float HungerThreshold;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* HurtSound;
	
	UPROPERTY(Replicated)
	float CurrentHealth;
	
	UPROPERTY(Replicated)
	float CurrentThirst;

	UPROPERTY(Replicated)
	float CurrentHunger;

	UPROPERTY(Replicated)
	float CurrentRadiation;

	UPROPERTY(Replicated)
	float CurrentBleeding;

	float EquippedRadiationProtection;

	bool HealthCanDeplete;
	bool ThirstCanDeplete;
	bool HungerCanDeplete;

	void CalculateDepletion();
	
	void HandleDelegates();

	bool BeginThirstBroadcasted;
	bool EndThirstBroadcasted;
	bool BeginStarvingBroadcasted;
	bool EndStarvingBroadcasted;

	void BroadcastBeginThirst();
	void BroadcastEndThirst();
	void BroadcastBeginStarving();
	void BroadcastEndStarving();

	UFUNCTION(NetMulticast, Reliable)
	void Client_PlayHurtSound();

	UFUNCTION()
	void OnOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

};
