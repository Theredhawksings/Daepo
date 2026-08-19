// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	return SNew(SBox)
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
				.Text(FText::FromString(TEXT("DAEPO SURVIVAL")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
				.ColorAndOpacity(FLinearColor::White)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(220.0f)
				.HeightOverride(48.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &UMainMenuWidget::OnHostClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Host Game")))
						.Justification(ETextJustify::Center)
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SBox)
					.WidthOverride(180.0f)
					.HeightOverride(36.0f)
					[
						SAssignNew(IPTextBox, SEditableTextBox)
						.HintText(FText::FromString(TEXT("서버 IP")))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(120.0f)
					.HeightOverride(48.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked(FOnClicked::CreateUObject(this, &UMainMenuWidget::OnJoinClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Join Game")))
							.Justification(ETextJustify::Center)
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
