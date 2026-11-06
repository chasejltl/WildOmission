// Copyright Telephone Studios. All Rights Reserved.

#include "Items/CrossbowItem.h"
#include "Projectiles/ArrowProjectile.h"

ACrossbowItem::ACrossbowItem()
{
	AmmoItemID = TEXT("WoodenArrow");
	ProjectileClass = AArrowProjectile::StaticClass();
	MaxAmmo = 1;
	VelocityMultiplier = 1.35f;
}

void ACrossbowItem::SetLoadedArrowType(const FName& NewArrowItemID)
{
	if (NewArrowItemID == TEXT("WoodenArrow") || NewArrowItemID == TEXT("BoneArrow"))
	{
		AmmoItemID = NewArrowItemID;
	}
}

FName ACrossbowItem::GetLoadedArrowType() const
{
	return AmmoItemID;
}

float ACrossbowItem::GetVelocityMultiplier() const
{
	return VelocityMultiplier;
}
