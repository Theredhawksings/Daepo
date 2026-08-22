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
	}
}

bool AMyProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
