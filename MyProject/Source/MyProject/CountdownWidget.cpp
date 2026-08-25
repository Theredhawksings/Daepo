// Fill out your copyright notice in the Description page of Project Settings.

#include "CountdownWidget.h"
#include "DaePoGameState.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCountdownWidget::RebuildWidget()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 60.0f, 0.0f, 0.0f)
		[
			SAssignNew(CountdownText, STextBlock)
			.Text(FText::GetEmpty())
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 72))
			.ColorAndOpacity(FLinearColor::White)
			.Justification(ETextJustify::Center)
		];
}

void UCountdownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRemoved || !CountdownText.IsValid() || !GetWorld())
	{
		return;
	}

	const ADaePoGameState* DaePoGameState = GetWorld()->GetGameState<ADaePoGameState>();
	if (!DaePoGameState)
	{
		return;
	}

	// 게임이 시작되면 카운트다운은 더 볼 필요가 없으므로 화면에서 완전히 치운다.
	if (DaePoGameState->bGameStarted)
	{
		RemoveFromParent();
		bRemoved = true;
		return;
	}

	// 0.9초 -> 1로 반올림 표시되게 올림 처리(마지막 1초 구간이 "0"으로 너무 빨리 안 바뀌도록).
	const int32 SecondsToShow = FMath::CeilToInt(DaePoGameState->ReplicatedPreGameRemaining);
	if (SecondsToShow == LastDisplayedSeconds)
	{
		return;
	}
	LastDisplayedSeconds = SecondsToShow;

	CountdownText->SetText(FText::FromString(FString::FromInt(SecondsToShow)));
}
