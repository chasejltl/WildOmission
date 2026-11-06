// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Structs/InventoryItem.h"
#include "UObject/NoExportTypes.h"
#include "CraftingRecipe.generated.h"


USTRUCT(BlueprintType)
struct CRAFTING_API FCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 YieldQuantity = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInventoryItem> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CraftTime = 0;

	/** Required workbench tier to craft this recipe (0 = hand-craftable, 1..3 = Workbench Tier 1..3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 RequiredWorkbenchTier = 0;

};
