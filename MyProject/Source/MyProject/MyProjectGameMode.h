// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyProjectGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AMyProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AMyProjectGameMode();

	/** BeginPlay 시 (대기시간 이후) 자동으로 생존 타이머를 시작할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	bool bAutoStart = true;

	/**
	 * 호스트 직후 실제 게임(대포 발사/생존 타이머)이 시작되기까지 대기하는 시간(초).
	 * 이 동안 다른 플레이어가 합류할 시간을 번다. 0 이면 대기 없이 바로 시작.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.0"))
	float PreGameDelay = 10.0f;

	/** 목표 생존 시간(초). 0 이하면 판정하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.0"))
	float SurvivalDuration = 30.0f;

	/** 목표 시간 달성 시 자동으로 이동할 다음 맵 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	TSoftObjectPtr<UWorld> NextLevel;

	/** 목표 시간 달성 시 자동으로 NextLevel 로 이동할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	bool bAutoTravelOnSuccess = true;

	/** 목표 시간 달성 후 실제로 다음 맵으로 이동하기까지 대기 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.0"))
	float TransitionDelay = 3.0f;

	/** 생존 타이머 시작(이미 실행 중이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	void StartSurvivalTimer();

	/** 생존 타이머 정지(경과 시간은 유지) */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	void StopSurvivalTimer();

	/** 경과 시간을 0 으로 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	void ResetSurvivalTimer();

	/** 현재까지 경과한 생존 시간(초). 맵에 놓인 타이머 디스플레이들이 이 값을 읽어서 표시한다 */
	UFUNCTION(BlueprintPure, Category = "Survival")
	double GetElapsedSurvivalSeconds() const;

	/** 초 단위 값을 "HH:MM:SS" 문자열로 바꾸는 공용 헬퍼 */
	UFUNCTION(BlueprintPure, Category = "Survival")
	static FText FormatElapsedTime(double Seconds);

	/** 목표 시간에 도달한 순간 한 번 호출된다(페이드아웃, 사운드 등 연출을 붙이고 싶으면 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Survival")
	void OnSurvivalTimeReached();

	/** NextLevel 로 즉시 레벨 이동 */
	UFUNCTION(BlueprintCallable, Category = "Survival")
	void TravelToNextLevel();

	/** 돌아갈 메인 메뉴 맵 (패키지 전체 경로로 지정해야 엔진 내장 에셋과 이름이 겹치지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	FName MainMenuMapName = FName(TEXT("/Game/Main"));

	/** 최후의 1인이 가려진 뒤 메인 메뉴로 돌아가기까지 대기 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.0"))
	float ReturnToMenuDelay = 2.0f;

	/**
	 * 캐릭터가 죽을 때마다 호출된다(사망 연출이 끝난 뒤). 살아있는 인원을 세어서 1명
	 * 이하로 남았으면(최후의 1인 결정) 잠시 후 접속해 있는 전원을 메인 메뉴로 돌려보낸다.
	 * 2명 이상 살아있으면 아무 것도 하지 않고, 죽은 캐릭터는 그대로 죽은 채로 게임은 계속된다.
	 */
	void NotifyPlayerDied();

	/**
	 * 플레이어가 죽지 않고 그냥 접속을 끊고 나갔을 때도(장시간 정지 페널티로 죽는 것과 달리
	 * 캐릭터의 bIsDead 가 그대로 false 라서) 최후의 1인 판정을 다시 확인해야 한다.
	 */
	virtual void Logout(AController* Exiting) override;

	/** 지뢰로 사용할 액터 클래스. 비워두면 지뢰 기능 자체가 꺼진다(스폰 안 함). */
	UPROPERTY(EditAnywhere, Category = "Landmine")
	TSubclassOf<class ADaePoLandmine> LandmineClass;

	/** 동시에 맵에 존재할 수 있는 최대 지뢰 개수 */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "0"))
	int32 MaxLandmineCount = 6;

	/** 이 주기(초)마다 살아있는 플레이어들의 현재 위치를 "발자국"으로 기록한다 */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "0.1"))
	float FootstepSampleInterval = 2.0f;

	/** 발자국을 남긴 뒤 실제로 그 자리에 지뢰가 깔리기까지 최소 대기 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "0.0"))
	float MinFootstepMineDelay = 3.0f;

	/** 발자국을 남긴 뒤 실제로 그 자리에 지뢰가 깔리기까지 최대 대기 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "0.0"))
	float MaxFootstepMineDelay = 8.0f;

	/** 지뢰가 하나 터졌을 때(ADaePoLandmine 이 호출) 개수만 줄인다. 새 지뢰는 발자국을 통해 자연스럽게 다시 채워진다. */
	void OnLandmineConsumed();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 게임이 이미 시작된 뒤(bGameStarted == true)에는 새로 들어오려는 접속을 거절한다 */
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

private:
	/** 실행 중일 때, 이번 구간이 시작된 실제(벽시계) 시각 */
	double SegmentStartRealSeconds = 0.0;

	/** 정지/리셋되기 전까지 누적된 경과 시간(초) */
	double AccumulatedSeconds = 0.0;

	bool bRunning = false;

	/** 목표 시간 도달 처리를 한 번만 하기 위한 플래그 */
	bool bCompleted = false;

	/** TransitionDelay 만큼 대기했다가 TravelToNextLevel 을 호출하는 타이머 */
	FTimerHandle TransitionTimerHandle;

	/** PreGameDelay 만큼 대기했다가 실제 게임을 시작시키는 타이머 */
	FTimerHandle PreGameTimerHandle;

	/** 대기시간이 끝나면 호출됨: GameState.bGameStarted 를 켜고 생존 타이머를 시작한다 */
	void BeginActualGame();

	/**
	 * NotifyPlayerDied/Logout 이 공유하는 실제 판정 로직. ExcludedController 를 넘기면
	 * (접속 종료 중인 컨트롤러) 그 폰의 생존 여부와 상관없이 무조건 인원 수에서 제외한다
	 * (아직 완전히 정리되기 전이라 PlayerControllerIterator 에 남아있을 수 있어서).
	 */
	void CheckForLastManStanding(const APlayerController* ExcludedController);

	/** ReturnToMenuDelay 만큼 대기했다가 접속해 있는 전원을 메인 메뉴로 돌려보내는 타이머 */
	FTimerHandle ReturnToMenuTimerHandle;

	/** 실제로 메인 메뉴 맵을 여는 함수(전원이 같이 이동함) */
	void ReturnAllPlayersToMainMenu();

	/** 현재 맵에 살아있는(아직 안 터진) 지뢰 개수 */
	int32 CurrentLandmineCount = 0;

	/** FootstepSampleInterval 마다 반복 호출되어 살아있는 플레이어들의 현재 위치를 기록해둔다 */
	FTimerHandle FootstepSampleTimerHandle;

	/** 살아있는 플레이어 각각의 현재 위치에, 무작위 지연 뒤 지뢰 스폰을 예약한다 */
	void SampleFootstepsForLandmines();

	/**
	 * 실제로 지뢰를 그 위치에 스폰한다. 발자국을 남긴 시점으로부터 지연이 있었으므로
	 * 이미 그 자리를 벗어났을 가능성이 높지만, 혹시 몰라 스폰 직전에 그 자리에 아무도
	 * 없는지 한 번 더 확인해서 즉시 밟혀 억울하게 피해를 입는 일을 막는다.
	 */
	void TrySpawnLandmineAt(FVector Location);
};



