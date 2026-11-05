// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTimerActor.h"
#include "Components/TextRenderComponent.h"

ADaePoTimerActor::ADaePoTimerActor()
{
	TimerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TimerText"));
	RootComponent = TimerText;
	SetActorHiddenInGame(true);
}
