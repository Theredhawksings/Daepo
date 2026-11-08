// Fill out your copyright notice in the Description page of Project Settings.

#include "ResultWidget.h"
#include "DaePoFont.h"
#include "DaePoGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UResultWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 24)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(bWon ? TEXT("승리") : TEXT("패배")))
				.Font(DaePoFont("Bold", 56))
				.ColorAndOpacity(bWon ? FLinearColor(1.0f, 0.85f, 0.1f) : FLinearColor(0.9f, 0.15f, 0.15f))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(200.0f)
				.HeightOverride(50.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &UResultWidget::OnMenuClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("메인 메뉴")))
						.Font(DaePoFont("Bold", 16))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		];
}

FReply UResultWidget::OnMenuClicked()
{
	if (const UDaePoGameInstance* GI = Cast<UDaePoGameInstance>(GetGameInstance()))
	{
		UGameplayStatics::OpenLevel(this, GI->MainMenuMapName);
	}
	return FReply::Handled();
}
