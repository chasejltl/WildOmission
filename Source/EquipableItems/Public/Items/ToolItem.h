// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SwingableItem.h"
#include "Enums/ToolType.h"
#include "ToolItem.generated.h"

UCLASS()
class EQUIPABLEITEMS_API AToolItem : public ASwingableItem
{
	GENERATED_BODY()

public:
	AToolItem();

	virtual void OnPrimaryHeld() override;

	float GetGatherMultiplier() const;
	
	bool IsQualityTool() const;

	EToolTier GetToolTier() const;

protected:
	/*
	Used to determine which harvestables this tool can gather from.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TEnumAsByte<EToolType> ToolType;
	
	/*
	Used as a decimal the it is multipled by the output of a harvestable resource,
	and that multiplied amount is given to the player.
	The base Value varies from harvestable to harvestable but is generally 100.
	The stone hatchet has a gather multiplier of 0.2, making it give 20 wood from a tree
	each hit.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	float GatherMultiplier;

	/* 
	Used by harvestable resources to determine if it should drop extra goodies reserved for the nicer tools 
	An example would be that metal tools can harvest Refined Metal when hitting metal nodes, because they are a "QualityTool" whereas
	the Stone Pickaxe cannot
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	bool QualityTool;

	/*
	Explicit Rust-style tool tier (Hand, Stone, Metal, Salvaged) used for yield scaling and minimum tier checks.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	EToolTier ToolTier;

	virtual void OnSwingImpact(const FHitResult& HitResult, const FVector& OwnerCharacterLookVector, bool FromFirstPersonInstance) override;

};
