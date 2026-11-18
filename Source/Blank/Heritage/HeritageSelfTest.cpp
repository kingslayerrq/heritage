#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "HeritageDemoRoom.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageObjectComponent.h"
#include "HeritageArchiveSubsystem.h"
#include "HeritageProps.h"
#include "HeritageTags.h"
#include "HeritageLog.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    constexpr int32 ExpectedChecks = 46;
    TSet<TWeakObjectPtr<UWorld>> ActiveTests;
    struct FHeritageChecks
    {
        int32 Passed = 0;
        int32 Checked = 0;
        void Check(bool bCondition, const TCHAR* Description)
        {
            ++Checked; if (bCondition) { ++Passed; }
            UE_LOG(LogHeritage, Display, TEXT("[%s] %d: %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), Checked, Description);
        }
        void Exercise(UWorld* World)
        {
            auto* Flow = World->GetSubsystem<UHeritageGameFlowSubsystem>();
            auto* Archive = World->GetSubsystem<UHeritageArchiveSubsystem>();
            auto* Player = Cast<AHeritageCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
            auto* Reality = Flow ? Flow->RealityRoom.Get() : nullptr;
            auto* Film = Flow ? Flow->FilmRoom.Get() : nullptr;
            auto* Director = Flow ? Flow->Director.Get() : nullptr;
            if (!Flow || !Archive || !Player || !Reality || !Film || !Director || !Reality->Reel1 || !Reality->Reel2 || !Reality->Portrait || !Reality->Key || !Reality->Door || !Reality->Shards || !Reality->Fireplace || !Film->Kettle || !Film->Fireplace || !Film->Key || !Film->Vase || !Film->Vase->FilmReel || !Film->Butler || !Film->Butler->Letter || !Film->Basin || !Film->Portrait || !Film->GetMainLight())
            {
                while (Checked < ExpectedChecks) { Check(false, TEXT("Required slice actors/subsystems unavailable")); } return;
            }
            Check(Flow->Era == EHeritageEra::Reality && Flow->OwnedReels.IsEmpty() && Archive->Entries.IsEmpty() && Reality->Reel2->IsHidden() && !Reality->Reel2->GetActorEnableCollision() && !Director->IsRestored(2) && !Director->IsRestored(4),
                TEXT("Fresh Reality, empty Notebook and rack, hidden Reel 2 and two unrestored frames"));
            Flow->CloseScreen(); Player->Interactor->Drop();
            FText Reason;
            FHeritageStimulus Use; Use.Type = HeritageTags::Stimulus_Use; Use.Source = Player; Use.Instigator = Player;
            FHeritageStimulus Impact; Impact.Type = HeritageTags::Stimulus_Impact; Impact.Magnitude = 1000.f; Impact.Source = Player; Impact.Instigator = Player;
            const int32 MementoTotal = Archive->GetTotalMementos();
            const auto HasCanisterEntry = [Archive](const AHeritageReelCanister* Canister)
            {
                return Canister->Object->bMemento && Canister->Object->HasState(HeritageTags::State_Scanned) && Archive->Entries.ContainsByPredicate([Canister](const auto& Entry)
                {
                    return Entry.Name.EqualTo(Canister->Object->DisplayName) && Entry.Note.EqualTo(Canister->Object->MementoNote);
                });
            };
            const bool bFirstScan = Archive->ScanMemento(Reality->Portrait->Object);
            Check(bFirstScan && Archive->Entries.Num() == 1 && Flow->OpenScreen == EHeritageScreen::None && Archive->CurrentToast.ToString().Contains(TEXT("[N]")),
                TEXT("First non-reel scan adds a Notebook entry and hints at N without opening the Archive"));
            const bool bNoPickup = !Reality->Reel1->Object->ReceiveStimulus(Use) && !Flow->OwnsReel(TEXT("Reel1"));
            // Aim down at the table to exercise focus, scan completion and the toast shown by the camera.
            const FTransform CameraBeforeScan = Player->Camera->GetRelativeTransform();
            Player->Camera->SetWorldLocationAndRotation(Reality->Reel1->GetActorLocation() + FVector(0, 0, 40), FRotator(-90, 0, 0));
            Player->Interactor->ExitCameraMode(); Player->Interactor->ToggleCameraMode();
            Player->Interactor->TickComponent(0.f, LEVELTICK_All, &Player->Interactor->PrimaryComponentTick);
            const bool bFocusedReel = Player->Interactor->GetFocused() == Reality->Reel1->Object.Get();
            Player->Interactor->TickComponent(Player->Interactor->ScanHoldTime, LEVELTICK_All, &Player->Interactor->PrimaryComponentTick);
            Player->Interactor->ExitCameraMode(); Player->Camera->SetRelativeTransform(CameraBeforeScan);
            Check(bNoPickup && bFocusedReel && Flow->OwnsReel(TEXT("Reel1")) && Flow->OpenScreen == EHeritageScreen::None &&
                Reality->Reel1->IsHidden() && !Reality->Reel1->GetActorEnableCollision() && Player->Interactor->GetFocused() != Reality->Reel1->Object.Get() &&
                Archive->CurrentToast.ToString().Contains(TEXT("[N]")), TEXT("Camera scan grants Reel 1, removes its focus and collision, and hints at the closed Archive"));
            Check(HasCanisterEntry(Reality->Reel1) && !Archive->ScanMemento(Reality->Reel1->Object) && Archive->Entries.Num() == 2 && Archive->GetTotalMementos() == MementoTotal,
                TEXT("Hidden Reel 1 keeps its Notebook entry and memento total and cannot be scanned twice"));
            const FString ReelObjective = Flow->GetObjectiveText().ToString();
            Check(ReelObjective.Contains(TEXT("[N]")) && ReelObjective.Contains(TEXT("[Enter]")), TEXT("Reality objective lists the Archive and direct entry keys after collecting Reel 1"));
            // This knowledge-gate check deliberately precedes every upload in the entire exercise.
            const bool bOpened = Reality->Portrait->Object->ReceiveStimulus(Use);
            Reality->Portrait->Tick(Reality->Portrait->SlideDuration);
            Check(bOpened && Reality->Portrait->Object->HasState(HeritageTags::State_Open) && !Reality->Key->IsHidden() && !Director->IsRestored(2) && !Director->IsRestored(4),
                TEXT("Fresh-state Reality portrait reveals key with zero frames restored"));
            const bool bKeyGated = !Reality->Key->Object->CanBeGrabbed(Reason);
            const bool bKeyScanned = Archive->ScanMemento(Reality->Key->Object);
            Check(bKeyGated && bKeyScanned && Reality->Key->Object->CanBeGrabbed(Reason) && Flow->OpenScreen == EHeritageScreen::None, TEXT("Revealed Reality key still requires scanning before grabbing and leaves the Archive closed"));
            const bool bRealityNoBlur = Player->Camera->PostProcessSettings.bOverride_MotionBlurAmount && Player->Camera->PostProcessSettings.MotionBlurAmount == 0.f;
            const FTransform ShardsBefore = Reality->Shards->GetActorTransform();
            const bool bHiddenUseRefused = !Reality->Reel1->Object->ReceiveStimulus(Use) && Flow->Era == EHeritageEra::Reality;
            Flow->ToggleReel(); const bool bEntered = Flow->Era == EHeritageEra::Film;
            Flow->ToggleReel(); const bool bLeft = Flow->Era == EHeritageEra::Reality;
            Flow->ToggleReel();
            Check(bHiddenUseRefused && bEntered && bLeft && Flow->Era == EHeritageEra::Film && Director->GetSceneIndex() == 0,
                TEXT("Use on hidden Reel 1 has no effect; ToggleReel enters, leaves and enters"));
            bool bAllFrozen = true;
            for (const auto& Pair : Director->GetActors()) { bAllFrozen &= Pair.Value && Pair.Value->IsFrozen() && !Pair.Value->Mesh->IsSimulatingPhysics() && !Pair.Value->Object->bMemento; }
            const FVector FilmLocal = Film->GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
            Check(bAllFrozen && FMath::Abs(FilmLocal.X) < Film->RoomSize.X / 2 && FMath::Abs(FilmLocal.Y) < Film->RoomSize.Y / 2 && !Flow->CanUseVerbs(),
                TEXT("Film visit freezes controlled props, transports player and gates Play verbs"));
            Check(bRealityNoBlur && Player->Camera->PostProcessSettings.bOverride_MotionBlurAmount && Player->Camera->PostProcessSettings.MotionBlurAmount == 0.f,
                TEXT("Motion blur explicitly zero in both eras"));
            Director->RecordFact(HeritageTags::Fact_KettleFilled, FText::GetEmpty());
            Check(Director->GetTakeFacts().IsEmpty(), TEXT("Playback cannot record facts"));
            Director->SetScene(1); Director->AdvanceTime(Director->FrameInterval);
            const bool bGapStart = Director->GetSceneIndex() == 2 && Film->Butler->IsHidden() && !Film->Kettle->Object->HasState(HeritageTags::State_Filled);
            Director->AdvanceTime(Director->GapSecondsPerAction * 0.5f);
            const bool bHalfGap = Director->GetSceneIndex() == 2 && FMath::IsNearlyEqual(Director->GetGapProgress01(), 0.5f);
            Director->AdvanceTime(Director->GapSecondsPerAction * 0.5f + 0.001f);
            Check(bGapStart && bHalfGap && Director->GetSceneIndex() == 3 && Film->Kettle->Object->HasState(HeritageTags::State_Filled),
                TEXT("Frame 3 gap lasts one action duration; frame 4 supplies full kettle without restoration"));
            const FText LockedStatus = NSLOCTEXT("Heritage", "LockedStatus", "LOCKED");
            const auto LockStatusesMatch = [Director, &LockedStatus](int32 LastReachable)
            {
                for (int32 Frame = 0; Frame < Director->GetSceneCount(); ++Frame)
                {
                    if (Director->GetSceneStatus(Frame).EqualTo(LockedStatus) != (Frame > LastReachable)) { return false; }
                }
                return true;
            };
            const FText RewindObjective = NSLOCTEXT("Heritage", "ObjectiveRewindToGap", "Rewind [Q] to frame {0}, then pause [Space] and act it out.");
            const FText LeaveObjective = NSLOCTEXT("Heritage", "V2ObjectiveLeave", "Leave the reel [Enter]. Use what you saw.");
            Director->AdvanceTime(100.f);
            Check(Director->GetSceneIndex() == 3 && LockStatusesMatch(3) && !Director->HasShownLastFrame() && Flow->GetObjectiveText().EqualTo(FText::Format(RewindObjective, FText::AsNumber(3))),
                TEXT("Unrestored reel stops on frame 4, reports frames 5-8 locked and directs the player back to frame 3"));
            Director->SetScene(0); Director->SetScene(7);
            Check(Director->GetSceneIndex() == 3, TEXT("Seeking to locked frame 8 clamps to frame 4"));
            Director->SetScene(0); Director->StepScene(20);
            Check(Director->GetSceneIndex() == 3, TEXT("Forward stepping from frame 1 stops on frame 4"));
            Director->SetScene(0); Director->SetTransport(true, false); Director->AdvanceTime(100.f);
            const bool bFastForwardClamped = Director->GetSceneIndex() == 3;
            Director->SetTransport(false, true); Director->AdvanceTime(Director->FrameInterval / Director->FastRate + 0.001f);
            Check(bFastForwardClamped && Director->GetSceneIndex() == 2 && Director->GetDirection() == -1 && Director->GetRate() == Director->FastRate,
                TEXT("Fast-forward stops on frame 4 and accelerated rewind returns to frame 3"));
            Director->SetTransport(false, false);
            bool bTargets = true;
            for (int32 Scene = 0; Scene <= 3; ++Scene)
            {
                Director->SetScene(Scene); Director->Pause();
                bTargets &= Director->GetTargetFrame() == (Scene <= 2 ? 2 : 4);
                Director->ResumePlay(1);
            }
            Check(bTargets, TEXT("Pause on frames 1-3 targets 3; pause on frame 4 targets 5"));
            Director->SetScene(1); Director->Pause();
            Film->Kettle->UseOn(Film->Basin, Player, Reason); Film->Kettle->UseOn(Film->Basin, Player, Reason);
            Check(Director->IsImaged(2) && !Director->IsImaged(4) && Director->GetFactsFor(2).Num() == 1 && Director->GetFactsFor(4).IsEmpty() && Flow->CanUseVerbs(),
                TEXT("Filling while paused images only frame 3, with per-session deduplication"));
            Director->ResumePlay(1); const bool bCanonicalEmpty = !Film->Kettle->Object->HasState(HeritageTags::State_Filled);
            Director->Pause();
            Check(bCanonicalEmpty && Film->Kettle->Object->HasState(HeritageTags::State_Filled), TEXT("Resume restores canonical state; re-pause restores working layer"));
            Director->ToggleFact(2, HeritageTags::Fact_KettleFilled); Director->Rollback();
            Check(!Director->IsImaged(2) && Director->GetFactsFor(2).IsEmpty() && Director->GetEdit(2).ChosenFacts.IsEmpty() && !Film->Kettle->Object->HasState(HeritageTags::State_Filled),
                TEXT("Rollback removes restored layer evidence, un-images target and prunes checklist"));
            Film->Kettle->UseOn(Film->Basin, Player, Reason);
            Director->ResumePlay(1); Director->SetScene(2); Director->Pause(); Director->Rollback();
            Check(!Director->IsImaged(2) && Director->GetFactsFor(2).IsEmpty(), TEXT("Rollback un-images its target even when recorded from a different pause scene"));
            Film->Kettle->UseOn(Film->Basin, Player, Reason);
            Film->Vase->Object->ReceiveStimulus(Impact);
            const auto Decoys = Director->GetFactsFor(2);
            Check(Flow->bLeafRewardUnlocked && !Reality->Reel2->IsHidden() && Reality->Reel2->GetActorEnableCollision() && !Film->Vase->FilmReel->IsHidden() && !Director->IsRestored(2) && !Director->IsRestored(4) &&
                Decoys.ContainsByPredicate([](const auto& F) { return F.Fact == HeritageTags::Fact_ReelFound; }) && Decoys.ContainsByPredicate([](const auto& F) { return F.Fact == HeritageTags::Fact_VaseBroken; }),
                TEXT("Vase break immediately reveals Reel 2; both facts remain checklist decoys without upload"));
            // Image frame 5 before frame 3's upload, so refusal proves ordering rather than missing evidence.
            Director->ResumePlay(1); Director->SetScene(3); Director->Pause();
            Film->Kettle->UseOn(Film->Fireplace, Player, Reason); Film->Key->OnGrabbed(Player);
            Director->ToggleFact(4, HeritageTags::Fact_FireDoused); Director->ToggleFact(4, HeritageTags::Fact_KeyTaken);
            Flow->ToggleScreen(EHeritageScreen::Archive);
            const bool bOrderedRefusal = !Director->Upload(4, Reason);
            Check(Director->IsImaged(4) && Director->GetEdit(4).ChosenFacts.Num() == 2 && Director->GetSceneStatus(4).EqualTo(LockedStatus) &&
                Flow->OpenScreen == EHeritageScreen::Archive && Director->GetMode() == EHeritageFilmMode::Pause && Director->GetSceneIndex() == 3 &&
                bOrderedRefusal && !Director->IsRestored(4) && Reason.EqualTo(FText::Format(NSLOCTEXT("Heritage", "RestoreEarlier", "Restore frame {0} first."), FText::AsNumber(3))),
                TEXT("Locked frame 5 upload returns its reason and keeps the Archive open on the same paused scene"));
            Flow->CloseScreen();
            Director->Rollback();
            Check(!Director->IsImaged(4) && Director->IsImaged(2), TEXT("Frame 5 rollback leaves frame 3 evidence intact"));
            Film->Kettle->UseOn(Film->Fireplace, Player, Reason); Film->Key->OnGrabbed(Player);
            Director->ToggleFact(4, HeritageTags::Fact_FireDoused); Director->CycleAudio(4, 1); Director->CycleLight(4, 1);
            // Retain the doused frame 4 layer and pause in reverse transport before uploading another frame.
            Director->ResumePlay(1); Director->SetTransport(false, true); Director->Pause();
            const bool bPausedInRewind = Director->GetDirection() == -1 && Director->GetRate() == Director->FastRate && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit);
            Flow->ToggleScreen(EHeritageScreen::Archive);
            const bool bEmptyRefused = !Director->Upload(2, Reason);
            Director->ToggleFact(2, HeritageTags::Fact_VaseBroken);
            const bool bWrongRefused = !Director->Upload(2, Reason);
            Director->ToggleFact(2, HeritageTags::Fact_KettleFilled);
            Check(bEmptyRefused && bWrongRefused && !Director->Upload(2, Reason) && !Director->IsRestored(2) &&
                Flow->OpenScreen == EHeritageScreen::Archive && Director->GetMode() == EHeritageFilmMode::Pause, TEXT("Empty, wrong and extra frame 3 compositions leave the Archive open and Film paused"));
            Director->ToggleFact(2, HeritageTags::Fact_VaseBroken);
            Director->CycleAudio(2, 1); Director->CycleLight(2, 2);
            const bool bUploaded3 = Director->Upload(2, Reason);
            Director->Tick(0.f); // A queued frame 5 imaging notification must not replace the upload toast.
            Check(bPausedInRewind && bUploaded3 && Flow->OpenScreen == EHeritageScreen::None && Director->GetMode() == EHeritageFilmMode::Play &&
                Director->GetDirection() == 1 && Director->GetRate() == 1.f && Director->GetSceneIndex() == 2 && !Player->Interactor->GetHeld() && Archive->CurrentToast.EqualTo(Reason),
                TEXT("Frame 3 upload closes the Archive, toasts success and starts forward 1x Play on the uploaded frame"));
            const auto& Frame5Edit = Director->GetEdit(4);
            Check(Director->IsImaged(4) && !Director->IsRestored(4) && Director->GetFactsFor(4).Num() == 2 &&
                Frame5Edit.ChosenFacts.Num() == 1 && Frame5Edit.ChosenFacts.Contains(HeritageTags::Fact_FireDoused) && Frame5Edit.AudioChoice == 1 && Frame5Edit.LightChoice == 1,
                TEXT("Frame 5 keeps its recorded facts, partial checklist, audio and light across the frame 3 upload"));
            Director->ToggleFact(2, HeritageTags::Fact_KettleFilled); Director->CycleAudio(2, 1); Director->CycleLight(2, 1);
            Check(bUploaded3 && !Director->Upload(2, Reason) && Director->IsRestored(2) && Director->GetEdit(2).ChosenFacts.Num() == 1 && Director->GetEdit(2).AudioChoice == 1 && Director->GetEdit(2).LightChoice == 2,
                TEXT("Frame 3 exact answer uploads and locks facts, audio and light"));
            Check(!Film->Butler->IsHidden() && Film->GetActorTransform().InverseTransformPosition(Film->Butler->GetActorLocation()).Equals(FVector(200, 190, 75), 0.1f) && Film->Kettle->Object->HasState(HeritageTags::State_Filled) && Director->GetAudioCaption().EqualTo(AHeritageFilmDirector::AudioName(1)) && FMath::IsNearlyEqual(Film->GetMainLight()->Intensity, Film->RoomLightIntensity * Director->DaylightMultiplier),
                TEXT("Restored frame 3 plays basin/full-kettle state and chosen presentation"));
            Director->AdvanceTime(Director->FrameInterval);
            Check(Director->GetSceneIndex() == 3, TEXT("Playback advances from the upload to frame 4 after one normal interval"));
            Director->AdvanceTime(Director->FrameInterval + Director->GapSecondsPerAction);
            const bool bHalf5 = Director->GetSceneIndex() == 4 && FMath::IsNearlyEqual(Director->GetGapProgress01(), 0.5f);
            Director->AdvanceTime(Director->GapSecondsPerAction + 0.001f);
            Check(bHalf5 && Director->GetSceneIndex() == 5 && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && !Film->Kettle->Object->HasState(HeritageTags::State_Filled) && !Film->Key->Object->HasState(HeritageTags::State_Hot),
                TEXT("Frame 5 gap lasts two action durations; frame 6 supplies complete post-gap state"));
            Director->AdvanceTime(2.f * Director->FrameInterval);
            Check(Director->GetSceneIndex() == 5 && LockStatusesMatch(5) && !Director->HasShownLastFrame(),
                TEXT("With frame 3 restored, playback stops on frame 6 and only frames 7-8 report locked"));
            Director->SetScene(4); Director->SetTransport(true, false);
            Director->AdvanceTime(2.f * Director->GapSecondsPerAction / Director->FastRate + 0.001f);
            Check(Director->GetSceneIndex() == 5 && Director->GetRate() == Director->FastRate, TEXT("Forward transport scales gap duration by rate"));
            Director->SetTransport(false, false); Director->SetScene(3); Director->Pause();
            Check(Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && Film->Kettle->Object->HasState(HeritageTags::State_Filled),
                TEXT("Pausing again on frame 4 shows its canonical fire and kettle; the doused working layer was discarded"));
            Director->Rollback(); Director->ResumePlay(1); Director->SetScene(5);
            Check(Flow->GetObjectiveText().EqualTo(FText::Format(RewindObjective, FText::AsNumber(5))), TEXT("At the new stop, missing evidence directs the player to rewind to frame 5"));
            Director->SetScene(3); Director->Pause();
            Film->Kettle->UseOn(Film->Fireplace, Player, Reason); Film->Key->OnGrabbed(Player);
            Check(Director->IsImaged(4) && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && Film->Key->Object->CanBeGrabbed(Reason), TEXT("Dousing fire and taking cooled key image frame 5"));
            Director->ToggleFact(4, HeritageTags::Fact_FireDoused);
            const bool bPartial5 = !Director->Upload(4, Reason);
            Director->ToggleFact(4, HeritageTags::Fact_KeyTaken);
            Flow->ToggleReel(); Flow->ToggleScreen(EHeritageScreen::Archive);
            const bool bUploaded5 = Director->Upload(4, Reason);
            Check(bUploaded5 && Director->IsRestored(4) && Flow->OpenScreen == EHeritageScreen::None && Flow->Era == EHeritageEra::Reality && Archive->CurrentToast.EqualTo(Reason),
                TEXT("Uploading frame 5 from Reality closes the Archive, toasts success and leaves the player in Reality"));
            Flow->ToggleReel(); Director->CycleLight(4, 1); Director->SetScene(4);
            Check(bPartial5 && bUploaded5 && !Director->Upload(4, Reason) && Director->GetEdit(4).LightChoice == 1 && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && !Film->Kettle->Object->HasState(HeritageTags::State_Filled) && !Film->Key->Object->HasState(HeritageTags::State_Hot) && Director->GetAudioCaption().EqualTo(AHeritageFilmDirector::AudioName(1)) && FMath::IsNearlyEqual(Film->GetMainLight()->Intensity, Film->RoomLightIntensity * Director->ColdLightMultiplier),
                TEXT("Frame 5 requires exact answer, locks upload and plays restored state with audio/light"));
            Check(LockStatusesMatch(7) && !Director->HasShownLastFrame() && Flow->GetObjectiveText().EqualTo(LeaveObjective),
                TEXT("Both uploads unlock the entire reel and show the leave objective before frame 8 is visited"));
            Director->Pause(); Flow->ToggleScreen(EHeritageScreen::Archive);
            const int32 BeforeFacts = Director->GetTakeFacts().Num();
            Director->RecordFact(HeritageTags::Fact_ButlerShoved, FText::GetEmpty());
            Player->Interactor->GrabOrDrop(); Player->Interactor->Throw();
            const bool bSuspended = !Flow->CanMove() && !Flow->CanUseVerbs() && Film->Kettle->IsFrozen() && !Film->Kettle->Mesh->IsSimulatingPhysics() && Director->GetTakeFacts().Num() == BeforeFacts && !Player->Interactor->GetHeld();
            Flow->CloseScreen();
            Check(bSuspended && Flow->CanMove() && Film->Kettle->Mesh->IsSimulatingPhysics(), TEXT("Open screen freezes Pause physics and rejects verbs/facts; close restores physics"));
            Director->ResumePlay(1); Director->AdvanceTime(3.f * Director->FrameInterval);
            const FVector ButlerLocal = Film->GetActorTransform().InverseTransformPosition(Film->Butler->GetActorLocation());
            Check(Director->GetSceneIndex() == 7 && Director->HasShownLastFrame() && !Film->Butler->IsHidden() && ButlerLocal.Equals(FVector(-300, 150, 75), 0.1f) && !Film->Portrait->Object->HasState(HeritageTags::State_Open) && Director->GetTargetFrame() == INDEX_NONE,
                TEXT("Restored reel reaches frame 8 with visible butler at closed portrait and no future target"));
            Director->SetScene(1); Director->AdvanceTime(0.2f);
            Flow->ToggleScreen(EHeritageScreen::Archive); Director->AdvanceTime(100.f);
            const bool bTimeFrozen = Director->GetSceneIndex() == 1;
            Flow->CloseScreen(); Director->AdvanceTime(Director->FrameInterval - 0.2f + 0.001f);
            Check(bTimeFrozen && Director->GetSceneIndex() == 2, TEXT("Screen suspends playback timer without consuming elapsed reel time"));
            Director->SetScene(0); Director->Pause(); Film->Butler->Object->ReceiveStimulus(Impact); Film->Vase->Object->ReceiveStimulus(Impact);
            const bool bDropped = Film->Butler->Letter->Object->HasState(HeritageTags::State_Dropped) && !Film->Butler->Letter->GetAttachParentActor();
            Director->Rollback();
            Check(bDropped && Film->Butler->Letter->GetAttachParentActor() == Film->Butler && !Film->Butler->Letter->Object->HasState(HeritageTags::State_Dropped) && !Film->Vase->Object->HasState(HeritageTags::State_Broken) && Film->Vase->FilmReel->IsHidden() && Flow->bLeafRewardUnlocked,
                TEXT("Rollback restores side props while discovered Reel 2 knowledge persists"));
            Flow->ToggleReel();
            Check(Flow->Era == EHeritageEra::Reality && !Reality->Fireplace->Object->HasState(HeritageTags::State_Lit) && Reality->Shards->GetActorTransform().Equals(ShardsBefore),
                TEXT("Leaving Film preserves Reality fireplace and shards"));
            const bool bRewardScan = Archive->ScanMemento(Reality->Reel2->Object);
            Flow->UnlockLeafReward();
            Check(bRewardScan && Flow->OwnsReel(TEXT("Reel2")) && Reality->Reel2->IsHidden() && !Reality->Reel2->GetActorEnableCollision() &&
                HasCanisterEntry(Reality->Reel2) && Archive->GetTotalMementos() == MementoTotal && !Archive->ScanMemento(Reality->Reel2->Object) &&
                Flow->OpenScreen == EHeritageScreen::None && !Flow->EnterFilm(TEXT("Reel2")) && !Reality->Reel2->Object->ReceiveStimulus(Use) && Flow->Era == EHeritageEra::Reality,
                TEXT("Reel 2 stays collected, logged and hidden after the vase reward repeats; scan, Use and entry are refused"));
            Reality->Key->UseOn(Reality->Door, Player, Reason);
            Check(Flow->bSliceComplete && Reality->Door->Object->HasState(HeritageTags::State_Open) && Flow->ObjectiveStage == 8 && !Archive->ActionLog.IsEmpty(),
                TEXT("Scanned Reality key opens door and completes slice with objective/log"));
        }
    };
}
void RunHeritageSelfTest(UWorld* World)
{
    if (!World || !World->IsGameWorld()) { UE_LOG(LogHeritage, Display, TEXT("Heritage.SelfTest requires a running game/PIE world.")); return; }
    if (ActiveTests.Contains(World)) { UE_LOG(LogHeritage, Display, TEXT("Heritage.SelfTest is already running.")); return; }
    ActiveTests.Add(World);
    FHeritageChecks Checks; Checks.Exercise(World);
    const bool bSuccess = Checks.Checked == ExpectedChecks && Checks.Passed == ExpectedChecks;
    UE_LOG(LogHeritage, Display, TEXT("HERITAGE SELFTEST: %s (%d/%d)"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Checks.Passed, Checks.Checked);
    ActiveTests.Remove(World);
    if (FParse::Param(FCommandLine::Get(), TEXT("HeritageSelfTest"))) { FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1); }
}
static FAutoConsoleCommandWithWorld HeritageSelfTestCommand(TEXT("Heritage.SelfTest"), TEXT("Run 46 game-loop v2 checks on a fresh level with default scan gating (1). This consumes the slice state."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&RunHeritageSelfTest));
