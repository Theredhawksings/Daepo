// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePo.generated.h"

class UArrowComponent;
class ADaePoProjectile;

UCLASS()
class MYPROJECT_API ADaePo : public AActor
{
	GENERATED_BODY()

public:
	ADaePo();

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
	float SpeedRandomRange = 150.0f;

	/** 발사 방향이 퍼지는 각도(도). 콘의 반각. 0이면 항상 정면. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire|Random", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadAngle = 3.0f;

	/** 발사체 수명(초). 이 시간이 지나면 풀로 반환된다. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (ClampMin = "0.1"))
	float ProjectileLifeTime = 5.0f;

	/** 발사되는 큐브 크기(스케일). 에디터에서 조정 가능. */
	UPROPERTY(EditAnywhere, Category = "DaePo|Fire", meta = (AllowPreserveRatio = "true"))
	FVector ProjectileScale = FVector(0.05f, 0.05f, 0.05f);

	/** 미리 생성해 둘 발사체 풀 크기 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Pool", meta = (ClampMin = "1"))
	int32 PoolSize = 20;

private:
	/** 타이머에서 주기적으로 호출 */
	void Fire();

	/** 풀에서 비활성 발사체를 찾아 반환(없으면 동적 확장) */
	ADaePoProjectile* GetPooledProjectile();

	/** 사전 생성된 발사체 풀 */
	UPROPERTY()
	TArray<TObjectPtr<ADaePoProjectile>> ProjectilePool;

	/** 주기적 발사 타이머 핸들 */
	FTimerHandle FireTimerHandle;
};
