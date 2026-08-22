// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/**
 * 대기실(로비) 전용 게임모드. 기존 게임플레이용 캐릭터/컨트롤러 블루프린트를 그대로 재사용해서
 * 입력/이동/플레이어 색상 시스템이 이미 다 되어 있는 상태로 대기실에서 돌아다닐 수 있게 한다.
 * abstract 로 표시하지 않아 블루프린트 없이 World Settings 에서 바로 선택 가능하다.
 */
UCLASS()
class MYPROJECT_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	/** 방장이 "Start Game"을 누르면 이동할 실제 게임 맵 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	FName GameplayMapName = FName(TEXT("Lvl_ThirdPerson"));

protected:
	virtual void BeginPlay() override;
};
