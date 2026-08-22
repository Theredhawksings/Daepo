// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 * 대기실 UI. 순수 C++/Slate 로 그린다(UMG 디자이너 작업 불필요).
 * 접속 인원 수를 계속 갱신해서 보여주고, 방장(서버)에게만 "Start Game" 버튼을 보여준다.
 * 나머지 플레이어에게는 "대기 중" 문구만 보인다.
 */
UCLASS()
class MYPROJECT_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** 인원 수 표시 텍스트(참조를 들고 있어야 매 프레임 값을 갱신할 수 있다) */
	TSharedPtr<class STextBlock> PlayerCountText;

	/** 마지막으로 표시한 인원 수(불필요한 SetText 호출 방지용) */
	int32 LastDisplayedCount = -1;

	FReply OnStartClicked();
};
