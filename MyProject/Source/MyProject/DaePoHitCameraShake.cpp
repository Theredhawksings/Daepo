// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoHitCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UDaePoHitCameraShake::UDaePoHitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 짧고 강하게 쳤다가 빠르게 잦아드는 패턴
	UPerlinNoiseCameraShakePattern* Pattern =
		ObjectInitializer.CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(this, TEXT("Pattern"));

	Pattern->Duration = 0.35f;
	Pattern->BlendInTime = 0.03f;   // 즉시 최대 강도로
	Pattern->BlendOutTime = 0.2f;   // 부드럽게 잦아듦

	// 회전 흔들림(체감이 가장 큰 요소)
	Pattern->Pitch.Amplitude = 5.0f;
	Pattern->Pitch.Frequency = 30.0f;
	Pattern->Yaw.Amplitude = 4.0f;
	Pattern->Yaw.Frequency = 25.0f;
	Pattern->Roll.Amplitude = 3.0f;
	Pattern->Roll.Frequency = 20.0f;

	// 위치 흔들림(살짝만)
	Pattern->Y.Amplitude = 6.0f;
	Pattern->Y.Frequency = 25.0f;
	Pattern->Z.Amplitude = 6.0f;
	Pattern->Z.Frequency = 25.0f;

	SetRootShakePattern(Pattern);
}
