// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * 순수 C++/Slate 로 만든 메인 메뉴. 색 선택과 시작 버튼만 있는 단순한 화면을 코드로 직접 그린다.
 */
UCLASS()
class MYPROJECT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 시작 버튼을 누르면 열 맵(패키지 전체 경로) */
	UPROPERTY(EditAnywhere, Category = "Menu")
	FName MapToStart = FName(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply OnColorClicked(int32 ColorIndex);
	FReply OnStartClicked();
};
