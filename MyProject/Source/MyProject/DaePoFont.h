// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"

/**
 * 게임 UI 공용 폰트(KBO 다이아고딕 Bold). 파일이 없으면 엔진 기본 폰트로 대신한다.
 * Bold 한 가지 굵기만 있어서 Style 은 기본 폰트로 떨어질 때만 쓰인다.
 */
inline FSlateFontInfo DaePoFont(FName Style, int32 Size)
{
	const FString FontPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Fonts/KBODiaGothic-Bold.ttf"));
	if (FPaths::FileExists(FontPath))
	{
		return FSlateFontInfo(FontPath, Size);
	}
	return FCoreStyle::GetDefaultFontStyle(Style, Size);
}
