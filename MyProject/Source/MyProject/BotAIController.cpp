// Fill out your copyright notice in the Description page of Project Settings.

#include "BotAIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ABotAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// BeginPlay 시점엔 아직 Possess 가 안 끝나서 GetPawn() 이 비어있다 - 실제로 폰을
	// 소유하는 이 시점에서 시작해야 한다.

	// AnimBP 의 Walk/Run 전환이 GetCurrentAcceleration() != 0 을 체크한다. AI 길찾기는 기본값으로
	// 가속도 없이 속도만 직접 박아서 Acceleration 이 늘 0 → 애니가 Idle 에 고정된다. 가속도 방식으로 바꾼다.
	if (ACharacter* BotCharacter = Cast<ACharacter>(InPawn))
	{
		if (UCharacterMovementComponent* Movement = BotCharacter->GetCharacterMovement())
		{
			Movement->GetNavMovementProperties()->bUseAccelerationForPaths = true;
		}
	}

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
