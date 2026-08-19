// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuGameMode.h"
#include "MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
	PlayerControllerClass = AMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr; // 메뉴에는 조작할 폰이 필요 없다
}
