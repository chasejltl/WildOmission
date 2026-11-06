// Copyright Telephone Studios. All Rights Reserved.


#include "Structs/InventoryItem.h"

namespace
{
	FName NormalizeItemAlias(const FName& InName)
	{
		if (InName == TEXT("wood") || InName == TEXT("Wood")) return TEXT("Wood");
		if (InName == TEXT("stone") || InName == TEXT("Stone")) return TEXT("Stone");
		if (InName == TEXT("cloth") || InName == TEXT("Cloth")) return TEXT("Cloth");
		if (InName == TEXT("ore.metal") || InName == TEXT("MetalOre")) return TEXT("MetalOre");
		if (InName == TEXT("metal") || InName == TEXT("MetalFragments")) return TEXT("MetalFragments");
		if (InName == TEXT("metal.refined") || InName == TEXT("HighQualityMetal")) return TEXT("HighQualityMetal");
		if (InName == TEXT("ore.sulfur") || InName == TEXT("SulfurOre")) return TEXT("SulfurOre");
		if (InName == TEXT("sulfur") || InName == TEXT("Sulfur")) return TEXT("Sulfur");
		if (InName == TEXT("charcoal") || InName == TEXT("Charcoal")) return TEXT("Charcoal");
		if (InName == TEXT("gunpowder") || InName == TEXT("Gunpowder")) return TEXT("Gunpowder");
		if (InName == TEXT("arrow.wooden") || InName == TEXT("WoodenArrow")) return TEXT("WoodenArrow");
		if (InName == TEXT("ammo.pistol") || InName == TEXT("PistolBullet")) return TEXT("PistolBullet");
		if (InName == TEXT("ammo.rifle") || InName == TEXT("RifleBullet")) return TEXT("RifleBullet");
		if (InName == TEXT("ammo.rocket.basic") || InName == TEXT("Rocket")) return TEXT("Rocket");
		if (InName == TEXT("hatchet.stone") || InName == TEXT("StoneHatchet")) return TEXT("StoneHatchet");
		if (InName == TEXT("pickaxe.stone") || InName == TEXT("StonePickaxe")) return TEXT("StonePickaxe");
		if (InName == TEXT("hatchet") || InName == TEXT("MetalHatchet")) return TEXT("MetalHatchet");
		if (InName == TEXT("pickaxe") || InName == TEXT("MetalPickaxe")) return TEXT("MetalPickaxe");
		if (InName == TEXT("bandage") || InName == TEXT("Bandage")) return TEXT("Bandage");
		if (InName == TEXT("syringe.medical") || InName == TEXT("MedicalSyringe")) return TEXT("MedicalSyringe");
		return InName;
	}
}

bool FInventoryItem::CompareNames(const FInventoryItem& Item, const FName& ItemName)
{
	return Item.Name == ItemName || NormalizeItemAlias(Item.Name) == NormalizeItemAlias(ItemName);
}

bool FInventoryItem::CompareID(const FInventoryItem& Item, const uint32& ItemID)
{
	return Item.UniqueID == ItemID;
}

void FInventoryItem::Set(const FInventoryItem& InItem)
{
	Name = InItem.Name;
	Quantity = InItem.Quantity;
	UniqueID = InItem.UniqueID;
}

void FInventoryItem::Set(const FName& InName, const int32& InQuantity)
{
	Name = InName;
	Quantity = InQuantity;
	UniqueID = 0;
}

bool FInventoryItem::IsZero() const
{
	return Quantity == 0;
}

void FInventoryItem::Clear()
{
	Name = TEXT("");
	Quantity = 0;
	UniqueID = 0;
}

int32 FInventoryItem::GetStat(const FName& StatName) const
{
	int32 StatValue = -1;

	if (Stats.Num() == 0)
	{
		return StatValue;
	}

	for (const FItemStat& Stat : Stats)
	{
		if (Stat.Name != StatName)
		{
			continue;
		}

		StatValue = Stat.Value;
		break;
	}

	return StatValue;
}

void FInventoryItem::SetStat(const FName& StatName, const int32& Value)
{
	bool StatExists = false;

	if (Stats.Num() == 0)
	{
		AddStat(StatName, Value);
	}

	for (FItemStat& Stat : Stats)
	{
		if (Stat.Name != StatName)
		{
			continue;
		}

		StatExists = true;
		Stat.Value = Value;
		break;
	}

	if (StatExists == false)
	{
		AddStat(StatName, Value);
	}
}

void FInventoryItem::AddStat(const FName& StatName, const int32& Value)
{
	FItemStat NewStat;
	NewStat.Name = StatName;
	NewStat.Value = Value;

	Stats.Add(NewStat);
}
