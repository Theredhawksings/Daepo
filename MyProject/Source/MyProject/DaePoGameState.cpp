// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoGameState.h"
#include "Net/UnrealNetwork.h"

void ADaePoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADaePoGameState, ReplicatedElapsedSeconds);
}
