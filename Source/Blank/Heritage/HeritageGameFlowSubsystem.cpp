#include "HeritageGameFlowSubsystem.h"
#include "HeritageDemoRoom.h"
#include "HeritageFilmDirector.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageProps.h"
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
    if (!bOpenedMap) { bOpenedMap = true; ChangeScreen(EHeritageScreen::Map); }
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
    ApplyPostProcess(); if (bKeySceneUploaded) { ObjectiveStage = 8; }
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Film, FText::GetEmpty(), FText::GetEmpty(), NSLOCTEXT("Heritage", "LeftFilm", "Returned to Reality."));
}
void UHeritageGameFlowSubsystem::ChangeScreen(EHeritageScreen Screen)
{
    if (Screen == OpenScreen) { return; }
    const bool bWasOpen = OpenScreen != EHeritageScreen::None;
    PreparePlayer(); OpenScreen = Screen;
    if (Director && Era == EHeritageEra::Film) { Director->SetScreenSuspended(Screen != EHeritageScreen::None); }
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
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
void UHeritageGameFlowSubsystem::NotifyScan()
{
    ObjectiveStage = FMath::Max(ObjectiveStage, 2);
    if (!bOpenedArchive) { bOpenedArchive = true; ChangeScreen(EHeritageScreen::Archive); }
}
void UHeritageGameFlowSubsystem::NotifyMissingFrame() { ObjectiveStage = FMath::Max(ObjectiveStage, 5); }
void UHeritageGameFlowSubsystem::NotifyImaged() { ObjectiveStage = FMath::Max(ObjectiveStage, 6); }
void UHeritageGameFlowSubsystem::NotifyKeyUploaded() { bKeySceneUploaded = true; ObjectiveStage = Era == EHeritageEra::Film ? 7 : 8; }
void UHeritageGameFlowSubsystem::UnlockLeafReward()
{
    bLeafRewardUnlocked = true;
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
    if (bKeySceneUploaded)
    {
        return Era == EHeritageEra::Film ? NSLOCTEXT("Heritage", "Objective7", "Leave the reel [M]. Use what you learned.") : NSLOCTEXT("Heritage", "Objective8", "Open the door.");
    }
    if (Era == EHeritageEra::Reality && OwnsReel(TEXT("Reel1"))) { return NSLOCTEXT("Heritage", "Objective3", "Open the Map [M] and enter the reel."); }
    switch (ObjectiveStage)
    {
    case 1: return NSLOCTEXT("Heritage", "Objective1", "Take out the camera [Tab] and scan something of value.");
    case 2: return NSLOCTEXT("Heritage", "Objective2", "Scan the film reel on the table, then pick it up [F].");
    case 3: return NSLOCTEXT("Heritage", "Objective3", "Open the Map [M] and enter the reel.");
    case 4: return NSLOCTEXT("Heritage", "Objective4", "Watch. [Space] pause | [E] forward | [Q] rewind.");
    case 5: return NSLOCTEXT("Heritage", "Objective5", "A frame is missing. Pause and act out what happened.");
    case 6: return NSLOCTEXT("Heritage", "Objective6", "Imaged. Open the Archive [N], compose the frame and upload.");
    case 7: return NSLOCTEXT("Heritage", "Objective7", "Leave the reel [M]. Use what you learned.");
    default: return NSLOCTEXT("Heritage", "Objective8", "Open the door.");
    }
}
