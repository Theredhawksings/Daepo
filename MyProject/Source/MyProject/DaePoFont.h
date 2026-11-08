// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

/** 게임 UI 공용 폰트. 재배포 가능한 엔진 기본 폰트(한글 포함)를 쓴다. */
inline FSlateFontInfo DaePoFont(FName Style, int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(Style, Size);
}
