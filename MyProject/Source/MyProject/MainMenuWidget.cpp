// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "DaePoGameInstance.h"
#include "MyProjectCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** 배경색이 있는 버튼을 만드는 헬퍼 */
	TSharedRef<SWidget> MakeColoredButton(const FString& Label, const FLinearColor& Tint, const FOnClicked& OnClicked)
	{
		return SNew(SButton)
			.ButtonColorAndOpacity(Tint)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ColorAndOpacity(FLinearColor::White)
				.Justification(ETextJustify::Center)
			];
	}
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	constexpr float ContentWidth = 420.0f;

	// 플레이어 색 선택 줄. 빨강은 봇 전용이라 뺀다. 고른 색은 흰 테두리로 표시한다.
	TSharedRef<SHorizontalBox> ColorRow = SNew(SHorizontalBox);
	const TArray<FLinearColor>& Colors = GetDefault<AMyProjectCharacter>()->GetPlayerColors();
	for (int32 i = 0; i < Colors.Num(); ++i)
	{
		if (i == AMyProjectCharacter::BotColorIndex)
		{
			continue;
		}

		ColorRow->AddSlot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(TAttribute<FSlateColor>::CreateWeakLambda(this, [this, i]()
			{
				const UDaePoGameInstance* GI = Cast<UDaePoGameInstance>(GetGameInstance());
				return FSlateColor((GI && GI->SelectedColorIndex == i) ? FLinearColor::White : FLinearColor::Transparent);
			}))
			.Padding(3.0f)
			[
				SNew(SBox)
				.WidthOverride(36.0f)
				.HeightOverride(36.0f)
				[
					SNew(SButton)
					.ContentPadding(0.0f)
					.OnClicked(FOnClicked::CreateUObject(this, &UMainMenuWidget::OnColorClicked, i))
					[
						SNew(SColorBlock)
						.Color(Colors[i])
						.Visibility(EVisibility::HitTestInvisible)
					]
				]
			]
		];
	}

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// 바깥쪽 옅은 테두리 + 안쪽 반투명 어두운 패널 — 카드처럼 경계가 뚜렷해 보이게 한다.
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.15f))
			.Padding(1.5f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f))
				.Padding(FMargin(40.0f, 32.0f))
				[
					SNew(SBox)
					.WidthOverride(ContentWidth)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("DAEPO SURVIVAL")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
							.ColorAndOpacity(FLinearColor::White)
							.Justification(ETextJustify::Center)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 24)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("최후의 1인 생존")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 20)
						.HAlign(HAlign_Center)
						[
							ColorRow
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Fill)
						[
							SNew(SBox)
							.HeightOverride(50.0f)
							[
								MakeColoredButton(TEXT("Start Game"), FLinearColor(0.15f, 0.6f, 0.25f),
									FOnClicked::CreateUObject(this, &UMainMenuWidget::OnStartClicked))
							]
						]
					]
				]
			]
		];
}

FReply UMainMenuWidget::OnColorClicked(int32 ColorIndex)
{
	if (UDaePoGameInstance* GI = Cast<UDaePoGameInstance>(GetGameInstance()))
	{
		GI->SelectedColorIndex = ColorIndex;
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::OnStartClicked()
{
	UGameplayStatics::OpenLevel(this, MapToStart);
	return FReply::Handled();
}
