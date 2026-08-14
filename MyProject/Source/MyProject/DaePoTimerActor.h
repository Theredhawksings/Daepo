// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoTimerActor.generated.h"

class UTextRenderComponent;

/**
 * 맵에 배치하는 3D 텍스트 타이머 디스플레이.
 * 시간 계산/생존 판정은 AMyProjectGameMode 가 가지고 있고, 이 액터는 그 값을 읽어서 보여주기만 한다.
 * 한 맵에 여러 개 놓아도 전부 같은 값을 보여준다.
 */
UCLASS()
class MYPROJECT_API ADaePoTimerActor : public AActor
{
	GENERATED_BODY()

public:
	ADaePoTimerActor();

	/** 텍스트 크기(월드 유닛) */
	UPROPERTY(EditAnywhere, Category = "Timer")
	float TextWorldSize = 100.0f;

	/** 텍스트 색상 */
	UPROPERTY(EditAnywhere, Category = "Timer")
	FColor TextColor = FColor::White;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 화면에 표시되는 3D 텍스트 컴포넌트(루트) */
	UPROPERTY(VisibleAnywhere, Category = "Timer")
	TObjectPtr<UTextRenderComponent> TimerText;

private:
	/** 마지막으로 표시한 값(100분의 1초 단위, 불필요한 SetText 호출 방지용) */
	int32 LastDisplayedCentiseconds = -1;

	/**
	 * 화면에 표시할 경과 시간(초). 서버가 보내는 값(GameState)은 네트워크 업데이트 주기로만
	 * 갱신되어 계단식으로 보이므로, 이 값은 매 프레임 로컬에서 흘려보내고 서버 값과
	 * 크게 벌어졌을 때만(초기 동기화, 일시정지 등) 스냅해서 보정한다.
	 */
	double DisplayedElapsedSeconds = 0.0;

	void UpdateDisplayedText(double Elapsed, bool bForce = false);
};
