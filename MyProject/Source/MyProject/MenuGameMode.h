// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

/**
 * 메인 메뉴 맵 전용 게임모드. 조작할 폰이 없고 UI만 띄운다.
 * (abstract 로 표시하지 않아 블루프린트 자식 없이도 World Settings 에서 바로 선택 가능)
 */
UCLASS()
class MYPROJECT_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();
};
