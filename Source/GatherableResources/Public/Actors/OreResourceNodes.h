// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/HarvestableNode.h"
#include "OreResourceNodes.generated.h"

/**
 * Fallen wood log pile node that yields Wood when harvested with hatchets or multi-tools.
 */
UCLASS()
class GATHERABLERESOURCES_API AWoodLogNode : public AHarvestableNode
{
	GENERATED_BODY()

public:
	AWoodLogNode();
};

/**
 * Raw Stone boulder node that yields Stone when harvested with pickaxes or multi-tools.
 */
UCLASS()
class GATHERABLERESOURCES_API AStoneOreNode : public AHarvestableNode
{
	GENERATED_BODY()

public:
	AStoneOreNode();
};

/**
 * Metal Ore node that yields Metal Ore on every hit and bonus High-Quality Metal Ore when harvested with a Metal+ Pickaxe.
 */
UCLASS()
class GATHERABLERESOURCES_API AMetalOreNode : public AHarvestableNode
{
	GENERATED_BODY()

public:
	AMetalOreNode();
};

/**
 * Sulfur Ore node that yields Sulfur Ore for smelting into Sulfur / Gunpowder.
 */
UCLASS()
class GATHERABLERESOURCES_API ASulfurOreNode : public AHarvestableNode
{
	GENERATED_BODY()

public:
	ASulfurOreNode();
};

/**
 * High-Quality Metal (HQM) rich ore node requiring at least a Metal-tier pickaxe to harvest.
 */
UCLASS()
class GATHERABLERESOURCES_API AHQMOreNode : public AHarvestableNode
{
	GENERATED_BODY()

public:
	AHQMOreNode();
};
