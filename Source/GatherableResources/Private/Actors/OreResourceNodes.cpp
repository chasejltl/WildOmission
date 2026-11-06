// Copyright Telephone Studios. All Rights Reserved.

#include "Actors/OreResourceNodes.h"

AWoodLogNode::AWoodLogNode()
{
	Identifier = TEXT("WoodLogNode");
	RequiredToolType = EToolType::WOOD;
	MinimumToolTier = EToolTier::Hand;
	Durability = 8;

	FInventoryItem WoodDrop;
	WoodDrop.Name = TEXT("Wood");
	WoodDrop.Quantity = 100;
	CommonDrops.Add(WoodDrop);
}

AStoneOreNode::AStoneOreNode()
{
	Identifier = TEXT("StoneOreNode");
	RequiredToolType = EToolType::STONE;
	MinimumToolTier = EToolTier::Hand;
	Durability = 10;

	FInventoryItem StoneDrop;
	StoneDrop.Name = TEXT("Stone");
	StoneDrop.Quantity = 100;
	CommonDrops.Add(StoneDrop);
}

AMetalOreNode::AMetalOreNode()
{
	Identifier = TEXT("MetalOreNode");
	RequiredToolType = EToolType::STONE;
	MinimumToolTier = EToolTier::Stone;
	Durability = 10;
	NormalizedQualityToolDropChance = 0.35f;

	FInventoryItem MetalOreDrop;
	MetalOreDrop.Name = TEXT("MetalOre");
	MetalOreDrop.Quantity = 60;
	CommonDrops.Add(MetalOreDrop);

	FInventoryItem HQMBonusDrop;
	HQMBonusDrop.Name = TEXT("HighQualityMetalOre");
	HQMBonusDrop.Quantity = 2;
	QualityToolDrops.Add(HQMBonusDrop);
}

ASulfurOreNode::ASulfurOreNode()
{
	Identifier = TEXT("SulfurOreNode");
	RequiredToolType = EToolType::STONE;
	MinimumToolTier = EToolTier::Stone;
	Durability = 10;

	FInventoryItem SulfurDrop;
	SulfurDrop.Name = TEXT("SulfurOre");
	SulfurDrop.Quantity = 30;
	CommonDrops.Add(SulfurDrop);
}

AHQMOreNode::AHQMOreNode()
{
	Identifier = TEXT("HQMOreNode");
	RequiredToolType = EToolType::STONE;
	MinimumToolTier = EToolTier::Metal;
	Durability = 12;

	FInventoryItem HQMDrop;
	HQMDrop.Name = TEXT("HighQualityMetalOre");
	HQMDrop.Quantity = 5;
	CommonDrops.Add(HQMDrop);
}
