// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Widgets/Colors/SColorBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "MyProject.h"

AMyProjectCharacter::AMyProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 플레이어 구분용 색깔 표시등. 머리 위(캡슐보다 살짝 위)에 배치.
	// 재질을 전혀 안 건드리고 순번별 색을 낼 수 있어 Substrate 머티리얼 그래프 작업이 필요 없다.
	PlayerColorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerColorLight"));
	PlayerColorLight->SetupAttachment(RootComponent);
	PlayerColorLight->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	PlayerColorLight->SetMobility(EComponentMobility::Movable);
	PlayerColorLight->Intensity = 2000.0f;
	PlayerColorLight->AttenuationRadius = 200.0f;
	PlayerColorLight->SourceRadius = 5.0f;
	PlayerColorLight->CastShadows = false;
	PlayerColorLight->SetLightColor(FLinearColor::White); // 순번을 알기 전 기본값. BeginPlay 에서 갱신됨.

	// 색깔 마커: 캐릭터 머리 위에 붙는 3D 오브젝트(World Space). UMG/Slate 콘텐츠라
	// 화면 공간 모드처럼 위치가 어긋나지 않고, 씬 조명의 영향도 받지 않아 항상 진하게 보인다.
	PlayerColorMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("PlayerColorMarker"));
	PlayerColorMarker->SetupAttachment(RootComponent);
	PlayerColorMarker->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	PlayerColorMarker->SetWidgetSpace(EWidgetSpace::World);
	PlayerColorMarker->SetDrawAtDesiredSize(false);
	PlayerColorMarker->SetDrawSize(FVector2D(20.0f, 20.0f));
	PlayerColorMarker->SetWorldScale3D(FVector(0.5f)); // World Space 는 실제 크기 단위라 축소해서 적당한 크기로
	PlayerColorMarker->SetSlateWidget(SNew(SColorBlock).Color(FLinearColor::White));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 방향별 사망 애니 기본값 (템플릿에 포함된 마네퀸 사망 애니)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathFrontAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (DeathFrontAsset.Succeeded()) { DeathAnimFront = DeathFrontAsset.Object; }

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathBackAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	if (DeathBackAsset.Succeeded()) { DeathAnimBack = DeathBackAsset.Object; }

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathLeftAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"));
	if (DeathLeftAsset.Succeeded()) { DeathAnimLeft = DeathLeftAsset.Object; }

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathRightAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01"));
	if (DeathRightAsset.Succeeded()) { DeathAnimRight = DeathRightAsset.Object; }
}

void AMyProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Look);
	}
	else
	{
		UE_LOG(LogMyProject, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMyProjectCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMyProjectCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMyProjectCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMyProjectCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMyProjectCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMyProjectCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMyProjectCharacter::ApplyDamageFromLocation(float DamageAmount, const FVector& SourceLocation)
{
	// 사망 시 쓰러질 방향을 알 수 있도록 피해 위치를 기록해 두고 일반 피해 처리로 넘긴다.
	LastDamageSourceLocation = SourceLocation;
	bHasDamageSource = true;
	ApplyDamage(DamageAmount);
}

void AMyProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 서버 자신이나(로컬 플레이어) PlayerState 가 이미 준비된 경우를 위해 여기서 한 번 시도.
	// 원격 클라이언트는 이 시점에 아직 PlayerState 복제가 안 왔을 수 있어 OnRep_PlayerState 에서 다시 시도한다.
	UpdatePlayerColor();
}

void AMyProjectCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 원격 클라이언트에서 PlayerState 복제가 도착한 시점 — 이제야 내 순번을 알 수 있다.
	UpdatePlayerColor();
}

void AMyProjectCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버 기준: BeginPlay 는 Possess() 보다 먼저 실행되어 그때는 PlayerState 가 아직
	// 없을 수 있다. 여기(Possess 시점)서 다시 시도해야 서버 화면에서도 색이 확실히 반영된다.
	UpdatePlayerColor();
}

void AMyProjectCharacter::UpdatePlayerColor()
{
	const APlayerState* PS = GetPlayerState();
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!PS || !GS || PlayerColors.Num() == 0)
	{
		return;
	}

	const int32 Index = GS->PlayerArray.IndexOfByKey(PS);
	if (Index == INDEX_NONE)
	{
		return;
	}

	const FLinearColor Color = PlayerColors[Index % PlayerColors.Num()];

	if (PlayerColorLight)
	{
		PlayerColorLight->SetLightColor(Color);
	}

	// 화면 고정 마커는 씬 조명과 무관하게 항상 이 원색 그대로 보인다(주 식별 수단).
	if (PlayerColorMarker)
	{
		PlayerColorMarker->SetSlateWidget(SNew(SColorBlock).Color(Color));
	}
}

void AMyProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyProjectCharacter, Health);
}

void AMyProjectCharacter::OnRep_Health(float OldHealth)
{
	// 서버가 이미 계산을 끝낸 결과가 도착한 것. 여기서는 화면 반응만 한다.
	OnHealthChanged(Health, Health - OldHealth);

	if (Health <= 0.0f)
	{
		HandleDeath();
	}
}

void AMyProjectCharacter::ClientShowDamageMessage_Implementation(const FString& PlayerLabel, float ActualDamage, float NewHealth)
{
	// Client RPC 라서 이 함수는 이 폰을 소유한 클라이언트에서만 실행된다.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
			FString::Printf(TEXT("[%s] 피해량: -%.0f (체력 %.0f)"), *PlayerLabel, ActualDamage, NewHealth));
	}
}

void AMyProjectCharacter::ApplyDamage(float DamageAmount)
{
	// 체력 계산(권위 있는 값)은 서버만 한다. 결과는 Health 복제로 모든 클라이언트에 자동 전달된다.
	if (!HasAuthority())
	{
		return;
	}

	if (bIsDead || bInvulnerable || DamageAmount <= 0.0f)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	const float ActualDamage = OldHealth - Health;

	// GameState 의 플레이어 목록에서 내 순번을 찾아 "Player1", "Player2" 식으로 표시한다.
	// (서버 로그 확인용 — 화면 메시지는 아래에서 각 플레이어 본인 화면에만 따로 띄운다)
	FString PlayerLabel = TEXT("Player?");
	if (const APlayerState* PS = GetPlayerState())
	{
		if (const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
		{
			const int32 Index = GS->PlayerArray.IndexOfByKey(PS);
			if (Index != INDEX_NONE)
			{
				PlayerLabel = FString::Printf(TEXT("Player%d"), Index + 1);
			}
		}
	}

	UE_LOG(LogMyProject, Log, TEXT("%s 피해량: -%.1f (체력 %.1f -> %.1f)"), *PlayerLabel, ActualDamage, OldHealth, Health);

	// 화면 메시지는 이 캐릭터를 소유한 클라이언트에서만 실행되므로, 맞은 당사자 화면에만 뜬다.
	ClientShowDamageMessage(PlayerLabel, ActualDamage, Health);

	OnHealthChanged(Health, Health - OldHealth);

	if (Health <= 0.0f)
	{
		HandleDeath();
	}
}

void AMyProjectCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	// 더 이상 움직이거나 조작할 수 없게 막는다.
	// (움직임 정지는 각 머신에서 로컬로도 안전하게 반복 호출 가능. 컨트롤러 입력 차단은
	//  이 폰을 소유한 머신에서만 실제로 의미가 있고, 그 외에서는 GetController() 가 null 이라 자동으로 무시된다.)
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// 쓰러지는 모습은 모두의 화면에 보여야 하므로 애니메이션은 항상 재생한다.
	// (서버에서 한 번, 그리고 이 폰을 보고 있는 각 클라이언트에서 Health 복제로 OnRep_Health 가
	//  호출될 때 한 번, 총 두 경로로 실행되어 모든 머신의 화면에서 재생된다.)
	if (UAnimSequence* DeathAnim = PickDeathAnim())
	{
		GetMesh()->PlayAnimation(DeathAnim, false);
	}

	// "GAME OVER" 표시와 사망 이벤트는 실제로 그 폰을 조작하던 화면에서만 띄운다.
	// (다른 플레이어가 죽는 걸 구경하는 화면에는 내가 죽은 게 아니므로 띄우지 않는다)
	if (IsLocallyControlled())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("GAME OVER"));
		}
		OnDeath();
	}

	// --- 슬로모션 → 잠시 후 레벨 재시작. 서버만 발동시킨다. ---
	// TimeDilation 은 WorldSettings 를 통해 엔진이 자동으로 모든 클라이언트에 복제하므로,
	// 서버가 한 번만 걸면 모두의 화면이 함께 느려진다. 클라이언트가 각자 걸면 서로 다른
	// 배율로 따로 놀거나 서버 값에 곧바로 덮어써진다.
	if (HasAuthority())
	{
		UGameplayStatics::SetGlobalTimeDilation(this, DeathSlowMotionScale);

		// 타이머는 느려진 게임 시간으로 흐르므로, 실제 체감 시간(DeathRestartDelay)이 되도록 배율을 곱한다.
		const float TimerDelay = DeathRestartDelay * DeathSlowMotionScale;
		GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &AMyProjectCharacter::RestartLevel, TimerDelay, false);
	}
}

UAnimSequence* AMyProjectCharacter::PickDeathAnim() const
{
	// 피해 위치 정보가 없으면 기본(앞으로 쓰러짐)
	if (!bHasDamageSource)
	{
		return DeathAnimFront;
	}

	// 폭발 지점 → 캐릭터 로컬 좌표계 기준으로 어느 쪽에서 맞았는지 계산
	const FVector ToSource = (LastDamageSourceLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector LocalDir = GetActorTransform().InverseTransformVectorNoScale(ToSource);

	// 맞은 반대 방향으로 쓰러진다: 앞에서 맞으면 뒤로, 오른쪽에서 맞으면 왼쪽으로.
	if (FMath::Abs(LocalDir.X) >= FMath::Abs(LocalDir.Y))
	{
		return (LocalDir.X >= 0.0f) ? DeathAnimBack : DeathAnimFront;
	}
	return (LocalDir.Y >= 0.0f) ? DeathAnimLeft : DeathAnimRight;
}

void AMyProjectCharacter::RestartLevel()
{
	// 레벨 재시작은 서버만 실행한다. 서버가 OpenLevel 을 호출하면 접속해 있는 모든
	// 클라이언트가 함께 그 맵으로 이동한다(서버 트래블). 클라이언트가 각자 로컬로
	// OpenLevel 을 부르면 멀티플레이 세션에서 혼자 튕겨나가 딴 게임이 되어버린다.
	if (!HasAuthority())
	{
		return;
	}

	// 시간 배율을 원래대로 돌리고 현재 레벨을 처음부터 다시 연다.
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
