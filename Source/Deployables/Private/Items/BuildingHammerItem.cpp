// Copyright Telephone Studios. All Rights Reserved.


#include "Items/BuildingHammerItem.h"
#include "Components/EquipComponent.h"
#include "Components/BuilderComponent.h"
#include "Interfaces/DurabilityInterface.h"
#include "Components/InventoryComponent.h"
#include "Components/CraftingComponent.h"
#include "UI/BuildingHammerWidget.h"
#include "Deployables/Deployable.h"
#include "Deployables/BuildingBlock.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"

static TSubclassOf<UBuildingHammerWidget> WidgetClass = nullptr;

ABuildingHammerItem::ABuildingHammerItem()
{
	DealDamageToActors = false;

	ToolType = BUILD;
	EffectiveRangeCentimeters = 300.0f;
	MaxRepairAmount = 15.0f;

	Widget = nullptr;

	static ConstructorHelpers::FClassFinder<UBuildingHammerWidget> BuildingHammerWidgetBlueprint(TEXT("/Game/Deployables/UI/WBP_BuildingHammerMenu"));
	if (BuildingHammerWidgetBlueprint.Succeeded())
	{
		WidgetClass = BuildingHammerWidgetBlueprint.Class;
	}
}

void ABuildingHammerItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateBuildingPrivilegeNotifications();
}

void ABuildingHammerItem::OnPrimaryHeld()
{
	Super::OnPrimaryHeld();

	Swing();
}

void ABuildingHammerItem::OnSecondaryPressed()
{
	Super::OnSecondaryPressed();

	if (GetOwnerPawn() == nullptr || GetOwnerPawn()->IsLocallyControlled() == false)
	{
		return;
	}

	APlayerController* OwnerPlayerController = Cast<APlayerController>(GetOwnerPawn()->GetController());
	if (OwnerPlayerController == nullptr)
	{
		return;
	}

	FHitResult HitResult;
	if (!LineTraceOnVisibility(HitResult))
	{
		return;
	}

	ADeployable* HitDeployable = Cast<ADeployable>(HitResult.GetActor());
	if (HitDeployable == nullptr || !HasBuildingPrivilege(HitDeployable->GetActorLocation()))
	{
		return;
	}

	if (Widget)
	{
		Widget->Teardown();
	}

	Widget = CreateWidget<UBuildingHammerWidget>(OwnerPlayerController, WidgetClass);
	if (Widget == nullptr)
	{
		return;
	}
	Widget->OnTeardown.AddDynamic(this, &ABuildingHammerItem::ClearWidget);
	Widget->Show(this, HitDeployable);
}


void ABuildingHammerItem::OnUnequip()
{
	Super::OnUnequip();

}

void ABuildingHammerItem::Destroyed()
{
	Super::Destroyed();
	
	if (Widget)
	{
		Widget->Teardown();
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UBuilderComponent* OwnerBuilderComponent = OwnerActor->FindComponentByClass<UBuilderComponent>();
	if (OwnerBuilderComponent == nullptr || !OwnerBuilderComponent->OnClearBuildingPrivilegeNotification.IsBound())
	{
		return;
	}

	OwnerBuilderComponent->OnClearBuildingPrivilegeNotification.Broadcast();
}

void ABuildingHammerItem::Server_UpgradeCurrentDeployable_Implementation()
{
	FHitResult HitResult;
	if (!LineTraceOnVisibility(HitResult))
	{
		return;
	}

	ABuildingBlock* HitBuildingBlock = Cast<ABuildingBlock>(HitResult.GetActor());
	if (HitBuildingBlock == nullptr || !HitBuildingBlock->IsUpgradable() || !HasBuildingPrivilege(HitBuildingBlock->GetActorLocation()))
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = Cast<UInventoryComponent>(OwnerActor->FindComponentByClass<UInventoryComponent>());
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	TArray<FInventoryItem> UpgradeCost;
	if (!GetUpgradeCostForDeployable(HitBuildingBlock, UpgradeCost))
	{
		return;
	}
	
	FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	for (const FInventoryItem& CostItem : UpgradeCost)
	{
		if (InventoryContents->GetItemQuantity(CostItem.Name) < CostItem.Quantity)
		{
			return;
		}

		OwnerInventoryComponent->RemoveItem(CostItem);
	}

	HitBuildingBlock->Upgrade();
	DecrementDurability();
}

void ABuildingHammerItem::Server_DestroyCurrentDeployable_Implementation()
{
	FHitResult HitResult;
	if (!LineTraceOnVisibility(HitResult))
	{
		return;
	}

	ADeployable* HitDeployable = Cast<ADeployable>(HitResult.GetActor());
	if (HitDeployable == nullptr || !HasBuildingPrivilege(HitDeployable->GetActorLocation()))
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = OwnerActor->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	TArray<FInventoryItem> Refund;
	if (GetDestructionRefundForDeployable(HitDeployable, Refund))
	{
		for (const FInventoryItem& RefundItem : Refund)
		{
			OwnerInventoryComponent->AddItem(RefundItem);
		}
	}

	HitDeployable->Multi_PlayDestructionEffects();
	HitDeployable->Destroy();
	DecrementDurability();
}

void ABuildingHammerItem::Server_RotateCurrentDeployable_Implementation()
{
	FHitResult HitResult;
	if (!LineTraceOnVisibility(HitResult))
	{
		return;
	}

	ADeployable* HitDeployable = Cast<ADeployable>(HitResult.GetActor());
	if (HitDeployable == nullptr || !HasBuildingPrivilege(HitDeployable->GetActorLocation()))
	{
		return;
	}

	HitDeployable->Rotate();
	DecrementDurability();
}

bool ABuildingHammerItem::GetLookingAtItemDurability(float& OutCurrentDurability, float& OutMaxDurability, FString& OutActorName) const
{
	if (GetOwnerPawn() == nullptr)
	{
		return false;
	}
	
	FHitResult HitResult;
	if (!LineTraceOnVisibility(HitResult))
	{
		return false;
	}

	IDurabilityInterface* DurabilityInterfaceActor = Cast<IDurabilityInterface>(HitResult.GetActor());
	if (DurabilityInterfaceActor == nullptr)
	{
		return false;
	}
	
	OutCurrentDurability = DurabilityInterfaceActor->GetCurrentDurability();
	OutMaxDurability = DurabilityInterfaceActor->GetMaxDurability();
	OutActorName = HitResult.GetActor()->GetActorNameOrLabel();
	return true;
}

bool ABuildingHammerItem::GetUpgradeCostForDeployable(ADeployable* Deployable, TArray<FInventoryItem>& OutCost)
{
	ABuildingBlock* BuildingBlock = Cast<ABuildingBlock>(Deployable);

	if (!IsValid(BuildingBlock))
	{
		return false;
	}
	
	const FString BuildingBlockIDString = BuildingBlock->GetItemID().ToString();
	FName UpgradeID = NAME_None;
	if (BuildingBlockIDString.Contains(TEXT("wood")))
	{
		UpgradeID = FName(BuildingBlockIDString.Replace(TEXT("wood"), TEXT("stone")));
	}
	else if (BuildingBlockIDString.Contains(TEXT("stone")))
	{
		UpgradeID = FName(BuildingBlockIDString.Replace(TEXT("stone"), TEXT("metal")));
	}
	else if (BuildingBlockIDString.Contains(TEXT("metal")))
	{
		return false;
	}

	FCraftingRecipe* UpgradeRecipe = UCraftingComponent::GetRecipe(UpgradeID);
	if (UpgradeRecipe == nullptr)
	{
		return BuildingBlock->GetNextUpgradeCost(OutCost);
	}

	for (const FInventoryItem& Ingredient : UpgradeRecipe->Ingredients)
	{
		const float IngredientCount = Ingredient.Quantity * 0.75f;
		if (IngredientCount < 1.0f)
		{
			continue;
		}

		FInventoryItem UpgradeCostItem;
		UpgradeCostItem.Name = Ingredient.Name;
		UpgradeCostItem.Quantity = IngredientCount;

		OutCost.Add(UpgradeCostItem);
	}

	return true;
}

bool ABuildingHammerItem::GetDestructionRefundForDeployable(ADeployable* Deployable, TArray<FInventoryItem>& OutRefund)
{
	if (!IsValid(Deployable))
	{
		return false;
	}

	const FName DeployableItemID = Deployable->GetItemID();

	// Get crafting recipe
	FCraftingRecipe* DeployableRecipe = UCraftingComponent::GetRecipe(DeployableItemID);

	// Return if no recipe found
	if (DeployableRecipe == nullptr)
	{
		return false;
	}

	// Get the ingredients
	for (const FInventoryItem& Ingredient : DeployableRecipe->Ingredients)
	{
		// Multiply by some factor
		const float IngredientRefund = Ingredient.Quantity * 0.25f;

		// Discard if less than 1
		if (IngredientRefund < 1.0f)
		{
			continue;
		}

		// Add this to cost
		FInventoryItem RefundItem;
		RefundItem.Name = Ingredient.Name;
		RefundItem.Quantity = FMath::CeilToInt32(IngredientRefund);
		OutRefund.Add(RefundItem);
	}

	return true;
}

bool ABuildingHammerItem::GetRepairCostForDeployable(ADeployable* Deployable, TArray<FInventoryItem>& OutCost)
{
	if (!IsValid(Deployable))
	{
		return false;
	}

	const FName DeployableItemID = Deployable->GetItemID();

	// Get crafting recipe
	FCraftingRecipe* DeployableRecipe = UCraftingComponent::GetRecipe(DeployableItemID);

	// Return if no crafting recipe
	if (DeployableRecipe == nullptr)
	{
		return false;
	}
	
	// Get the ingredients
	for (const FInventoryItem& Ingredient : DeployableRecipe->Ingredients)
	{
		// Multiply by some factor
		const float IngredientCost = Ingredient.Quantity * 0.05f;

		// Discard if less than 1
		if (IngredientCost < 1.0f)
		{
			continue;
		}

		// Add this to cost
		FInventoryItem CostItem;
		CostItem.Name = Ingredient.Name;
		CostItem.Quantity = FMath::CeilToInt32(IngredientCost);
		OutCost.Add(CostItem);
	}

	return true;
}

void ABuildingHammerItem::OnSwingImpact(const FHitResult& HitResult, const FVector& OwnerCharacterLookVector, bool FromFirstPersonInstance)
{
	Super::OnSwingImpact(HitResult, OwnerCharacterLookVector, FromFirstPersonInstance);
	
	if (!HasAuthority())
	{
		return;
	}

	ADeployable* HitDeployable = Cast<ADeployable>(HitResult.GetActor());
	if (HitDeployable == nullptr)
	{
		return;
	}
	
	AttemptDeployableRepair(HitDeployable, HitResult, OwnerCharacterLookVector);
}

void ABuildingHammerItem::ClearWidget()
{
	Widget = nullptr;
}

void ABuildingHammerItem::UpdateBuildingPrivilegeNotifications()
{
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}
	
	UBuilderComponent* OwnerBuilderComponent = OwnerActor->FindComponentByClass<UBuilderComponent>();
	if (OwnerBuilderComponent == nullptr)
	{
		return;
	}

	FVector TestLocation = OwnerActor->GetActorLocation();

	FHitResult HitResult;
	if (LineTraceOnVisibility(HitResult))
	{
		TestLocation = HitResult.ImpactPoint;
	}

	const bool RestrictedZone = OwnerBuilderComponent->IsBuildRestrictedZone(TestLocation);
	const bool BuildingPrivilege = OwnerBuilderComponent->HasBuildingPrivilege(TestLocation);

	if (RestrictedZone && OwnerBuilderComponent->OnAddBuildingPrivilegeNotification.IsBound())
	{
		OwnerBuilderComponent->OnAddBuildingPrivilegeNotification.Broadcast(BuildingPrivilege);
	}
	else if (!RestrictedZone && OwnerBuilderComponent->OnClearBuildingPrivilegeNotification.IsBound())
	{
		OwnerBuilderComponent->OnClearBuildingPrivilegeNotification.Broadcast();
	}
}

bool ABuildingHammerItem::IsBuildRestrictedZone(const FVector& LocationToTest) const
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return true;
	}

	UBuilderComponent* OwnerBuilderComponent = OwnerActor->FindComponentByClass<UBuilderComponent>();
	if (OwnerBuilderComponent == nullptr)
	{
		return true;
	}

	return OwnerBuilderComponent->IsBuildRestrictedZone(LocationToTest);
}

bool ABuildingHammerItem::HasBuildingPrivilege(const FVector& LocationToTest) const
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	UBuilderComponent* OwnerBuilderComponent = OwnerActor->FindComponentByClass<UBuilderComponent>();
	if (OwnerBuilderComponent == nullptr)
	{
		return false;
	}

	return OwnerBuilderComponent->HasBuildingPrivilege(LocationToTest);
}

void ABuildingHammerItem::AttemptDeployableRepair(ADeployable* DeployableToRepair, const FHitResult& HitResult, const FVector& DirectionVector)
{
	// Check if this deployable is damaged
	if (DeployableToRepair->GetCurrentDurability() >= DeployableToRepair->GetMaxDurability())
	{
		return;
	}
	
	UInventoryComponent* OwnerInventoryComponent = GetOwner()->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}
	
	TArray<FInventoryItem> RepairCost;
	if (!GetRepairCostForDeployable(DeployableToRepair, RepairCost))
	{
		return;
	}

	FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	// Check first if we have enough to repair
	for (const FInventoryItem& CostItem : RepairCost)
	{
		if (InventoryContents->GetItemQuantity(CostItem.Name) < CostItem.Quantity)
		{
			return;
		}
	}

	for (const FInventoryItem& CostItem : RepairCost)
	{
		OwnerInventoryComponent->RemoveItem(CostItem);
	}
	
	const float RepairAmount = -10;
	FPointDamageEvent HitByToolEvent(RepairAmount, HitResult, DirectionVector, nullptr);
	DeployableToRepair->TakeDamage(RepairAmount, HitByToolEvent, GetOwnerPawn()->GetController(), this);
}

bool ABuildingHammerItem::LineTraceOnVisibility(FHitResult& OutHitResult) const
{
	const UEquipComponent* OwnerEquipComponent = GetOwnerEquipComponent();
	if (OwnerEquipComponent == nullptr)
	{
		return false;
	}
	
	const FVector OwnerCharacterLookVector = UKismetMathLibrary::GetForwardVector(OwnerEquipComponent->GetOwnerControlRotation());
	const FVector Start = GetOwnerPawn()->FindComponentByClass<UCameraComponent>()->GetComponentLocation();
	const FVector End = Start + (OwnerCharacterLookVector * EffectiveRangeCentimeters);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.bTraceComplex = true;
	CollisionParams.bReturnPhysicalMaterial = true;
	
	return GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility, CollisionParams);
}
