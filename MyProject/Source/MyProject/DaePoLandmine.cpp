// Fill out your copyright notice in the Description page of Project Settings.

#include "DaePoLandmine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "MyProjectCharacter.h"
#include "MyProjectGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/OverlapResult.h"

ADaePoLandmine::ADaePoLandmine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
	RootComponent = MineMesh;

	// 플레이어 이동을 막으면 안 되므로(밟고 지나갈 수 있어야 함) 순수 시각적 표시로만 쓴다.
	MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MineMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MineMeshAsset.Succeeded())
	{
		MineMesh->SetStaticMesh(MineMeshAsset.Object);
	}
	// 납작한 원판처럼 보이게 눌러서 경고 표시(지뢰처럼) 형태로 만든다.
	MineMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.05f));

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(RootComponent);
	TriggerSphere->SetSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
}

void ADaePoLandmine::BeginPlay()
{
	Super::BeginPlay();

	if (WarningMaterial && MineMesh)
	{
		const int32 NumMats = MineMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			MineMesh->SetMaterial(i, WarningMaterial);
		}
	}

	// 트리거 감지/피해 판정은 서버에서만 한다.
	if (HasAuthority())
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ADaePoLandmine::OnTriggerBeginOverlap);
	}
}

void ADaePoLandmine::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bArmed || !HasAuthority())
	{
		return;
	}

	const AMyProjectCharacter* Character = Cast<AMyProjectCharacter>(OtherActor);
	if (!Character || Character->IsDead())
	{
		return;
	}

	// 밟은 순간 바로 터지지 않고, ArmDelay 뒤에 터지게 예약한다(도망칠 틈을 준다).
	bArmed = true;
	GetWorldTimerManager().SetTimer(ExplodeTimerHandle, this, &ADaePoLandmine::Explode, ArmDelay, false);
}

void ADaePoLandmine::Explode()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World && DamageAmount > 0.0f)
	{
		TArray<FOverlapResult> Overlaps;
		const FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);
		World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn), Sphere);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AMyProjectCharacter* Player = Cast<AMyProjectCharacter>(Overlap.GetActor()))
			{
				Player->ApplyDamageFromLocation(DamageAmount, GetActorLocation());
			}
		}
	}

	MulticastPlayExplosionEffects();

	// GameMode 에 알려서 다른 곳에 새 지뢰가 다시 채워지게 한다.
	if (AMyProjectGameMode* GM = World ? World->GetAuthGameMode<AMyProjectGameMode>() : nullptr)
	{
		GM->OnLandmineConsumed();
	}

	Destroy();
}

void ADaePoLandmine::MulticastPlayExplosionEffects_Implementation()
{
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	// 실제 폭발 파티클을 아직 안 붙였다면, 범위를 눈으로 확인할 수 있게 와이어프레임 구를 잠깐 그린다.
	DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 12, FColor::Red, false, 1.0f, 0, 2.0f);
}
