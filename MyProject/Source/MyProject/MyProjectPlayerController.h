// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyProjectPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AMyProjectPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/**
	 * 대기실(로비) 레벨에서만 띄우는 UI(인원수 + 방장 전용 시작 버튼). 게임플레이 맵에서는
	 * GameState.bIsLobbyLevel 이 false 라 이 값이 설정돼 있어도 자동으로 안 뜬다.
	 */
	UPROPERTY(EditAnywhere, Category = "Lobby")
	TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

	/** 스폰된 대기실 위젯(있다면) */
	UPROPERTY()
	TObjectPtr<class ULobbyWidget> LobbyWidget;

	/** ESC 를 누르면 뜨는 투명 종료 메뉴. 비워두면 ESC 를 눌러도 아무 일도 안 일어난다. */
	UPROPERTY(EditAnywhere, Category = "Pause")
	TSubclassOf<class UPauseMenuWidget> PauseMenuWidgetClass;

	/** 현재 떠 있는 종료 메뉴 위젯(없으면 null) */
	UPROPERTY()
	TObjectPtr<class UPauseMenuWidget> PauseMenuWidget;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** ESC 키 콜백: 메뉴가 떠 있으면 닫고, 없으면 새로 띄운다 */
	void TogglePauseMenu();

};
