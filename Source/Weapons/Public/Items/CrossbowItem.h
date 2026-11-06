// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/BowItem.h"
#include "CrossbowItem.generated.h"

/**
 * Primitive Crossbow firing Wooden Arrows or Bone Arrows with higher velocity and impact than the Hunting Bow.
 */
UCLASS()
class WEAPONS_API ACrossbowItem : public ABowItem
{
	GENERATED_BODY()

public:
	ACrossbowItem();

	UFUNCTION(BlueprintCallable, Category = "Crossbow")
	void SetLoadedArrowType(const FName& NewArrowItemID);

	UFUNCTION(BlueprintPure, Category = "Crossbow")
	FName GetLoadedArrowType() const;

	UFUNCTION(BlueprintPure, Category = "Crossbow")
	float GetVelocityMultiplier() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Crossbow")
	float VelocityMultiplier;
};
