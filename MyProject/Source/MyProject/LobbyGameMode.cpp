// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "DaePoGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

ALobbyGameMode::ALobbyGameMode()
{
	// 경과 시간/로비 여부 등을 클라이언트에도 보여주기 위해 복제되는 게임 스테이트를 쓴다.
	GameStateClass = ADaePoGameState::StaticClass();

	// 기존 게임플레이용 캐릭터/컨트롤러를 그대로 재사용한다. 이미 입력 매핑, 이동, 플레이어
	// 색상 시스템이 다 되어 있어서 대기실에서도 그대로 돌아다니고 서로 색으로 구분할 수 있다.
	static ConstructorHelpers::FClassFinder<ACharacter> CharacterBP(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (CharacterBP.Succeeded())
	{
		DefaultPawnClass = CharacterBP.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (ControllerBP.Succeeded())
	{
		PlayerControllerClass = ControllerBP.Class;
	}

	// Start Game 시 접속자 전원이 끊기지 않고 같이 다음 맵으로 넘어가려면 필요하다.
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ADaePoGameState* GS = GetGameState<ADaePoGameState>())
	{
		GS->bIsLobbyLevel = true;
	}
}
