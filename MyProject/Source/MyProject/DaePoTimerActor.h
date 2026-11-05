// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoTimerActor.generated.h"

class UTextRenderComponent;

/** shortcut: 생존 타이머는 폐지됨, 맵에 놓인 액터를 에디터에서 지운 뒤 이 클래스도 삭제한다. */
UCLASS()
class MYPROJECT_API ADaePoTimerActor : public AActor
{
	GENERATED_BODY()

public:
	ADaePoTimerActor();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Timer")
	TObjectPtr<UTextRenderComponent> TimerText;
};
