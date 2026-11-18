#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HeritageTypes.h"
#include "HeritageGameFlowSubsystem.generated.h"

class AHeritageDemoRoom;
class AHeritageFilmDirector;

UCLASS()
class BLANK_API UHeritageGameFlowSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    EHeritageEra Era = EHeritageEra::Reality;
    EHeritageScreen OpenScreen = EHeritageScreen::None;
    UPROPERTY() TObjectPtr<AHeritageDemoRoom> RealityRoom;
    UPROPERTY() TObjectPtr<AHeritageDemoRoom> FilmRoom;
    UPROPERTY() TObjectPtr<AHeritageFilmDirector> Director;
    TArray<FName> OwnedReels;
    bool bLeafRewardUnlocked = false;
    bool bSliceComplete = false;
    int32 ObjectiveStage = 1;
    bool OwnsReel(FName ReelId) const;
    void CollectReel(FName ReelId);
    bool EnterFilm(FName ReelId);
    void LeaveFilm();
    void ToggleReel();
    void ToggleScreen(EHeritageScreen Screen);
    void CloseScreen();
    bool CanUseVerbs() const;
    bool CanUseCamera() const;
    bool CanMove() const;
    bool RequiresScanHere() const;
    void NotifyScan();
    void NotifyMissingFrame();
    void UnlockLeafReward();
    void CompleteSlice();
    void ApplyPostProcess();
    FText GetObjectiveText() const;
    UPROPERTY(EditAnywhere, Category="Heritage") float RealitySaturation = 0.55f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FilmSaturation = 0.25f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector4 FilmGain = FVector4(1.08, 1.0, 0.8, 1.0);
    UPROPERTY(EditAnywhere, Category="Heritage") float FilmGrain = 0.6f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FilmVignette = 0.8f;
private:
    void PreparePlayer();
    void ChangeScreen(EHeritageScreen Screen);
    void TeleportBetween(AHeritageDemoRoom* From, AHeritageDemoRoom* To);
};
