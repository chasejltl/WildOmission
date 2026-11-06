// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SemiAutomaticFirearmItem.h"
#include "HandmadeRevolverItem.generated.h"

/**
 * Six-shot primitive revolver firing PistolBullet cartridges.
 */
UCLASS()
class WEAPONS_API AHandmadeRevolverItem : public ASemiAutomaticFirearmItem
{
	GENERATED_BODY()

public:
	AHandmadeRevolverItem();

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetCylinderCapacity() const;
};
