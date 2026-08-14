// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoMortar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "MyProjectCharacter.h"

ADaePoMortar::ADaePoMortar()
{
	// --- 루트를 씬 컴포넌트로 교체 ---
	// 메시에 회전(-90 Roll)/스케일을 박아도 액터 배치 회전과 섞이지 않도록,
	// 메시를 루트가 아닌 자식으로 내린다.
	USceneComponent* MortarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MortarRoot"));
	SetRootComponent(MortarRoot);

	// --- 박격포 메시 (실측 배치값: Roll -90, 스케일 0.7 / 0.5 / 0.7) ---
	CannonMesh->SetupAttachment(MortarRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MortarMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/Fab/Wall_Cannon/gamecannon/StaticMeshes/gamecannon.gamecannon'"));
	if (MortarMeshAsset.Succeeded() && CannonMesh)
	{
		CannonMesh->SetStaticMesh(MortarMeshAsset.Object);
	}
	CannonMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f)); // (Pitch, Yaw, Roll)
	CannonMesh->SetRelativeScale3D(FVector(0.7f, 0.5f, 0.7f));

	// 포구는 루트에 붙이고, 정확한 위치는 BeginPlay 에서 메시 크기로 계산한다.
	MuzzleArrow->SetupAttachment(MortarRoot);
	MuzzleArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	MuzzleArrow->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); // 위쪽(곡사)

	// 곡사에 맞는 기본값: 발사 간격은 길게, 수명은 넉넉하게.
	FireInterval = 2.5f;
	FireIntervalRandomRange = 0.5f;
	ProjectileLifeTime = 8.0f;
}

void ADaePoMortar::BeginPlay()
{
	Super::BeginPlay();

	// --- 발사 위치 자동 계산 ---
	// 메시가 회전/스케일된 상태의 실제(월드) 바운드를 읽어,
	// 포구를 메시 꼭대기 중앙에서 MuzzleHeightOffset 만큼 위에 배치한다.
	if (CannonMesh && MuzzleArrow)
	{
		const FBoxSphereBounds Bounds = CannonMesh->Bounds;
		MuzzleArrow->SetWorldLocation(Bounds.Origin + FVector(0.0f, 0.0f, Bounds.BoxExtent.Z + MuzzleHeightOffset));
	}
}

void ADaePoMortar::Fire()
{
	// 이 오버라이드는 부모의 HasAuthority() 가드를 거치지 않고 통째로 대체하므로
	// 여기서 직접 서버 여부를 확인해야 한다. 안 그러면 클라이언트도 독자적으로
	// 각자의 목표/탄도를 계산해 서버와 다른 곳에 큐브가 떨어진다.
	if (!HasAuthority())
	{
		return;
	}

	// 발사 여부와 관계없이 체인 유지
	ScheduleNextShot();

	if (bPreviewMode || !MuzzleArrow)
	{
		return;
	}

	// --- 대상 확인: 접속한 모든 플레이어 중 사거리 안의 가장 가까운(살아있는) 플레이어 ---
	APawn* Player = FindNearestPlayerInRange(MortarRange);
	if (!Player)
	{
		return;
	}

	// --- 착탄 지점: 플레이어 위치 ± 무작위 오프셋 ---
	FVector Target = Player->GetActorLocation();
	if (LandingSpread > 0.0f)
	{
		const FVector2D Offset = FMath::RandPointInCircle(LandingSpread);
		Target += FVector(Offset.X, Offset.Y, 0.0f);
	}

	// --- 곡사 탄도 계산: 시작점→목표점을 잇는 포물선의 초기 속도를 엔진이 역산 ---
	const FVector Start = MuzzleArrow->GetComponentLocation();
	FVector LaunchVelocity;
	if (!UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this, LaunchVelocity, Start, Target, /*OverrideGravityZ=*/0.0f, ArcParam))
	{
		return; // 닿을 수 없는 위치(너무 멀거나 막힘)
	}

	LaunchProjectile(Start, LaunchVelocity);

	// --- 착탄 경고 원: 비행 시간 동안 바닥에 표시 ---
	if (bShowLandingWarning)
	{
		// 수평 거리 ÷ 수평 속도 = 대략적인 비행 시간
		const float HorizSpeed = FVector(LaunchVelocity.X, LaunchVelocity.Y, 0.0f).Size();
		const float FlightTime = (HorizSpeed > 1.0f)
			? FVector::Dist2D(Start, Target) / HorizSpeed
			: 1.0f;

		// 목표 지점에서 아래로 트레이스해 실제 바닥 높이에 원을 붙인다.
		FVector Center = Target;
		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByChannel(GroundHit,
				Target + FVector(0, 0, 100.0f), Target - FVector(0, 0, 2000.0f), ECC_Visibility))
		{
			Center = GroundHit.ImpactPoint + FVector(0, 0, 3.0f);
		}

		DrawDebugCircle(GetWorld(), Center, WarningRadius, 32, FColor::Red,
			false, FlightTime, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
}
