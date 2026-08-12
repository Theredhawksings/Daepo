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

	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.0f;

	/** 현재 체력 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 100.0f;

	/** 대포에 맞는 등으로 피해를 입는다. 체력이 0 이하가 되면 사망(게임 종료) 처리된다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	/** 피해 발생 위치(폭발 지점)를 함께 기록하며 피해를 입는다. 사망 시 쓰러지는 방향 판정에 쓰인다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamageFromLocation(float DamageAmount, const FVector& SourceLocation);

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

	/** 체력이 바뀔 때 호출(체력바 UI 갱신 등은 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthChanged(float NewHealth, float Delta);

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
