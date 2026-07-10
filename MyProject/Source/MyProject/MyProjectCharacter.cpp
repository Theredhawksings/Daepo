// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "DaePo.h"
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

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
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

		// Build (설치) 모드
		if (ToggleBuildAction)
		{
			EnhancedInputComponent->BindAction(ToggleBuildAction, ETriggerEvent::Started, this, &AMyProjectCharacter::ToggleBuildMode);
		}
		if (PlaceBuildAction)
		{
			EnhancedInputComponent->BindAction(PlaceBuildAction, ETriggerEvent::Started, this, &AMyProjectCharacter::ConfirmPlacement);
		}
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

void AMyProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 빌드 모드일 때 미리보기 대포를 조준 지점으로 계속 이동시킨다.
	if (bInBuildMode && PreviewCannon)
	{
		FTransform PlaceTM;
		if (GetPlacementTransform(PlaceTM))
		{
			const FVector PlaceLoc = PlaceTM.GetLocation();
			PreviewCannon->SetActorHiddenInGame(false);
			PreviewCannon->SetActorLocationAndRotation(PlaceLoc, PlaceTM.GetRotation());

			// 겹침 검사 → 상태가 바뀔 때만 미리보기 색 전환(유효=파랑, 불가=빨강).
			const bool bValid = IsPlacementValid(PlaceLoc);
			if (bValid != bCanPlaceHere)
			{
				bCanPlaceHere = bValid;
				UMaterialInterface* Mat = bValid ? PreviewMaterial : InvalidPreviewMaterial;
				if (Mat)
				{
					PreviewCannon->SetPreviewMaterial(Mat);
				}
			}
		}
		else
		{
			// 유효한 지면이 없으면 잠시 숨긴다.
			PreviewCannon->SetActorHiddenInGame(true);
			bCanPlaceHere = false;
		}
	}
}

bool AMyProjectCharacter::IsPlacementValid(const FVector& Location) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 이미 배치된 대포들과의 수평 거리를 검사. 미리보기 자신은 제외.
	for (TActorIterator<ADaePo> It(World); It; ++It)
	{
		const ADaePo* Other = *It;
		if (!Other || Other == PreviewCannon)
		{
			continue;
		}

		if (FVector::Dist2D(Other->GetActorLocation(), Location) < PlacementSpacing)
		{
			return false;
		}
	}

	return true;
}

bool AMyProjectCharacter::GetPlacementTransform(FTransform& OutTransform) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return false;
	}

	// 카메라 시점에서 정면으로 라인트레이스하여 지면/벽 위치를 찾는다.
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector Start = CamLoc;
	const FVector End = Start + CamRot.Vector() * BuildTraceDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (PreviewCannon)
	{
		Params.AddIgnoredActor(PreviewCannon);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return false;
	}

	// 위치는 충돌 지점, 회전은 캐릭터가 보는 방향(Yaw)만 사용해 수평 유지.
	const FRotator PlaceRot(0.0f, GetActorRotation().Yaw, 0.0f);
	OutTransform = FTransform(PlaceRot, Hit.Location);
	return true;
}

void AMyProjectCharacter::ToggleBuildMode()
{
	// 이미 빌드 모드면 종료(미리보기 제거).
	if (bInBuildMode)
	{
		bInBuildMode = false;
		if (PreviewCannon)
		{
			PreviewCannon->Destroy();
			PreviewCannon = nullptr;
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !CannonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("빌드 모드 실패: CannonClass 가 지정되지 않았습니다."));
		return;
	}

	bInBuildMode = true;

	// 초기 위치를 구해 미리보기 대포를 지연 스폰(발사 타이머 전에 프리뷰 설정).
	FTransform SpawnTM = FTransform::Identity;
	GetPlacementTransform(SpawnTM);

	ADaePo* Preview = World->SpawnActorDeferred<ADaePo>(
		CannonClass, SpawnTM, this, GetInstigator(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Preview)
	{
		Preview->SetPreviewMode(true, PreviewMaterial);
		Preview->FinishSpawning(SpawnTM);
		PreviewCannon = Preview;
	}
}

void AMyProjectCharacter::ConfirmPlacement()
{
	// 빌드 모드에서만, 유효한 지면이 있을 때만 설치.
	if (!bInBuildMode || !CannonClass)
	{
		return;
	}

	FTransform PlaceTM;
	if (!GetPlacementTransform(PlaceTM))
	{
		return;
	}

	// 다른 대포와 너무 가까우면 설치 취소.
	if (!IsPlacementValid(PlaceTM.GetLocation()))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("설치 불가: 다른 대포와 너무 가깝습니다."));
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 실제(발사하는) 대포 설치. 지면과 겹쳐도 스폰되도록 조정.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World->SpawnActor<ADaePo>(CannonClass, PlaceTM, SpawnParams);

	// 빌드 모드는 유지하여 연속 설치 가능(E 로 종료).
}
