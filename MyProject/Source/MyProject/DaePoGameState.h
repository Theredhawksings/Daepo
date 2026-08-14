// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DaePoGameState.generated.h"

/**
 * 생존 경과 시간을 모든 클라이언트 화면에 보여주기 위한 게임 스테이트.
 * GameMode 는 서버에만 존재해 클라이언트가 직접 값을 읽을 수 없으므로,
 * 서버가 계산한 경과 시간을 여기 복제해서 타이머 디스플레이가 서버/클라이언트 모두에서 똑같이 보이게 한다.
 */
UCLASS()
class MYPROJECT_API ADaePoGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** 서버가 매 틱 갱신하는 생존 경과 시간(초) */
	UPROPERTY(Replicated)
	float ReplicatedElapsedSeconds = 0.0f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
