// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTimerActor.h"
#include "Components/TextRenderComponent.h"
#include "MyProjectGameMode.h"

ADaePoTimerActor::ADaePoTimerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TimerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TimerText"));
	RootComponent = TimerText;

	TimerText->SetHorizontalAlignment(EHTA_Center);
	TimerText->SetVerticalAlignment(EVRTA_TextCenter);
	TimerText->SetWorldSize(TextWorldSize);
	TimerText->SetTextRenderColor(TextColor);
	TimerText->SetText(FText::FromString(TEXT("00:00:00")));
}

void ADaePoTimerActor::BeginPlay()
{
	Super::BeginPlay();

	TimerText->SetWorldSize(TextWorldSize);
	TimerText->SetTextRenderColor(TextColor);

	UpdateDisplayedText(true);
}

void ADaePoTimerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateDisplayedText();
}

void ADaePoTimerActor::UpdateDisplayedText(bool bForce)
{
	const AMyProjectGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMyProjectGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}

	const double Elapsed = GameMode->GetElapsedSurvivalSeconds();
	const int32 TotalSeconds = FMath::FloorToInt(Elapsed);
	if (!bForce && TotalSeconds == LastDisplayedSeconds)
	{
		return;
	}
	LastDisplayedSeconds = TotalSeconds;

	TimerText->SetText(AMyProjectGameMode::FormatElapsedTime(Elapsed));
}
