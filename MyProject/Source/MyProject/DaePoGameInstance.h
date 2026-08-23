// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DaePoGameInstance.generated.h"

/**
 * 네트워크 연결이 끊기면(서버가 정상 종료됐든, 크래시/강제종료됐든 상관없이) 자동으로
 * 메인 메뉴로 돌아가게 하는 게임 인스턴스. 이게 없으면 클라이언트는 연결이 끊긴 채
 * 화면이 멈춘 것처럼 보이게 된다.
 */
UCLASS()
class MYPROJECT_API UDaePoGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 연결이 끊겼을 때 돌아갈 메인 메뉴 맵 이름(패키지 이름, 확장자 없이) */
	UPROPERTY(EditDefaultsOnly, Category = "Network")
	FName MainMenuMapName = FName(TEXT("Main"));

	virtual void Init() override;

private:
	/** 서버가 정상 종료됐든 강제종료(크래시/작업관리자)됐든, 연결이 끊기면 여기로 온다 */
	UFUNCTION()
	void HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
};
