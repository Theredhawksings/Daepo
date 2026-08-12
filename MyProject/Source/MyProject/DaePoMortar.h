// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DaePo.h"
#include "DaePoMortar.generated.h"

/**
 * 박격포.
 * 사거리 안의 플레이어 위치를 향해 큐브를 높은 포물선으로 쏘아 올려 떨어뜨린다.
 * 떨어질 자리에는 착탄 경고 원이 미리 표시된다. 나머지 기능(풀/사운드/반동/데미지)은 ADaePo 와 동일.
 */
UCLASS()
class MYPROJECT_API ADaePoMortar : public ADaePo
{
	GENERATED_BODY()

public:
	ADaePoMortar();

protected:
	virtual void BeginPlay() override;

	/** 사거리 안에 플레이어가 있을 때만, 그 위치를 향해 곡사로 발사 */
	virtual void Fire() override;

	/** 플레이어 감지 사거리(cm) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar", meta = (ClampMin = "100.0"))
	float MortarRange = 4000.0f;

	/** 착탄 지점이 플레이어 위치에서 무작위로 벗어나는 최대 반경(cm). 0이면 정확히 조준 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar", meta = (ClampMin = "0.0"))
	float LandingSpread = 250.0f;

	/** 포물선 높이(0에 가까울수록 수직에 가깝게 높이 솟고, 1에 가까울수록 직사) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float ArcParam = 0.35f;

	/** 떨어질 자리에 경고 원을 표시할지 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar")
	bool bShowLandingWarning = true;

	/** 경고 원 반지름(cm) */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar", meta = (ClampMin = "10.0"))
	float WarningRadius = 150.0f;

	/** 발사 위치 높이 보정(cm). 메시 꼭대기에서 이만큼 더 위에서 발사된다 */
	UPROPERTY(EditAnywhere, Category = "DaePo|Mortar", meta = (ClampMin = "0.0"))
	float MuzzleHeightOffset = 80.0f;
};
