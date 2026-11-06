// Copyright Telephone Studios. All Rights Reserved.


#include "RecipeIconWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "UI/CraftingMenuWidget.h"
#include "Structs/ItemData.h"
#include "Color/UIColors.h"

void URecipeIconWidget::Setup(UCraftingMenuWidget* InParentMenu, const FCraftingRecipeEntry& InParentEntry)
{
	ParentMenu = InParentMenu;
	ParentEntry = InParentEntry;

	if (ParentEntry.YieldItemData != nullptr)
	{
		RecipeIconBorder->SetBrushFromMaterial(ParentEntry.YieldItemData->Thumbnail);
		const FText TooltipText = FText::FromString(FString::Printf(TEXT("%s\n%s"), *ParentEntry.YieldItemData->DisplayName, *ParentEntry.YieldItemData->Description));
		SetToolTipText(TooltipText);
		if (RecipeButton != nullptr)
		{
			RecipeButton->SetToolTipText(TooltipText);
		}
	}

	RecipeButton->OnClicked.AddDynamic(this, &URecipeIconWidget::OnClicked);
}

bool URecipeIconWidget::IsSelected() const
{
	if (ParentMenu == nullptr)
	{
		return false;
	}

	return ParentMenu->GetSelectedRecipe() == ParentEntry.RecipeID;
}

bool URecipeIconWidget::IsCraftable() const
{
	return ParentEntry.CanCraft;
}

void URecipeIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (IsSelected())
	{
		FUIColor* Blue = UUIColors::GetBaseColor(FName("Blue"));
		if (Hovering)
		{
			BackgroundBorder->SetBrushColor(UUIColors::GetHighlightedColor(Blue) - FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		}
		else
		{
			BackgroundBorder->SetBrushColor(Blue->Default - FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		}
	}
	else
	{
		FUIColor* White = UUIColors::GetBaseColor(FName("White"));

		if (Hovering)
		{
			BackgroundBorder->SetBrushColor(UUIColors::GetHighlightedColor(White) - FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		}
		else
		{
			BackgroundBorder->SetBrushColor(White->Default - FLinearColor(0.0f,0.0f,0.0f,0.7f));
		}
	}

	if (IsCraftable())
	{
		RecipeIconBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else
	{
		RecipeIconBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));
	}
}

void URecipeIconWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	Hovering = true;
}

void URecipeIconWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	Hovering = false;
}

void URecipeIconWidget::OnClicked()
{
	ParentMenu->SetSelectedRecipe(ParentEntry.RecipeID);
}
