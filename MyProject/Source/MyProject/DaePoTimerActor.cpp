// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTimerActor.h"
#include "Components/TextRenderComponent.h"
#include "HAL/PlatformTime.h"

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

	// Tick 의 DeltaSeconds 를 누적하지 않고, 실제 벽시계 기준으로 경과 시간을 계산한다.
	// (프레임 히칭 시 엔진이 DeltaSeconds 를 clamp 해서 실제 시간보다 느려지는 것을 방지)
	UpdateDisplayedText();
}

void ADaePoTimerActor::StartTimer()
{
	if (bRunning)
	{
		return;
	}
	SegmentStartRealSeconds = FPlatformTime::Seconds();
	bRunning = true;
}

void ADaePoTimerActor::StopTimer()
{
	if (!bRunning)
	{
		return;
	}
	AccumulatedSeconds += FPlatformTime::Seconds() - SegmentStartRealSeconds;
	bRunning = false;
}

void ADaePoTimerActor::ResetTimer()
{
	AccumulatedSeconds = 0.0;
	SegmentStartRealSeconds = FPlatformTime::Seconds();
	UpdateDisplayedText(true);
}

double ADaePoTimerActor::GetElapsedSeconds() const
{
	if (!bRunning)
	{
		return AccumulatedSeconds;
	}
	return AccumulatedSeconds + (FPlatformTime::Seconds() - SegmentStartRealSeconds);
}

void ADaePoTimerActor::UpdateDisplayedText(bool bForce)
{
	const int32 TotalSeconds = FMath::FloorToInt(GetElapsedSeconds());
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
