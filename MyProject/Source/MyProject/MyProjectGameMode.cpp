// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectGameMode.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "MyProjectCharacter.h"
#include "DaePoGameState.h"

AMyProjectGameMode::AMyProjectGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	NextLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson1.Lvl_ThirdPerson1")));

	// 경과 시간을 클라이언트에도 보여주기 위해 복제되는 게임 스테이트를 사용한다.
	GameStateClass = ADaePoGameState::StaticClass();
}

void AMyProjectGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoStart)
	{
		return;
	}

	if (PreGameDelay > 0.0f)
	{
		// 대기시간 동안은 대포도 안 쏘고(GameState.bGameStarted == false) 타이머도 안 돈다.
		// 그동안 다른 플레이어가 합류할 시간을 번다.
		GetWorldTimerManager().SetTimer(PreGameTimerHandle, this, &AMyProjectGameMode::BeginActualGame, PreGameDelay, false);
	}
	else
	{
		BeginActualGame();
	}
}

void AMyProjectGameMode::BeginActualGame()
{
	// 대포들이 이 신호를 보고 그때부터 발사를 시작한다(값이 복제되어 클라이언트에도 전달됨).
	if (ADaePoGameState* GS = GetGameState<ADaePoGameState>())
	{
		GS->bGameStarted = true;
	}

	StartSurvivalTimer();
}

void AMyProjectGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// GameMode 는 서버에만 존재하므로 이 Tick 자체가 항상 서버에서만 실행된다.
	// 클라이언트가 경과 시간을 볼 수 있도록 GameState(복제됨)에 매 틱 반영한다.
	if (ADaePoGameState* DaePoGameState = GetGameState<ADaePoGameState>())
	{
		DaePoGameState->ReplicatedElapsedSeconds = static_cast<float>(GetElapsedSurvivalSeconds());
	}

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
	const int32 Centiseconds = FMath::FloorToInt((Seconds - TotalSeconds) * 100.0);

	return FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d.%02d"), Hours, Minutes, Secs, Centiseconds));
}

void AMyProjectGameMode::TravelToNextLevel()
{
	if (NextLevel.IsNull())
	{
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*NextLevel.ToSoftObjectPath().GetLongPackageName()));
}
