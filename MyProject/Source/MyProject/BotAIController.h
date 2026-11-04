// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BotAIController.generated.h"

/** 혼자 테스트할 때 다른 플레이어 자리를 채우는 더미 봇. 계속 이어서 무작위 지점을 향해 걷는다. */
UCLASS()
class MYPROJECT_API ABotAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** 현재 위치에서 이 반경(cm) 안의 무작위 지점으로 걷는다. 레벨에 NavMeshBoundsVolume 이 있어야 동작한다. */
	UPROPERTY(EditAnywhere, Category = "Bot")
	float WanderRadius = 1500.0f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void PickNewWanderTarget();
};
