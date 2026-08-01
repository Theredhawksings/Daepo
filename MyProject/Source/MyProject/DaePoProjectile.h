// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoProjectile.generated.h"

class UProjectileMovementComponent;
class USoundBase;
class USoundAttenuation;
class UMaterialInterface;

/**
 * 대포에서 발사되는 큐브 발사체.
 * 오브젝트 풀링으로 재사용되며, ProjectileMovementComponent 로 벽에 튕긴다.
 */
UCLASS()
class MYPROJECT_API ADaePoProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADaePoProjectile();

	/** 풀에서 꺼내 발사: 위치/방향/속도/수명/크기 지정 후 활성화 */
	void Launch(const FVector& InLocation, const FVector& Direction, float Speed, float LifeTime, const FVector& Scale);

	/** 풀로 반환: 숨기고 충돌/이동 비활성화 */
	void Deactivate();

	/** 현재 사용 중인지(발사된 상태인지) */
	FORCEINLINE bool IsInUse() const { return bInUse; }

protected:
	/** 큐브 메시 (루트, 충돌 담당) */
	UPROPERTY(VisibleAnywhere, Category = "DaePo")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	/** 포물선 이동 + 튕김 처리 */
	UPROPERTY(VisibleAnywhere, Category = "DaePo")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** true면 무엇에든 부딪히는 즉시 사라진다(튕김 없음). false면 아래 튕김 설정을 사용 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Bounce")
	bool bDestroyOnImpact = true;

	/** 몇 번 튕기면 사라질지. 0이면 횟수 제한 없음(수명까지 계속 튕김) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Bounce", meta = (ClampMin = "0", EditCondition = "!bDestroyOnImpact"))
	int32 MaxBounces = 3;

	/** 반발 계수. 낮을수록 덜 튕긴다(0 = 튕기지 않고 바로 멈춤) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Bounce", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bounciness = 0.35f;

	/** 표면 마찰. 높을수록 미끄러지지 않고 빨리 멈춘다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Bounce", meta = (ClampMin = "0.0"))
	float BounceFriction = 0.6f;

	/** 튕길 때 속도가 이 값 미만이면 즉시 정지(바닥에서 떠는 현상 방지) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Bounce", meta = (ClampMin = "0.0"))
	float BounceStopSpeed = 150.0f;

	/** 튕길 때마다 호출되어 횟수를 세고, 한도를 넘으면 풀로 반환 */
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	/** 이동이 멈췄을 때(부딪혀 정지 / 튕김이 잦아듦) 호출 → 풀로 반환 */
	UFUNCTION()
	void OnStop(const FHitResult& ImpactResult);

	/** 충돌음. 부딪힌 지점에서 3D로 재생된다. 비우면 무음. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound")
	TObjectPtr<USoundBase> ImpactSound;

	/** 충돌음 볼륨 배수(발사음보다 크게) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float ImpactSoundVolume = 3.0f;

	/** 충돌음을 몇 초만 재생하고 끊을지. 0이면 끝까지 재생. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float ImpactSoundDuration = 0.5f;

	/** 이 거리(cm) 안에서는 원래 볼륨으로 들린다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float SoundInnerRadius = 600.0f;

	/** 위 반경을 벗어난 뒤 이 거리(cm)에 걸쳐 서서히 안 들리게 된다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float SoundFalloffDistance = 4000.0f;

	/** 직접 만든 감쇠 에셋을 쓰고 싶을 때 지정. 비우면 위 두 값으로 자동 생성 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound")
	TObjectPtr<USoundAttenuation> SoundAttenuationOverride;

	/** 지정 위치에서 충돌음을 재생하고 ImpactSoundDuration 뒤에 끊는다 */
	void PlayImpactSound(const FVector& Location);

	/** 사용할 거리 감쇠 설정을 반환(없으면 한 번 만들어 캐시) */
	USoundAttenuation* GetSoundAttenuation();

	/** 충돌 지점에 디버그 구를 그릴지 여부 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Impact")
	bool bShowImpactDebugSphere = true;

	/** 디버그 구 반지름(cm) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Impact", meta = (ClampMin = "1.0"))
	float ImpactSphereRadius = 40.0f;

	/** 디버그 구가 화면에 남아있는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Impact", meta = (ClampMin = "0.1"))
	float ImpactSphereDuration = 3.0f;

	/** 디버그 구 색상 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Impact")
	FColor ImpactSphereColor = FColor::Red;

	/** 부딪힌 지점에 충돌구(디버그 구)를 그린다 */
	void SpawnImpactDecal(const FVector& Location, const FVector& Normal);

private:
	/** 수명 종료 시 풀로 반환하는 타이머 */
	FTimerHandle LifeTimerHandle;

	/** 발사되어 사용 중인지 여부 */
	bool bInUse = false;

	/** 이번 발사에서 튕긴 횟수 */
	int32 BounceCount = 0;

	/** 런타임에 생성한 감쇠 설정 캐시 */
	UPROPERTY()
	TObjectPtr<USoundAttenuation> RuntimeAttenuation;
};
