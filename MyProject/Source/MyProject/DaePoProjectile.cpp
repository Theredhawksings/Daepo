// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "DaePoHitCameraShake.h"
#include "Engine/OverlapResult.h"
#include "MyProjectCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

ADaePoProjectile::ADaePoProjectile()
{
	// 액터 틱은 끈다. 이동은 ProjectileMovementComponent 가 자체적으로 처리한다(최적화).
	PrimaryActorTick.bCanEverTick = false;

	// 이 발사체는 서버에서만 스폰/발사되므로(대포가 HasAuthority() 로 가드됨),
	// 클라이언트에서도 보이려면 액터 자체와 이동을 복제해야 한다.
	SetReplicates(true);
	SetReplicateMovement(true);

	// --- 발사체 메시 (루트, 충돌 담당) ---
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (SphereMeshAsset.Succeeded())
	{
		MeshComp->SetStaticMesh(SphereMeshAsset.Object);
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
	ProjectileMovement->ProjectileGravityScale = 1.0f; // 대포처럼 포물선 낙하
	ProjectileMovement->bAutoActivate = false;         // 발사 전까지 비활성(풀링)

	// 튕김 횟수 집계 / 정지 시점 콜백 등록
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &ADaePoProjectile::OnBounce);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ADaePoProjectile::OnStop);

	// 기본 충돌음 지정
	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundAsset(
		TEXT("/Script/Engine.SoundWave'/Game/MP_Grenade.MP_Grenade'"));
	if (ImpactSoundAsset.Succeeded())
	{
		ImpactSound = ImpactSoundAsset.Object;
	}

	// 기본 피격 카메라 흔들림 지정
	HitCameraShake = UDaePoHitCameraShake::StaticClass();

	// 기본 폭발 이펙트 지정(블루프린트 없이 C++ 기본값으로 바로 적용됨)
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactVFXAsset(
		TEXT("/Script/Niagara.NiagaraSystem'/Game/Fire_EXP_Vol01_Free/Niagara/EXP/NS_Sub_EXP_Mid_002_02.NS_Sub_EXP_Mid_002_02'"));
	if (ImpactVFXAsset.Succeeded())
	{
		ImpactVFX = ImpactVFXAsset.Object;
	}
}

void ADaePoProjectile::ShakeNearbyPlayer(const FVector& Location, const AActor* DirectHitActor) const
{
	UWorld* World = GetWorld();
	if (!HitCameraShake || !World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		float Scale = 0.0f;
		if (Pawn == DirectHitActor)
		{
			// 직격: 최대 강도
			Scale = 1.0f;
		}
		else if (ShakeRadius > 0.0f)
		{
			// 근처 폭발: 거리에 비례해 약해지고 ShakeRadius 밖은 0
			const float Dist = FVector::Dist(Pawn->GetActorLocation(), Location);
			Scale = FMath::Clamp(1.0f - Dist / ShakeRadius, 0.0f, 1.0f);
		}

		if (Scale > KINDA_SMALL_NUMBER)
		{
			PC->ClientStartCameraShake(HitCameraShake, Scale * HitShakeScale);
		}
	}
}

USoundAttenuation* ADaePoProjectile::GetSoundAttenuation()
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

void ADaePoProjectile::PlayImpactSound(const FVector& Location)
{
	if (!ImpactSound)
	{
		return;
	}

	// 오디오 컴포넌트를 받아둬야 도중에 끊을 수 있다.
	UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
		this, ImpactSound, Location, FRotator::ZeroRotator, ImpactSoundVolume, 1.0f,
		0.0f, GetSoundAttenuation());

	// 지정 시간이 지나면 정지(0이면 끝까지 재생).
	if (AudioComp && ImpactSoundDuration > 0.0f)
	{
		TWeakObjectPtr<UAudioComponent> WeakAudio(AudioComp);
		FTimerHandle StopHandle;
		GetWorldTimerManager().SetTimer(StopHandle, [WeakAudio]()
		{
			if (UAudioComponent* Comp = WeakAudio.Get())
			{
				Comp->Stop();
			}
		}, ImpactSoundDuration, false);
	}
}

void ADaePoProjectile::ApplyAreaDamage(const FVector& Location)
{
	// 피해 판정은 서버만 한다. 클라이언트에도 이 발사체의 복제본이 있고
	// 거기서도 충돌이 로컬로 감지될 수 있는데, 거기서까지 피해를 적용하면
	// 서버와 다른 결과가 나오거나 중복 적용될 수 있다.
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || DamageAmount <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);
	World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), Sphere);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (AMyProjectCharacter* Player = Cast<AMyProjectCharacter>(HitActor))
		{
			// 같은 발사체가 튕기면서 같은 대상을 여러 번 때리는 것을 방지(대상 하나당 한 번만 피해).
			if (DamagedActorsThisFlight.Contains(HitActor))
			{
				continue;
			}
			DamagedActorsThisFlight.Add(HitActor);

			// 폭발 지점을 함께 넘겨 사망 시 쓰러지는 방향 판정에 쓰이게 한다.
			Player->ApplyDamageFromLocation(DamageAmount, Location);
		}
	}
}

void ADaePoProjectile::MulticastPlayImpactEffects_Implementation(const FVector& Location, const FVector& Normal, AActor* HitActor)
{
	PlayImpactSound(Location);
	SpawnImpactDecal(Location, Normal);
	SpawnImpactVFX(Location, Normal);
	ShakeNearbyPlayer(Location, HitActor);
}

void ADaePoProjectile::SpawnImpactVFX(const FVector& Location, const FVector& Normal)
{
	if (!ImpactVFX)
	{
		return;
	}

	// 충돌 표면에 딱 붙여서 스폰하면 구형으로 퍼지는 파티클 절반이 벽 안으로 파고들어
	// 벽을 뚫고 나온 것처럼 보인다. 표면 바깥쪽(Normal 방향)으로 살짝 띄워서 스폰한다.
	const FVector SpawnLocation = Location + Normal.GetSafeNormal() * ImpactVFXSurfaceOffset;

	// bPreCullCheck 를 꺼서, 나이아가라가 "안 보여도 될 것 같다"고 자체 판단해 조용히
	// 스폰을 건너뛰는 일이 없게 한다(대포 폭발처럼 플레이어가 직접 일으킨 이펙트는 항상 보여야 함).
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactVFX, SpawnLocation, Normal.Rotation(),
		FVector(ImpactVFXScale), true, true, ENCPoolMethod::AutoRelease, false);
}

void ADaePoProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// 이 콜백은 실제로는 서버에서만 발생한다(클라이언트의 복제본은 이동 컴포넌트가
	// 비활성 상태). 그래도 방어적으로 가드해 둔다.
	if (HasAuthority())
	{
		MulticastPlayImpactEffects(ImpactResult.ImpactPoint, ImpactResult.ImpactNormal, ImpactResult.GetActor());
	}
	ApplyAreaDamage(ImpactResult.ImpactPoint);

	// 0이면 횟수 제한 없음(수명 다할 때까지 튕김)
	if (MaxBounces <= 0)
	{
		return;
	}

	if (++BounceCount >= MaxBounces)
	{
		Deactivate(); // 정해진 횟수를 넘기면 즉시 풀로 반환
	}
}

void ADaePoProjectile::OnStop(const FHitResult& ImpactResult)
{
	// bDestroyOnImpact 면 첫 충돌에서 곧바로 여기로 온다(튕김이 꺼져 있으므로).
	// 튕김 모드일 때는 속도가 죽어 멈춘 시점이며, 어느 쪽이든 풀로 반환한다.
	if (HasAuthority())
	{
		MulticastPlayImpactEffects(ImpactResult.ImpactPoint, ImpactResult.ImpactNormal, ImpactResult.GetActor());
	}
	ApplyAreaDamage(ImpactResult.ImpactPoint);
	Deactivate();
}

void ADaePoProjectile::SpawnImpactDecal(const FVector& Location, const FVector& Normal)
{
	if (!bShowImpactDebugSphere)
	{
		return;
	}

	// 충돌 지점에 실제 피해 범위(DamageRadius) 크기의 와이어프레임 구를 그린다. Duration 이 지나면 자동으로 사라진다.
	DrawDebugSphere(GetWorld(), Location, DamageRadius, 12, ImpactSphereColor,
		false, ImpactSphereDuration, 0, 1.5f);
}

void ADaePoProjectile::Launch(const FVector& InLocation, const FVector& Direction, float Speed, float LifeTime, const FVector& Scale)
{
	bInUse = true;
	BounceCount = 0;
	DamagedActorsThisFlight.Reset();

	// 튕김 설정을 발사 시점에 적용(에디터에서 바꾼 값이 바로 반영되도록)
	// bDestroyOnImpact 면 튕김을 꺼서 첫 충돌에 바로 멈추고 OnStop 으로 사라진다.
	ProjectileMovement->bShouldBounce = !bDestroyOnImpact;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = BounceFriction;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = BounceStopSpeed;

	const FVector ShotDir = Direction.GetSafeNormal();

	// 자기를 발사한 대포와는 충돌하지 않도록(포구에서 걸리는 것 방지)
	if (AActor* OwnerActor = GetOwner())
	{
		MeshComp->IgnoreActorWhenMoving(OwnerActor, true);
	}

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
