// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deployables/StorageBox.h"
#include "Deployables/ItemSmelterBase.h"
#include "IndustrialDeployables.generated.h"

/**
 * 30-slot Large Wooden Storage Box (supports KeyLock or CodeLock via LockComponent).
 */
UCLASS()
class DEPLOYABLES_API ALargeStorageBox : public AStorageBox
{
	GENERATED_BODY()

public:
	ALargeStorageBox();
};

/**
 * Tier 1 Small Furnace (6 slots, 3.0s per smelt cycle).
 */
UCLASS()
class DEPLOYABLES_API ASmallFurnaceDeployable : public AItemSmelterBase
{
	GENERATED_BODY()

public:
	ASmallFurnaceDeployable();
};

/**
 * Tier 2 Large Industrial Furnace (18 slots, 1.5s per smelt cycle = 2x speed throughput).
 */
UCLASS()
class DEPLOYABLES_API ALargeFurnaceDeployable : public AItemSmelterBase
{
	GENERATED_BODY()

public:
	ALargeFurnaceDeployable();
};

/**
 * Small Oil Refinery (distills 1 CrudeOil -> 3 LowGradeFuel using Wood fuel).
 */
UCLASS()
class DEPLOYABLES_API AOilRefineryDeployable : public AItemSmelterBase
{
	GENERATED_BODY()

public:
	AOilRefineryDeployable();
};
