// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyProjectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimSequence;
class UPointLightComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMyProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** 플레이어 구분용 색깔 표시등. 머리 위에서 순번별 색으로 빛나 은은한 림라이트 효과를 준다(보조 수단). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* PlayerColorLight;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AMyProjectCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.0f;

	/** 현재 체력. 서버가 바꾸면 모든 클라이언트에 자동으로 복제되고 OnRep_Health 가 호출된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
	float Health = 100.0f;

	/** 대포에 맞는 등으로 피해를 입는다. 체력이 0 이하가 되면 사망(게임 종료) 처리된다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	/** 피해 발생 위치(폭발 지점)를 함께 기록하며 피해를 입는다. 사망 시 쓰러지는 방향 판정에 쓰인다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamageFromLocation(float DamageAmount, const FVector& SourceLocation);

	/**
	 * 이 폰을 소유한 클라이언트에서만 실행되는 디버그 메시지(서버가 호출).
	 * 그래서 Player1 이 맞으면 Player1 화면에만, Player2 가 맞으면 Player2 화면에만 뜬다.
	 */
	UFUNCTION(Client, Reliable)
	void ClientShowDamageMessage(const FString& PlayerLabel, float ActualDamage, float NewHealth);

	/** 무적 상태 on/off. 무적 중에는 ApplyDamage 가 무시된다(생존 성공 후 다음 맵 이동 대기 중 등에 사용) */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }

	/** 현재 무적 상태인지 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsInvulnerable() const { return bInvulnerable; }

	/** 사망했는지 여부 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

	/** PlayerState 가 늦게 복제돼 도착했을 때(원격 클라이언트) 호출 — 그 시점에 다시 색을 갱신한다 */
	virtual void OnRep_PlayerState() override;

	/**
	 * 서버에서 이 폰에 컨트롤러가 연결되는 시점(Possess). BeginPlay 는 이보다 먼저 실행돼
	 * 그때는 아직 PlayerState 가 없을 수 있으므로, 여기서 다시 한번 색을 갱신해야 한다.
	 */
	virtual void PossessedBy(AController* NewController) override;

	/**
	 * 순번별로 순환해서 쓸 플레이어 구분 색상 목록. 8명까지 서로 확실히 구별되는
	 * 진한 원색/보색 위주로 구성. 9번째부터는 다시 처음 색부터 순환된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Player Color")
	TArray<FLinearColor> PlayerColors = {
		FLinearColor(0.0f, 0.0f, 1.0f),    // Player1: 파랑
		FLinearColor(1.0f, 0.0f, 0.0f),    // Player2: 빨강
		FLinearColor(0.0f, 1.0f, 0.0f),    // Player3: 초록
		FLinearColor(1.0f, 0.85f, 0.0f),   // Player4: 노랑
		FLinearColor(0.55f, 0.0f, 1.0f),   // Player5: 보라
		FLinearColor(0.0f, 0.9f, 1.0f),    // Player6: 하늘색(시안)
		FLinearColor(1.0f, 0.45f, 0.0f),   // Player7: 주황
		FLinearColor(1.0f, 0.0f, 0.6f)     // Player8: 분홍
	};

	/**
	 * 캐릭터 메시를 통째로 물들이는 데 쓸 기본 재질. Vector Parameter 이름이 "Color" 인
	 * 언릿(Unlit) 재질 하나만 있으면 된다. 비워두면 색 적용을 건너뛴다(라이트만 적용).
	 */
	UPROPERTY(EditAnywhere, Category = "Player Color")
	TObjectPtr<class UMaterialInterface> PlayerColorMaterialBase;

	/** 런타임에 생성해 재사용하는 동적 머티리얼 인스턴스 */
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> PlayerColorMID;

	/** GameState 의 플레이어 목록에서 내 순번을 찾아 표시등/메시 색을 갱신한다 */
	void UpdatePlayerColor();

	/** 체력이 바뀔 때 호출(체력바 UI 갱신 등은 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthChanged(float NewHealth, float Delta);

	/** Health 가 서버에서 바뀌어 복제돼 도착했을 때 클라이언트에서 호출됨(서버 자신에게는 호출되지 않음) */
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	/** 체력이 0이 되어 사망(게임 종료)했을 때 호출(게임오버 UI/재시작 등은 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnDeath();

	/** 사망 시 조작을 막고 게임을 종료 처리한다 */
	void HandleDeath();

	/** 사망 순간 세상이 느려지는 배율(1 = 정상 속도, 0.2 = 5배 느림) */
	UPROPERTY(EditAnywhere, Category = "Health|Death", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DeathSlowMotionScale = 0.2f;

	/** 사망 후 레벨이 재시작되기까지의 시간(실제 초, 슬로모션과 무관) */
	UPROPERTY(EditAnywhere, Category = "Health|Death", meta = (ClampMin = "0.1"))
	float DeathRestartDelay = 2.0f;

	/** 슬로모션을 풀고 현재 레벨을 처음부터 다시 연다 */
	void RestartLevel();

	/** 사망 → 재시작 지연 타이머 */
	FTimerHandle RestartTimerHandle;

	/** 앞으로 쓰러지는 애니(뒤에서 맞았을 때) */
	UPROPERTY(EditAnywhere, Category = "Health|Death")
	TObjectPtr<UAnimSequence> DeathAnimFront;

	/** 뒤로 넘어가는 애니(앞에서 맞았을 때) */
	UPROPERTY(EditAnywhere, Category = "Health|Death")
	TObjectPtr<UAnimSequence> DeathAnimBack;

	/** 왼쪽으로 쓰러지는 애니(오른쪽에서 맞았을 때) */
	UPROPERTY(EditAnywhere, Category = "Health|Death")
	TObjectPtr<UAnimSequence> DeathAnimLeft;

	/** 오른쪽으로 쓰러지는 애니(왼쪽에서 맞았을 때) */
	UPROPERTY(EditAnywhere, Category = "Health|Death")
	TObjectPtr<UAnimSequence> DeathAnimRight;

	/** 마지막 피해가 날아온 위치(쓰러질 방향 판정용) */
	FVector LastDamageSourceLocation = FVector::ZeroVector;

	/** 위 값이 유효한지(위치 정보 없이 죽으면 기본 애니 사용) */
	bool bHasDamageSource = false;

	/** 피해 방향에 맞는 사망 애니를 고른다 */
	UAnimSequence* PickDeathAnim() const;

	/** 이미 사망 처리되었는지(중복 처리 방지) */
	bool bIsDead = false;

	/** true 인 동안 ApplyDamage 무시 */
	bool bInvulnerable = false;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
