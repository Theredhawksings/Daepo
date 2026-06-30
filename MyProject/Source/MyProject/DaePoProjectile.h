// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoProjectile.generated.h"

class UProjectileMovementComponent;

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

private:
	/** 수명 종료 시 풀로 반환하는 타이머 */
	FTimerHandle LifeTimerHandle;

	/** 발사되어 사용 중인지 여부 */
	bool bInUse = false;
};
