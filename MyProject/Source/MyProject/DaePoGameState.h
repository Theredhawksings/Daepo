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
	/**
	 * true 가 되기 전까지는 대포가 발사하지 않는다.
	 * 호스트하자마자 바로 시작되면 다른 사람이 합류할 시간이 없어서, 서버가
	 * 일정 대기시간이 지난 뒤 한 번만 이 값을 true 로 바꾼다(그 결과가 복제됨).
	 */
	UPROPERTY(Replicated)
	bool bGameStarted = false;

	/**
	 * 게임이 시작되기까지 남은 대기시간(초). 서버가 매 틱 갱신한다(bGameStarted 가 true가
	 * 되면 더 이상 갱신되지 않고 0으로 취급됨). 대기실 카운트다운 표시용.
	 */
	UPROPERTY(Replicated)
	float ReplicatedPreGameRemaining = 0.0f;

	/**
	 * 지금이 대기실(로비) 레벨인지 여부. 로비 게임모드가 BeginPlay 에서 한 번 켜서 복제한다.
	 * 클라이언트의 PlayerController 가 이 값을 보고 대기실 UI를 띄울지 판단한다.
	 */
	UPROPERTY(Replicated)
	bool bIsLobbyLevel = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
