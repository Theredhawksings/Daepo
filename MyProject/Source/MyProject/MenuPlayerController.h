// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

class UMainMenuWidget;

/**
 * 메인 메뉴 전용 플레이어 컨트롤러. 맵에 들어오면 곧바로 메인 메뉴 위젯을 띄우고
 * 마우스로 버튼을 누를 수 있게 UI 전용 입력 모드로 전환한다.
 */
UCLASS()
class MYPROJECT_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** 비워두면 기본 UMainMenuWidget(C++ 전용, UMG 작업 불필요)을 그대로 쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UMainMenuWidget> MenuWidgetClass;
};
