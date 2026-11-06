// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * 순수 C++/Slate 로 만든 메인 메뉴. 색 선택/시작 화면과 맵 선택 화면을 코드로 직접 그린다.
 */
UCLASS()
class MYPROJECT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply OnColorClicked(int32 ColorIndex);
	/** 메뉴 화면(0)과 맵 선택 화면(1)을 전환한다 */
	TSharedPtr<class SWidgetSwitcher> ScreenSwitcher;

	/** 맵 카드 썸네일. SImage 는 브러시를 포인터로만 들고 있어서 위젯이 살아있는 동안 여기서 유지한다. */
	TArray<TSharedPtr<struct FSlateBrush>> ThumbnailBrushes;

	FReply ShowScreen(int32 ScreenIndex);
	FReply OnMapClicked(FName MapName);
};
