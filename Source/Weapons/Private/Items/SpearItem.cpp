// Copyright Telephone Studios. All Rights Reserved.

#include "Items/SpearItem.h"

ASpearItem::ASpearItem()
{
	bStoneTipped = false;
	EffectiveRangeCentimeters = 320.0f;
	DamageMultiplier = 1.4f;
	SwingSpeedRate = 0.9f;
	DealDamageToActors = true;
}

void ASpearItem::OnPrimaryPressed()
{
	Super::OnPrimaryPressed();
	Swing();
}

bool ASpearItem::IsStoneTipped() const
{
	return bStoneTipped;
}
