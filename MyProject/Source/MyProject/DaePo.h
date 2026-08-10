// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePo.generated.h"

class UArrowComponent;
class ADaePoProjectile;
class UMaterialInterface;
class USoundBase;
class USoundAttenuation;

UCLASS()
class MYPROJECT_API ADaePo : public AActor
{
	GENERATED_BODY()

public:
	ADaePo();

	/**
	 * 미리보기(설치 전 고스트) 모드로 전환한다.
	 * 활성화하면 발사/충돌을 끄고, PreviewMat 이 있으면 대포 재질을 그것으로 바꾼다.
	 * 반드시 BeginPlay 전에(SpawnActorDeferred 후) 호출해야 발사 타이머/풀 생성을 건너뛴다.
	 */
	void SetPreviewMode(bool bEnabled, UMaterialInterface* PreviewMat = nullptr);

	/** 미리보기 상태에서 대포 재질만 교체(유효/불가 색 전환용) */
	void SetPreviewMaterial(UMaterialInterface* Mat);

	/** 현재 포구 기준 발사 궤적을 한 프레임 동안 그린다(빌드 모드 미리보기용) */
	void DrawFireTrajectory() const;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** 대포 본체 메시 (루트) */
	UPROPERTY(VisibleAnywhere, Category = "DaePo")
	TObjectPtr<UStaticMeshComponent> CannonMesh;

	/** 발사 위치/방향. 에디터 뷰포트에서 직접 옮겨 발사 지점을 지정한다. */
	UPROPERTY(VisibleAnywhere, Category = "DaePo")
	TObjectPtr<UArrowComponent> MuzzleArrow;

	/** 발사할 발사체 클래스 (기본: ADaePoProjectile) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire")
	TSubclassOf<ADaePoProjectile> ProjectileClass;

	/** 발사 간격(초) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (ClampMin = "0.05"))
	float FireInterval = 1.0f;

	/** 포구 속도(cm/s) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (ClampMin = "0.0"))
	float MuzzleSpeed = 2000.0f;

	/** 발사마다 속도에 더해지는 무작위 폭(±cm/s). 0이면 항상 일정. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire|Random", meta = (ClampMin = "0.0"))
	float SpeedRandomRange = 350.0f;

	/** 발사 방향이 퍼지는 각도(도). 콘의 반각. 0이면 항상 정면. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire|Random", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadAngle = 7.0f;

	/** 발사체 수명(초). 이 시간이 지나면 풀로 반환된다. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (ClampMin = "0.1"))
	float ProjectileLifeTime = 5.0f;

	/** 발사되는 큐브 크기(스케일). 에디터에서 조정 가능. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (AllowPreserveRatio = "true"))
	FVector ProjectileScale = FVector(0.3f, 0.3f, 0.3f);

	/** 발사음. 포구 위치에서 3D 사운드로 재생된다. 비우면 무음. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound")
	TObjectPtr<USoundBase> FireSound;

	/** 발사음 볼륨 배수 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float FireSoundVolume = 1.0f;

	/** 발사음 피치 무작위 폭(±). 0이면 항상 같은 톤. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float FireSoundPitchRandom = 0.1f;

	/** 발사음을 몇 초만 재생하고 끊을지. 0이면 끝까지 재생. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float FireSoundDuration = 0.3f;

	/** 이 거리(cm) 안에서는 원래 볼륨으로 들린다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float SoundInnerRadius = 400.0f;

	/** 위 반경을 벗어난 뒤 이 거리(cm)에 걸쳐 서서히 안 들리게 된다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound", meta = (ClampMin = "0.0"))
	float SoundFalloffDistance = 3000.0f;

	/** 직접 만든 감쇠 에셋을 쓰고 싶을 때 지정. 비우면 위 두 값으로 자동 생성 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Sound")
	TObjectPtr<USoundAttenuation> SoundAttenuationOverride;

	/** 미리 생성해 둘 발사체 풀 크기 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Pool", meta = (ClampMin = "1"))
	int32 PoolSize = 20;

	/** 체크하면 설치 지점 주변을 무작위로 살짝살짝 움직인다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move")
	bool bRandomMove = false;

	/** 설치 지점에서 벗어날 수 있는 최대 반경(cm) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move", meta = (ClampMin = "10.0", EditCondition = "bRandomMove"))
	float MoveRadius = 200.0f;

	/** 이동 속도(cm/s). 낮을수록 슬금슬금 움직인다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move", meta = (ClampMin = "1.0", EditCondition = "bRandomMove"))
	float MoveSpeed = 80.0f;

	/** 목표 지점 도착 후 다음 이동까지 쉬는 시간(초). 0이면 쉬지 않고 계속 이동 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move", meta = (ClampMin = "0.0", EditCondition = "bRandomMove"))
	float MovePauseTime = 1.0f;

	/** 체크하면 발사 방향(Yaw 각도)도 무작위로 살짝살짝 돌린다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move")
	bool bRandomRotate = false;

	/** 설치 당시 방향 기준 좌우로 최대 몇 도까지 돌아갈지 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bRandomRotate"))
	float RotateRange = 60.0f;

	/** 회전 속도(도/초). 낮을수록 천천히 돈다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Move", meta = (ClampMin = "1.0", EditCondition = "bRandomRotate"))
	float RotateSpeed = 45.0f;

	/** 발사 시 반동(뒤로 밀렸다 복귀) 사용 여부 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Recoil")
	bool bRecoil = true;

	/** 반동으로 뒤로 밀리는 거리(cm) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Recoil", meta = (ClampMin = "0.0", EditCondition = "bRecoil"))
	float RecoilDistance = 30.0f;

	/** 원위치로 돌아오는 속도(보간 계수). 클수록 빨리 복귀 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Recoil", meta = (ClampMin = "0.1", EditCondition = "bRecoil"))
	float RecoilRecoverSpeed = 6.0f;

private:
	/** 타이머에서 주기적으로 호출 */
	void Fire();

	/** 풀에서 비활성 발사체를 찾아 반환(없으면 동적 확장) */
	ADaePoProjectile* GetPooledProjectile();

	/** 사용할 거리 감쇠 설정을 반환(없으면 한 번 만들어 캐시) */
	USoundAttenuation* GetSoundAttenuation();

	/** 런타임에 생성한 감쇠 설정 캐시 */
	UPROPERTY()
	TObjectPtr<USoundAttenuation> RuntimeAttenuation;

	/** 사전 생성된 발사체 풀 */
	UPROPERTY()
	TArray<TObjectPtr<ADaePoProjectile>> ProjectilePool;

	/** 주기적 발사 타이머 핸들 */
	FTimerHandle FireTimerHandle;

	/** 미리보기(고스트) 상태면 발사/풀 생성을 하지 않는다 */
	bool bPreviewMode = false;

	/** 무작위 이동의 기준점(설치된 위치) */
	FVector WanderAnchor = FVector::ZeroVector;

	/** 현재 향해 가는 목표 지점 */
	FVector WanderTarget = FVector::ZeroVector;

	/** 설치 당시의 기준 Yaw 각도 */
	float WanderAnchorYaw = 0.0f;

	/** 현재 향해 도는 목표 Yaw 각도 */
	float WanderTargetYaw = 0.0f;

	/** 도착 후 잠시 쉬는 중인지 */
	bool bWanderPaused = false;

	/** 쉬는 시간용 타이머 */
	FTimerHandle WanderPauseHandle;

	/** 기준점 주변에서 새 목표 지점을 뽑는다 */
	void PickNewWanderTarget();

	/** 아직 복귀하지 못한 반동 잔량(cm). 0이면 반동 없음 */
	float CurrentRecoil = 0.0f;
};
