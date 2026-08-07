// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "DaePoHitCameraShake.generated.h"

/**
 * 플레이어가 대포 큐브에 맞았을 때 재생되는 카메라 흔들림.
 * 펄린 노이즈 기반의 짧고 굵은 타격감.
 */
UCLASS()
class MYPROJECT_API UDaePoHitCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UDaePoHitCameraShake(const FObjectInitializer& ObjectInitializer);
};
