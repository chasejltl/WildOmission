// Copyright Telephone Studios. All Rights Reserved.

// Player Controller Core
#include "WildOmissionPlayerController.h"
#include "WildOmissionCore/Characters/WildOmissionCharacter.h"
#include "WildOmissionCore/Characters/WildOmissionSpectatorPawn.h"
#include "GameFramework/PlayerState.h"

// Wild Omission Components
#include "Components/VitalsComponent.h"
#include "Components/InventoryManipulatorComponent.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/PlayerSaveManagerComponent.h"
#include "Components/MusicPlayerComponent.h"
#include "WildOmissionCore/Components/WOInGameAchievementsComponent.h"

// Wild Omission Stuff
#include "WildOmissionCore/WildOmissionGameInstance.h"
#include "WildOmissionCore/GameModes/WildOmissionGameMode.h"
#include "SaveManager.h"
#include "ChunkManager.h"
#include "Actors/Chunk.h"
#include "Actors/ChunkInvokerActor.h"
#include "GameChatManager.h"
#include "UI/GameplayMenuWidget.h"
#include "WildOmissionCore/UI/Player/DeathMenuWidget.h"

// Deployables & Base Builder Stuff
#include "Deployables/Deployable.h"
#include "Deployables/BuildingBlock.h"
#include "Deployables/ToolCupboard.h"
#include "Items/DeployableItemBase.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Engine Stuff
#include "GameFramework/GameSession.h"
#include "Net/UnrealNetwork.h"

AWildOmissionPlayerController::AWildOmissionPlayerController()
{
	bIsStillLoading = true;
	SpawnChunk = FIntVector2();

	TempChunkInvoker = nullptr;

	GameModeIndex = 0;

	BedUniqueID = -1;
	BedChunkLocation = FIntVector2();

	Administrator = false;

	MusicPlayerComponent = nullptr;

	AchievementsComponent = CreateDefaultSubobject<UWOInGameAchievementsComponent>(TEXT("AchievementsComponent"));

	static ConstructorHelpers::FClassFinder<UDeathMenuWidget> DeathMenuWidgetBlueprint(TEXT("/Game/WildOmissionCore/UI/Player/WBP_DeathMenu"));
	if (DeathMenuWidgetBlueprint.Succeeded())
	{
		DeathMenuWidgetClass = DeathMenuWidgetBlueprint.Class;
	}
}

void AWildOmissionPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, GameModeIndex, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, InCheatedWorld, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, BedUniqueID, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, SpawnChunk, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, LastDeathLocation, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, SavedLocation, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWildOmissionPlayerController, Administrator, COND_OwnerOnly);
}

FPlayerSaveData AWildOmissionPlayerController::SavePlayer()
{
	FPlayerSaveData PlayerSaveData;

	if (!HasAuthority())
	{
		return PlayerSaveData;
	}

	APlayerState* CurrentPlayerState = PlayerState.Get();
	if (CurrentPlayerState == nullptr || AchievementsComponent == nullptr)
	{
		return PlayerSaveData;
	}

	PlayerSaveData.UniqueID = CurrentPlayerState->GetUniqueId().ToString();
	PlayerSaveData.GameMode = GameModeIndex;
	PlayerSaveData.BedUniqueID = BedUniqueID;
	PlayerSaveData.BedChunkLocation = BedChunkLocation;
	PlayerSaveData.AchievementStatsData = AchievementsComponent->GetStatsData();
	PlayerSaveData.NewPlayer = false;

	AWildOmissionCharacter* WildOmissionCharacter = Cast<AWildOmissionCharacter>(GetCharacter());
	if (WildOmissionCharacter == nullptr)
	{
		PlayerSaveData.IsAlive = false;
		return PlayerSaveData;
	}

	PlayerSaveData.WorldLocation = WildOmissionCharacter->GetActorLocation();

	PlayerSaveData.LastDeathLocation = LastDeathLocation;
	PlayerSaveData.SavedLocation = SavedLocation;

	PlayerSaveData.IsAlive = true;
	PlayerSaveData.IsHost = IsHost();

	UVitalsComponent* PlayerVitalsComponent = WildOmissionCharacter->GetVitalsComponent();
	if (PlayerVitalsComponent)
	{
		PlayerSaveData.Vitals.Health = PlayerVitalsComponent->GetHealth();
		PlayerSaveData.Vitals.Hunger = PlayerVitalsComponent->GetHunger();
		PlayerSaveData.Vitals.Thirst = PlayerVitalsComponent->GetThirst();
	}

	UPlayerInventoryComponent* PlayerInventoryComponent = WildOmissionCharacter->GetInventoryComponent();
	if (PlayerInventoryComponent)
	{
		PlayerSaveData.Inventory.ByteData = PlayerInventoryComponent->Save();
	}

	UInventoryManipulatorComponent* PlayerInventoryManipulatorComponent = WildOmissionCharacter->GetInventoryManipulatorComponent();
	if (PlayerInventoryManipulatorComponent)
	{
		PlayerSaveData.SelectedItemByteData = PlayerInventoryManipulatorComponent->GetSelectedItemAsByteData();
	}

	return PlayerSaveData;
}

void AWildOmissionPlayerController::LoadPlayerSave(const FPlayerSaveData& SaveData)
{
	BedUniqueID = SaveData.BedUniqueID;
	BedChunkLocation = SaveData.BedChunkLocation;
	AchievementsComponent->SetStatsData(SaveData.AchievementStatsData);

	LastDeathLocation = SaveData.LastDeathLocation;
	SavedLocation = SaveData.SavedLocation;

	StoredPlayerSaveData = SaveData;
}

bool AWildOmissionPlayerController::IsStillLoading() const
{
	return bIsStillLoading;
}

FString AWildOmissionPlayerController::GetUniqueID() const
{
	FString ID = TEXT("");

	if (PlayerState)
	{
		ID = PlayerState->GetUniqueId().ToString();
	}

	return ID;
}

bool AWildOmissionPlayerController::IsHost() const
{
	return GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy;
}

void AWildOmissionPlayerController::SendMessage(APlayerState* Sender, const FString& Message)
{
	Server_SendMessage(Sender, Message);
}

void AWildOmissionPlayerController::Sleep()
{
	// TODO sleep logic here
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	AWildOmissionGameMode* GameMode = Cast<AWildOmissionGameMode>(World->GetAuthGameMode());
	if (GameMode == nullptr)
	{
		return;
	}

	GameMode->OnPlayerSleep(this);
}

void AWildOmissionPlayerController::SetCurrentBed(const int32& InBedUniqueID, const FIntVector2& InBedChunkLocation)
{
	BedUniqueID = InBedUniqueID;
	BedChunkLocation = InBedChunkLocation;
}

int32 AWildOmissionPlayerController::GetBedUniqueID() const
{
	return BedUniqueID;
}

FIntVector2 AWildOmissionPlayerController::GetBedChunkLocation() const
{
	return BedChunkLocation;
}

void AWildOmissionPlayerController::SetAdministrator(bool InAdministrator)
{
	Administrator = InAdministrator;
}

bool AWildOmissionPlayerController::IsAdministrator() const
{
	return Administrator;
}

void AWildOmissionPlayerController::KickPlayer(APlayerController* PlayerControllerToKick)
{
	Server_KickPlayer(PlayerControllerToKick);
}

void AWildOmissionPlayerController::SetGameModeIndex(const uint8& NewGameModeIndex)
{
	GameModeIndex = NewGameModeIndex;
}

uint8 AWildOmissionPlayerController::GetGameModeIndex() const
{
	return GameModeIndex;
}

void AWildOmissionPlayerController::SetInCheatedWorld(bool CheatedWorld)
{
	InCheatedWorld = CheatedWorld;

	if (AchievementsComponent)
	{
		AchievementsComponent->SetAchievementsEnabled(InCheatedWorld == false);
	}
}

bool AWildOmissionPlayerController::IsInCheatedWorld() const
{
	return InCheatedWorld;
}

bool AWildOmissionPlayerController::IsSurvivalMode() const
{
	return GameModeIndex == 0;
}

bool AWildOmissionPlayerController::IsCreativeMode() const
{
	return GameModeIndex == 1;
}

void AWildOmissionPlayerController::Save()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	Server_AddToPendingSaves();

	if (!HasAuthority())
	{
		return;
	}
	
	AWildOmissionGameMode* GameMode = Cast<AWildOmissionGameMode>(World->GetAuthGameMode());
	if (GameMode == nullptr)
	{
		return;
	}

	GameMode->SaveWorld();
}

bool AWildOmissionPlayerController::IsEditorPlayer() const
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

void AWildOmissionPlayerController::Server_AddToPendingSaves_Implementation()
{
	ASaveManager* SaveManager = ASaveManager::GetSaveManager();
	if (SaveManager == nullptr)
	{
		return;
	}

	UPlayerSaveManagerComponent* PlayerSaveManager = SaveManager->GetPlayerManager();
	if (PlayerSaveManager == nullptr)
	{
		return;
	}

	PlayerSaveManager->AddToPending(this);
}

void AWildOmissionPlayerController::Server_KillThisPlayer_Implementation()
{
	// Get pawn
	APawn* OurPawn = GetPawn();
	if (OurPawn == nullptr)
	{
		return;
	}

	// Get vitals from pawn
	UVitalsComponent* PawnVitals = OurPawn->FindComponentByClass<UVitalsComponent>();
	if (PawnVitals == nullptr)
	{
		return;
	}

	// Set health to zero
	PawnVitals->SetHealth(0.0f);
}

//*****************************
// Console functions

void AWildOmissionPlayerController::OnPlayerDeath(const FVector& DeathLocation)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	SetLastDeathLocation(DeathLocation);

	TempChunkInvoker = World->SpawnActor<AChunkInvokerActor>(AChunkInvokerActor::StaticClass(), DeathLocation, FRotator::ZeroRotator);
}

void AWildOmissionPlayerController::Kill()
{
	Server_KillThisPlayer();
}

void AWildOmissionPlayerController::LogLocalInventoryContents()
{
	if (!IsLocalController())
	{
		return;
	}
	AWildOmissionCharacter* WildOmissionCharacter = Cast<AWildOmissionCharacter>(GetPawn());
	if (WildOmissionCharacter == nullptr)
	{
		return;
	}

	// Get their inventory components
	for (const FInventoryItem& ItemData : GetPawn()->FindComponentByClass<UPlayerInventoryComponent>()->GetContents()->Contents)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.0f, FColor::Orange, FString::Printf(TEXT("Item: %s, Quantity: %i"), *ItemData.Name.ToString(), ItemData.Quantity));
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.0f, FColor::Green, FString::Printf(TEXT("Player: %s"), *WildOmissionCharacter->GetActorNameOrLabel()));
}

void AWildOmissionPlayerController::BuildBase(const FString& BlueprintName)
{
	Server_BuildBase(BlueprintName);
}

void AWildOmissionPlayerController::ExportBase(const FString& BlueprintName, float RadiusCm)
{
	Server_ExportBase(BlueprintName, RadiusCm);
}

void AWildOmissionPlayerController::GiveItem(const FString& ItemName, int32 Quantity)
{
	Server_GiveItem(FName(*ItemName), FMath::Max(1, Quantity));
}

void AWildOmissionPlayerController::GiveStarterKit()
{
	Server_GiveStarterKit();
}

void AWildOmissionPlayerController::SetSavedLocationToCurrentLocation()
{
	Server_SetSavedLocationToCurrentLocation();
}

void AWildOmissionPlayerController::SetLastDeathLocation(const FVector& DeathLocation)
{
	LastDeathLocation = DeathLocation / AChunk::GetVertexDistanceScale();
}

FVector AWildOmissionPlayerController::GetCurrentCoordinateLocation()
{
	APawn* OurPawn = GetPawn();
	if (!IsValid(OurPawn))
	{
		return FVector::ZeroVector;
	}

	return OurPawn->GetActorLocation() / AChunk::GetVertexDistanceScale();
}

FVector AWildOmissionPlayerController::GetSavedLocation()
{
	return SavedLocation;
}

FVector AWildOmissionPlayerController::GetLastDeathLocation()
{
	return LastDeathLocation;
}

void AWildOmissionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetupPlayerOnServer();
	}

	if (!IsLocalController())
	{
		return;
	}
	
	StartLoading();

	SetupMusicPlayerComponent();

	HasAuthority() ? SetupLocalAsHost() : SetupLocalAsClient();
}

void AWildOmissionPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (AWildOmissionSpectatorPawn* WildOmissionSpectatorPawn = Cast<AWildOmissionSpectatorPawn>(aPawn))
	{
		TempChunkInvoker = 
			World->SpawnActor<AChunkInvokerActor>
			(AChunkInvokerActor::StaticClass(),
			WildOmissionSpectatorPawn->GetActorLocation(),
			WildOmissionSpectatorPawn->GetActorRotation());
		return;
	}

	AWildOmissionCharacter* WildOmissionCharacter = Cast<AWildOmissionCharacter>(aPawn);
	if (WildOmissionCharacter == nullptr)
	{
		return;
	}

	WildOmissionCharacter->OnPlayerDeath.AddDynamic(this, &AWildOmissionPlayerController::OnPlayerDeath);

	if (bIsStillLoading == false || StoredPlayerSaveData.IsAlive == false || StoredPlayerSaveData.NewPlayer == true)
	{
		return;
	}

	WildOmissionCharacter->SetActorLocation(StoredPlayerSaveData.WorldLocation);

	UVitalsComponent* CharacterVitalsComponent = WildOmissionCharacter->GetVitalsComponent();
	if (CharacterVitalsComponent == nullptr)
	{
		return;
	}

	CharacterVitalsComponent->SetHealth(StoredPlayerSaveData.Vitals.Health);
	CharacterVitalsComponent->SetHunger(StoredPlayerSaveData.Vitals.Hunger);
	CharacterVitalsComponent->SetThirst(StoredPlayerSaveData.Vitals.Thirst);

	UInventoryComponent* CharacterInventoryComponent = WildOmissionCharacter->GetInventoryComponent();
	UInventoryManipulatorComponent* CharacterInventoryManipulatorComponent = WildOmissionCharacter->GetInventoryManipulatorComponent();
	if (CharacterInventoryComponent == nullptr || CharacterInventoryManipulatorComponent == nullptr)
	{
		return;
	}

	CharacterInventoryComponent->Load(StoredPlayerSaveData.Inventory.ByteData);
	CharacterInventoryManipulatorComponent->LoadSelectedItemFromByteDataAndDropInWorld(StoredPlayerSaveData.SelectedItemByteData);

	StoredPlayerSaveData = FPlayerSaveData();

	if (TempChunkInvoker == nullptr)
	{
		return;
	}

	TempChunkInvoker->Destroy();
	TempChunkInvoker = nullptr;
}

void AWildOmissionPlayerController::SetupPlayerOnServer()
{
	UWorld* World = GetWorld();
	AChunkManager* ChunkManager = AChunkManager::GetChunkManager();
	if (World == nullptr || ChunkManager == nullptr)
	{
		return;
	}

	const bool UseDefaultSpawn = StoredPlayerSaveData.IsAlive == false || StoredPlayerSaveData.NewPlayer == true;
	const FVector SpawnPoint = UseDefaultSpawn ? ChunkManager->GetWorldSpawnPoint() : StoredPlayerSaveData.WorldLocation;

	// Spawn Wild Omission spectator (Has chunk invoker)
	AWildOmissionSpectatorPawn* SpecPawn = World->SpawnActor<AWildOmissionSpectatorPawn>(AWildOmissionSpectatorPawn::StaticClass(), SpawnPoint, FRotator::ZeroRotator);
	if (SpecPawn)
	{
		this->Possess(SpecPawn);
	}

	const float ChunkSize = AChunk::GetVertexSize() * AChunk::GetVertexDistanceScale();
	SpawnChunk = FIntVector2(
		FMath::RoundToInt32(SpawnPoint.X / ChunkSize),
		FMath::RoundToInt32(SpawnPoint.Y / ChunkSize));
}

void AWildOmissionPlayerController::StartLoading()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UWildOmissionGameInstance* GameInstance = UWildOmissionGameInstance::GetWildOmissionGameInstance(World);
	if (GameInstance == nullptr)
	{
		return;
	}

	GameInstance->StartLoading();
	if (!IsHost())
	{
		GameInstance->SetLoadingSubtitle(TEXT("Loading world state."));
	}
	UE_LOG(LogPlayerController, Verbose, TEXT("BeginPlay: Brought up loading screen."));
}

void AWildOmissionPlayerController::SetupMusicPlayerComponent()
{
	if (MusicPlayerComponent != nullptr)
	{
		return;
	}

	MusicPlayerComponent = NewObject<UMusicPlayerComponent>(this, UMusicPlayerComponent::StaticClass(), TEXT("MusicPlayerComponent"));
	if (MusicPlayerComponent == nullptr)
	{
		return;
	}
	
	MusicPlayerComponent->RegisterComponent();
}

void AWildOmissionPlayerController::SetupLocalAsHost()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FTimerHandle LoadTimerHandle;
	FTimerDelegate LoadTimerDelegate;
	LoadTimerDelegate.BindUObject(this, &AWildOmissionPlayerController::StopLoading);
	World->GetTimerManager().SetTimer(LoadTimerHandle, LoadTimerDelegate, 2.0f, false);
	UE_LOG(LogPlayerController, Verbose, TEXT("BeginPlay: Setup load timer."));
}

void AWildOmissionPlayerController::SetupLocalAsClient()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FTimerDelegate CheckSpawnChunkValidTimerDelegate;
	CheckSpawnChunkValidTimerDelegate.BindUObject(this, &AWildOmissionPlayerController::CheckSpawnChunkValid);
	World->GetTimerManager().SetTimer(CheckSpawnChunkValidTimerHandle, CheckSpawnChunkValidTimerDelegate, 2.0f, true);
	UE_LOG(LogPlayerController, Verbose, TEXT("BeginPlay: Setup check spawn chunk timer."));
}

void AWildOmissionPlayerController::CheckSpawnChunkValid()
{
	UWorld* World = GetWorld();
	AChunkManager* ChunkManager = AChunkManager::GetChunkManager();
	if (World == nullptr || ChunkManager == nullptr)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("CheckSpawnChunkValid: Failed because World or ChunkManager is nullptr."))
		return;
	}

	FSpawnedChunk SpawnedChunk;
	ChunkManager->GetSpawnedChunk(SpawnChunk, SpawnedChunk);

	const int32 DataCount = (AChunk::GetVertexSize() + 1) * (AChunk::GetVertexSize() + 1);
	if (!IsValid(SpawnedChunk.Chunk) || SpawnedChunk.Chunk->GetHeightData().Num() != DataCount)
	{
		UE_LOG(LogPlayerController, Verbose, TEXT("CheckSpawnChunkValid: Chunk is still loading."));
		return;
	}

	UE_LOG(LogPlayerController, Verbose, TEXT("CheckSpawnChunkValid: Chunk loaded successfully, requesting spawn."));
	World->GetTimerManager().ClearTimer(CheckSpawnChunkValidTimerHandle);
	StopLoading();
}

void AWildOmissionPlayerController::StopLoading()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	Server_Spawn();
	
	UWildOmissionGameInstance* GameInstance = UWildOmissionGameInstance::GetWildOmissionGameInstance(World);
	if (GameInstance == nullptr)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("Couldnt Stop Loading, GameInstance returned a nullptr."));
		return;
	}

	GameInstance->StopLoading();
}

void AWildOmissionPlayerController::Server_SetSavedLocationToCurrentLocation_Implementation()
{
	APawn* OurPawn = GetPawn();
	if (!IsValid(OurPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to save current location, player controller doesn't have a pawn."));
		return;
	}

	SavedLocation = OurPawn->GetActorLocation() / AChunk::GetVertexDistanceScale();
}

void AWildOmissionPlayerController::Server_SendMessage_Implementation(APlayerState* Sender, const FString& Message)
{
	AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager();

	FString Trimmed = Message.TrimStartAndEnd();
	if (Trimmed.StartsWith(TEXT("/")))
	{
		TArray<FString> Tokens;
		Trimmed.ParseIntoArrayWS(Tokens);
		if (Tokens.Num() > 0)
		{
			const FString Cmd = Tokens[0].ToLower();
			if (Cmd == TEXT("/buildbase") && Tokens.Num() >= 2)
			{
				Server_BuildBase(Tokens[1]);
				return;
			}
			else if (Cmd == TEXT("/exportbase") && Tokens.Num() >= 2)
			{
				const float Radius = (Tokens.Num() >= 3) ? FCString::Atof(*Tokens[2]) : 3500.0f;
				Server_ExportBase(Tokens[1], Radius);
				return;
			}
			else if (Cmd == TEXT("/give") && Tokens.Num() >= 2)
			{
				const int32 Qty = (Tokens.Num() >= 3) ? FCString::Atoi(*Tokens[2]) : 1;
				Server_GiveItem(FName(*Tokens[1]), FMath::Max(1, Qty));
				return;
			}
			else if (Cmd == TEXT("/kit") || Cmd == TEXT("/giveall"))
			{
				Server_GiveStarterKit();
				return;
			}
		}
	}

	if (ChatManager == nullptr)
	{
		return;
	}

	ChatManager->SendMessage(Sender, Message, EChatMessageType::STANDARD);
}

void AWildOmissionPlayerController::Server_BuildBase_Implementation(const FString& BlueprintName)
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	FString CleanName = BlueprintName.TrimStartAndEnd();
	if (CleanName.EndsWith(TEXT(".json")))
	{
		CleanName.LeftChopInline(5);
	}

	FString CandidateSaved = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BaseBlueprints"), CleanName + TEXT(".json"));
	FString CandidateTools = FPaths::Combine(FPaths::ProjectDir(), TEXT("Tools/BaseBuilder/Blueprints"), CleanName + TEXT(".json"));

	FString FilePath = FPaths::FileExists(CandidateSaved) ? CandidateSaved : CandidateTools;
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		const FString ErrMsg = FString::Printf(TEXT("[BaseBuilder] Blueprint '%s' not found in Saved/BaseBlueprints or Tools/BaseBuilder/Blueprints."), *CleanName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, FColor::Red, ErrMsg);
		}
		if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
		{
			ChatManager->SendMessage(PlayerState, ErrMsg, EChatMessageType::STANDARD);
		}
		return;
	}

	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		return;
	}

	// Calculate BaseOrigin 600cm in front of the player, snapped to terrain height
	const float SnappedYaw = FMath::RoundToFloat(GetControlRotation().Yaw / 90.0f) * 90.0f;
	const FRotator BaseFrameRot(0.0f, SnappedYaw, 0.0f);
	FVector BaseOrigin = OurPawn->GetActorLocation() + BaseFrameRot.Vector() * 600.0f;

	FHitResult GroundHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(OurPawn);
	if (World->LineTraceSingleByChannel(GroundHit, BaseOrigin + FVector(0.0f, 0.0f, 1500.0f), BaseOrigin - FVector(0.0f, 0.0f, 2500.0f), ECC_Visibility, TraceParams))
	{
		BaseOrigin.Z = GroundHit.ImpactPoint.Z;
	}

	const TArray<TSharedPtr<FJsonValue>>* PiecesArray = nullptr;
	if (!RootObj->TryGetArrayField(TEXT("pieces"), PiecesArray) || PiecesArray == nullptr)
	{
		return;
	}

	int32 SpawnedCount = 0;
	for (const TSharedPtr<FJsonValue>& Val : *PiecesArray)
	{
		const TSharedPtr<FJsonObject> PieceObj = Val->AsObject();
		if (!PieceObj.IsValid())
		{
			continue;
		}

		FString ItemIDStr;
		if (!PieceObj->TryGetStringField(TEXT("item"), ItemIDStr))
		{
			continue;
		}

		FVector Offset = FVector::ZeroVector;
		const TArray<TSharedPtr<FJsonValue>>* OffsetArr = nullptr;
		if (PieceObj->TryGetArrayField(TEXT("offset"), OffsetArr) && OffsetArr != nullptr && OffsetArr->Num() >= 3)
		{
			Offset.X = (*OffsetArr)[0]->AsNumber();
			Offset.Y = (*OffsetArr)[1]->AsNumber();
			Offset.Z = (*OffsetArr)[2]->AsNumber();
		}

		FRotator LocalRot = FRotator::ZeroRotator;
		const TArray<TSharedPtr<FJsonValue>>* RotArr = nullptr;
		if (PieceObj->TryGetArrayField(TEXT("rotation"), RotArr) && RotArr != nullptr && RotArr->Num() >= 3)
		{
			LocalRot.Pitch = (*RotArr)[0]->AsNumber();
			LocalRot.Yaw = (*RotArr)[1]->AsNumber();
			LocalRot.Roll = (*RotArr)[2]->AsNumber();
		}

		// Resolve deployable class from DT_Items / RegisteredItemData
		UClass* DeployableClass = nullptr;
		FItemData* ItemData = UInventoryComponent::GetItemData(FName(*ItemIDStr));
		if (ItemData != nullptr && ItemData->EquipItemClass != nullptr)
		{
			if (const ADeployableItemBase* EquipCDO = Cast<ADeployableItemBase>(ItemData->EquipItemClass->GetDefaultObject()))
			{
				DeployableClass = EquipCDO->GetDeployableActorClass();
			}
		}
		if (DeployableClass == nullptr)
		{
			DeployableClass = ABuildingBlock::StaticClass();
		}

		const FVector WorldPos = BaseOrigin + BaseFrameRot.RotateVector(Offset);
		const FRotator WorldRot = FRotator(LocalRot.Pitch, SnappedYaw + LocalRot.Yaw, LocalRot.Roll);
		FTransform SpawnTransform(WorldRot, WorldPos);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ADeployable* Spawned = World->SpawnActor<ADeployable>(DeployableClass, SpawnTransform, SpawnParams);
		if (Spawned != nullptr)
		{
			Spawned->OnSpawn();
			if (AToolCupboard* TC = Cast<AToolCupboard>(Spawned))
			{
				TC->AuthorizePlayer(GetUniqueID());
			}
			SpawnedCount++;
		}
	}

	const FString SuccessMsg = FString::Printf(TEXT("[BaseBuilder] Spawned Python base '%s' (%d structures placed)!"), *CleanName, SpawnedCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, FColor::Green, SuccessMsg);
	}
	if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
	{
		ChatManager->SendMessage(PlayerState, SuccessMsg, EChatMessageType::STANDARD);
	}
}

void AWildOmissionPlayerController::Server_ExportBase_Implementation(const FString& BlueprintName, float RadiusCm)
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	FString CleanName = BlueprintName.TrimStartAndEnd();
	if (CleanName.IsEmpty())
	{
		CleanName = TEXT("exported_base");
	}

	const FVector Origin = OurPawn->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> PiecesArray;

	for (TActorIterator<ADeployable> It(World); It; ++It)
	{
		ADeployable* Dep = *It;
		if (!IsValid(Dep))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Origin, Dep->GetActorLocation());
		if (DistSq > FMath::Square(RadiusCm))
		{
			continue;
		}

		TSharedPtr<FJsonObject> PieceObj = MakeShared<FJsonObject>();
		PieceObj->SetStringField(TEXT("item"), Dep->GetItemID().IsNone() ? TEXT("foundation.stone") : Dep->GetItemID().ToString());

		const FVector RelOffset = Dep->GetActorLocation() - Origin;
		TArray<TSharedPtr<FJsonValue>> OffsetVals;
		OffsetVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(RelOffset.X)));
		OffsetVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(RelOffset.Y)));
		OffsetVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(RelOffset.Z)));
		PieceObj->SetArrayField(TEXT("offset"), OffsetVals);

		const FRotator Rot = Dep->GetActorRotation();
		TArray<TSharedPtr<FJsonValue>> RotVals;
		RotVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(Rot.Pitch)));
		RotVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(Rot.Yaw)));
		RotVals.Add(MakeShared<FJsonValueNumber>(FMath::RoundToFloat(Rot.Roll)));
		PieceObj->SetArrayField(TEXT("rotation"), RotVals);

		PiecesArray.Add(MakeShared<FJsonValueObject>(PieceObj));
	}

	TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>();
	RootObj->SetStringField(TEXT("name"), CleanName);
	RootObj->SetArrayField(TEXT("pieces"), PiecesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer);

	const FString OutPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BaseBlueprints"), CleanName + TEXT(".json"));
	FFileHelper::SaveStringToFile(OutputString, *OutPath);

	const FString Msg = FString::Printf(TEXT("[BaseBuilder] Exported %d structures to Saved/BaseBlueprints/%s.json"), PiecesArray.Num(), *CleanName);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, FColor::Cyan, Msg);
	}
	if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
	{
		ChatManager->SendMessage(PlayerState, Msg, EChatMessageType::STANDARD);
	}
}

void AWildOmissionPlayerController::Server_GiveItem_Implementation(const FName& ItemName, int32 Quantity)
{
	APawn* OurPawn = GetPawn();
	if (OurPawn == nullptr)
	{
		return;
	}

	UInventoryComponent* Inv = OurPawn->FindComponentByClass<UInventoryComponent>();
	if (Inv == nullptr)
	{
		return;
	}

	FInventoryItem Item;
	Item.Name = ItemName;
	Item.Quantity = FMath::Max(1, Quantity);
	Inv->AddItem(Item);
}

void AWildOmissionPlayerController::Server_GiveStarterKit_Implementation()
{
	APawn* OurPawn = GetPawn();
	if (OurPawn == nullptr)
	{
		return;
	}

	UInventoryComponent* Inv = OurPawn->FindComponentByClass<UInventoryComponent>();
	if (Inv == nullptr)
	{
		return;
	}

	const TArray<TPair<FName, int32>> KitItems = {
		{ TEXT("AssaultRifle"), 1 },
		{ TEXT("CombatShotgun"), 1 },
		{ TEXT("BoltActionRifle"), 1 },
		{ TEXT("RifleBullet"), 128 },
		{ TEXT("TwelveGaugeBuckshot"), 64 },
		{ TEXT("TimedExplosiveCharge"), 10 },
		{ TEXT("StorageBarrel"), 1 },
		{ TEXT("SolarPanel"), 1 },
		{ TEXT("IndustrialWallLight"), 1 },
		{ TEXT("CeilingFloodLight"), 1 },
		{ TEXT("ArmoredFoundation"), 10 },
		{ TEXT("ArmoredWall"), 10 },
		{ TEXT("HighExternalStoneWall"), 5 },
		{ TEXT("MedicalSyringe"), 2 },
		{ TEXT("HazmatSuit"), 1 }
	};

	for (const TPair<FName, int32>& Pair : KitItems)
	{
		FInventoryItem Item;
		Item.Name = Pair.Key;
		Item.Quantity = Pair.Value;
		Inv->AddItem(Item);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, FColor::Green, TEXT("[BaseBuilder] Added full Guns, Ammo, Storage Barrel, Electrical, Lighting & Armored Building Kit to Inventory!"));
	}
}

bool AWildOmissionPlayerController::Server_KickPlayer_Validate(APlayerController* PlayerControllerToKick)
{
	if (!this->IsAdministrator())
	{
		return false;
	}

	return true;
}

void AWildOmissionPlayerController::Server_KickPlayer_Implementation(APlayerController* PlayerControllerToKick)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (GameMode == nullptr)
	{
		return;
	}

	AGameSession* GameSession = GameMode->GameSession.Get();
	if (GameSession == nullptr)
	{
		return;
	}

	if (PlayerControllerToKick == nullptr)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("Failed to kick player, couldn't get player controller."));
		return;
	}

	GameSession->KickPlayer(PlayerControllerToKick, FText::FromString(TEXT("Kicked by Server Administrator.")));
}

void AWildOmissionPlayerController::Server_Spawn_Implementation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	AWildOmissionGameMode* GameMode = Cast<AWildOmissionGameMode>(World->GetAuthGameMode());
	if (GameMode && (StoredPlayerSaveData.IsAlive || StoredPlayerSaveData.NewPlayer))
	{
		GameMode->SpawnHumanForController(this);
	}
	else
	{
		Client_ShowDeathMenu();
	}

	if (bIsStillLoading == true)
	{
		bIsStillLoading = false;
	}
}

void AWildOmissionPlayerController::Client_ShowDeathMenu_Implementation()
{
	UWorld* World = GetWorld();
	if (World == nullptr || !IsLocalController())
	{
		return;
	}

	// Make Sure to Close Pause Menu So it Doesn't Get Stuck on Screen!
	UWildOmissionGameInstance* WOGameInstance = UWildOmissionGameInstance::GetWildOmissionGameInstance(World);
	if (WOGameInstance)
	{
		UGameplayMenuWidget* OpenGameplayMenu = WOGameInstance->GetGameplayMenuWidget();
		if (OpenGameplayMenu)
		{
			OpenGameplayMenu->Teardown();
		}
	}
	
	UE_LOG(LogPlayerController, Verbose, TEXT("Bringing Up Death Menu."));

	UDeathMenuWidget* DeathMenu = CreateWidget<UDeathMenuWidget>(this, DeathMenuWidgetClass);
	if (DeathMenu == nullptr)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("Failed to show death menu."));
		return;
	}

	DeathMenu->Show();
}
