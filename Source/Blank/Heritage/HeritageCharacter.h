#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HeritageCharacter.generated.h"

class UCameraComponent;
class UPhysicsHandleComponent;
class UInputAction;
class UInputMappingContext;
class UHeritageInteractorComponent;

UCLASS()
class BLANK_API AHeritageCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AHeritageCharacter();
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void OnJumped_Implementation() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void Dash();
    void EndDash();
    void ClearTransport();

    UPROPERTY(EditAnywhere, Category="Heritage") float DashSpeed = 1600.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DashDuration = 0.25f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DashCooldown = 0.8f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DashImpactMagnitude = 800.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DashPushSpeed = 500.f; // cm/s velocity change, mass-independent
    UPROPERTY(EditAnywhere, Category="Heritage") float DashSweepPadding = 15.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DashSweepDistance = 60.f;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UPhysicsHandleComponent> PhysicsHandle;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UHeritageInteractorComponent> Interactor;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector CameraOffset = FVector(0, 0, 64);
    UPROPERTY(EditAnywhere, Category="Heritage") float WalkSpeed = 450.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float JumpSpeed = 500.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float AirControlAmount = 0.35f;
protected:
    void InstallMappingContext();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartJump();
    void EndJump();
    void StartCrouch();
    void EndCrouch();
    void ResetLevel();
    void ToggleMap();
    void ToggleArchive();
    void TransportPressed(int32 Direction);
    void TransportReleased(int32 Direction);
    void Back();
    void Confirm();
    void Navigate(int32 X, int32 Y);

    UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
    UPROPERTY() TObjectPtr<UInputAction> MoveAction;
    UPROPERTY() TObjectPtr<UInputAction> LookAction;
    UPROPERTY() TObjectPtr<UInputAction> JumpAction;
    UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
    UPROPERTY() TObjectPtr<UInputAction> ResetAction;
    UPROPERTY() TObjectPtr<UInputAction> GrabAction;
    UPROPERTY() TObjectPtr<UInputAction> ThrowAction;
    UPROPERTY() TObjectPtr<UInputAction> UseAction;
    UPROPERTY() TObjectPtr<UInputAction> DashAction;
    UPROPERTY() TObjectPtr<UInputAction> CameraModeAction;
    UPROPERTY() TObjectPtr<UInputAction> MapAction;
    UPROPERTY() TObjectPtr<UInputAction> ArchiveAction;
    UPROPERTY() TObjectPtr<UInputAction> ForwardAction;
    UPROPERTY() TObjectPtr<UInputAction> RewindAction;
    UPROPERTY() TObjectPtr<UInputAction> BackAction;
    UPROPERTY() TObjectPtr<UInputAction> ConfirmAction;
    UPROPERTY() TObjectPtr<UInputAction> NavUpAction;
    UPROPERTY() TObjectPtr<UInputAction> NavDownAction;
    UPROPERTY() TObjectPtr<UInputAction> NavLeftAction;
    UPROPERTY() TObjectPtr<UInputAction> NavRightAction;
private:
    bool bForwardHeld = false;
    bool bRewindHeld = false;
    bool bDashing = false;
    float DashEndTime = 0.f;
    float NextDashTime = 0.f;
    float SavedFriction = 0.f;
    float SavedBraking = 0.f;
    FVector DashDirection = FVector::ForwardVector;
    TSet<TWeakObjectPtr<AActor>> DashHits;
    FText FirstDashHit;
};
