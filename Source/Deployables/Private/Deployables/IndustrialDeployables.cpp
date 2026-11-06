// Copyright Telephone Studios. All Rights Reserved.

#include "Deployables/IndustrialDeployables.h"
#include "Components/InventoryComponent.h"

ALargeStorageBox::ALargeStorageBox()
{
	ContainerName = TEXT("Large Wooden Box");
	MaxDurability = 400.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("LargeStorageBox");
	Identifier = TEXT("LargeStorageBox");

	if (UInventoryComponent* BoxInventory = GetInventoryComponent())
	{
		BoxInventory->SetDisplayName(TEXT("Large Wooden Box"));
		BoxInventory->SetSlotCount(30);
	}
}

ASmallFurnaceDeployable::ASmallFurnaceDeployable()
{
	ContainerName = TEXT("Small Furnace");
	SmeltTimeSeconds = 3.0f;
	MaxDurability = 500.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("SmallFurnace");
	Identifier = TEXT("SmallFurnace");

	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->SetDisplayName(TEXT("Small Furnace"));
		Inv->SetSlotCount(6);
	}
}

ALargeFurnaceDeployable::ALargeFurnaceDeployable()
{
	ContainerName = TEXT("Large Furnace");
	SmeltTimeSeconds = 1.5f;
	MaxDurability = 1000.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("LargeFurnace");
	Identifier = TEXT("LargeFurnace");

	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->SetDisplayName(TEXT("Large Furnace"));
		Inv->SetSlotCount(18);
	}
}

AOilRefineryDeployable::AOilRefineryDeployable()
{
	ContainerName = TEXT("Small Oil Refinery");
	SmeltTimeSeconds = 2.5f;
	MaxDurability = 750.0f;
	CurrentDurability = MaxDurability;
	ItemID = TEXT("OilRefinery");
	Identifier = TEXT("OilRefinery");

	if (UInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->SetDisplayName(TEXT("Small Oil Refinery"));
		Inv->SetSlotCount(6);
	}

	Smeltables.Empty();
	FSmeltResult CrudeToLGF;
	CrudeToLGF.RawItem.Name = TEXT("CrudeOil");
	CrudeToLGF.RawItem.Quantity = 1;
	CrudeToLGF.SmeltedItem.Name = TEXT("LowGradeFuel");
	CrudeToLGF.SmeltedItem.Quantity = 3;
	Smeltables.Add(CrudeToLGF);
}
