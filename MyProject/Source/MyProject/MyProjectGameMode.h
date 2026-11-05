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

	/** BeginPlay 시 (대기시간 이후) 자동으로 게임을 시작할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	bool bAutoStart = true;

	/**
	 * 호스트 직후 실제 게임(대포 발사)이 시작되기까지 대기하는 시간(초).
	 * 이 동안 다른 플레이어가 합류할 시간을 번다. 0 이면 대기 없이 바로 시작.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival", meta = (ClampMin = "0.0"))
	float PreGameDelay = 10.0f;

	/** 캐릭터가 죽는 즉시 호출된다. 플레이어가 죽었거나 플레이어만 남았으면 게임을 끝낸다. */
	void NotifyPlayerDied();

	/**
	 * 플레이어가 죽지 않고 접속을 끊고 나갔을 때도(bIsDead 가 false 라서) 승패를 다시 판정해야 한다.
	 */
	virtual void Logout(AController* Exiting) override;

	/** 혼자 테스트할 때 채워 넣을 더미 봇 숫자. 0이면 안 만든다. */
	UPROPERTY(EditAnywhere, Category = "Bot", meta = (ClampMin = "0"))
	int32 NumberOfBots = 0;

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
	/** PreGameDelay 만큼 대기했다가 실제 게임을 시작시키는 타이머 */
	FTimerHandle PreGameTimerHandle;

	/** 대기시간이 끝나면 호출됨: GameState.bGameStarted 를 켠다 */
	void BeginActualGame();

	/** NumberOfBots 만큼 더미 봇을 PlayerStart 위치에 스폰한다(실제 플레이어 폰 클래스를 그대로 재사용) */
	void SpawnBots();

	/**
	 * NotifyPlayerDied/Logout 이 공유하는 승패 판정. ExcludedController 를 넘기면 접속 종료 중인
	 * 컨트롤러로 보고 생존 여부와 상관없이 인원에서 뺀다.
	 */
	void CheckMatchEnd(const APlayerController* ExcludedController);

	/** 결과 화면을 한 번만 띄우기 위한 플래그 */
	bool bMatchEnded = false;

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



