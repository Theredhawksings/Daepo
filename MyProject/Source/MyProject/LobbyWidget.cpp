// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyWidget.h"
#include "LobbyGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

namespace
{
	/** 이 PC 가 LAN 에서 쓰는 로컬 IP 를 알아낸다(호스트 IP 표시용) */
	FString GetLocalIPAddressString()
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (!SocketSubsystem)
		{
			return TEXT("알 수 없음");
		}

		bool bCanBind = false;
		const TSharedPtr<FInternetAddr> LocalIp = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBind);
		if (LocalIp.IsValid() && LocalIp->IsValid())
		{
			return LocalIp->ToString(false);
		}

		return TEXT("알 수 없음");
	}
}

TSharedRef<SWidget> ULobbyWidget::RebuildWidget()
{
	// 이 폰을 소유한 컨트롤러가 서버 권위를 갖고 있으면(=리슨 서버의 방장 자신) Start 버튼을 보여준다.
	// 원격 클라이언트는 자기 컨트롤러가 권위를 가질 수 없으므로 항상 false 가 되어 대기 문구만 본다.
	const APlayerController* OwningPC = GetOwningPlayer();
	const bool bIsHost = OwningPC && OwningPC->HasAuthority();

	TSharedRef<SWidget> IPRowWidget = SNullWidget::NullWidget;
	if (bIsHost)
	{
		IPRowWidget = SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("내 IP: %s"), *GetLocalIPAddressString())))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			.ColorAndOpacity(FLinearColor(0.6f, 0.85f, 1.0f));
	}

	TSharedRef<SWidget> BottomSlotWidget = SNullWidget::NullWidget;
	if (bIsHost)
	{
		BottomSlotWidget = SNew(SBox)
			.WidthOverride(200.0f)
			.HeightOverride(50.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.15f, 0.6f, 0.25f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked(FOnClicked::CreateUObject(this, &ULobbyWidget::OnStartClicked))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Start Game")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
				]
			];
	}
	else
	{
		BottomSlotWidget = SNew(STextBlock)
			.Text(FText::FromString(TEXT("방장이 시작하기를 기다리는 중...")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f));
	}

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.15f))
			.Padding(1.5f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f))
				.Padding(FMargin(40.0f, 32.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 16)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("WAITING ROOM")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
						.ColorAndOpacity(FLinearColor::White)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 8)
					.HAlign(HAlign_Center)
					[
						SAssignNew(PlayerCountText, STextBlock)
						.Text(FText::FromString(TEXT("현재 인원: 0명")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
						.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 24)
					.HAlign(HAlign_Center)
					[
						IPRowWidget
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 16)
					.HAlign(HAlign_Center)
					[
						BottomSlotWidget
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(160.0f)
						.HeightOverride(38.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.5f, 0.15f, 0.15f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked(FOnClicked::CreateUObject(this, &ULobbyWidget::OnCancelClicked))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("나가기")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
								.ColorAndOpacity(FLinearColor::White)
								.Justification(ETextJustify::Center)
							]
						]
					]
				]
			]
		];
}

void ULobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!PlayerCountText.IsValid() || !GetWorld())
	{
		return;
	}

	// GameState->PlayerArray 는 엔진이 기본으로 모든 클라이언트에 복제해주므로 별도 코드 없이 쓸 수 있다.
	const AGameStateBase* GS = GetWorld()->GetGameState();
	const int32 Count = GS ? GS->PlayerArray.Num() : 0;
	if (Count != LastDisplayedCount)
	{
		LastDisplayedCount = Count;
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("현재 인원: %d명"), Count)));
	}
}

FReply ULobbyWidget::OnStartClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->HasAuthority())
	{
		return FReply::Handled();
	}

	if (ALobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr)
	{
		LobbyGM->StartGame();
	}

	return FReply::Handled();
}

FReply ULobbyWidget::OnCancelClicked()
{
	// 서버(방장)에서 부르면 세션 자체가 끝나서 접속해 있던 클라이언트도 같이 메인 메뉴로 넘어가고,
	// 클라이언트에서 부르면 자기만 접속을 끊고 나간다(OpenLevel 이 서버/클라이언트 여부에 따라 알아서 처리).
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
	return FReply::Handled();
}
