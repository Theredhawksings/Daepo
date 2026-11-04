// Fill out your copyright notice in the Description page of Project Settings.

#include "BotAIController.h"
#include "NavigationSystem.h"

void ABotAIController::BeginPlay()
{
	Super::BeginPlay();
	PickNewWanderTarget();
}

void ABotAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// 도착하자마자 바로 다음 목적지로 이어서 걷는다 - 멈춰서 기다리는 부자연스러운 구간이 없어짐.
	PickNewWanderTarget();
}

void ABotAIController::PickNewWanderTarget()
{
	if (!GetPawn())
	{
		return;
	}

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Result;
	if (Nav && Nav->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), WanderRadius, Result))
	{
		MoveToLocation(Result.Location);
	}
}
