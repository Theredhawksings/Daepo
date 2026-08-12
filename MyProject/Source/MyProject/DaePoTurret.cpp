// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTurret.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "MyProjectCharacter.h"

ADaePoTurret::ADaePoTurret()
{
	// 추적 대포 전용 메시로 교체(나머지 설정은 부모 생성자가 이미 해둠)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TurretMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/Fab/Non-realistic_Cannon/non_realistic_cannon/StaticMeshes/non_realistic_cannon1.non_realistic_cannon1'"));
	if (TurretMeshAsset.Succeeded() && CannonMesh)
	{
		CannonMesh->SetStaticMesh(TurretMeshAsset.Object);
	}

	// 추적 대포는 조준이 생명이므로 퍼짐/속도 무작위를 줄여 명중률을 높인다.
	SpreadAngle = 2.0f;
	SpeedRandomRange = 100.0f;
}

void ADaePoTurret::BeginPlay()
{
	Super::BeginPlay();

	// 추적 회전을 위해 항상 틱 사용(미리보기는 제외)
	if (!bPreviewMode)
	{
		SetActorTickEnabled(true);
	}
}

APawn* ADaePoTurret::FindPlayerInRange() const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return nullptr;
	}

	// 죽은 플레이어는 조준하지 않는다
	if (const AMyProjectCharacter* Player = Cast<AMyProjectCharacter>(PlayerPawn))
	{
		if (Player->IsDead())
		{
			return nullptr;
		}
	}

	const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
	return (Dist <= DetectionRange) ? PlayerPawn : nullptr;
}

void ADaePoTurret::Tick(float DeltaTime)
{
	// 부모 틱: 반동 복귀 + (옵션) 무작위 이동/회전 처리
	Super::Tick(DeltaTime);

	if (bPreviewMode)
	{
		return;
	}

	// 감지 범위 표시(개발용). Shipping 빌드에서는 자동 제외된다.
	if (bShowDetectionRange)
	{
		DrawDebugCircle(GetWorld(), GetActorLocation(), DetectionRange, 32,
			bTargetInRange ? FColor::Red : FColor::Cyan, false, -1.0f, 0, 2.0f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	APawn* Target = FindPlayerInRange();
	bTargetInRange = (Target != nullptr);
	if (!Target)
	{
		return;
	}

	// 플레이어 방향(Yaw)으로 부드럽게 회전
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float TargetYaw = ToTarget.Rotation().Yaw;

	FRotator Rot = GetActorRotation();
	Rot.Yaw = FMath::FixedTurn(Rot.Yaw, TargetYaw, TrackRotateSpeed * DeltaTime);
	SetActorRotation(Rot);
}

void ADaePoTurret::Fire()
{
	// 범위 밖이면 쏘지 않고 다음 기회만 예약한다.
	if (!bTargetInRange)
	{
		ScheduleNextShot();
		return;
	}

	// 범위 안: 부모의 발사 로직 그대로(풀·무작위·사운드·반동 전부 동일)
	Super::Fire();
}
