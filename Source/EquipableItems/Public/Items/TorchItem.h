// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SwingableItem.h"
#include "TorchItem.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UAudioComponent;
class UPointLightComponent;

UCLASS()
class EQUIPABLEITEMS_API ATorchItem : public ASwingableItem
{
	GENERATED_BODY()
	
public:
	ATorchItem();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Destroyed() override;

	virtual void OnPrimaryPressed() override;

	virtual void OnSecondaryPressed() override;
	virtual void OnSecondaryAnimationClimax(bool FromFirstPersonInstance) override;

	virtual void OnUnequip() override;
	
	virtual FRotator GetRightArmVelocityOffset() const override;

private:
	UPROPERTY(VisibleInstanceOnly, Replicated, ReplicatedUsing = OnRep_IsBurning)
	bool IsBurning;

	UPROPERTY(EditDefaultsOnly)
	UNiagaraSystem* FireParticleSystem;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* BurningSound;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* LightMontage;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* PutOutMontage;

	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* OnPose;

	UPROPERTY(EditDefaultsOnly)
	UAnimSequence* OffPose;

	/** Seconds between 1-point durability ticks while burning (default 3.0s = 3x longer burn time). */
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	float BurnTickInterval;

	//********Spawned Stuff********
	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* SpawnedFireParticles;
	UPROPERTY(VisibleAnywhere)
	UAudioComponent* SpawnedBurningSound;
	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* SpawnedLightComponent;
	//********End Spawned Stuff****

	UFUNCTION()
	void OnRep_IsBurning();

	UPROPERTY()
	FTimerHandle DecrementDurabilityTimerHandle;

	USkeletalMeshComponent* GetMeshComponentToAttachTo() const;

	void StartFireEffects();
	void StopFireEffects();

};
