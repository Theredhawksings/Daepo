// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePo.h"
#include "DaePoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "MyProjectCharacter.h"
#include "Net/UnrealNetwork.h"

ADaePo::ADaePo()
{
	// 이 대포는 맵에 미리 배치되는 액터라 서버/클라이언트 모두에 이미 존재하지만,
	// 무작위 이동/회전/반동으로 인한 실제 위치 변화는 서버만 계산하고(HasAuthority 가드),
	// 그 결과 위치를 클라이언트에 복제해서 보여준다. 그래야 창마다 다르게 흔들리지 않는다.
	// 엔진 내장 이동 복제(ReplicatedMovement) 대신 직접 만든 프로퍼티(ReplicatedLocation/Yaw)로
	// 복제한다 — 부착 구조가 있는 박격포 등에서도 확실하게 동작하도록.
	SetReplicates(true);
	SetReplicateMovement(false);

	// 틱은 가능하게 두되 꺼진 채 시작한다.
	// bRandomMove 가 켜진 대포만 BeginPlay 에서 틱을 켠다(고정 대포는 비용 0 = 최적화).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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
	// MuzzleArrow 는 0.05 로 축소된 메시의 자식이라 상대값이 스케일만큼 줄어든다.
	// 실측으로 포구 끝이 되는 상대 위치(앞 3500, 위 3000)를 기본값으로 지정.
	MuzzleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleArrow"));
	MuzzleArrow->SetupAttachment(CannonMesh);
	MuzzleArrow->SetRelativeLocation(FVector(3500.0f, 0.0f, 3000.0f));
	MuzzleArrow->ArrowSize = 1.5f;
	MuzzleArrow->SetHiddenInGame(true); // 화살표는 에디터에서만 보이게

	// 기본 발사체 클래스 지정
	ProjectileClass = ADaePoProjectile::StaticClass();

	// 기본 발사음 지정
	static ConstructorHelpers::FObjectFinder<USoundBase> FireSoundAsset(
		TEXT("/Script/Engine.SoundWave'/Game/MP_Bomb.MP_Bomb'"));
	if (FireSoundAsset.Succeeded())
	{
		FireSound = FireSoundAsset.Object;
	}
}

USoundAttenuation* ADaePo::GetSoundAttenuation()
{
	// 에디터에서 지정한 감쇠 에셋이 있으면 그것을 우선 사용
	if (SoundAttenuationOverride)
	{
		return SoundAttenuationOverride;
	}

	// 없으면 반경/감쇠거리 값으로 한 번만 만들어 캐시
	if (!RuntimeAttenuation)
	{
		RuntimeAttenuation = NewObject<USoundAttenuation>(this);

		FSoundAttenuationSettings& Settings = RuntimeAttenuation->Attenuation;
		Settings.bAttenuate = true;   // 거리에 따라 볼륨 감소
		Settings.bSpatialize = true;  // 좌우 방향감 적용
		Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound; // 자연스러운 감쇠 곡선
		Settings.AttenuationShape = EAttenuationShape::Sphere;
		Settings.AttenuationShapeExtents = FVector(SoundInnerRadius, 0.0f, 0.0f);
		Settings.FalloffDistance = SoundFalloffDistance;
	}

	return RuntimeAttenuation;
}

void ADaePo::SetPreviewMode(bool bEnabled, UMaterialInterface* PreviewMat)
{
	bPreviewMode = bEnabled;

	if (!bEnabled)
	{
		return;
	}

	// 미리보기는 충돌하지 않게(설치 라인트레이스/캐릭터를 방해하지 않도록)
	SetActorEnableCollision(false);
	if (CannonMesh)
	{
		CannonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 파란 반투명 등 미리보기 재질을 모든 슬롯에 적용
		if (PreviewMat)
		{
			const int32 NumMats = CannonMesh->GetNumMaterials();
			for (int32 i = 0; i < NumMats; ++i)
			{
				CannonMesh->SetMaterial(i, PreviewMat);
			}
		}
	}
}

void ADaePo::SetPreviewMaterial(UMaterialInterface* Mat)
{
	if (!CannonMesh || !Mat)
	{
		return;
	}

	const int32 NumMats = CannonMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		CannonMesh->SetMaterial(i, Mat);
	}
}

void ADaePo::DrawFireTrajectory() const
{
	if (!MuzzleArrow)
	{
		return;
	}

	// 실제 발사와 같은 조건(포구 위치/방향, 포구 속도, 중력)으로 궤적을 시뮬레이션.
	// 무작위(퍼짐/속도)는 빼고 기준 궤적만 그린다.
	FPredictProjectilePathParams Params;
	Params.StartLocation = MuzzleArrow->GetComponentLocation();
	Params.LaunchVelocity = MuzzleArrow->GetForwardVector() * MuzzleSpeed;
	Params.bTraceWithCollision = true;              // 지형에 맞으면 거기서 궤적 종료
	Params.ProjectileRadius = 10.0f;
	Params.MaxSimTime = ProjectileLifeTime;
	Params.SimFrequency = 15.0f;                    // 시뮬레이션 정밀도(초당 샘플 수)
	Params.TraceChannel = ECC_Visibility;
	Params.ActorsToIgnore.Add(const_cast<ADaePo*>(this));

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(this, Params, Result);

	// 샘플 지점들을 이어 포물선을 그린다(한 프레임만 유지 → 매 틱 갱신).
	UWorld* World = GetWorld();
	const int32 NumPoints = Result.PathData.Num();
	for (int32 i = 1; i < NumPoints; ++i)
	{
		DrawDebugLine(World, Result.PathData[i - 1].Location, Result.PathData[i].Location,
			FColor::Yellow, false, -1.0f, 0, 2.0f);
	}

	// 예상 착탄 지점 표시
	if (Result.HitResult.bBlockingHit)
	{
		DrawDebugSphere(World, Result.HitResult.ImpactPoint, 25.0f, 12, FColor::Orange,
			false, -1.0f, 0, 2.0f);
	}
}

APawn* ADaePo::FindNearestPlayerInRange(float Range) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APawn* Best = nullptr;
	float BestDistSq = FMath::Square(Range);

	// GetPlayerPawn(0) 은 "이 머신의 로컬 플레이어"만 찾기 때문에, 서버에서 호출하면
	// 네트워크로 접속한 다른 클라이언트의 폰은 절대 찾지 못한다(호스트 자신만 보임).
	// 접속한 모든 플레이어 컨트롤러를 순회해야 멀티플레이어에서 제대로 동작한다.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		if (const AMyProjectCharacter* Character = Cast<AMyProjectCharacter>(Pawn))
		{
			if (Character->IsDead())
			{
				continue;
			}
		}

		const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Pawn;
		}
	}

	return Best;
}

void ADaePo::BeginPlay()
{
	Super::BeginPlay();

	// 미리보기(고스트)는 발사 타이머/풀을 만들지 않는다.
	if (bPreviewMode)
	{
		return;
	}

	// 발사 판정(언제/어디로 쏠지)은 서버만 한다. 클라이언트가 각자 계산하면
	// 서버와 클라이언트마다 다른 무작위값으로 큐브가 나가 화면이 서로 어긋난다.
	// 클라이언트에서는 풀도 만들지 않고 타이머도 걸지 않는다(발사체는 서버가 복제해서 보여준다).
	if (!HasAuthority())
	{
		return;
	}

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

	// --- 첫 발사 예약(이후 발사할 때마다 무작위 간격으로 다음 발사를 예약하는 체인) ---
	ScheduleNextShot();

	// --- 무작위 이동/회전 옵션이 켜져 있으면 틱 시작 ---
	if (bRandomMove || bRandomRotate)
	{
		WanderAnchor = GetActorLocation();          // 설치 지점을 기준점으로 저장
		WanderAnchorYaw = GetActorRotation().Yaw;   // 설치 방향을 기준 각도로 저장
		PickNewWanderTarget();
		SetActorTickEnabled(true);
	}
}

void ADaePo::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 실제 위치/회전을 바꾸는 계산(반동 복귀, 무작위 이동/회전)은 서버만 한다.
	// 클라이언트가 각자 계산하면 무작위값이 서로 달라 창마다 다르게 흔들린다.
	// 서버가 바꾼 결과는 ReplicatedLocation/ReplicatedYaw 로 복제되어 클라이언트에 반영된다.
	if (!HasAuthority())
	{
		return;
	}

	// --- 반동 복귀: 남은 반동량을 줄이며 그만큼 앞으로 되돌린다(증분 방식이라 이동/회전과 공존) ---
	if (CurrentRecoil > 0.0f)
	{
		const float NewRecoil = FMath::FInterpTo(CurrentRecoil, 0.0f, DeltaTime, RecoilRecoverSpeed);
		AddActorLocalOffset(FVector(CurrentRecoil - NewRecoil, 0.0f, 0.0f));
		CurrentRecoil = (NewRecoil < 0.1f) ? 0.0f : NewRecoil;

		// 반동만을 위해 틱이 켜졌던 고정 대포는 복귀가 끝나면 다시 틱을 끈다(최적화)
		if (CurrentRecoil <= 0.0f && !bRandomMove && !bRandomRotate)
		{
			SetActorTickEnabled(false);
		}
	}

	if ((!bRandomMove && !bRandomRotate) || bWanderPaused)
	{
		// 반동만 처리하고 끝나는 경우도 있으므로, 여기서도 최신 위치를 복제 프로퍼티에 반영해야 한다.
		SyncReplicatedTransform();
		return;
	}

	bool bArrived = true;

	// 목표 지점을 향해 일정 속도로 부드럽게 이동
	if (bRandomMove)
	{
		const FVector Next = FMath::VInterpConstantTo(GetActorLocation(), WanderTarget, DeltaTime, MoveSpeed);
		SetActorLocation(Next);
		bArrived &= FVector::DistSquared(Next, WanderTarget) < FMath::Square(5.0f);
	}

	// 목표 각도를 향해 일정 속도로 부드럽게 회전(Yaw만)
	if (bRandomRotate)
	{
		FRotator Rot = GetActorRotation();
		Rot.Yaw = FMath::FixedTurn(Rot.Yaw, WanderTargetYaw, RotateSpeed * DeltaTime);
		SetActorRotation(Rot);
		bArrived &= FMath::Abs(FMath::FindDeltaAngleDegrees(Rot.Yaw, WanderTargetYaw)) < 1.0f;
	}

	// 이동/회전이 모두 끝나면 잠시 쉬었다가 다음 목표를 뽑는다
	if (bArrived)
	{
		if (MovePauseTime > 0.0f)
		{
			bWanderPaused = true;
			GetWorldTimerManager().SetTimer(WanderPauseHandle, [this]()
			{
				bWanderPaused = false;
				PickNewWanderTarget();
			}, MovePauseTime, false);
		}
		else
		{
			PickNewWanderTarget();
		}
	}

	// 이번 프레임에 계산된 최종 위치/각도를 복제 프로퍼티에 반영한다.
	SyncReplicatedTransform();
}

void ADaePo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADaePo, ReplicatedLocation);
	DOREPLIFETIME(ADaePo, ReplicatedYaw);
}

void ADaePo::SyncReplicatedTransform()
{
	ReplicatedLocation = GetActorLocation();
	ReplicatedYaw = GetActorRotation().Yaw;
}

void ADaePo::OnRep_ReplicatedTransform()
{
	// 서버에서 온 최신 위치/각도를 그대로 반영한다(클라이언트는 스스로 계산하지 않음).
	SetActorLocationAndRotation(ReplicatedLocation, FRotator(0.0f, ReplicatedYaw, 0.0f));
}

void ADaePo::PickNewWanderTarget()
{
	// 기준점(설치 지점) 주변 원 안에서 무작위 지점을 뽑는다. 높이는 유지(수평 이동만).
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Dist = FMath::FRandRange(MoveRadius * 0.3f, MoveRadius);
	WanderTarget = WanderAnchor + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);

	// 설치 당시 방향 기준 ±RotateRange 안에서 새 목표 각도를 뽑는다.
	WanderTargetYaw = WanderAnchorYaw + FMath::FRandRange(-RotateRange, RotateRange);
}

void ADaePo::ScheduleNextShot()
{
	// 기본 간격 ± 무작위 폭. 너무 짧아지지 않게 최소 0.1초 보장.
	const float NextInterval = FMath::Max(0.1f,
		FireInterval + FMath::FRandRange(-FireIntervalRandomRange, FireIntervalRandomRange));
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ADaePo::Fire, NextInterval, false);
}

void ADaePo::Fire()
{
	// 서버가 아니면 절대 발사하지 않는다(이중 안전장치, BeginPlay 가드와 별개로).
	if (!HasAuthority())
	{
		return;
	}

	// 발사 성공 여부와 관계없이 다음 발사를 먼저 예약해 체인을 유지한다.
	ScheduleNextShot();

	if (!MuzzleArrow)
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

	LaunchProjectile(SpawnLoc, ShotDir * ShotSpeed);
}

void ADaePo::LaunchProjectile(const FVector& StartLoc, const FVector& Velocity)
{
	ADaePoProjectile* Proj = GetPooledProjectile();
	if (!Proj)
	{
		return;
	}

	Proj->Launch(StartLoc, Velocity.GetSafeNormal(), Velocity.Size(), ProjectileLifeTime, ProjectileScale);

	// 반동: 즉시 뒤로 밀리고, Tick 에서 서서히 원위치로 복귀
	if (bRecoil && RecoilDistance > 0.0f)
	{
		AddActorLocalOffset(FVector(-RecoilDistance, 0.0f, 0.0f));
		CurrentRecoil += RecoilDistance;
		SetActorTickEnabled(true);
	}

	// 발사음: 포구 위치에서 3D 재생. 매번 피치를 살짝 바꿔 반복감을 줄인다.
	if (FireSound)
	{
		const float Pitch = 1.0f + FMath::FRandRange(-FireSoundPitchRandom, FireSoundPitchRandom);

		// SpawnSoundAtLocation 은 오디오 컴포넌트를 돌려주므로 도중에 끊을 수 있다.
		UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
			this, FireSound, StartLoc, FRotator::ZeroRotator, FireSoundVolume, Pitch,
			0.0f, GetSoundAttenuation());

		// 지정 시간이 지나면 정지(0이면 끝까지 재생).
		if (AudioComp && FireSoundDuration > 0.0f)
		{
			TWeakObjectPtr<UAudioComponent> WeakAudio(AudioComp);
			FTimerHandle StopHandle;
			GetWorldTimerManager().SetTimer(StopHandle, [WeakAudio]()
			{
				if (UAudioComponent* Comp = WeakAudio.Get())
				{
					Comp->Stop();
				}
			}, FireSoundDuration, false);
		}
	}
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
