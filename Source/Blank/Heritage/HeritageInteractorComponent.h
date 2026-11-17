#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "HeritageInteractorComponent.generated.h"
class AHeritageCharacter;
class UHeritageObjectComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Heritage))
class BLANK_API UHeritageInteractorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHeritageInteractorComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    void GrabOrDrop();
    void Drop();
    void Throw();
    void Use();
    void ToggleCameraMode();
    void ExitCameraMode();
    bool IsCameraMode() const { return bCameraMode; }
    float GetScanProgress01() const;
    float GetFlashAlpha() const;
    bool CanReach(AActor* Actor, FText& OutReason) const;
    UHeritageObjectComponent* GetFocused() const;
    AActor* GetHeld() const;
    FText GetPromptText() const;
    UPROPERTY(EditAnywhere, Category="Heritage") float FocusDistance = 400.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ReachDistance = 220.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float MaxReachHeight = 190.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float HoldDistance = 110.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float HoldVerticalOffset = -15.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float BreakDistance = 220.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ThrowSpeed = 1100.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float PawnCollisionRestoreDelay = 0.3f;
    UPROPERTY(EditAnywhere, Category="Heritage") float UseAssistRadius = 160.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float UseAssistDistance = 100.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScanDistance = 800.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScanHoldTime = 1.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float NormalFOV = 90.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float CameraFOV = 55.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FOVInterpSpeed = 10.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FlashDuration = 0.15f;
private:
    bool bCameraMode = false;
    float ScanProgress = 0.f;
    float FlashEndTime = 0.f;
    AActor* UseTarget() const;
    FVector HoldTarget() const;
    void Release(bool bThrow, bool bLog);
    UPROPERTY() TObjectPtr<AHeritageCharacter> Character;
    TWeakObjectPtr<UHeritageObjectComponent> Focused;
    TWeakObjectPtr<AActor> Held;
    TWeakObjectPtr<UPrimitiveComponent> HeldBody;
    float FocusHitDistance = 0.f;
    struct FCollisionRestore
    {
        ECollisionResponse Response = ECR_Block;
        FTimerHandle Timer;
    };
    TMap<TWeakObjectPtr<UPrimitiveComponent>, FCollisionRestore> CollisionRestores;
};
