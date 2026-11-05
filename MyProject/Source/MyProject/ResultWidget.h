// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultWidget.generated.h"

/** 게임이 끝나면 뜨는 승리/패배 화면. 순수 Slate 로 그린다. */
UCLASS()
class MYPROJECT_API UResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** AddToViewport 전에 지정한다 */
	bool bWon = false;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply OnMenuClicked();
};
