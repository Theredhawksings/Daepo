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

		if (!DaePoGameState->bGameStarted)
		{
			// PreGameTimerHandle 이 유효하지 않으면(대기시간 0 등) 음수가 나오므로 0으로 클램프.
			const float Remaining = GetWorldTimerManager().GetTimerRemaining(PreGameTimerHandle);
			DaePoGameState->ReplicatedPreGameRemaining = FMath::Max(Remaining, 0.0f);
		}
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

void AMyProjectGameMode::NotifyPlayerDied()
{
	if (!HasAuthority())
	{
		return;
	}

	int32 AliveCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const AMyProjectCharacter* Character = PC ? Cast<AMyProjectCharacter>(PC->GetPawn()) : nullptr;
		if (Character && !Character->IsDead())
		{
			++AliveCount;
		}
	}

	// 살아있는 인원이 1명 이하로 남으면(최후의 1인 결정, 또는 동시 전멸) 라운드를 끝내고
	// 잠시 후 접속해 있는 전원(승자든 이미 죽은 사람이든)을 메인 메뉴로 돌려보낸다.
	if (AliveCount <= 1)
	{
		GetWorldTimerManager().SetTimer(ReturnToMenuTimerHandle, this, &AMyProjectGameMode::ReturnAllPlayersToMainMenu, ReturnToMenuDelay, false);
	}
}

void AMyProjectGameMode::ReturnAllPlayersToMainMenu()
{
	UGameplayStatics::OpenLevel(this, MainMenuMapName, true);
}

void AMyProjectGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (const ADaePoGameState* DaePoGameState = GetGameState<ADaePoGameState>())
	{
		if (DaePoGameState->bGameStarted)
		{
			ErrorMessage = TEXT("게임이 이미 시작되어 참여할 수 없습니다.");
		}
	}
}

void AMyProjectGameMode::TravelToNextLevel()
{
	if (NextLevel.IsNull())
	{
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*NextLevel.ToSoftObjectPath().GetLongPackageName()));
}
