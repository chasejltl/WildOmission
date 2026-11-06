// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/ArrowProjectile.h"
#include "BoneArrowProjectile.generated.h"

/**
 * Bone-tipped primitive arrow with a larger collision sphere radius and higher base impact damage
 * at the cost of slightly lower flight velocity compared to Wooden Arrows.
 */
UCLASS()
class WEAPONS_API ABoneArrowProjectile : public AArrowProjectile
{
	GENERATED_BODY()

public:
	ABoneArrowProjectile();
};
