// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectGameMode.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "MyProjectCharacter.h"

AMyProjectGameMode::AMyProjectGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	NextLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson1.Lvl_ThirdPerson1")));
}

void AMyProjectGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartSurvivalTimer();
	}
}

void AMyProjectGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRunning || bCompleted || SurvivalDuration <= 0.0f)
	{
		return;
	}

	if (GetElapsedSurvivalSeconds() >= SurvivalDuration)
	{
		bCompleted = true;
		StopSurvivalTimer();

		// 다음 맵 이동 대기 시간 동안 플레이어를 무적으로 만든다.
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (AMyProjectCharacter* PlayerCharacter = Cast<AMyProjectCharacter>(PC->GetPawn()))
			{
				PlayerCharacter->SetInvulnerable(true);
			}
		}

		OnSurvivalTimeReached();

		if (bAutoTravelOnSuccess)
		{
			if (TransitionDelay > 0.0f)
			{
				// 바로 이동하지 않고 TransitionDelay 만큼 멈춰있다가 이동한다.
				GetWorldTimerManager().SetTimer(TransitionTimerHandle, this,
					&AMyProjectGameMode::TravelToNextLevel, TransitionDelay, false);
			}
			else
			{
				TravelToNextLevel();
			}
		}
	}
}

void AMyProjectGameMode::StartSurvivalTimer()
{
	if (bRunning)
	{
		return;
	}
	SegmentStartRealSeconds = FPlatformTime::Seconds();
	bRunning = true;
}

void AMyProjectGameMode::StopSurvivalTimer()
{
	if (!bRunning)
	{
		return;
	}
	AccumulatedSeconds += FPlatformTime::Seconds() - SegmentStartRealSeconds;
	bRunning = false;
}

void AMyProjectGameMode::ResetSurvivalTimer()
{
	AccumulatedSeconds = 0.0;
	SegmentStartRealSeconds = FPlatformTime::Seconds();
	bCompleted = false;
}

double AMyProjectGameMode::GetElapsedSurvivalSeconds() const
{
	if (!bRunning)
	{
		return AccumulatedSeconds;
	}
	return AccumulatedSeconds + (FPlatformTime::Seconds() - SegmentStartRealSeconds);
}

FText AMyProjectGameMode::FormatElapsedTime(double Seconds)
{
	const int32 TotalSeconds = FMath::FloorToInt(Seconds);
	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds % 3600) / 60;
	const int32 Secs = TotalSeconds % 60;

	return FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Secs));
}

void AMyProjectGameMode::TravelToNextLevel()
{
	if (NextLevel.IsNull())
	{
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*NextLevel.ToSoftObjectPath().GetLongPackageName()));
}
