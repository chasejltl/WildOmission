// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/SavablePlayer.h"
#include "Interfaces/PlayerControllerMessageSender.h"
#include "Interfaces/BedController.h"
#include "Interfaces/MapInterface.h"
#include "Interfaces/ServerAdministrator.h"
#include "WildOmissionPlayerController.generated.h"

UCLASS()
class WILDOMISSIONCORE_API AWildOmissionPlayerController 
	: public APlayerController, public ISavablePlayer, public IPlayerControllerMessageSender, public IBedController, public IMapInterface, public IServerAdministrator
{
	GENERATED_BODY()

public:
	AWildOmissionPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Begin ISavablePlayer Implementation
	virtual FPlayerSaveData SavePlayer() override;
	virtual void LoadPlayerSave(const FPlayerSaveData& SaveData) override;
	virtual bool IsStillLoading() const override;
	virtual FString GetUniqueID() const override;
	virtual bool IsHost() const override;
	// End ISavablePlayer Implementation

	// Begin IPlayerControllerMessageSender Implementation
	virtual void SendMessage(APlayerState* Sender, const FString& Message) override;
	// End IPlayerControllerMessageSender Implementation

	// Begin IBedController Implementation
	virtual void Sleep() override;
	virtual void SetCurrentBed(const int32& InBedUniqueID, const FIntVector2& InBedChunkLocation) override;
	virtual int32 GetBedUniqueID() const override;
	virtual FIntVector2 GetBedChunkLocation() const override;
	// End IBedController Implementation

	// Begin IServerAdministrator Implementation
	virtual void SetAdministrator(bool InAdministrator) override;
	virtual bool IsAdministrator() const override;
	virtual void KickPlayer(APlayerController* PlayerControllerToKick) override;
	// End IServerAdministrator Implementation

	UFUNCTION(BlueprintCallable)
	void SetGameModeIndex(const uint8 & NewGameModeIndex);
	UFUNCTION(BlueprintCallable)
	uint8 GetGameModeIndex() const;

	UFUNCTION(BlueprintCallable)
	void SetInCheatedWorld(bool CheatedWorld);
	UFUNCTION(BlueprintCallable)
	bool IsInCheatedWorld() const;


	UFUNCTION(BlueprintCallable)
	bool IsSurvivalMode() const;
	UFUNCTION(BlueprintCallable)
	bool IsCreativeMode() const;

	void Save();

	UFUNCTION(BlueprintCallable)
	bool IsEditorPlayer() const;

	UFUNCTION(Client, Reliable)
	void Client_ShowDeathMenu();

	UFUNCTION(Server, Reliable)
	void Server_Spawn();

	UFUNCTION()
	void OnPlayerDeath(const FVector& DeathLocation);

	//*****************************
	// Console, Admin Control Panel & Python Base Builder functions
	UFUNCTION(Exec)
	void Kill();

	UFUNCTION(Exec)
	void LogLocalInventoryContents();

	/** Spawns a base from a Python-generated JSON blueprint in Tools/BaseBuilder/Blueprints/<BlueprintName>.json or Saved/BaseBlueprints/<BlueprintName>.json */
	UFUNCTION(Exec, BlueprintCallable, Category = "BaseBuilder")
	void BuildBase(const FString& BlueprintName);

	/** Exports all deployables/building blocks within RadiusCm of the player to Saved/BaseBlueprints/<BlueprintName>.json */
	UFUNCTION(Exec, BlueprintCallable, Category = "BaseBuilder")
	void ExportBase(const FString& BlueprintName, float RadiusCm = 3500.0f);

	/** Gives any item by ID and quantity to the player's inventory */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void GiveItem(const FString& ItemName, int32 Quantity = 1);

	/** Alias for GiveItem matching Rust's inventory.give */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminGive(const FString& ItemName, int32 Quantity = 1);

	/** Deletes a specific quantity of an item from the player's inventory (or all if Quantity <= 0) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminDeleteItem(const FString& ItemName, int32 Quantity = 1);

	/** Clears all items from the player's inventory */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminClearInventory();

	/** Toggles Creative / Free-Build Mode (build, upgrade, and craft without consuming resources) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminToggleCreativeMode();

	/** Toggles God Mode (invincibility, infinite hunger/thirst, no radiation or bleeding) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminToggleGodMode();

	/** Toggles Fly / Noclip movement mode */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminToggleFly();

	/** Sets movement/flying speed multiplier (default 1.0, e.g. 2.0, 5.0) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminSetSpeed(float SpeedMultiplier = 2.0f);

	/** Fully heals Health, Hunger, Thirst, and cures Bleeding, Radiation, and Temperature */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminHeal();

	/** Sets the server-wide Resource Gathering Multiplier (e.g. 1x, 2x, 5x, 10x, 50x, 100x) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminSetGatherRate(float Multiplier = 2.0f);

	/** Destroys the structure, deployable, resource node, or NPC under the player's crosshair (Rust ent kill) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminEntKill();

	/** Repairs all deployables and building blocks within RadiusCm (default 5000cm = 50m) to 100% durability */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminRepairBase(float RadiusCm = 5000.0f);

	/** Upgrades all building blocks within RadiusCm to TierIndex (1=Wood, 2=Stone, 3=SheetMetal, 4=Armored) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminUpgradeBase(int32 TierIndex = 4, float RadiusCm = 5000.0f);

	/** Clears all dropped AWorldItem actors in the world */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminClearWorldItems();

	/** Teleports the player to the point under their crosshair */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminTeleportToLook();

	/** Teleports the player to exact world coordinates */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminTeleportXYZ(float X, float Y, float Z);

	/** Sets the time of day in hours (0.0 to 24.0, e.g. 12.0 = Noon) */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminSetTime(float Hour0To24 = 12.0f);

	/** Sets the weather ("Clear", "Rain", or "Tornado") */
	UFUNCTION(Exec, BlueprintCallable, Category = "AdminPanel")
	void AdminSetWeather(const FString& WeatherType = TEXT("Clear"));

	/** Gives a starter kit of weapons, ammo, storage barrels, electrical, lighting, and building materials */
	UFUNCTION(Exec, BlueprintCallable, Category = "BaseBuilder")
	void GiveStarterKit();

	virtual void SetSavedLocationToCurrentLocation() override;
	virtual void SetLastDeathLocation(const FVector& DeathLocation) override;
	virtual FVector GetCurrentCoordinateLocation() override;
	virtual FVector GetSavedLocation() override;
	virtual FVector GetLastDeathLocation() override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* aPawn) override;

private:
	UPROPERTY(VisibleAnywhere)
	class UMusicPlayerComponent* MusicPlayerComponent;

	UPROPERTY(VisibleAnywhere)
	class UWOInGameAchievementsComponent* AchievementsComponent;

	UPROPERTY()
	TSubclassOf<class UDeathMenuWidget> DeathMenuWidgetClass;

	UPROPERTY(VisibleAnywhere, Replicated)
	uint8 GameModeIndex;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool InCheatedWorld;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 BedUniqueID;

	UPROPERTY(VisibleAnywhere)
	FIntVector2 BedChunkLocation;

	UPROPERTY(Replicated)
	FVector LastDeathLocation;

	UPROPERTY(Replicated)
	FVector SavedLocation;

	UPROPERTY(Replicated)
	bool Administrator;

	UPROPERTY()
	class AChunkInvokerActor* TempChunkInvoker;

	UPROPERTY(Replicated)
	FIntVector2 SpawnChunk;

	FTimerHandle CheckSpawnChunkValidTimerHandle;

	void SetupPlayerOnServer();
	void StartLoading();
	void SetupMusicPlayerComponent();
	void SetupLocalAsHost();
	void SetupLocalAsClient();

	UFUNCTION()
	void CheckSpawnChunkValid();
	
	void StopLoading();

	// Used by the server to figure out if a player is still loading
	// into the server, it will wait until the player if fully loaded before attempting
	// to load their data from the save file.
	bool bIsStillLoading;
	FPlayerSaveData StoredPlayerSaveData;

	UFUNCTION(Server, Reliable)
	void Server_SendMessage(APlayerState* Sender, const FString& Message);

	UFUNCTION(Server, Reliable)
	void Server_AddToPendingSaves();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_KickPlayer(APlayerController* PlayerControllerToKick);

	UFUNCTION(Server, Reliable)
	void Server_KillThisPlayer();

	UFUNCTION(Server, Reliable)
	void Server_SetSavedLocationToCurrentLocation();

	UFUNCTION(Server, Reliable)
	void Server_BuildBase(const FString& BlueprintName);

	UFUNCTION(Server, Reliable)
	void Server_ExportBase(const FString& BlueprintName, float RadiusCm);

	UFUNCTION(Server, Reliable)
	void Server_GiveItem(const FName& ItemName, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void Server_GiveStarterKit();

	UFUNCTION(Server, Reliable)
	void Server_AdminDeleteItem(const FName& ItemName, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void Server_AdminClearInventory();

	UFUNCTION(Server, Reliable)
	void Server_AdminToggleCreativeMode();

	UFUNCTION(Server, Reliable)
	void Server_AdminToggleGodMode();

	UFUNCTION(Server, Reliable)
	void Server_AdminToggleFly();

	UFUNCTION(Server, Reliable)
	void Server_AdminSetSpeed(float SpeedMultiplier);

	UFUNCTION(Server, Reliable)
	void Server_AdminHeal();

	UFUNCTION(Server, Reliable)
	void Server_AdminSetGatherRate(float Multiplier);

	UFUNCTION(Server, Reliable)
	void Server_AdminEntKill();

	UFUNCTION(Server, Reliable)
	void Server_AdminRepairBase(float RadiusCm);

	UFUNCTION(Server, Reliable)
	void Server_AdminUpgradeBase(int32 TierIndex, float RadiusCm);

	UFUNCTION(Server, Reliable)
	void Server_AdminClearWorldItems();

	UFUNCTION(Server, Reliable)
	void Server_AdminTeleportToLook();

	UFUNCTION(Server, Reliable)
	void Server_AdminTeleportXYZ(float X, float Y, float Z);

	UFUNCTION(Server, Reliable)
	void Server_AdminSetTime(float Hour0To24);

	UFUNCTION(Server, Reliable)
	void Server_AdminSetWeather(const FString& WeatherType);
};
