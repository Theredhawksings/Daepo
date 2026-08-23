// Copyright Epic Games, Inc. All Rights Reserved.


#include "MyProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MyProject.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "LobbyWidget.h"
#include "DaePoGameState.h"
#include "PauseMenuWidget.h"
#include "InputCoreTypes.h"

void AMyProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 메인 메뉴(UI 전용 입력 모드 + 마우스 커서 표시)를 거쳐 이 맵으로 넘어왔을 수 있으므로,
	// 확실하게 게임 입력 모드로 되돌리고 커서를 숨긴다. 안 하면 WASD 가 게임으로 전달되지 않는다.
	if (IsLocalPlayerController())
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}

	// 대기실 레벨이면(GameState.bIsLobbyLevel) 대기실 UI를 띄운다. 게임플레이 맵에서는
	// 이 플래그가 false 라 LobbyWidgetClass 가 지정돼 있어도 자동으로 안 뜬다.
	if (IsLocalPlayerController() && LobbyWidgetClass)
	{
		if (const ADaePoGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADaePoGameState>() : nullptr)
		{
			if (GS->bIsLobbyLevel)
			{
				LobbyWidget = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
				if (LobbyWidget)
				{
					bShowMouseCursor = true;
					SetInputMode(FInputModeGameAndUI());
					LobbyWidget->AddToViewport();
				}
			}
		}
	}

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMyProject, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMyProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// ESC 는 입력 매핑 에셋 없이 클래식 방식으로 바로 바인딩한다(간단한 시스템 키라 충분함).
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMyProjectPlayerController::TogglePauseMenu);
		}
	}
}

void AMyProjectPlayerController::TogglePauseMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 이미 떠 있으면 닫고 원래 게임 입력으로 되돌린다.
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;

		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		return;
	}

	// 없으면 새로 띄우고 마우스를 보이게 한다. 클래스가 지정 안 돼 있으면 아무 일도 안 한다.
	if (!PauseMenuWidgetClass)
	{
		return;
	}

	PauseMenuWidget = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
	if (PauseMenuWidget)
	{
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
		PauseMenuWidget->AddToViewport(10); // 대기실 UI 등보다 위에 그려지도록 ZOrder 를 높게
	}
}

bool AMyProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
