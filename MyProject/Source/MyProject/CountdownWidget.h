// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CountdownWidget.generated.h"

/**
 * 게임플레이 맵의 대기시간(PreGameDelay) 동안 화면 중앙에 남은 초를 크게 보여주는 UI.
 * 순수 Slate 로 그린다(UMG 디자이너 작업 불필요). 게임이 시작되면(GameState.bGameStarted)
 * 자동으로 화면에서 사라진다.
 */
UCLASS()
class MYPROJECT_API UCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** 카운트다운 숫자 표시 텍스트(참조를 들고 있어야 매 프레임 값을 갱신할 수 있다) */
	TSharedPtr<class STextBlock> CountdownText;

	/** 마지막으로 표시한 값(정수 초, 불필요한 SetText 호출 방지용). -2 는 "아직 표시 안 함" */
	int32 LastDisplayedSeconds = -2;

	/** 게임이 시작돼서 이미 화면에서 치운 상태인지(매 프레임 반복 처리 방지) */
	bool bRemoved = false;
};
