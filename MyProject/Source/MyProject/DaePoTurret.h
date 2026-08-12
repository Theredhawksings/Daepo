// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DaePo.h"
#include "DaePoTurret.generated.h"

/**
 * 플레이어 추적 대포.
 * 기능은 ADaePo 와 동일하지만, 감지 범위 안에 플레이어가 들어오면
 * 포신이 플레이어를 향해 회전하며 그때만 발사한다. 범위 밖이면 쏘지 않는다.
 */
UCLASS()
class MYPROJECT_API ADaePoTurret : public ADaePo
{
	GENERATED_BODY()

public:
	ADaePoTurret();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** 범위 안에 플레이어가 있을 때만 발사 */
	virtual void Fire() override;

	/** 플레이어 감지 범위(cm). 기본 5m */
	UPROPERTY(EditAnywhere, Category = "DaePo|Turret", meta = (ClampMin = "50.0"))
	float DetectionRange = 500.0f;

	/** 플레이어를 향해 도는 속도(도/초) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Turret", meta = (ClampMin = "1.0"))
	float TrackRotateSpeed = 120.0f;

	/** 에디터/디버그용: 감지 범위를 원으로 표시 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Turret")
	bool bShowDetectionRange = true;

private:
	/** 감지 범위 안의 플레이어 폰을 반환(없으면 nullptr) */
	APawn* FindPlayerInRange() const;

	/** 이번 프레임 기준, 발사 가능한 대상이 범위 안에 있는지 */
	bool bTargetInRange = false;
};
