// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/SwingableItem.h"
#include "SpearItem.generated.h"

/**
 * Primitive long-reach melee spear (Wooden Spear / Stone Spear variants).
 */
UCLASS()
class WEAPONS_API ASpearItem : public ASwingableItem
{
	GENERATED_BODY()

public:
	ASpearItem();

	virtual void OnPrimaryPressed() override;

	bool IsStoneTipped() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Spear")
	bool bStoneTipped;
};
