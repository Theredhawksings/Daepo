// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaePoLandmine.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USoundBase;
class UMaterialInterface;

/**
 * 밟으면 잠깐 뒤에 터지는 지뢰. GameMode 가 라운드 시작 시 맵 곳곳에 무작위로 배치하고,
 * 하나가 터질 때마다 GameMode 에 알려서(OnLandmineConsumed) 다른 곳에 새 지뢰가 다시 채워지게 한다.
 */
UCLASS()
class MYPROJECT_API ADaePoLandmine : public AActor
{
	GENERATED_BODY()

public:
	ADaePoLandmine();

	/** 밟은 뒤 실제로 터지기까지 걸리는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Landmine")
	float ArmDelay = 0.7f;

	/** 폭발 피해량 */
	UPROPERTY(EditAnywhere, Category = "Landmine")
	float DamageAmount = 60.0f;

	/** 폭발 피해 범위(cm) */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "0.0"))
	float DamageRadius = 300.0f;

	/** 밟았다고 인식할 감지 반경(cm) */
	UPROPERTY(EditAnywhere, Category = "Landmine", meta = (ClampMin = "1.0"))
	float TriggerRadius = 60.0f;

	/** 경고용으로 눈에 보이는 지뢰 표시 메시에 입힐 재질(비우면 기본 재질 사용) */
	UPROPERTY(EditAnywhere, Category = "Landmine")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	/** 폭발음(비우면 무음) */
	UPROPERTY(EditAnywhere, Category = "Landmine")
	TObjectPtr<USoundBase> ExplosionSound;

protected:
	virtual void BeginPlay() override;

	/** 경고 표시용 메시(루트) */
	UPROPERTY(VisibleAnywhere, Category = "Landmine")
	TObjectPtr<UStaticMeshComponent> MineMesh;

	/** 밟았는지 감지하는 트리거 볼륨 */
	UPROPERTY(VisibleAnywhere, Category = "Landmine")
	TObjectPtr<USphereComponent> TriggerSphere;

private:
	/** 이미 밟혀서 터지는 중인지(중복 트리거 방지) */
	bool bArmed = false;

	/** 밟은 시점부터 ArmDelay 뒤 폭발시키는 타이머 */
	FTimerHandle ExplodeTimerHandle;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 실제 폭발 처리(피해 판정 등, 서버 전용) */
	void Explode();

	/** 모든 클라이언트에서 폭발 효과(소리 등)를 재생 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosionEffects();
};
