// Copyright Telephone Studios. All Rights Reserved.


#include "UI/CraftingMenuWidget.h"
#include "Components/WrapBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "CraftingRecipeCategoryButton.h"
#include "RecipeIconWidget.h"
#include "IngredientRowWidget.h"
#include "Components/CraftingComponent.h"
#include "Components/InventoryComponent.h"
#include "UObject/ConstructorHelpers.h"

UCraftingMenuWidget::UCraftingMenuWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<URecipeIconWidget> RecipeIconWidgetBlueprint(TEXT("/Game/Crafting/UI/WBP_RecipeIcon"));
	ConstructorHelpers::FClassFinder<UIngredientRowWidget> IngredientRowWidgetBlueprint(TEXT("/Game/Crafting/UI/WBP_IngredientRow"));

	if (RecipeIconWidgetBlueprint.Class == nullptr || IngredientRowWidgetBlueprint.Class == nullptr)
	{
		return;
	}

	RecipeIconWidgetClass = RecipeIconWidgetBlueprint.Class;
	IngredientRowWidgetClass = IngredientRowWidgetBlueprint.Class;
}

void UCraftingMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn == nullptr)
	{
		return;
	}

	UInventoryComponent* OwnerInventory = OwningPawn->FindComponentByClass<UInventoryComponent>();
	if (OwnerInventory == nullptr)
	{
		return;
	}

	OwnerInventory->OnUpdate.AddDynamic(this, &UCraftingMenuWidget::Refresh);

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

	CraftButton->OnClicked.AddDynamic(this, &UCraftingMenuWidget::Craft);

	SelectedRecipe = TEXT("");
	Refresh();
}

void UCraftingMenuWidget::Refresh()
{
	RefreshRecipesList();
	RefreshDetailsPanel();
}

void UCraftingMenuWidget::SetCategoryFilter(TEnumAsByte<EItemCategory> NewCategory)
{
	CategoryFilter = NewCategory;
	SelectedRecipe = FName();
	RefreshRecipesList();
	RefreshDetailsPanel();
}

TEnumAsByte<EItemCategory> UCraftingMenuWidget::GetCategoryFilter() const
{
	return CategoryFilter;
}

void UCraftingMenuWidget::SetSelectedRecipe(const FName& SelectedRecipeName)
{
	SelectedRecipe = SelectedRecipeName;
	RefreshDetailsPanel();
}

FName UCraftingMenuWidget::GetSelectedRecipe() const
{
	return SelectedRecipe;
}

void UCraftingMenuWidget::RefreshRecipesList()
{
	TArray<FCraftingRecipeEntry> RecipeEntries;
	for (const FName& RecipeID : UCraftingComponent::GetAllRecipes())
	{
		FItemData* YieldItemData = UInventoryComponent::GetItemData(RecipeID);
		if (YieldItemData == nullptr || (CategoryFilter != EItemCategory::All && YieldItemData->Category != CategoryFilter))
		{
			continue;
		}

		FCraftingRecipeEntry Entry;
		Entry.RecipeID = RecipeID;
		Entry.CanCraft = CanCraftRecipe(RecipeID);
		Entry.IngredientPercentage = GetRecipeIngredientPercentage(RecipeID);
		Entry.YieldItemData = YieldItemData;

		RecipeEntries.Add(Entry);
	}
	
	RecipeEntries.Sort();

	if (SelectedRecipe == FName() && RecipeEntries.Num() > 0)
	{
		SelectedRecipe = RecipeEntries[0].RecipeID;
	}

	RecipesWrapBox->ClearChildren();
	for (int32 i = 0; i < RecipeEntries.Num(); i++)
	{
		const FCraftingRecipeEntry& RecipeEntry = RecipeEntries[i];

		URecipeIconWidget* NewRecipeIcon = CreateWidget<URecipeIconWidget>(this, RecipeIconWidgetClass);
		if (NewRecipeIcon == nullptr)
		{
			continue;
		}

		NewRecipeIcon->Setup(this, RecipeEntry);
		RecipesWrapBox->AddChild(NewRecipeIcon);
	}

}

void UCraftingMenuWidget::RefreshDetailsPanel()
{
	if (SelectedRecipe == FName())
	{
		ClearDetailsPanel();
		return;
	}

	FCraftingRecipe* RecipeData = UCraftingComponent::GetRecipe(SelectedRecipe);
	if (RecipeData == nullptr)
	{
		return;
	}

	FItemData* RecipeYieldItemData = UInventoryComponent::GetItemData(SelectedRecipe);
	if (RecipeYieldItemData == nullptr)
	{
		return;
	}

	SelectedRecipeNameTextBlock->SetText(FText::FromString(RecipeYieldItemData->DisplayName));
	
	SelectedRecipeDescriptionTextBlock->SetText(FText::FromString(RecipeYieldItemData->Description));

	SelectedRecipeIconImage->SetBrushFromMaterial(RecipeYieldItemData->Thumbnail);
	SelectedRecipeIconImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	
	FString RecipeYieldQuantityString;

	if (RecipeData->YieldQuantity > 1)
	{
		RecipeYieldQuantityString = FString::Printf(TEXT("x%i"), RecipeData->YieldQuantity);
	}

	SelectedRecipeYieldTextBlock->SetText(FText::FromString(RecipeYieldQuantityString));

	RefreshIngredientList();

	CraftButton->SetIsEnabled(CanCraftRecipe(SelectedRecipe));
}

void UCraftingMenuWidget::ClearDetailsPanel()
{
	SelectedRecipeNameTextBlock->SetText(FText::FromString(FString()));
	
	SelectedRecipeDescriptionTextBlock->SetText(FText::FromString(FString()));

	SelectedRecipeIconImage->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

	CraftButton->SetIsEnabled(false);

	IngredientListBox->ClearChildren();
}

void UCraftingMenuWidget::RefreshIngredientList()
{
	IngredientListBox->ClearChildren();
	
	FCraftingRecipe* RecipeData = UCraftingComponent::GetRecipe(SelectedRecipe);
	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->FindComponentByClass<UInventoryComponent>();
	if (RecipeData == nullptr || OwnerInventoryComponent == nullptr)
	{
		return;
	}

	FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return;
	}

	for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
	{
		const int32 HasAmount = InventoryContents->GetItemQuantity(Ingredient.Name);
		FItemData* IngredientItemData = UInventoryComponent::GetItemData(Ingredient.Name);
		if (IngredientItemData == nullptr)
		{
			return;
		}

		UIngredientRowWidget* NewIngredientRow = CreateWidget<UIngredientRowWidget>(this, IngredientRowWidgetClass);
		if (NewIngredientRow == nullptr)
		{
			return;
		}
		NewIngredientRow->Setup(IngredientItemData->DisplayName, Ingredient.Quantity, HasAmount);
		IngredientListBox->AddChild(NewIngredientRow);
		
		UVerticalBoxSlot* IngredientRowSlot = Cast<UVerticalBoxSlot>(NewIngredientRow->Slot);
		if (IngredientRowSlot == nullptr)
		{
			return;
		}
		FMargin RowPadding;
		RowPadding.Top = 5.0f;
		RowPadding.Bottom = 5.0f;
		RowPadding.Left = 5.0f;
		RowPadding.Right = 5.0f;

		IngredientRowSlot->SetPadding(RowPadding);
	}
}

void UCraftingMenuWidget::Craft()
{
	UCraftingComponent* OwnerCraftingComponent = GetOwningPlayerPawn()->FindComponentByClass<UCraftingComponent>();
	if (OwnerCraftingComponent == nullptr)
	{
		return;
	}
	
	OwnerCraftingComponent->Server_CraftItem(SelectedRecipe);
}

bool UCraftingMenuWidget::CanCraftRecipe(const FName& RecipeName)
{
	FCraftingRecipe* RecipeData = UCraftingComponent::GetRecipe(RecipeName);
	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->FindComponentByClass<UInventoryComponent>();
	if (RecipeData == nullptr || OwnerInventoryComponent == nullptr)
	{
		return false;
	}

	FInventoryContents* InventoryContents = OwnerInventoryComponent->GetContents();
	if (InventoryContents == nullptr)
	{
		return false;
	}

	for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
	{
		const int32 IngredientHasAmount = InventoryContents->GetItemQuantity(Ingredient.Name);
		
		if (IngredientHasAmount < Ingredient.Quantity)
		{
			return false;
		}
	}
	
	return true;
}

int32  UCraftingMenuWidget::GetRecipeIngredientPercentage(const FName& RecipeName)
{
	FCraftingRecipe* RecipeData = UCraftingComponent::GetRecipe(RecipeName);
	UInventoryComponent* OwnerInventoryComponent = GetOwningPlayerPawn()->FindComponentByClass<UInventoryComponent>();
	if (RecipeData == nullptr || OwnerInventoryComponent == nullptr)
	{
		return 0;
	}

	int32 IngredientTotal = 0;
	int32 IngredientPossession = 0;
	for (const FInventoryItem& Ingredient : RecipeData->Ingredients)
	{
		int32 IngredientHasAmount = OwnerInventoryComponent->GetContents()->GetItemQuantity(Ingredient.Name);

		IngredientTotal += Ingredient.Quantity;
		IngredientPossession += IngredientHasAmount;
	}

	if (IngredientTotal <= 0)
	{
		return 100;
	}

	float NormalizedRatio = static_cast<float>(IngredientPossession) / static_cast<float>(IngredientTotal);
	return FMath::FloorToInt32(NormalizedRatio * 100.0f);
}
