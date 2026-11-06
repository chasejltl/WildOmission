// Copyright Telephone Studios. All Rights Reserved.

#include "Projectiles/BoneArrowProjectile.h"
#include "Components/SphereComponent.h"

ABoneArrowProjectile::ABoneArrowProjectile()
{
	Damage = 42.0f;
	Velocity = 3900.0f;

	if (CollisionComponent != nullptr)
	{
		CollisionComponent->SetSphereRadius(12.0f);
	}
}
