// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTurret.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "MyProjectCharacter.h"
#include "Net/UnrealNetwork.h"

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

	// 터렛은 매 프레임 플레이어 감지/조준을 해야 하므로, 반동이 다 회복돼도
	// 부모가 틱을 자동으로 끄지 않게 한다(안 그러면 첫 발사 후 감지 로직 자체가 멈춘다).
	bAlwaysTick = true;
}

void ADaePoTurret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADaePoTurret, bTargetInRange);
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
	// 접속한 모든 플레이어 중 사거리 안의 가장 가까운(살아있는) 폰을 찾는다.
	return FindNearestPlayerInRange(DetectionRange);
}

void ADaePoTurret::Tick(float DeltaTime)
{
	// 부모 틱: 반동 복귀 + (옵션) 무작위 이동/회전 처리
	Super::Tick(DeltaTime);

	if (bPreviewMode)
	{
		return;
	}

	// 감지 범위 표시(개발용, Shipping 빌드에서는 자동 제외됨)는 순수 시각 정보이므로
	// 서버/클라이언트 모두에서 그린다. bTargetInRange 가 복제되므로 색도 양쪽에서 정확하다.
	if (bShowDetectionRange)
	{
		DrawDebugCircle(GetWorld(), GetActorLocation(), DetectionRange, 32,
			bTargetInRange ? FColor::Red : FColor::Cyan, false, -1.0f, 0, 2.0f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	// 조준 회전(및 아래의 대상 판정)은 서버만 계산한다(대포 이동/회전과 같은 이유).
	// 클라이언트는 ReplicatedYaw/bTargetInRange 로 복제된 값을 그대로 받아 보여준다.
	if (!HasAuthority())
	{
		return;
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

	// 부모 Tick 이후 여기서 추가로 돈 만큼, 최종 각도를 다시 복제 프로퍼티에 반영한다.
	SyncReplicatedTransform();
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
