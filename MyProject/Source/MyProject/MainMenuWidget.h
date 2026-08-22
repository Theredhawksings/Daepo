// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

/**
 * 순수 C++/Slate 로 만든 메인 메뉴. UMG 디자이너 작업 없이 Host/Join 버튼과
 * IP 입력창만 있는 단순한 화면을 코드로 직접 그린다.
 */
UCLASS()
class MYPROJECT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 호스트가 열 맵 이름(패키지 이름, 확장자 없이). 게임을 바로 시작하지 않고
	 * 대기실(로비) 맵을 먼저 열어서, 방장이 인원이 모인 뒤 직접 시작할 수 있게 한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Menu")
	FName MapToHost = FName(TEXT("Lvl_Lobby"));

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** 접속할 IP 를 입력받는 텍스트박스(참조를 들고 있어야 클릭 시 값을 읽을 수 있다) */
	TSharedPtr<class SEditableTextBox> IPTextBox;

	FReply OnHostClicked();
	FReply OnJoinClicked();
};
