// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "MyProjectCharacter.h"
#include "MyProjectPlayerController.h"
#include "DaePoGameState.h"
#include "DaePoLandmine.h"
#include "Engine/OverlapResult.h"
#include "Math/RotationMatrix.h"
#include "BotAIController.h"
#include "EngineUtils.h"

AMyProjectGameMode::AMyProjectGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	GameStateClass = ADaePoGameState::StaticClass();
}

void AMyProjectGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnBots();

	if (!bAutoStart)
	{
		return;
	}

	if (PreGameDelay > 0.0f)
	{
		// 대기시간 동안은 대포도 안 쏜다(GameState.bGameStarted == false).
		// 그동안 다른 플레이어가 합류할 시간을 번다.
		GetWorldTimerManager().SetTimer(PreGameTimerHandle, this, &AMyProjectGameMode::BeginActualGame, PreGameDelay, false);
	}
	else
	{
		BeginActualGame();
	}
}

void AMyProjectGameMode::BeginActualGame()
{
	// 대포들이 이 신호를 보고 그때부터 발사를 시작한다(값이 복제되어 클라이언트에도 전달됨).
	if (ADaePoGameState* GS = GetGameState<ADaePoGameState>())
	{
		GS->bGameStarted = true;
	}

	// 지뢰는 미리 깔아두는 대신, 게임이 시작된 뒤 플레이어들이 실제로 지나간 자리를
	// 따라 자연스럽게 채워진다(FootstepSampleInterval 마다 반복 호출됨).
	if (LandmineClass)
	{
		GetWorldTimerManager().SetTimer(FootstepSampleTimerHandle, this,
			&AMyProjectGameMode::SampleFootstepsForLandmines, FootstepSampleInterval, true);
	}
}

void AMyProjectGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 남은 대기시간을 클라이언트의 카운트다운 UI 가 읽을 수 있게 GameState 에 반영한다.
	ADaePoGameState* DaePoGameState = GetGameState<ADaePoGameState>();
	if (DaePoGameState && !DaePoGameState->bGameStarted)
	{
		// PreGameTimerHandle 이 유효하지 않으면(대기시간 0 등) 음수가 나오므로 0으로 클램프.
		const float Remaining = GetWorldTimerManager().GetTimerRemaining(PreGameTimerHandle);
		DaePoGameState->ReplicatedPreGameRemaining = FMath::Max(Remaining, 0.0f);
	}
}

void AMyProjectGameMode::NotifyPlayerDied()
{
	CheckMatchEnd(nullptr);
}

void AMyProjectGameMode::Logout(AController* Exiting)
{
	// 플레이어가 죽지 않고 그냥 접속을 끊고 나간 경우도, 남은 인원으로 승패를
	// 다시 판정한다. Super::Logout 이 실제 정리(PlayerArray 제거 등)를 하기 전에
	// 먼저 판정해서, 나가는 컨트롤러를 명시적으로 제외한 채로 센다(아직 이터레이터에
	// 남아있을 수 있고, 그 폰의 IsDead() 는 false 라서 그냥 세면 살아있는 사람으로 잘못 집계됨).
	CheckMatchEnd(Cast<APlayerController>(Exiting));

	Super::Logout(Exiting);
}

void AMyProjectGameMode::CheckMatchEnd(const APlayerController* ExcludedController)
{
	if (bMatchEnded)
	{
		return;
	}

	// 봇은 PlayerControllerIterator 에 안 잡히므로 캐릭터 자체를 센다.
	int32 AlivePlayers = 0;
	int32 AliveBots = 0;
	for (TActorIterator<AMyProjectCharacter> It(GetWorld()); It; ++It)
	{
		const AMyProjectCharacter* Character = *It;
		if (Character->IsDead() || (ExcludedController && Character->GetController() == ExcludedController))
		{
			continue;
		}

		++(Character->IsPlayerControlled() ? AlivePlayers : AliveBots);
	}

	// 플레이어가 전멸하면 패배, 봇이 전멸하고 플레이어가 혼자 남으면 승리.
	const bool bPlayerWon = AlivePlayers == 1 && AliveBots == 0;
	if (AlivePlayers > 0 && !bPlayerWon)
	{
		return;
	}

	bMatchEnded = true;
	GetWorldTimerManager().ClearTimer(FootstepSampleTimerHandle);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyProjectPlayerController* PC = Cast<AMyProjectPlayerController>(It->Get());
		if (!PC || PC == ExcludedController)
		{
			continue;
		}

		// 결과 화면이 뜬 뒤 남은 대포/지뢰에 승자가 죽지 않게 한다.
		AMyProjectCharacter* Character = Cast<AMyProjectCharacter>(PC->GetPawn());
		const bool bWon = bPlayerWon && Character && !Character->IsDead();
		if (bWon)
		{
			Character->SetInvulnerable(true);
		}
		PC->ClientShowResult(bWon);
	}
}

void AMyProjectGameMode::OnLandmineConsumed()
{
	// 개수만 줄인다 - 새 지뢰는 SampleFootstepsForLandmines 가 계속 돌면서
	// 플레이어들이 지나간 자리에 자연스럽게 다시 채운다.
	--CurrentLandmineCount;
}

void AMyProjectGameMode::SampleFootstepsForLandmines()
{
	if (!LandmineClass || CurrentLandmineCount >= MaxLandmineCount)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const AMyProjectCharacter* Character = PC ? Cast<AMyProjectCharacter>(PC->GetPawn()) : nullptr;
		if (!Character || Character->IsDead())
		{
			continue;
		}

		// 지금 이 순간 실제로 캐릭터가 서 있던(=밟고 지나갈 수 있음이 보장된) 자리를
		// 기록해두고, 무작위 지연 뒤에 그 자리에 지뢰를 스폰한다. 지연을 두는 이유는
		// 방금까지 서 있던 그 사람이 바로 밟혀서 억울하게 피해를 입지 않게 하기 위함이다.
		//
		// GetActorLocation() 은 캡슐의 중심(허리 높이)을 반환하므로, 그 절반 높이만큼
		// 빼서 실제 발이 닿는 바닥 높이로 보정한다(안 하면 지뢰가 허공에 붕 떠서 스폰됨).
		FVector FootLocation = Character->GetActorLocation();
		FootLocation.Z -= Character->GetSimpleCollisionHalfHeight();
		const float Delay = FMath::FRandRange(MinFootstepMineDelay, MaxFootstepMineDelay);

		FTimerHandle Unused;
		GetWorldTimerManager().SetTimer(Unused, FTimerDelegate::CreateUObject(
			this, &AMyProjectGameMode::TrySpawnLandmineAt, FootLocation), Delay, false);
	}
}

void AMyProjectGameMode::TrySpawnLandmineAt(FVector Location)
{
	if (!LandmineClass || CurrentLandmineCount >= MaxLandmineCount)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 예약된 지연 동안 이미 그 자리를 벗어났을 가능성이 높지만, 혹시 몰라 스폰 직전에
	// 그 자리에 아무도 없는지 한 번 더 확인해서 즉시 밟혀 억울하게 피해를 입는 일을 막는다.
	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(100.0f);
	World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), Sphere);
	if (Overlaps.Num() > 0)
	{
		return;
	}

	// 오르막길 등 경사면에도 지면에 딱 붙어 보이도록, 그 지점에서 실제 바닥 법선을
	// 구해서 지뢰의 "위" 축을 그 법선에 맞춘다(평지면 그대로 수평으로 나옴).
	FVector SpawnLocation = Location;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FHitResult GroundHit;
	const FVector TraceStart = Location + FVector(0.0f, 0.0f, 50.0f);
	const FVector TraceEnd = Location - FVector(0.0f, 0.0f, 50.0f);
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility))
	{
		SpawnLocation = GroundHit.Location;
		SpawnRotation = FRotationMatrix::MakeFromZ(GroundHit.Normal).Rotator();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (World->SpawnActor<ADaePoLandmine>(LandmineClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		++CurrentLandmineCount;
	}
}

void AMyProjectGameMode::SpawnBots()
{
	if (!HasAuthority() || NumberOfBots <= 0)
	{
		return;
	}

	for (int32 i = 0; i < NumberOfBots; ++i)
	{
		AActor* Start = FindPlayerStart(nullptr);
		if (!Start)
		{
			continue;
		}

		APawn* BotPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, Start->GetActorLocation(), Start->GetActorRotation());
		if (!BotPawn)
		{
			continue;
		}

		if (ABotAIController* BotController = GetWorld()->SpawnActor<ABotAIController>(Start->GetActorLocation(), Start->GetActorRotation()))
		{
			BotController->Possess(BotPawn);
		}
	}
}

void AMyProjectGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (const ADaePoGameState* DaePoGameState = GetGameState<ADaePoGameState>())
	{
		if (DaePoGameState->bGameStarted)
		{
			ErrorMessage = TEXT("게임이 이미 시작되어 참여할 수 없습니다.");
		}
	}
}
