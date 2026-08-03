// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyProjectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UMaterialInterface;
class ADaePo;
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

	/** 빌드(설치) 모드 토글 - E 키에 매핑 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleBuildAction;

	/** 미리보기 위치에 대포 설치 - 마우스 왼쪽 버튼에 매핑 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PlaceBuildAction;

	/** 설치할 대포 클래스 (BP_DaePo 등) */
	UPROPERTY(EditAnywhere, Category="Build")
	TSubclassOf<ADaePo> CannonClass;

	/** 미리보기용 재질(파란 반투명). 비우면 원본 재질 그대로 미리보기 */
	UPROPERTY(EditAnywhere, Category="Build")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	/** 설치 불가 위치일 때 미리보기 재질(빨강 등). 비우면 색 전환 없이 설치만 막음 */
	UPROPERTY(EditAnywhere, Category="Build")
	TObjectPtr<UMaterialInterface> InvalidPreviewMaterial;

	/** 설치 위치를 찾는 라인트레이스 최대 거리 */
	UPROPERTY(EditAnywhere, Category="Build", meta=(ClampMin="100.0"))
	float BuildTraceDistance = 3000.0f;

	/** 대포끼리 최소 간격(cm). 이보다 가까우면 설치 불가 */
	UPROPERTY(EditAnywhere, Category="Build", meta=(ClampMin="0.0"))
	float PlacementSpacing = 200.0f;

public:

	/** Constructor */
	AMyProjectCharacter();

	virtual void Tick(float DeltaSeconds) override;

	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.0f;

	/** 현재 체력 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 100.0f;

	/** 대포에 맞는 등으로 피해를 입는다. 체력이 0 이하가 되면 사망(게임 종료) 처리된다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	/** 사망했는지 여부 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 빌드 모드 진입/종료 (E) */
	void ToggleBuildMode();

	/** 현재 미리보기 위치에 대포 설치 (좌클릭) */
	void ConfirmPlacement();

	/** 카메라가 바라보는 지면 위치/회전을 구한다. 지면을 못 찾으면 false. */
	bool GetPlacementTransform(FTransform& OutTransform) const;

	/** 해당 위치에 설치 가능한지(근처에 다른 대포가 없는지) */
	bool IsPlacementValid(const FVector& Location) const;

	/** 현재 빌드 모드인지 */
	bool bInBuildMode = false;

	/** 현재 미리보기 위치가 설치 가능한지(재질 전환/설치 판정용) */
	bool bCanPlaceHere = false;

	/** 설치 전 미리보기(고스트) 대포 */
	UPROPERTY()
	TObjectPtr<ADaePo> PreviewCannon;

	/** 체력이 바뀔 때 호출(체력바 UI 갱신 등은 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthChanged(float NewHealth, float Delta);

	/** 체력이 0이 되어 사망(게임 종료)했을 때 호출(게임오버 UI/재시작 등은 블루프린트에서 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnDeath();

	/** 사망 시 조작을 막고 게임을 종료 처리한다 */
	void HandleDeath();

	/** 이미 사망 처리되었는지(중복 처리 방지) */
	bool bIsDead = false;

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

