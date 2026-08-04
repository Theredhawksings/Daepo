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

void ADaePo::BeginPlay()
{
	Super::BeginPlay();

	// 미리보기(고스트)는 발사 타이머/풀을 만들지 않는다.
	if (bPreviewMode)
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

	// 발사음: 포구 위치에서 3D 재생. 매번 피치를 살짝 바꿔 반복감을 줄인다.
	if (FireSound)
	{
		const float Pitch = 1.0f + FMath::FRandRange(-FireSoundPitchRandom, FireSoundPitchRandom);

		// SpawnSoundAtLocation 은 오디오 컴포넌트를 돌려주므로 도중에 끊을 수 있다.
		UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
			this, FireSound, SpawnLoc, FRotator::ZeroRotator, FireSoundVolume, Pitch,
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
