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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

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
};



