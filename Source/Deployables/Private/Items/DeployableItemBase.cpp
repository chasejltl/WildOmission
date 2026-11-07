// Copyright Telephone Studios. All Rights Reserved.


#include "Items/DeployableItemBase.h"
#include "Deployables/Deployable.h"
#include "Actors/DeployablePreview.h"
#include "Components/PlayerInventoryComponent.h"
#include "Deployables/ToolCupboard.h"
#include "Components/BuilderComponent.h"
#include "ServerRules.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"

ADeployableItemBase::ADeployableItemBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DeployableActorClass = nullptr;
	DeployableRange = 500.0f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HammerMesh(TEXT("/Game/Deployables/Art/SK_Hammer"));
	if (HammerMesh.Succeeded())
	{
		MeshComponent->SetSkeletalMeshAsset(HammerMesh.Object);
	}
}

void ADeployableItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePreview();
	UpdateBuildingPrivilegeNotifications();
}

void ADeployableItemBase::Equip(APawn* InOwnerPawn, USkeletalMeshComponent* InThirdPersonMeshComponent, const FName& InItemName, const int8& InFromSlotIndex, const uint32& InUniqueID)
{
	Super::Equip(InOwnerPawn, InThirdPersonMeshComponent, InItemName, InFromSlotIndex, InUniqueID);

	Client_SpawnPreview();
}

void ADeployableItemBase::OnUnequip()
{
	Super::OnUnequip();
}

void ADeployableItemBase::Destroyed()
{
	Super::Destroyed();
	
	if (IsValid(PreviewActor))
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
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

void ADeployableItemBase::OnPrimaryPressed()
{
	Super::OnPrimaryPressed();
	
	if (!HasAuthority())
	{
		return;
	}

	OnPlace();
}

UClass* ADeployableItemBase::GetDeployableActorClass() const
{
	return DeployableActorClass;
}

void ADeployableItemBase::OnPlace()
{
	// In Creative / Free-Build Mode, do not consume the item from the player's inventory when placing!
	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && Rules->bCreativeFreeBuildEnabled)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UPlayerInventoryComponent* OwnerInventory = OwnerActor->FindComponentByClass<UPlayerInventoryComponent>();
	if (OwnerInventory == nullptr)
	{
		return;
	}

	OwnerInventory->RemoveHeldItem();
}

bool ADeployableItemBase::LineTraceOnChannel(TEnumAsByte<ECollisionChannel> ChannelToTrace, FHitResult& OutHitResult) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	APawn* OwnerPawn = GetOwnerPawn();
	if (OwnerPawn == nullptr)
	{
		return false;
	}

	UCameraComponent* PawnCameraComponent = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (PawnCameraComponent == nullptr)
	{
		return false;
	}

	FVector Start = PawnCameraComponent->GetComponentLocation();
	FVector End = Start + (UKismetMathLibrary::GetForwardVector(OwnerPawn->GetControlRotation()) * DeployableRange);	// In this case the replicated rotation isnt needed,
	FCollisionQueryParams Params;																							// this is because the autonomys proxy and the authority are the only ones
																															// who will be reading this data, both are well aware of the control rotatation
	Params.AddIgnoredActor(OwnerPawn);
	Params.AddIgnoredActor(this);
	if (PreviewActor)
	{
		Params.AddIgnoredActor(PreviewActor);
	}

	if (World->LineTraceSingleByChannel(OutHitResult, Start, End, ChannelToTrace, Params))
	{
		return true;
	}

	return false;
}

bool ADeployableItemBase::GetPlacementTransform(FTransform& OutPlacementTransform)
{
	OutPlacementTransform = GetFreehandPlacementTransform();
	return false;
}

FTransform ADeployableItemBase::GetFreehandPlacementTransform()
{
	if (DeployableActorClass == nullptr)
	{
		return FTransform::Identity;
	}

	ADeployable* DefaultDeployableActorClass = DeployableActorClass.GetDefaultObject();
	if (DefaultDeployableActorClass == nullptr)
	{
		return FTransform::Identity;
	}

	FVector PlacementLocation = FVector::ZeroVector;
	FRotator PlacementRotation = FRotator::ZeroRotator;
	FVector PlacementUp = FVector::UpVector;

	FHitResult HitResult;
	if (LineTraceOnChannel(ECC_Visibility, HitResult))
	{
		PlacementLocation = HitResult.ImpactPoint;
		if (DeployableActorClass && DefaultDeployableActorClass->FollowsSurfaceNormal())
		{
			PlacementUp = HitResult.ImpactNormal;
		}
	}
	else
	{
		PlacementLocation = GetLocationInFrontOfPlayer();
	}

	PlacementRotation = GetFacePlayerRotation(PlacementLocation, PlacementUp);

	FTransform PlacementTransform;
	PlacementTransform.SetLocation(PlacementLocation);
	PlacementTransform.SetRotation(PlacementRotation.Quaternion());

	return PlacementTransform;
}

FVector ADeployableItemBase::GetLocationInFrontOfPlayer() const
{
	APawn* OwnerPawn = GetOwnerPawn();
	if (OwnerPawn == nullptr)
	{
		return FVector::ZeroVector;
	}

	UCameraComponent* PawnCameraComponent = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (PawnCameraComponent == nullptr)
	{
		return FVector::ZeroVector;
	}

	const FVector PawnCameraLocation = PawnCameraComponent->GetComponentLocation();
	const FVector PawnForwardVector = UKismetMathLibrary::GetForwardVector(OwnerPawn->GetControlRotation());

	return PawnCameraLocation + (PawnForwardVector * DeployableRange);
}

FRotator ADeployableItemBase::GetFacePlayerRotation(const FVector& PlacementLocation, const FVector& Up) const
{
	APawn* OwnerPawn = GetOwnerPawn();
	if (OwnerPawn == nullptr)
	{
		return FRotator::ZeroRotator;
	}

	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerPawn->GetActorLocation(), PlacementLocation);

	const FVector PlacementForward = -UKismetMathLibrary::GetForwardVector(FRotator(90.0f - Up.Rotation().Pitch, LookAtRotation.Yaw, 0.0f));
	const FVector PlacementRight = -UKismetMathLibrary::GetRightVector(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
	
	return UKismetMathLibrary::MakeRotationFromAxes(PlacementForward, PlacementRight, Up);
}

UStaticMesh* ADeployableItemBase::GetPreviewMesh()
{
	return nullptr;
}

void ADeployableItemBase::UpdatePreview()
{
	if (!IsValid(PreviewActor))
	{
		return;
	}

	FTransform PlacementTransform;
	bool SpawnValid = GetPlacementTransform(PlacementTransform);
	PreviewActor->SetActorTransform(PlacementTransform);
	PreviewActor->Update(SpawnValid);
	bPrimaryEnabled = SpawnValid;
}

void ADeployableItemBase::UpdateBuildingPrivilegeNotifications()
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
	if (LineTraceOnChannel(ECollisionChannel::ECC_Visibility, HitResult))
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

void ADeployableItemBase::Client_SpawnPreview_Implementation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	PreviewActor = World->SpawnActor<ADeployablePreview>();
	if (PreviewActor == nullptr)
	{
		return;
	}

	if (DeployableActorClass.GetDefaultObject() != nullptr)
	{
		PreviewActor->Setup(DeployableActorClass.GetDefaultObject());
	}
	else if (GetPreviewMesh() != nullptr)
	{
		PreviewActor->Setup(GetPreviewMesh());
	}
}

void ADeployableItemBase::Client_DestroyPreview_Implementation()
{
	if (PreviewActor == nullptr)
	{
		return;
	}

	PreviewActor->Destroy();
	PreviewActor = nullptr;
}

bool ADeployableItemBase::HasBuildingPrivilege(const FVector& LocationToTest) const
{
	const UServerRules* Rules = UServerRules::Get(this);
	if (Rules != nullptr && Rules->bCreativeFreeBuildEnabled)
	{
		return true;
	}

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
