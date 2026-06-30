// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePo.h"
#include "DaePoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

ADaePo::ADaePo()
{
	// 대포 본체는 움직이지 않으므로 액터 틱 비활성(최적화).
	PrimaryActorTick.bCanEverTick = false;

	// --- 대포 본체 메시 (루트) ---
	CannonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CannonMesh"));
	RootComponent = CannonMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CannonMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/Fab/Non-realistic_Cannon/non_realistic_cannon/StaticMeshes/non_realistic_cannon.non_realistic_cannon'"));
	if (CannonMeshAsset.Succeeded())
	{
		CannonMesh->SetStaticMesh(CannonMeshAsset.Object);
	}
	CannonMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.05f));

	// --- 발사 위치/방향 (에디터에서 옮겨 지정) ---
	MuzzleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleArrow"));
	MuzzleArrow->SetupAttachment(CannonMesh);
	MuzzleArrow->SetRelativeLocation(FVector(100.0f, 0.0f, 0.0f)); // 포구 앞쪽 기본값
	MuzzleArrow->ArrowSize = 1.5f;
	MuzzleArrow->SetHiddenInGame(true); // 화살표는 에디터에서만 보이게

	// 기본 발사체 클래스 지정
	ProjectileClass = ADaePoProjectile::StaticClass();
}

void ADaePo::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return;
	}

	// --- 오브젝트 풀 사전 생성(최적화: 런타임에 Spawn/Destroy 반복 회피) ---
	ProjectilePool.Reserve(PoolSize);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < PoolSize; ++i)
	{
		ADaePoProjectile* Proj = World->SpawnActor<ADaePoProjectile>(
			ProjectileClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (Proj)
		{
			Proj->Deactivate(); // 숨긴 채 대기
			ProjectilePool.Add(Proj);
		}
	}

	// --- 주기적 발사 타이머 ---
	World->GetTimerManager().SetTimer(FireTimerHandle, this, &ADaePo::Fire, FireInterval, true, FireInterval);
}

void ADaePo::Fire()
{
	ADaePoProjectile* Proj = GetPooledProjectile();
	if (!Proj || !MuzzleArrow)
	{
		return;
	}

	const FVector SpawnLoc = MuzzleArrow->GetComponentLocation();
	const FVector BaseDir = MuzzleArrow->GetForwardVector();

	// 방향에 약간의 퍼짐(콘) 적용
	const FVector ShotDir = (SpreadAngle > 0.0f)
		? FMath::VRandCone(BaseDir, FMath::DegreesToRadians(SpreadAngle))
		: BaseDir;

	// 속도에 ±무작위 폭 적용(음수 방지)
	const float ShotSpeed = FMath::Max(0.0f, MuzzleSpeed + FMath::FRandRange(-SpeedRandomRange, SpeedRandomRange));

	Proj->Launch(SpawnLoc, ShotDir, ShotSpeed, ProjectileLifeTime, ProjectileScale);
}

ADaePoProjectile* ADaePo::GetPooledProjectile()
{
	// 비활성 발사체 재사용
	for (const TObjectPtr<ADaePoProjectile>& Proj : ProjectilePool)
	{
		if (Proj && !Proj->IsInUse())
		{
			return Proj;
		}
	}

	// 풀이 전부 사용 중이면 하나 동적 확장(이후 재사용됨)
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ADaePoProjectile* Proj = World->SpawnActor<ADaePoProjectile>(
			ProjectileClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (Proj)
		{
			ProjectilePool.Add(Proj);
			return Proj;
		}
	}
	return nullptr;
}
