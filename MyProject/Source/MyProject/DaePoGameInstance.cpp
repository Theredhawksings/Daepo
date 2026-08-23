// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoGameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void UDaePoGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UDaePoGameInstance::HandleNetworkFailure);
	}
}

void UDaePoGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 서버가 정상적으로 나갔든(Quit 버튼), 크래시/작업관리자로 강제종료됐든, 클라이언트
	// 입장에서는 둘 다 "연결이 갑자기 끊김"으로 여기에 들어온다. 화면이 멈춘 채로
	// 남아있지 않도록 무조건 메인 메뉴로 돌려보낸다.
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
}
