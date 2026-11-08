// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "DaePoFont.h"
#include "DaePoGameInstance.h"
#include "MyProjectCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"

namespace
{
	struct FMapCard
	{
		const TCHAR* Label;
		const TCHAR* Path; // 비어 있으면 "준비 중" 카드
	};

	// 맵 선택 화면의 카드(위 3개, 아래 3개). 새 맵은 DefaultGame.ini 의 MapsToCook 에도 넣어야 패키징된다.
	// 썸네일은 텍스처 에셋 /Game/Thumbnails/Map<카드번호> 를 쓰고, 없으면 글자만 나온다.
	const FMapCard MapCards[] = {
		{ TEXT("Map 1"), TEXT("/Game/ThirdPerson/Lvl_ThirdPerson") },
		{ TEXT("Map 2"), TEXT("/Game/ThirdPerson/Lvl_ThirdPerson1") },
		{ TEXT("근정전"), TEXT("/Game/GeunjeongjeonComplex/Map/Demo_Geunjeongjeon") },
		{ TEXT("준비 중"), TEXT("") },
		{ TEXT("준비 중"), TEXT("") },
		{ TEXT("준비 중"), TEXT("") },
	};

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
				.Font(DaePoFont("Bold", 16))
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

	TSharedRef<SUniformGridPanel> MapGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(12.0f));
	for (int32 i = 0; i < UE_ARRAY_COUNT(MapCards); ++i)
	{
		const bool bAvailable = MapCards[i].Path[0] != 0;

		TSharedRef<SWidget> Thumbnail = SNullWidget::NullWidget;
		const FString TexturePath = FString::Printf(TEXT("/Game/Thumbnails/Map%d.Map%d"), i + 1, i + 1);
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			ThumbnailTextures.Add(Texture);
			TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Texture);
			Brush->ImageSize = FVector2D(212.0, 120.0);
			ThumbnailBrushes.Add(Brush);
			Thumbnail = SNew(SImage).Image(Brush.Get());
		}

		MapGrid->AddSlot(i % 3, i / 3)
		[
			SNew(SBox)
			.WidthOverride(220.0f)
			.HeightOverride(170.0f)
			.IsEnabled(bAvailable)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.15f, 0.35f, 0.75f))
				.ContentPadding(4.0f)
				.OnClicked(FOnClicked::CreateUObject(this, &UMainMenuWidget::OnMapClicked, FName(MapCards[i].Path)))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						Thumbnail
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 2)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(MapCards[i].Label))
						.Font(DaePoFont("Bold", 16))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		];
	}

	TSharedRef<SWidget> MenuScreen = SNew(SBox)
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
							.Font(DaePoFont("Bold", 30))
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
							.Font(DaePoFont("Regular", 12))
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
									FOnClicked::CreateUObject(this, &UMainMenuWidget::ShowScreen, 1))
							]
						]
					]
				]
			]
		];

	TSharedRef<SWidget> MapScreen = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f))
			.Padding(FMargin(32.0f, 24.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 12)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("맵 선택")))
					.Font(DaePoFont("Bold", 26))
					.ColorAndOpacity(FLinearColor::White)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MapGrid
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 12, 0, 0)
				.HAlign(HAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(160.0f)
					.HeightOverride(42.0f)
					[
						MakeColoredButton(TEXT("뒤로"), FLinearColor(0.5f, 0.15f, 0.15f),
							FOnClicked::CreateUObject(this, &UMainMenuWidget::ShowScreen, 0))
					]
				]
			]
		];

	return SAssignNew(ScreenSwitcher, SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()
		[
			MenuScreen
		]
		+ SWidgetSwitcher::Slot()
		[
			MapScreen
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

FReply UMainMenuWidget::ShowScreen(int32 ScreenIndex)
{
	ScreenSwitcher->SetActiveWidgetIndex(ScreenIndex);
	return FReply::Handled();
}

FReply UMainMenuWidget::OnMapClicked(FName MapName)
{
	UGameplayStatics::OpenLevel(this, MapName);
	return FReply::Handled();
}
