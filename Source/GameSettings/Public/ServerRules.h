// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ServerRules.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerRuleChangedSignature, FName, RuleName);

/**
 * Server-level gameplay toggles and multipliers for Rust-style survival systems.
 * Configurable via DefaultGame.ini / Game.ini ([/Script/GameSettings.ServerRules]),
 * Blueprint / Admin UI panels, and console commands (wo.rules.list / wo.rules.set / wo.rules.save).
 */
UCLASS(Config = Game, DefaultConfig, BlueprintType)
class GAMESETTINGS_API UServerRules : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UServerRules();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Convenience static accessor that returns the active subsystem or the class default object. */
	UFUNCTION(BlueprintPure, Category = "Server Rules", meta = (WorldContext = "WorldContextObject"))
	static UServerRules* Get(const UObject* WorldContextObject = nullptr);

	/** Read-only fallback to CDO when no WorldContextObject is available. */
	static const UServerRules* GetDefault();

	/** Reset all rules to standard Rust-styled survival defaults. */
	UFUNCTION(BlueprintCallable, Category = "Server Rules")
	void ResetToDefaults();

	/** Apply a rule by key name (used by admin panel and console command wo.rules.set). */
	UFUNCTION(BlueprintCallable, Category = "Server Rules")
	bool SetRuleByString(const FString& Key, const FString& Value);

	/** Format all current rules as key=value lines for admin inspection. */
	UFUNCTION(BlueprintCallable, Category = "Server Rules")
	TArray<FString> GetRulesSummary() const;

	// --- Core Economy & Gathering (Phase 1) ---

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Economy")
	float ResourceGatherMultiplier;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Economy")
	bool bEnforceToolTierGates;

	// --- Building, Upkeep & Base Defense (Phase 1 & 2) ---

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bTwigDecayEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	float TwigDecayDamagePerMinute;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bUpkeepEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	float UpkeepIntervalHours;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	float UpkeepCostMultiplier;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bBuildingDecayEnabled;

	// --- Combat & Raiding (Phase 1-3) ---

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bPvPEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bOfflineRaidProtection;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bWorkbenchTierGatingEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bElectricityEnabled;

	// --- Environment, Weather & AI (Phase 0, 3 & 4) ---

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Environment")
	float DayNightSpeedMultiplier;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Environment")
	float TorchBurnTickSeconds;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bRadiationEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bTemperatureEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bStructuralWindDestructionEnabled;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bAdvancedAIEnabled;

	UPROPERTY(BlueprintAssignable, Category = "Server Rules")
	FOnServerRuleChangedSignature OnServerRuleChanged;
};
