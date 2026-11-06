// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/ItemContainerBase.h"
#include "Structs/InventoryItem.h"
#include "ItemSmelterBase.generated.h"

USTRUCT(BlueprintType)
struct FFuelSource
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FInventoryItem Fuel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FInventoryItem Byproduct;

};

USTRUCT(BlueprintType)
struct FSmeltResult
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FInventoryItem RawItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FInventoryItem SmeltedItem;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSmeltedSignature, const FInventoryItem&, RawItem, const FInventoryItem&, SmeltedItem);

UCLASS()
class DEPLOYABLES_API AItemSmelterBase : public AItemContainerBase
{
	GENERATED_BODY()
	
public:
	AItemSmelterBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(Server, Reliable)
	void Server_ToggleState(bool NewState);
	
	bool IsTurnedOn() const;

	void EnsureDefaultSmeltingRules();

	FOnItemSmeltedSignature OnItemSmelted;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSmelt();

	virtual void OnTurnedOn();
	virtual void OnTurnedOff();

	UPROPERTY(EditDefaultsOnly, Category = "Smelting")
	float SmeltTimeSeconds;
	
	// Key is Fuel Source (Wood), Value is Byproduct (Charcoal)
	UPROPERTY(EditDefaultsOnly, Category = "Smelting")
	FFuelSource FuelSource;

	UPROPERTY(EditDefaultsOnly, Category = "Smelting")
	TArray<FSmeltResult> Smeltables;

private:
	UPROPERTY(VisibleAnywhere)
	class UPointLightComponent* LightComponent;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* ParticlesComponent;

	UPROPERTY(VisibleAnywhere)
	UAudioComponent* AudioComponent;

	UPROPERTY(Replicated, ReplicatedUsing = "OnRep_TurnedOn", SaveGame)
	bool bTurnedOn;
	
	UPROPERTY()
	float SmeltCounter;

	bool HasFuel() const;
	bool BurnFuel();
	void SmeltAllSmeltables();
	void ProduceByproduct();

	UFUNCTION()
	void OnRep_TurnedOn();

	virtual void OnLoadComplete_Implementation() override;

};
