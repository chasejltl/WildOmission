// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Clothing.generated.h"


// TODO JACKET
UENUM()
enum EClothingType
{
	HAT,
	TORSO,
	LEGGINGS,
	BOOTS
};

/**
 * Rust-style clothing & armor protection modifiers:
 * Projectile protection, Melee protection, Radiation resistance (0..1), and Cold/Heat insulation.
 */
USTRUCT(BlueprintType)
struct CLOTHING_API FClothingProtectionStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection")
	float ProjectileProtection = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection")
	float MeleeProtection = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection")
	float RadiationProtection = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protection")
	float ColdInsulationCelsius = 0.0f;

	static FClothingProtectionStats GetPresetForItem(const FName& ItemID);
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CLOTHING_API UClothing : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClothing();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	TEnumAsByte<EClothingType> GetType() const;

	void ApplyItemPreset(const FName& ItemID);
	const FClothingProtectionStats& GetProtectionStats() const { return ProtectionStats; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<EClothingType> Type;

	UPROPERTY(EditDefaultsOnly, Category = "Protection")
	FClothingProtectionStats ProtectionStats;

};
