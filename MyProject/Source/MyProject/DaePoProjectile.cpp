// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

ADaePoProjectile::ADaePoProjectile()
{
	// 액터 틱은 끈다. 이동은 ProjectileMovementComponent 가 자체적으로 처리한다(최적화).
	PrimaryActorTick.bCanEverTick = false;

	// --- 큐브 메시 (루트, 충돌 담당) ---
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Engine/EngineMeshes/Cube.Cube'"));
	if (CubeMeshAsset.Succeeded())
	{
		MeshComp->SetStaticMesh(CubeMeshAsset.Object);
	}
	MeshComp->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.05f));

	// 충돌: 물리 시뮬레이션 대신 QueryOnly + Block 으로 가볍게 처리하고
	// 실제 튕김은 ProjectileMovementComponent 가 담당한다(최적화).
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	MeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	MeshComp->CanCharacterStepUpOn = ECB_No;
	MeshComp->SetCanEverAffectNavigation(false);

	// --- 발사체 이동 컴포넌트 ---
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = MeshComp;
	ProjectileMovement->bShouldBounce = true;          // 벽에 튕김
	ProjectileMovement->Bounciness = 0.6f;             // 반발 계수
	ProjectileMovement->Friction = 0.2f;               // 표면 마찰
	ProjectileMovement->ProjectileGravityScale = 1.0f; // 대포처럼 포물선 낙하
	ProjectileMovement->bAutoActivate = false;         // 발사 전까지 비활성(풀링)
}

void ADaePoProjectile::Launch(const FVector& InLocation, const FVector& Direction, float Speed, float LifeTime, const FVector& Scale)
{
	bInUse = true;

	const FVector ShotDir = Direction.GetSafeNormal();

	// 위치/회전/크기 배치
	SetActorLocation(InLocation, false, nullptr, ETeleportType::ResetPhysics);
	SetActorRotation(ShotDir.Rotation());
	SetActorScale3D(Scale); // 대포에서 지정한 발사체 크기 적용

	// 표시 및 충돌 켜기
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// 이동 활성화 + 초기 속도 부여
	ProjectileMovement->SetUpdatedComponent(MeshComp);
	ProjectileMovement->Velocity = ShotDir * Speed;
	ProjectileMovement->Activate(true);

	// 일정 시간 뒤 풀로 반환
	GetWorldTimerManager().SetTimer(LifeTimerHandle, this, &ADaePoProjectile::Deactivate, LifeTime, false);
}

void ADaePoProjectile::Deactivate()
{
	bInUse = false;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	// 이동 정지 및 비활성화(컴포넌트 틱 중단 -> 최적화)
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();

	// 숨기고 충돌 끄기
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}
