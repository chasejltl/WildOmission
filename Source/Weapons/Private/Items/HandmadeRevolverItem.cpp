// Copyright Telephone Studios. All Rights Reserved.

#include "Items/HandmadeRevolverItem.h"
#include "Projectiles/WeaponProjectile.h"

AHandmadeRevolverItem::AHandmadeRevolverItem()
{
	AmmoItemID = TEXT("PistolBullet");
	ProjectileClass = AWeaponProjectile::StaticClass();
	MaxAmmo = 6;
	CurrentAmmo = 0;
	Durability = 150;
}

int32 AHandmadeRevolverItem::GetCylinderCapacity() const
{
	return MaxAmmo;
}
