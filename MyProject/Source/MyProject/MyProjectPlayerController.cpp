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
#include "CountdownWidget.h"
#include "ResultWidget.h"
#include "MyProjectCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
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

	// 게임플레이 맵(로비가 아닌 곳)에서만 대기시간 카운트다운 UI를 띄운다.
	if (IsLocalPlayerController() && CountdownWidgetClass)
	{
		if (const ADaePoGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADaePoGameState>() : nullptr)
		{
			if (!GS->bIsLobbyLevel)
			{
				if (UCountdownWidget* CountdownWidget = CreateWidget<UCountdownWidget>(this, CountdownWidgetClass))
				{
					CountdownWidget->AddToViewport();
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
#if !UE_BUILD_SHIPPING
			InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AMyProjectPlayerController::ToggleDebugInvulnerable);
			InputComponent->BindKey(EKeys::O, IE_Pressed, this, &AMyProjectPlayerController::ToggleFloorDebug);
#endif
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

void AMyProjectPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

#if !UE_BUILD_SHIPPING
	const ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	if (!bShowFloorDebug || !GEngine || !MyCharacter)
	{
		return;
	}

	// 캐릭터 이동 컴포넌트가 "지금 서 있다"고 판단한 바닥 = 실제로 밟고 있는 물체.
	const UCharacterMovementComponent* Movement = MyCharacter->GetCharacterMovement();
	const FFindFloorResult& Floor = Movement->CurrentFloor;
	const FHitResult& Hit = Floor.HitResult;
	const AActor* HitActor = Hit.GetActor();
	const UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Hit.GetComponent());

	FString ActorName = GetNameSafe(HitActor);
#if WITH_EDITOR
	if (HitActor)
	{
		ActorName = HitActor->GetActorLabel(); // 아웃라이너에 보이는 이름
	}
#endif

	// 같은 키(9001)로 매 프레임 덮어써서 화면 위에 한 줄로 계속 갱신된다.
	GEngine->AddOnScreenDebugMessage(9001, 0.0f, FColor::Yellow, FString::Printf(
		TEXT("[밟은 물체] 액터: %s | 컴포넌트: %s | 메시: %s | 이동: %s | 걸을 수 있음: %s | 떠 있는 거리: %.1fcm"),
		*ActorName, *GetNameSafe(Hit.GetComponent()), MeshComp ? *GetNameSafe(MeshComp->GetStaticMesh()) : TEXT("-"),
		*Movement->GetMovementName(), Floor.IsWalkableFloor() ? TEXT("O") : TEXT("X"), Floor.FloorDist));

	if (Hit.bBlockingHit)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 8.0f, 8, FColor::Red);
	}
#endif
}

void AMyProjectPlayerController::ToggleDebugInvulnerable()
{
	// shortcut: 서버에 있는 폰만 바뀌므로 싱글플레이/호스트 전용, 원격 클라이언트에서는 효과 없음.
	if (AMyProjectCharacter* MyCharacter = Cast<AMyProjectCharacter>(GetPawn()))
	{
		MyCharacter->SetInvulnerable(!MyCharacter->IsInvulnerable());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, MyCharacter->IsInvulnerable() ? TEXT("무적 ON") : TEXT("무적 OFF"));
		}
	}
}

void AMyProjectPlayerController::ClientShowResult_Implementation(bool bWon)
{
	if (UResultWidget* Widget = CreateWidget<UResultWidget>(this, UResultWidget::StaticClass()))
	{
		Widget->bWon = bWon;
		Widget->AddToViewport(20); // ESC 메뉴보다 위에
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
	}
}

bool AMyProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
