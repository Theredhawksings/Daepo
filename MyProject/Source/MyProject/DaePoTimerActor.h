// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoTimerActor.generated.h"

class UTextRenderComponent;

/**
 * 맵에 직접 배치해서 쓰는 3D 텍스트 스톱워치.
 * 00:00:00 부터 시작해서 흘러간다.
 */
UCLASS()
class MYPROJECT_API ADaePoTimerActor : public AActor
{
	GENERATED_BODY()

public:
	ADaePoTimerActor();

	/** BeginPlay 시 자동으로 타이머를 시작할지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	bool bAutoStart = true;

	/** 텍스트 크기(월드 유닛) */
	UPROPERTY(EditAnywhere, Category = "Timer")
	float TextWorldSize = 100.0f;

	/** 텍스트 색상 */
	UPROPERTY(EditAnywhere, Category = "Timer")
	FColor TextColor = FColor::White;

	/** 타이머 시작(이미 실행 중이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StartTimer();

	/** 타이머 정지(경과 시간은 유지) */
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StopTimer();

	/** 경과 시간을 00:00:00 으로 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void ResetTimer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 화면에 표시되는 3D 텍스트 컴포넌트(루트) */
	UPROPERTY(VisibleAnywhere, Category = "Timer")
	TObjectPtr<UTextRenderComponent> TimerText;

private:
	/** 경과 시간(초) */
	float ElapsedSeconds = 0.0f;

	bool bRunning = false;

	/** 마지막으로 표시한 초(불필요한 SetText 호출 방지용) */
	int32 LastDisplayedSeconds = -1;

	void UpdateDisplayedText(bool bForce = false);
};
