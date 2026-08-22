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

	/** 방장이 Start 버튼을 누르면(또는 디버그 자동 시작 시간이 지나면) 실제 게임 맵으로 전원을 데리고 이동한다 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void StartGame();

	/**
	 * 테스트/디버그용: 0보다 크면 아무도 안 눌러도 이 시간(초) 뒤 자동으로 StartGame() 을 호출한다.
	 * 실제 서비스에서는 0으로 두고 Start 버튼으로만 시작하게 한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	float DebugAutoStartAfterSeconds = 0.0f;

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle DebugAutoStartTimerHandle;
};
