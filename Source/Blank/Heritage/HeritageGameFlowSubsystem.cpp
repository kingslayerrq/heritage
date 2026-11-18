#include "HeritageGameFlowSubsystem.h"
#include "HeritageDemoRoom.h"
#include "HeritageFilmDirector.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageProps.h"
#include "HeritageObjectComponent.h"
#include "HeritageHUD.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UHeritageGameFlowSubsystem::OwnsReel(FName ReelId) const { return OwnedReels.Contains(ReelId); }
void UHeritageGameFlowSubsystem::CollectReel(FName ReelId)
{
    if (OwnsReel(ReelId)) { return; }
    OwnedReels.Add(ReelId); ObjectiveStage = FMath::Max(ObjectiveStage, 3);
    UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "ReelOwned", "Reel added. Press [N] to open the Archive."));
}
void UHeritageGameFlowSubsystem::PreparePlayer()
{
    if (auto* Player = Cast<AHeritageCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Player->EndDash(); Player->StopJumping(); Player->Interactor->Drop();
        Player->Interactor->ExitCameraMode(); Player->ClearTransport();
        Player->GetCharacterMovement()->StopMovementImmediately();
    }
}
void UHeritageGameFlowSubsystem::TeleportBetween(AHeritageDemoRoom* From, AHeritageDemoRoom* To)
{
    if (auto* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && From && To)
    {
        const FTransform Local = Player->GetActorTransform().GetRelativeTransform(From->GetActorTransform());
        Player->SetActorTransform(Local * To->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
    }
}
bool UHeritageGameFlowSubsystem::EnterFilm(FName ReelId)
{
    if (!OwnsReel(ReelId)) { return false; }
    if (ReelId != FName(TEXT("Reel1")))
    {
        UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "UnreadableReel", "This reel cannot be read yet.")); return false;
    }
    if (Era != EHeritageEra::Reality || !RealityRoom || !FilmRoom || !Director) { return false; }
    PreparePlayer(); CloseScreen(); TeleportBetween(RealityRoom, FilmRoom); Era = EHeritageEra::Film;
    Director->BeginVisit(); ApplyPostProcess(); ObjectiveStage = FMath::Max(ObjectiveStage, 4);
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Film, FText::GetEmpty(), FText::GetEmpty(), NSLOCTEXT("Heritage", "EnteredFilm", "Entered Reel 1."));
    return true;
}
void UHeritageGameFlowSubsystem::LeaveFilm()
{
    if (Era != EHeritageEra::Film || !Director || !RealityRoom || !FilmRoom) { return; }
    PreparePlayer(); Director->EndVisit(); CloseScreen(); TeleportBetween(FilmRoom, RealityRoom); Era = EHeritageEra::Reality;
    ApplyPostProcess(); if (Director->HasShownLastFrame() || Director->GetFirstUnrestoredFrame() == INDEX_NONE) { ObjectiveStage = 8; }
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Film, FText::GetEmpty(), FText::GetEmpty(), NSLOCTEXT("Heritage", "LeftFilm", "Returned to Reality."));
}
void UHeritageGameFlowSubsystem::ToggleReel()
{
    if (Era == EHeritageEra::Film) { LeaveFilm(); }
    else { EnterFilm(TEXT("Reel1")); }
}
void UHeritageGameFlowSubsystem::ChangeScreen(EHeritageScreen Screen)
{
    if (Screen == OpenScreen) { return; }
    const bool bWasOpen = OpenScreen != EHeritageScreen::None;
    PreparePlayer(); OpenScreen = Screen;
    if (Director && Era == EHeritageEra::Film) { Director->SetScreenSuspended(Screen != EHeritageScreen::None); }
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        const bool bOpen = Screen != EHeritageScreen::None;
        PC->bShowMouseCursor = bOpen;
        if (bOpen)
        {
            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(InputMode);
        }
        else
        {
            // This mode sets viewport focus and high-precision capture immediately in 5.8.
            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(true);
            PC->SetInputMode(InputMode);
        }
        if (auto* HUD = Cast<AHeritageHUD>(PC->GetHUD())) { HUD->ResetNavigation(); }
    }
    // Stop movement already in progress, not just subsequent input events.
    if (auto* Player = Cast<AHeritageCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if (Screen != EHeritageScreen::None) { Player->GetCharacterMovement()->DisableMovement(); }
        else if (bWasOpen) { Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling); }
    }
}
void UHeritageGameFlowSubsystem::ToggleScreen(EHeritageScreen Screen)
{
    ChangeScreen(OpenScreen == EHeritageScreen::None ? Screen : EHeritageScreen::None);
}
void UHeritageGameFlowSubsystem::CloseScreen() { ChangeScreen(EHeritageScreen::None); }
bool UHeritageGameFlowSubsystem::CanUseVerbs() const
{
    return CanMove() && (Era == EHeritageEra::Reality || (Director && Director->GetMode() == EHeritageFilmMode::Pause));
}
bool UHeritageGameFlowSubsystem::CanUseCamera() const { return CanMove() && Era == EHeritageEra::Reality; }
bool UHeritageGameFlowSubsystem::CanMove() const { return OpenScreen == EHeritageScreen::None; }
bool UHeritageGameFlowSubsystem::RequiresScanHere() const { return Era == EHeritageEra::Reality && UHeritageStatics::RequiresScan(); }
void UHeritageGameFlowSubsystem::NotifyScan() { ObjectiveStage = FMath::Max(ObjectiveStage, 2); }
void UHeritageGameFlowSubsystem::NotifyMissingFrame() { ObjectiveStage = FMath::Max(ObjectiveStage, 5); }
void UHeritageGameFlowSubsystem::UnlockLeafReward()
{
    if (bLeafRewardUnlocked) { return; }
    bLeafRewardUnlocked = true;
    UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "KnowWhere", "You know where to look now."));
    if (RealityRoom && RealityRoom->Reel2 && !OwnsReel(TEXT("Reel2")))
    {
        RealityRoom->Reel2->SetActorHiddenInGame(false); RealityRoom->Reel2->SetActorEnableCollision(true);
    }
}
void UHeritageGameFlowSubsystem::CompleteSlice() { bSliceComplete = true; ObjectiveStage = 8; }
void UHeritageGameFlowSubsystem::ApplyPostProcess()
{
    auto* Player = Cast<AHeritageCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player || !Player->Camera) { return; }
    auto& Settings = Player->Camera->PostProcessSettings;
    const bool bFilm = Era == EHeritageEra::Film;
    Settings.bOverride_ColorSaturation = true; Settings.ColorSaturation = FVector4(1, 1, 1, bFilm ? FilmSaturation : RealitySaturation);
    Settings.bOverride_ColorGain = true; Settings.ColorGain = bFilm ? FilmGain : FVector4(1, 1, 1, 1);
    Settings.bOverride_FilmGrainIntensity = true; Settings.FilmGrainIntensity = bFilm ? FilmGrain : 0.f;
    Settings.bOverride_VignetteIntensity = true; Settings.VignetteIntensity = bFilm ? FilmVignette : 0.f;
    Settings.bOverride_MotionBlurAmount = true; Settings.MotionBlurAmount = 0.f;
    Player->Camera->PostProcessBlendWeight = 1.f;
}
FText UHeritageGameFlowSubsystem::GetObjectiveText() const
{
    if (bSliceComplete) { return NSLOCTEXT("Heritage", "SliceCompleteObjective", "SLICE COMPLETE - [R] Reset"); }
    const int32 FirstUnrestored = Director ? Director->GetFirstUnrestoredFrame() : INDEX_NONE;
    if (Director && (Director->HasShownLastFrame() || FirstUnrestored == INDEX_NONE))
    {
        return Era == EHeritageEra::Film ? NSLOCTEXT("Heritage", "V2ObjectiveLeave", "Leave the reel [Enter]. Use what you saw.") : NSLOCTEXT("Heritage", "Objective8", "Open the door.");
    }
    if (Era == EHeritageEra::Reality)
    {
        if (RealityRoom && RealityRoom->Portrait && RealityRoom->Portrait->Object->HasState(HeritageTags::State_Open))
        {
            return NSLOCTEXT("Heritage", "Objective8", "Open the door.");
        }
        if (OwnsReel(TEXT("Reel1"))) { return NSLOCTEXT("Heritage", "V2ObjectiveEnter", "Open the Archive [N], or enter the reel [Enter]."); }
        return ObjectiveStage <= 1 ? NSLOCTEXT("Heritage", "Objective1", "Take out the camera [Tab] and scan something of value.") :
            NSLOCTEXT("Heritage", "V2ObjectiveScan", "Scan the film reel on the table.");
    }
    if (Director)
    {
        if (Director->IsImaged(FirstUnrestored))
        {
            return FText::Format(NSLOCTEXT("Heritage", "V2ObjectiveImaged", "Frame {0} imaged. Open the Archive [N] and upload."), FText::AsNumber(FirstUnrestored + 1));
        }
        if (Director->IsLocked(Director->GetSceneIndex() + 1))
        {
            return FText::Format(NSLOCTEXT("Heritage", "ObjectiveRewindToGap", "Rewind [Q] to frame {0}, then pause [Space] and act it out."), FText::AsNumber(FirstUnrestored + 1));
        }
    }
    return ObjectiveStage >= 5 ? NSLOCTEXT("Heritage", "V2ObjectiveMissing", "A frame is missing. Pause [Space] and act out what happened.") :
        NSLOCTEXT("Heritage", "Objective4", "Watch. [Space] pause | [E] forward | [Q] rewind.");
}
