// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoTimerActor.h"
#include "Components/TextRenderComponent.h"
#include "MyProjectGameMode.h"
#include "DaePoGameState.h"

ADaePoTimerActor::ADaePoTimerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TimerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TimerText"));
	RootComponent = TimerText;

	TimerText->SetHorizontalAlignment(EHTA_Center);
	TimerText->SetVerticalAlignment(EVRTA_TextCenter);
	TimerText->SetWorldSize(TextWorldSize);
	TimerText->SetTextRenderColor(TextColor);
	TimerText->SetText(FText::FromString(TEXT("00:00:00.00")));
}

void ADaePoTimerActor::BeginPlay()
{
	Super::BeginPlay();

	TimerText->SetWorldSize(TextWorldSize);
	TimerText->SetTextRenderColor(TextColor);

	// 이미 진행 중인 세션에 늦게 들어온 경우를 대비해, GameState 에 값이 있으면 그걸로 시작한다.
	if (const ADaePoGameState* DaePoGameState = GetWorld() ? GetWorld()->GetGameState<ADaePoGameState>() : nullptr)
	{
		DisplayedElapsedSeconds = DaePoGameState->ReplicatedElapsedSeconds;
	}

	UpdateDisplayedText(DisplayedElapsedSeconds, true);
}

void ADaePoTimerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// GameMode 는 서버에만 존재해 클라이언트에서는 항상 nullptr 이라 여기서 쓸 수 없다.
	// GameState 는 서버/클라이언트 모두에 복제되어 존재하므로 이걸 읽는다.
	const ADaePoGameState* DaePoGameState = GetWorld() ? GetWorld()->GetGameState<ADaePoGameState>() : nullptr;
	if (!DaePoGameState)
	{
		return;
	}

	// 대기시간(PreGameDelay) 동안은 서버도 경과 시간을 흘려보내지 않고 0으로 고정해두는데,
	// 여기서 로컬 증가분을 무조건 더해버리면 서버 값과의 오차 보정 주기(0.5초) 동안 계속
	// 0~0.5초 사이를 오르락내리락하는 톱니 모양으로 보인다. 그래서 대기 중엔 아예 로컬 증가를
	// 멈추고 서버 값(0)을 그대로 표시한다.
	if (!DaePoGameState->bGameStarted)
	{
		DisplayedElapsedSeconds = DaePoGameState->ReplicatedElapsedSeconds;
		UpdateDisplayedText(DisplayedElapsedSeconds);
		return;
	}

	// 서버 값(ReplicatedElapsedSeconds)은 네트워크 업데이트 주기로만 갱신되어 계단식으로 보인다.
	// 매 프레임 로컬에서 시간을 흘려보내고, 실제 값과 0.5초 이상 벌어졌을 때만(초기 동기화,
	// 일시정지/재시작 등) 스냅해서 맞춘다. 이러면 사이 구간은 부드럽게, 큰 변화는 정확하게 반영된다.
	DisplayedElapsedSeconds += DeltaSeconds;

	const double ServerElapsed = DaePoGameState->ReplicatedElapsedSeconds;
	if (FMath::Abs(DisplayedElapsedSeconds - ServerElapsed) > 0.5)
	{
		DisplayedElapsedSeconds = ServerElapsed;
	}

	UpdateDisplayedText(DisplayedElapsedSeconds);
}

void ADaePoTimerActor::UpdateDisplayedText(double Elapsed, bool bForce)
{
	const int32 TotalCentiseconds = FMath::FloorToInt(Elapsed * 100.0);
	if (!bForce && TotalCentiseconds == LastDisplayedCentiseconds)
	{
		return;
	}
	LastDisplayedCentiseconds = TotalCentiseconds;

	TimerText->SetText(AMyProjectGameMode::FormatElapsedTime(Elapsed));
}
