// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * ESC 를 누르면 뜨는 투명 배경 메뉴. 순수 C++/Slate 로 그린다(UMG 디자이너 작업 불필요).
 * 화면 전체를 옅게 어둡게 덮고, 가운데에 Quit 버튼 하나만 둔다.
 */
UCLASS()
class MYPROJECT_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply OnQuitClicked();
};
