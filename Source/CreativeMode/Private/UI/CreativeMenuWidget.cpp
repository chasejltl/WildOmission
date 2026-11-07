// Copyright Telephone Studios. All Rights Reserved.


#include "UI/CreativeMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WrapBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "ItemCategoryButton.h"
#include "ItemIconWidget.h"
#include "Components/InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

UCreativeMenuWidget::UCreativeMenuWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UItemIconWidget> ItemIconWidgetBlueprint(TEXT("/Game/CreativeMode/UI/WBP_ItemIcon"));

	if (ItemIconWidgetBlueprint.Class == nullptr)
	{
		return;
	}

	ItemIconWidgetClass = ItemIconWidgetBlueprint.Class;
}

void UCreativeMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	CategoryFilter = EItemCategory::All;

	SortByButton_All->Setup(this, EItemCategory::All);
	SortByButton_Items->Setup(this, EItemCategory::Item);
	SortByButton_Resources->Setup(this, EItemCategory::Resource);
	SortByButton_Tools->Setup(this, EItemCategory::Tool);
	SortByButton_Building->Setup(this, EItemCategory::Building);
	SortByButton_Weapons->Setup(this, EItemCategory::Weapon);
	SortByButton_Ammo->Setup(this, EItemCategory::Ammo);
	SortByButton_Medical->Setup(this, EItemCategory::Medical);
	SortByButton_Clothing->Setup(this, EItemCategory::Clothing);

	SwitchMenuButton->OnClicked.RemoveAll(this);
	SwitchMenuButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::SwitchMenu);

	GiveSingleButton->OnClicked.RemoveAll(this);
	GiveSingleButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::GiveItemSingle);
	GiveStackButton->OnClicked.RemoveAll(this);
	GiveStackButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::GiveItemStack);

	BuildAdminControlsUI();
	
	SelectedItem = TEXT("");
	Refresh();
}

void UCreativeMenuWidget::BuildAdminControlsUI()
{
	if (bAdminControlsInitialized || WidgetTree == nullptr)
	{
		return;
	}
	bAdminControlsInitialized = true;

	// 1. Add Custom Quantity Input, Give Custom Qty, Delete Selected Item, and Clear Inventory to the Item Spawner Details Panel
	if (GiveStackButton != nullptr)
	{
		if (UPanelWidget* DetailsPanel = GiveStackButton->GetParent())
		{
			CustomQuantityTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AdminCustomQuantityTextBox"));
			if (CustomQuantityTextBox != nullptr)
			{
				CustomQuantityTextBox->SetText(FText::FromString(TEXT("100")));
				CustomQuantityTextBox->SetHintText(FText::FromString(TEXT("Custom Qty (1-100000)")));
				DetailsPanel->AddChild(CustomQuantityTextBox);
			}

			GiveCustomQuantityButton = CreateAdminActionButton(DetailsPanel, TEXT("Give Custom Quantity"), FLinearColor(0.12f, 0.42f, 0.20f, 1.0f));
			if (GiveCustomQuantityButton != nullptr)
			{
				GiveCustomQuantityButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::GiveItemCustomQuantity);
			}

			DeleteSelectedItemButton = CreateAdminActionButton(DetailsPanel, TEXT("Delete Custom Qty"), FLinearColor(0.48f, 0.22f, 0.12f, 1.0f));
			if (DeleteSelectedItemButton != nullptr)
			{
				DeleteSelectedItemButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::DeleteSelectedItemCustomQuantity);
			}

			DeleteAllSelectedButton = CreateAdminActionButton(DetailsPanel, TEXT("Delete All of Selected"), FLinearColor(0.55f, 0.14f, 0.14f, 1.0f));
			if (DeleteAllSelectedButton != nullptr)
			{
				DeleteAllSelectedButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::DeleteAllOfSelectedItem);
			}

			ClearInventoryButton = CreateAdminActionButton(DetailsPanel, TEXT("Clear Entire Inventory"), FLinearColor(0.40f, 0.10f, 0.10f, 1.0f));
			if (ClearInventoryButton != nullptr)
			{
				ClearInventoryButton->OnClicked.AddDynamic(this, &UCreativeMenuWidget::ClearEntireInventory);
			}
		}
	}

	// 2. Add Rust-Style Admin Control Panel buttons to CommandsWrapBox inside CommandsMenu
	UPanelWidget* CommandsPanel = WidgetTree->FindWidget<UPanelWidget>(TEXT("CommandsWrapBox"));
	if (CommandsPanel == nullptr)
	{
		CommandsPanel = Cast<UPanelWidget>(CommandsMenu);
	}

	if (CommandsPanel != nullptr)
	{
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Toggle Creative / Free-Build"), FLinearColor(0.14f, 0.45f, 0.24f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickToggleCreativeMode);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Toggle Fly / Noclip"), FLinearColor(0.14f, 0.35f, 0.50f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickToggleFlyMode);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Full Heal & Cure Vitals"), FLinearColor(0.16f, 0.45f, 0.32f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickFullHeal);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 1x (Vanilla)"), FLinearColor(0.22f, 0.24f, 0.28f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate1x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 2x"), FLinearColor(0.20f, 0.30f, 0.38f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate2x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 5x"), FLinearColor(0.20f, 0.34f, 0.44f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate5x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 10x"), FLinearColor(0.18f, 0.38f, 0.52f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate10x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 50x"), FLinearColor(0.32f, 0.28f, 0.52f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate50x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Gather Rate: 100x"), FLinearColor(0.45f, 0.24f, 0.52f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGatherRate100x);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Delete Target (ent kill)"), FLinearColor(0.55f, 0.16f, 0.16f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickEntKill);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Repair Base (50m)"), FLinearColor(0.18f, 0.42f, 0.28f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickRepairBase);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Upgrade Base Armored (50m)"), FLinearColor(0.22f, 0.36f, 0.52f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickUpgradeBaseArmored);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Clear Dropped World Items"), FLinearColor(0.48f, 0.24f, 0.14f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickClearWorldItems);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Give Full Combat & Build Kit"), FLinearColor(0.44f, 0.34f, 0.12f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickGiveStarterKit);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Spawn Base: Starter 2x2"), FLinearColor(0.25f, 0.32f, 0.20f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickSpawnBaseStarter2x2);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Spawn Base: Clan Fortress"), FLinearColor(0.25f, 0.32f, 0.20f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickSpawnBaseClanFortress);
		}
		if (UButton* Btn = CreateAdminActionButton(CommandsPanel, TEXT("Spawn Base: Industrial Compound"), FLinearColor(0.25f, 0.32f, 0.20f, 1.0f)))
		{
			Btn->OnClicked.AddDynamic(this, &UCreativeMenuWidget::OnClickSpawnBaseIndustrialCompound);
		}
	}
}

UButton* UCreativeMenuWidget::CreateAdminActionButton(UPanelWidget* ParentPanel, const FString& LabelText, const FLinearColor& ButtonColor)
{
	if (ParentPanel == nullptr || WidgetTree == nullptr)
	{
		return nullptr;
	}

	UButton* NewButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	if (NewButton == nullptr)
	{
		return nullptr;
	}
	NewButton->SetBackgroundColor(ButtonColor);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (Label != nullptr)
	{
		Label->SetText(FText::FromString(LabelText));
		FSlateFontInfo FontInfo = Label->GetFont();
		FontInfo.Size = 11;
		Label->SetFont(FontInfo);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		NewButton->AddChild(Label);
	}

	ParentPanel->AddChild(NewButton);
	return NewButton;
}

int32 UCreativeMenuWidget::GetCustomQuantityInput() const
{
	if (CustomQuantityTextBox == nullptr)
	{
		return 100;
	}

	const FString RawString = CustomQuantityTextBox->GetText().ToString().TrimStartAndEnd();
	const int32 Parsed = FCString::Atoi(*RawString);
	return FMath::Clamp(Parsed > 0 ? Parsed : 1, 1, 100000);
}

void UCreativeMenuWidget::ExecuteAdminConsoleCommand(const FString& Command)
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ConsoleCommand(Command, true);
	}
}

void UCreativeMenuWidget::Refresh()
{
	RefreshItemList();
	RefreshDetailsPanel();
}

void UCreativeMenuWidget::SetCategoryFilter(TEnumAsByte<EItemCategory> NewCategory)
{
	CategoryFilter = NewCategory;
	SelectedItem = FName();
	RefreshItemList();
	RefreshDetailsPanel();
}

TEnumAsByte<EItemCategory> UCreativeMenuWidget::GetCategoryFilter() const
{
	return CategoryFilter;
}

void UCreativeMenuWidget::SetSelectedItem(const FName& SelectedItemName)
{
	SelectedItem = SelectedItemName;
	RefreshDetailsPanel();
}

FName UCreativeMenuWidget::GetSelectedItem() const
{
	return SelectedItem;
}

void UCreativeMenuWidget::RefreshItemList()
{
	TArray<FName> ItemList;
	for (const FName& ItemID : UInventoryComponent::GetAllItems())
	{
		FItemData* ItemData = UInventoryComponent::GetItemData(ItemID);
		if (ItemData == nullptr || (CategoryFilter != EItemCategory::All && ItemData->Category != CategoryFilter) || ItemData->HiddenInCreativeMode)
		{
			continue;
		}
		
		ItemList.Add(ItemID);
	}

	if (SelectedItem == FName() && ItemList.Num() > 0)
	{
		SelectedItem = ItemList[0];
	}

	ItemsWrapBox->ClearChildren();
	for (int32 i = 0; i < ItemList.Num(); i++)
	{
		const FName& ItemID = ItemList[i];

		UItemIconWidget* NewItemIcon = CreateWidget<UItemIconWidget>(this, ItemIconWidgetClass);
		if (NewItemIcon == nullptr)
		{
			continue;
		}

		NewItemIcon->Setup(this, ItemID);
		ItemsWrapBox->AddChild(NewItemIcon);
	}
}

void UCreativeMenuWidget::RefreshDetailsPanel()
{
	if (SelectedItem == FName())
	{
		ClearDetailsPanel();
		return;
	}

	FItemData* SelectedItemData = UInventoryComponent::GetItemData(SelectedItem);
	if (SelectedItemData == nullptr)
	{
		return;
	}

	SelectedItemNameTextBlock->SetText(FText::FromString(SelectedItemData->DisplayName));
	
	SelectedItemDescriptionTextBlock->SetText(FText::FromString(SelectedItemData->Description));

	SelectedItemIconImage->SetBrushFromMaterial(SelectedItemData->Thumbnail);
	SelectedItemIconImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

	const FString StackButtonString = FString::Printf(TEXT("Give Stack (x%i)"), SelectedItemData->StackSize);
	GiveStackButtonTextBlock->SetText(FText::FromString(StackButtonString));
}

void UCreativeMenuWidget::ClearDetailsPanel()
{
	SelectedItemNameTextBlock->SetText(FText::FromString(FString()));
	
	SelectedItemDescriptionTextBlock->SetText(FText::FromString(FString()));

	SelectedItemIconImage->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
}

void UCreativeMenuWidget::SwitchMenu()
{
	if (MenuSwitcher->GetActiveWidget() == ItemMenu)
	{
		// switch to commands menu
		MenuSwitcher->SetActiveWidget(CommandsMenu);
		SwitchMenuButtonTextBlock->SetText(FText::FromString(TEXT("Items")));
	}
	else
	{
		// switch to item menu
		MenuSwitcher->SetActiveWidget(ItemMenu);
		SwitchMenuButtonTextBlock->SetText(FText::FromString(TEXT("Commands")));
	}
}

void UCreativeMenuWidget::GiveItemSingle()
{
	if (!GetOwningPlayerPawn() || SelectedItem.IsNone())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	FInventoryItem ItemToAdd;
	ItemToAdd.Name = SelectedItem;
	ItemToAdd.Quantity = 1;

	OwnerInventoryComponent->AddItem(ItemToAdd);
}

void UCreativeMenuWidget::GiveItemStack()
{
	if (!GetOwningPlayerPawn() || SelectedItem.IsNone())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	FItemData* SelectedItemData = UInventoryComponent::GetItemData(SelectedItem);
	if (SelectedItemData == nullptr)
	{
		return;
	}

	FInventoryItem ItemToAdd;
	ItemToAdd.Name = SelectedItem;
	ItemToAdd.Quantity = SelectedItemData->StackSize;

	OwnerInventoryComponent->AddItem(ItemToAdd);
}

void UCreativeMenuWidget::GiveItemCustomQuantity()
{
	if (!GetOwningPlayerPawn() || SelectedItem.IsNone())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	FInventoryItem ItemToAdd;
	ItemToAdd.Name = SelectedItem;
	ItemToAdd.Quantity = GetCustomQuantityInput();

	OwnerInventoryComponent->AddItem(ItemToAdd);
}

void UCreativeMenuWidget::DeleteSelectedItemCustomQuantity()
{
	if (!GetOwningPlayerPawn() || SelectedItem.IsNone())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	OwnerInventoryComponent->RemoveItemQuantity(SelectedItem, GetCustomQuantityInput());
}

void UCreativeMenuWidget::DeleteAllOfSelectedItem()
{
	if (!GetOwningPlayerPawn() || SelectedItem.IsNone())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	OwnerInventoryComponent->RemoveItemQuantity(SelectedItem, 1000000);
}

void UCreativeMenuWidget::ClearEntireInventory()
{
	if (!GetOwningPlayerPawn())
	{
		return;
	}

	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UInventoryComponent>();
	if (OwnerInventoryComponent == nullptr)
	{
		return;
	}

	OwnerInventoryComponent->ClearAllItems();
}

void UCreativeMenuWidget::OnClickToggleCreativeMode()
{
	ExecuteAdminConsoleCommand(TEXT("AdminToggleCreativeMode"));
}

void UCreativeMenuWidget::OnClickToggleFlyMode()
{
	ExecuteAdminConsoleCommand(TEXT("AdminToggleFly"));
}

void UCreativeMenuWidget::OnClickFullHeal()
{
	ExecuteAdminConsoleCommand(TEXT("AdminHeal"));
}

void UCreativeMenuWidget::OnClickGatherRate1x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 1"));
}

void UCreativeMenuWidget::OnClickGatherRate2x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 2"));
}

void UCreativeMenuWidget::OnClickGatherRate5x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 5"));
}

void UCreativeMenuWidget::OnClickGatherRate10x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 10"));
}

void UCreativeMenuWidget::OnClickGatherRate50x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 50"));
}

void UCreativeMenuWidget::OnClickGatherRate100x()
{
	ExecuteAdminConsoleCommand(TEXT("AdminSetGatherRate 100"));
}

void UCreativeMenuWidget::OnClickEntKill()
{
	ExecuteAdminConsoleCommand(TEXT("AdminEntKill"));
}

void UCreativeMenuWidget::OnClickRepairBase()
{
	ExecuteAdminConsoleCommand(TEXT("AdminRepairBase 5000"));
}

void UCreativeMenuWidget::OnClickUpgradeBaseArmored()
{
	ExecuteAdminConsoleCommand(TEXT("AdminUpgradeBase 4 5000"));
}

void UCreativeMenuWidget::OnClickClearWorldItems()
{
	ExecuteAdminConsoleCommand(TEXT("AdminClearWorldItems"));
}

void UCreativeMenuWidget::OnClickGiveStarterKit()
{
	ExecuteAdminConsoleCommand(TEXT("GiveStarterKit"));
}

void UCreativeMenuWidget::OnClickSpawnBaseStarter2x2()
{
	ExecuteAdminConsoleCommand(TEXT("BuildBase starter_2x2"));
}

void UCreativeMenuWidget::OnClickSpawnBaseClanFortress()
{
	ExecuteAdminConsoleCommand(TEXT("BuildBase honeycomb_clan_fortress"));
}

void UCreativeMenuWidget::OnClickSpawnBaseIndustrialCompound()
{
	ExecuteAdminConsoleCommand(TEXT("BuildBase industrial_compound"));
}
