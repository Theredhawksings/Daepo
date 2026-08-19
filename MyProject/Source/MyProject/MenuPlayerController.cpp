// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuPlayerController.h"
#include "MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 서버에는 접속한 모든 플레이어의 컨트롤러가 존재하는데(내 것뿐 아니라 남의 것도),
	// BeginPlay 는 그 전부에서 실행된다. UI/커서는 "이 화면의 진짜 주인"에게만 붙여야 하므로
	// 로컬 컨트롤러가 아니면(=서버 입장에서 남의 컨트롤러면) 여기서 끝낸다.
	if (!IsLocalController())
	{
		return;
	}

	// 메뉴 화면에선 마우스로 버튼을 눌러야 하니 커서를 보여주고 입력을 UI 전용으로 돌린다.
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);

	TSubclassOf<UMainMenuWidget> ClassToUse = MenuWidgetClass;
	if (!ClassToUse)
	{
		ClassToUse = UMainMenuWidget::StaticClass();
	}

	if (UMainMenuWidget* Widget = CreateWidget<UMainMenuWidget>(this, ClassToUse))
	{
		Widget->AddToViewport();
	}
}
