// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTimerActor.h"
#include "Components/TextRenderComponent.h"

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

	if (bAutoStart)
	{
		StartTimer();
	}
	else
	{
		UpdateDisplayedText(true);
	}
}

void ADaePoTimerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRunning)
	{
		return;
	}

	ElapsedSeconds += DeltaSeconds;
	UpdateDisplayedText();
}

void ADaePoTimerActor::StartTimer()
{
	bRunning = true;
}

void ADaePoTimerActor::StopTimer()
{
	bRunning = false;
}

void ADaePoTimerActor::ResetTimer()
{
	ElapsedSeconds = 0.0f;
	UpdateDisplayedText(true);
}

void ADaePoTimerActor::UpdateDisplayedText(bool bForce)
{
	const int32 TotalSeconds = FMath::FloorToInt(ElapsedSeconds);
	if (!bForce && TotalSeconds == LastDisplayedSeconds)
	{
		return;
	}
	LastDisplayedSeconds = TotalSeconds;

	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds % 3600) / 60;
	const int32 Seconds = TotalSeconds % 60;

	TimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds)));
}
