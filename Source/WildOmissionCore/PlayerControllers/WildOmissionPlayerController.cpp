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
#include "ServerRules.h"
#include "WorldItem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DamageEvents.h"
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

void AWildOmissionPlayerController::AdminGive(const FString& ItemName, int32 Quantity)
{
	Server_GiveItem(FName(*ItemName), FMath::Max(1, Quantity));
}

void AWildOmissionPlayerController::AdminDeleteItem(const FString& ItemName, int32 Quantity)
{
	Server_AdminDeleteItem(FName(*ItemName), Quantity);
}

void AWildOmissionPlayerController::AdminClearInventory()
{
	Server_AdminClearInventory();
}

void AWildOmissionPlayerController::AdminToggleCreativeMode()
{
	Server_AdminToggleCreativeMode();
}

void AWildOmissionPlayerController::AdminToggleGodMode()
{
	Server_AdminToggleGodMode();
}

void AWildOmissionPlayerController::AdminToggleFly()
{
	Server_AdminToggleFly();
}

void AWildOmissionPlayerController::AdminSetSpeed(float SpeedMultiplier)
{
	Server_AdminSetSpeed(SpeedMultiplier);
}

void AWildOmissionPlayerController::AdminHeal()
{
	Server_AdminHeal();
}

void AWildOmissionPlayerController::AdminSetGatherRate(float Multiplier)
{
	Server_AdminSetGatherRate(Multiplier);
}

void AWildOmissionPlayerController::AdminEntKill()
{
	Server_AdminEntKill();
}

void AWildOmissionPlayerController::AdminRepairBase(float RadiusCm)
{
	Server_AdminRepairBase(RadiusCm);
}

void AWildOmissionPlayerController::AdminUpgradeBase(int32 TierIndex, float RadiusCm)
{
	Server_AdminUpgradeBase(TierIndex, RadiusCm);
}

void AWildOmissionPlayerController::AdminClearWorldItems()
{
	Server_AdminClearWorldItems();
}

void AWildOmissionPlayerController::AdminTeleportToLook()
{
	Server_AdminTeleportToLook();
}

void AWildOmissionPlayerController::AdminTeleportXYZ(float X, float Y, float Z)
{
	Server_AdminTeleportXYZ(X, Y, Z);
}

void AWildOmissionPlayerController::AdminSetTime(float Hour0To24)
{
	Server_AdminSetTime(Hour0To24);
}

void AWildOmissionPlayerController::AdminSetWeather(const FString& WeatherType)
{
	Server_AdminSetWeather(WeatherType);
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
			else if ((Cmd == TEXT("/delete") || Cmd == TEXT("/remove") || Cmd == TEXT("/del")) && Tokens.Num() >= 2)
			{
				const int32 Qty = (Tokens.Num() >= 3) ? FCString::Atoi(*Tokens[2]) : 1000000;
				Server_AdminDeleteItem(FName(*Tokens[1]), Qty);
				return;
			}
			else if (Cmd == TEXT("/clearinv") || Cmd == TEXT("/clear"))
			{
				Server_AdminClearInventory();
				return;
			}
			else if (Cmd == TEXT("/creative") || Cmd == TEXT("/freebuild"))
			{
				Server_AdminToggleCreativeMode();
				return;
			}
			else if (Cmd == TEXT("/god") || Cmd == TEXT("/godmode"))
			{
				Server_AdminToggleGodMode();
				return;
			}
			else if (Cmd == TEXT("/fly") || Cmd == TEXT("/noclip"))
			{
				Server_AdminToggleFly();
				return;
			}
			else if (Cmd == TEXT("/speed") && Tokens.Num() >= 2)
			{
				Server_AdminSetSpeed(FCString::Atof(*Tokens[1]));
				return;
			}
			else if (Cmd == TEXT("/heal") || Cmd == TEXT("/cure"))
			{
				Server_AdminHeal();
				return;
			}
			else if ((Cmd == TEXT("/gather") || Cmd == TEXT("/gatherrate")) && Tokens.Num() >= 2)
			{
				Server_AdminSetGatherRate(FCString::Atof(*Tokens[1]));
				return;
			}
			else if (Cmd == TEXT("/entkill") || Cmd == TEXT("/destroy"))
			{
				Server_AdminEntKill();
				return;
			}
			else if (Cmd == TEXT("/repairbase"))
			{
				const float Radius = (Tokens.Num() >= 2) ? FCString::Atof(*Tokens[1]) : 5000.0f;
				Server_AdminRepairBase(Radius);
				return;
			}
			else if (Cmd == TEXT("/upgradebase"))
			{
				const int32 TierIdx = (Tokens.Num() >= 2) ? FCString::Atoi(*Tokens[1]) : 4;
				const float Radius = (Tokens.Num() >= 3) ? FCString::Atof(*Tokens[2]) : 5000.0f;
				Server_AdminUpgradeBase(TierIdx, Radius);
				return;
			}
			else if (Cmd == TEXT("/clearground") || Cmd == TEXT("/clearitems"))
			{
				Server_AdminClearWorldItems();
				return;
			}
			else if (Cmd == TEXT("/tp") || Cmd == TEXT("/teleport"))
			{
				if (Tokens.Num() >= 4)
				{
					Server_AdminTeleportXYZ(FCString::Atof(*Tokens[1]), FCString::Atof(*Tokens[2]), FCString::Atof(*Tokens[3]));
				}
				else
				{
					Server_AdminTeleportToLook();
				}
				return;
			}
			else if (Cmd == TEXT("/tpxyz") && Tokens.Num() >= 4)
			{
				Server_AdminTeleportXYZ(FCString::Atof(*Tokens[1]), FCString::Atof(*Tokens[2]), FCString::Atof(*Tokens[3]));
				return;
			}
			else if (Cmd == TEXT("/time") && Tokens.Num() >= 2)
			{
				Server_AdminSetTime(FCString::Atof(*Tokens[1]));
				return;
			}
			else if (Cmd == TEXT("/weather") && Tokens.Num() >= 2)
			{
				Server_AdminSetWeather(Tokens[1]);
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

void AWildOmissionPlayerController::Server_AdminDeleteItem_Implementation(const FName& ItemName, int32 Quantity)
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

	const int32 QtyToRemove = (Quantity <= 0) ? 1000000 : Quantity;
	const int32 Removed = Inv->RemoveItemQuantity(ItemName, QtyToRemove);

	const FString Msg = FString::Printf(TEXT("[AdminPanel] Deleted %d x '%s' from inventory."), Removed, *ItemName.ToString());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Yellow, Msg);
	}
}

void AWildOmissionPlayerController::Server_AdminClearInventory_Implementation()
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

	Inv->ClearAllItems();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Yellow, TEXT("[AdminPanel] Cleared entire player inventory."));
	}
}

void AWildOmissionPlayerController::Server_AdminToggleCreativeMode_Implementation()
{
	const UServerRules* CurrentRules = UServerRules::Get(this);
	const bool bNewFreeBuild = (CurrentRules != nullptr) ? !CurrentRules->bCreativeFreeBuildEnabled : true;
	UServerRules::SetGlobalCreativeFreeBuild(this, bNewFreeBuild);

	GameModeIndex = bNewFreeBuild ? 1 : 0;

	if (AWildOmissionCharacter* Char = Cast<AWildOmissionCharacter>(GetPawn()))
	{
		Char->SetGodMode(bNewFreeBuild);
	}

	const FString Msg = FString::Printf(TEXT("[AdminPanel] Creative / Free-Build Mode %s! (Infinite building, free Hammer upgrades/repairs, free crafting, God Mode %s)"),
		bNewFreeBuild ? TEXT("ENABLED") : TEXT("DISABLED"),
		bNewFreeBuild ? TEXT("ON") : TEXT("OFF"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, bNewFreeBuild ? FColor::Green : FColor::Orange, Msg);
	}
	if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
	{
		ChatManager->SendMessage(PlayerState, Msg, EChatMessageType::STANDARD);
	}
}

void AWildOmissionPlayerController::Server_AdminToggleGodMode_Implementation()
{
	AWildOmissionCharacter* Char = Cast<AWildOmissionCharacter>(GetPawn());
	if (Char == nullptr)
	{
		return;
	}

	UVitalsComponent* Vitals = Char->GetVitalsComponent();
	const bool bNewGod = (Vitals != nullptr) ? !Vitals->IsGodMode() : true;
	Char->SetGodMode(bNewGod);

	const FString Msg = FString::Printf(TEXT("[AdminPanel] God Mode %s!"), bNewGod ? TEXT("ENABLED (Invincible + Full Vitals)") : TEXT("DISABLED"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, bNewGod ? FColor::Green : FColor::Orange, Msg);
	}
	if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
	{
		ChatManager->SendMessage(PlayerState, Msg, EChatMessageType::STANDARD);
	}
}

void AWildOmissionPlayerController::Server_AdminToggleFly_Implementation()
{
	AWildOmissionCharacter* Char = Cast<AWildOmissionCharacter>(GetPawn());
	if (Char == nullptr)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		return;
	}

	const bool bNewFlying = !MoveComp->IsFlying();
	MoveComp->MaxFlySpeed = FMath::Max(MoveComp->MaxFlySpeed, 1800.0f);
	MoveComp->SetMovementMode(bNewFlying ? EMovementMode::MOVE_Flying : EMovementMode::MOVE_Walking);

	const FString Msg = FString::Printf(TEXT("[AdminPanel] Fly Mode %s! (Hold Space to ascend, Ctrl to descend)"), bNewFlying ? TEXT("ENABLED") : TEXT("DISABLED"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, bNewFlying ? FColor::Cyan : FColor::Orange, Msg);
	}
}

void AWildOmissionPlayerController::Server_AdminSetSpeed_Implementation(float SpeedMultiplier)
{
	AWildOmissionCharacter* Char = Cast<AWildOmissionCharacter>(GetPawn());
	if (Char == nullptr)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
	if (MoveComp == nullptr)
	{
		return;
	}

	const float ClampedMult = FMath::Clamp(SpeedMultiplier, 0.25f, 20.0f);
	MoveComp->MaxWalkSpeed = 600.0f * ClampedMult;
	MoveComp->MaxFlySpeed = 1200.0f * ClampedMult;
	MoveComp->MaxSwimSpeed = 400.0f * ClampedMult;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Cyan,
			FString::Printf(TEXT("[AdminPanel] Movement & Fly Speed set to %.1fx!"), ClampedMult));
	}
}

void AWildOmissionPlayerController::Server_AdminHeal_Implementation()
{
	APawn* OurPawn = GetPawn();
	if (OurPawn == nullptr)
	{
		return;
	}

	UVitalsComponent* Vitals = OurPawn->FindComponentByClass<UVitalsComponent>();
	if (Vitals == nullptr)
	{
		return;
	}

	Vitals->SetHealth(Vitals->GetMaxHealth());
	Vitals->SetThirst(Vitals->GetMaxThirst());
	Vitals->SetHunger(Vitals->GetMaxHunger());
	Vitals->AddRadiationExposure(-1000.0f);
	Vitals->AddBleeding(-1000.0f);
	Vitals->ApplyMedicalItem(TEXT("Bandage"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Green,
			TEXT("[AdminPanel] Restored 100% Health, Thirst, Hunger & cured all Bleeding/Radiation!"));
	}
}

void AWildOmissionPlayerController::Server_AdminSetGatherRate_Implementation(float Multiplier)
{
	const float Clamped = FMath::Clamp(Multiplier, 0.1f, 1000.0f);
	UServerRules::SetGlobalGatherMultiplier(this, Clamped);

	const FString Msg = FString::Printf(TEXT("[AdminPanel] Server Gathering Multiplier set to %.1fx! (Applies immediately to all trees, ore nodes & ground pickups)"), Clamped);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8.0f, FColor::Green, Msg);
	}
	if (AGameChatManager* ChatManager = AGameChatManager::GetGameChatManager())
	{
		ChatManager->SendMessage(PlayerState, Msg, EChatMessageType::STANDARD);
	}
}

void AWildOmissionPlayerController::Server_AdminEntKill_Implementation()
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	FVector Start = OurPawn->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	if (UCameraComponent* Cam = OurPawn->FindComponentByClass<UCameraComponent>())
	{
		Start = Cam->GetComponentLocation();
	}
	const FVector End = Start + GetControlRotation().Vector() * 10000.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OurPawn);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) || !IsValid(Hit.GetActor()))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Orange, TEXT("[AdminPanel] ent kill: No target actor found under crosshair."));
		}
		return;
	}

	AActor* TargetActor = Hit.GetActor();
	if (TargetActor->IsA<AChunk>() || TargetActor == OurPawn)
	{
		return;
	}

	const FString TargetName = TargetActor->GetActorNameOrLabel();
	if (ADeployable* Dep = Cast<ADeployable>(TargetActor))
	{
		Dep->Multi_PlayDestructionEffects();
	}
	TargetActor->Destroy();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Red,
			FString::Printf(TEXT("[AdminPanel] ent kill: Destroyed '%s'!"), *TargetName));
	}
}

void AWildOmissionPlayerController::Server_AdminRepairBase_Implementation(float RadiusCm)
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	const FVector Origin = OurPawn->GetActorLocation();
	const float RadiusSq = FMath::Square(FMath::Max(100.0f, RadiusCm));
	int32 RepairedCount = 0;

	for (TActorIterator<ADeployable> It(World); It; ++It)
	{
		ADeployable* Dep = *It;
		if (!IsValid(Dep) || FVector::DistSquared(Origin, Dep->GetActorLocation()) > RadiusSq)
		{
			continue;
		}

		if (Dep->GetCurrentDurability() < Dep->GetMaxDurability())
		{
			FHitResult DummyHit;
			FPointDamageEvent HealEvent(-100000.0f, DummyHit, FVector::UpVector, nullptr);
			Dep->TakeDamage(-100000.0f, HealEvent, this, OurPawn);
			RepairedCount++;
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Green,
			FString::Printf(TEXT("[AdminPanel] Repaired %d structures within %.0fm to 100%% durability!"), RepairedCount, RadiusCm / 100.0f));
	}
}

void AWildOmissionPlayerController::Server_AdminUpgradeBase_Implementation(int32 TierIndex, float RadiusCm)
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	const FVector Origin = OurPawn->GetActorLocation();
	const float RadiusSq = FMath::Square(FMath::Max(100.0f, RadiusCm));
	const EBuildingTier TargetTier = static_cast<EBuildingTier>(FMath::Clamp(TierIndex, 0, 4));
	int32 UpgradedCount = 0;

	for (TActorIterator<ABuildingBlock> It(World); It; ++It)
	{
		ABuildingBlock* Block = *It;
		if (!IsValid(Block) || FVector::DistSquared(Origin, Block->GetActorLocation()) > RadiusSq)
		{
			continue;
		}

		Block->UpgradeToTier(TargetTier);
		UpgradedCount++;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Cyan,
			FString::Printf(TEXT("[AdminPanel] Upgraded %d building blocks within %.0fm to Tier %d!"), UpgradedCount, RadiusCm / 100.0f, TierIndex));
	}
}

void AWildOmissionPlayerController::Server_AdminClearWorldItems_Implementation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	int32 Cleared = 0;
	for (TActorIterator<AWorldItem> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			It->Destroy();
			Cleared++;
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.0f, FColor::Yellow,
			FString::Printf(TEXT("[AdminPanel] Cleared %d dropped world items."), Cleared));
	}
}

void AWildOmissionPlayerController::Server_AdminTeleportToLook_Implementation()
{
	UWorld* World = GetWorld();
	APawn* OurPawn = GetPawn();
	if (World == nullptr || OurPawn == nullptr)
	{
		return;
	}

	FVector Start = OurPawn->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	if (UCameraComponent* Cam = OurPawn->FindComponentByClass<UCameraComponent>())
	{
		Start = Cam->GetComponentLocation();
	}
	const FVector End = Start + GetControlRotation().Vector() * 50000.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OurPawn);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		OurPawn->SetActorLocation(Hit.ImpactPoint + FVector(0.0f, 0.0f, 110.0f));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("[AdminPanel] Teleported to (%.0f, %.0f, %.0f)"), Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z));
		}
	}
}

void AWildOmissionPlayerController::Server_AdminTeleportXYZ_Implementation(float X, float Y, float Z)
{
	if (APawn* OurPawn = GetPawn())
	{
		OurPawn->SetActorLocation(FVector(X, Y, Z));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("[AdminPanel] Teleported to (%.0f, %.0f, %.0f)"), X, Y, Z));
		}
	}
}

void AWildOmissionPlayerController::Server_AdminSetTime_Implementation(float Hour0To24)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (AWildOmissionGameMode* GM = Cast<AWildOmissionGameMode>(World->GetAuthGameMode()))
	{
		const float Normalized = FMath::Clamp(Hour0To24 / 24.0f, 0.0f, 1.0f);
		GM->SetTime(Normalized);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("[AdminPanel] Time of Day set to %.1f:00 (normalized %.2f)"), Hour0To24, Normalized));
		}
	}
}

void AWildOmissionPlayerController::Server_AdminSetWeather_Implementation(const FString& WeatherType)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (AWildOmissionGameMode* GM = Cast<AWildOmissionGameMode>(World->GetAuthGameMode()))
	{
		FString CleanWeather = WeatherType.TrimStartAndEnd();
		if (CleanWeather.Equals(TEXT("clear"), ESearchCase::IgnoreCase))
		{
			CleanWeather = TEXT("Clear");
		}
		else if (CleanWeather.Equals(TEXT("rain"), ESearchCase::IgnoreCase))
		{
			CleanWeather = TEXT("Rain");
		}
		else if (CleanWeather.Equals(TEXT("tornado"), ESearchCase::IgnoreCase) || CleanWeather.Equals(TEXT("storm"), ESearchCase::IgnoreCase))
		{
			CleanWeather = TEXT("Tornado");
		}
		GM->Weather(CleanWeather);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("[AdminPanel] Weather set to '%s'"), *CleanWeather));
		}
	}
}
