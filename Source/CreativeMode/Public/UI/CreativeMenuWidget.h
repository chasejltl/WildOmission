// Copyright Telephone Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Structs/ItemData.h"
#include "CreativeMenuWidget.generated.h"

UCLASS()
class CREATIVEMODE_API UCreativeMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UCreativeMenuWidget(const FObjectInitializer& ObjectInializer);
	
	virtual void NativeConstruct() override;

	UFUNCTION()
	void Refresh();

	void SetCategoryFilter(TEnumAsByte<EItemCategory> NewCategory);
	TEnumAsByte<EItemCategory> GetCategoryFilter() const;

	void SetSelectedItem(const FName& SelectedItemName);
	FName GetSelectedItem() const;

private:
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_All;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Items;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Resources;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Tools;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Building;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Weapons;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Ammo;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Medical;
	UPROPERTY(Meta = (BindWidget))
	class UItemCategoryButton* SortByButton_Clothing;

	UPROPERTY(Meta = (BindWidget))
	class UWrapBox* ItemsWrapBox;
	
	UPROPERTY(Meta = (BindWidget))
	class UTextBlock* SelectedItemNameTextBlock;

	UPROPERTY(Meta = (BindWidget))
	class UTextBlock* GiveStackButtonTextBlock;
	
	UPROPERTY(Meta = (BindWidget))
	class UImage* SelectedItemIconImage;

	UPROPERTY(Meta = (BindWidget))
	class UTextBlock* SelectedItemDescriptionTextBlock;

	UPROPERTY(Meta = (BindWidget))
	class UButton* GiveSingleButton;

	UPROPERTY(Meta = (BindWidget))
	class UButton* GiveStackButton;

	UPROPERTY(Meta = (BindWidget))
	class UButton* SwitchMenuButton;

	UPROPERTY(Meta = (BindWidget))
	class UTextBlock* SwitchMenuButtonTextBlock;

	UPROPERTY(Meta = (BindWidget))
	class UWidgetSwitcher* MenuSwitcher;

	UPROPERTY(Meta = (BindWidget))
	class UWidget* ItemMenu;

	UPROPERTY(Meta = (BindWidget))
	class UWidget* CommandsMenu;

	UPROPERTY()
	class UEditableTextBox* CustomQuantityTextBox;

	UPROPERTY()
	class UButton* GiveCustomQuantityButton;

	UPROPERTY()
	class UButton* DeleteSelectedItemButton;

	UPROPERTY()
	class UButton* DeleteAllSelectedButton;

	UPROPERTY()
	class UButton* ClearInventoryButton;

	UPROPERTY()
	TSubclassOf<class UItemIconWidget> ItemIconWidgetClass;
	
	TEnumAsByte<EItemCategory> CategoryFilter;

	FName SelectedItem;

	bool bAdminControlsInitialized = false;

	void RefreshItemList();

	void RefreshDetailsPanel();
	void ClearDetailsPanel();

	void BuildAdminControlsUI();
	class UButton* CreateAdminActionButton(class UPanelWidget* ParentPanel, const FString& LabelText, const FLinearColor& ButtonColor = FLinearColor(0.15f, 0.18f, 0.22f, 1.0f));
	int32 GetCustomQuantityInput() const;
	void ExecuteAdminConsoleCommand(const FString& Command);

	UFUNCTION()
	void SwitchMenu();

	UFUNCTION()
	void GiveItemSingle();

	UFUNCTION()
	void GiveItemStack();

	UFUNCTION()
	void GiveItemCustomQuantity();

	UFUNCTION()
	void DeleteSelectedItemCustomQuantity();

	UFUNCTION()
	void DeleteAllOfSelectedItem();

	UFUNCTION()
	void ClearEntireInventory();

	UFUNCTION()
	void OnClickToggleCreativeMode();

	UFUNCTION()
	void OnClickToggleFlyMode();

	UFUNCTION()
	void OnClickFullHeal();

	UFUNCTION()
	void OnClickGatherRate1x();

	UFUNCTION()
	void OnClickGatherRate2x();

	UFUNCTION()
	void OnClickGatherRate5x();

	UFUNCTION()
	void OnClickGatherRate10x();

	UFUNCTION()
	void OnClickGatherRate50x();

	UFUNCTION()
	void OnClickGatherRate100x();

	UFUNCTION()
	void OnClickEntKill();

	UFUNCTION()
	void OnClickRepairBase();

	UFUNCTION()
	void OnClickUpgradeBaseArmored();

	UFUNCTION()
	void OnClickClearWorldItems();

	UFUNCTION()
	void OnClickGiveStarterKit();

	UFUNCTION()
	void OnClickSpawnBaseStarter2x2();

	UFUNCTION()
	void OnClickSpawnBaseClanFortress();

	UFUNCTION()
	void OnClickSpawnBaseIndustrialCompound();
};
