// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** 배경색이 다른 버튼(호스트=초록, 참가=파랑 식으로 구분)을 만드는 헬퍼 */
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
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// 반투명 어두운 패널로 메뉴 전체를 감싸서 빈 배경 위에 그냥 떠 있지 않게 한다.
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.75f))
			.Padding(FMargin(40.0f, 32.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 28)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DAEPO SURVIVAL")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
					.ColorAndOpacity(FLinearColor::White)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				.HAlign(HAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(240.0f)
					.HeightOverride(50.0f)
					[
						MakeColoredButton(TEXT("Host Game"), FLinearColor(0.15f, 0.6f, 0.25f),
							FOnClicked::CreateUObject(this, &UMainMenuWidget::OnHostClicked))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Server IP")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0, 0, 8, 0)
						[
							SNew(SBox)
							.WidthOverride(190.0f)
							.HeightOverride(36.0f)
							[
								SAssignNew(IPTextBox, SEditableTextBox)
								.HintText(FText::FromString(TEXT("예: 127.0.0.1")))
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(130.0f)
							.HeightOverride(50.0f)
							[
								MakeColoredButton(TEXT("Join Game"), FLinearColor(0.15f, 0.35f, 0.75f),
									FOnClicked::CreateUObject(this, &UMainMenuWidget::OnJoinClicked))
							]
						]
					]
				]
			]
		];
}

FReply UMainMenuWidget::OnHostClicked()
{
	// 리슨 서버로 이 맵을 연다 — 누른 사람이 서버 겸 첫 번째 플레이어가 된다.
	UGameplayStatics::OpenLevel(this, MapToHost, true, TEXT("listen"));
	return FReply::Handled();
}

FReply UMainMenuWidget::OnJoinClicked()
{
	if (!IPTextBox.IsValid())
	{
		return FReply::Handled();
	}

	const FString IP = IPTextBox->GetText().ToString().TrimStartAndEnd();
	if (IP.IsEmpty())
	{
		return FReply::Handled();
	}

	// 입력한 IP 로 접속을 시도한다(그 주소의 서버가 리슨 서버로 열려 있어야 함).
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(IP, TRAVEL_Absolute);
	}

	return FReply::Handled();
}
