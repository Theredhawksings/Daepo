// Fill out your copyright notice in the Description page of Project Settings.

#include "PauseMenuWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	return SNew(SBox)
		// 화면 전체를 옅게 덮는 반투명 배경. 뒤에 게임 화면이 살짝 비친다.
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 20)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PAUSED")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
					.ColorAndOpacity(FLinearColor::White)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(180.0f)
					.HeightOverride(48.0f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(FLinearColor(0.7f, 0.15f, 0.15f))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked(FOnClicked::CreateUObject(this, &UPauseMenuWidget::OnQuitClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Quit Game")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
							.ColorAndOpacity(FLinearColor::White)
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

FReply UPauseMenuWidget::OnQuitClicked()
{
	// 정상적으로 엔진을 종료한다. 호스트라면 접속해 있던 클라이언트들은 연결이 끊기는데,
	// 그건 UDaePoGameInstance::HandleNetworkFailure 가 감지해서 각자 메인 메뉴로 돌려보낸다.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	return FReply::Handled();
}
