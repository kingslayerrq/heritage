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
            if (!Flow || !Archive || !Player || !Reality || !Film || !Director || !Reality->Reel1 || !Reality->Reel2 || !Reality->Portrait || !Reality->Key || !Reality->Door || !Reality->Shards || !Reality->Fireplace || !Film->Kettle || !Film->Fireplace || !Film->Key || !Film->Vase || !Film->Vase->FilmReel || !Film->Butler || !Film->Butler->Letter || !Film->Basin)
            {
                while (Checked < 20) { Check(false, TEXT("Required slice actors/subsystems unavailable")); } return;
            }
            Check(Flow->Era == EHeritageEra::Reality && Flow->OwnedReels.IsEmpty() && Reality->Era == EHeritageEra::Reality && Film->Era == EHeritageEra::Film,
                TEXT("Starts in Reality, both rooms and director exist, rack empty"));
            Flow->CloseScreen(); Player->Interactor->Drop();
            FText Reason;
            FHeritageStimulus Use; Use.Type = HeritageTags::Stimulus_Use; Use.Source = Player; Use.Instigator = Player;
            const bool bRefused = !Reality->Reel1->Object->ReceiveStimulus(Use) && !Flow->OwnsReel(TEXT("Reel1"));
            const bool bScanned = Archive->ScanMemento(Reality->Reel1->Object);
            const bool bAutoArchive = Flow->OpenScreen == EHeritageScreen::Archive;
            Flow->CloseScreen();
            const bool bCollected = Reality->Reel1->Object->ReceiveStimulus(Use);
            Check(bRefused && bScanned && bAutoArchive && bCollected && Flow->OwnsReel(TEXT("Reel1")) && Flow->OpenScreen == EHeritageScreen::Map,
                TEXT("Reality reel requires scan; first scan opens Archive; collection opens Map"));
            const FTransform ShardsBefore = Reality->Shards->GetActorTransform();
            const bool bEntered = Flow->EnterFilm(TEXT("Reel1"));
            bool bAllFrozen = true;
            for (const auto& Pair : Director->GetActors())
            {
                bAllFrozen &= Pair.Value && Pair.Value->IsFrozen() && !Pair.Value->Mesh->IsSimulatingPhysics() && !Pair.Value->Object->bMemento;
            }
            const FVector FilmLocal = Film->GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
            Check(bEntered && Flow->Era == EHeritageEra::Film && FMath::Abs(FilmLocal.X) < Film->RoomSize.X / 2 && FMath::Abs(FilmLocal.Y) < Film->RoomSize.Y / 2 && Director->GetSceneIndex() == 0 && Director->GetMode() == EHeritageFilmMode::Play && bAllFrozen,
                TEXT("Reel entry transports into Film Scene 1; all controlled props frozen, no Film mementos"));
            Director->SetScene(2); const bool bFull = Film->Kettle->Object->HasState(HeritageTags::State_Filled);
            Director->SetScene(0);
            Check(bFull && !Film->Kettle->Object->HasState(HeritageTags::State_Filled) && Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && Film->Key->Object->HasState(HeritageTags::State_Hot),
                TEXT("Canonical rewind restores empty kettle, lit fire and hot key"));
            for (int32 Step = 0; Step < 20; ++Step) { Director->StepScene(1); }
            Check(Director->GetSceneIndex() == Director->GetSegment().KeySceneIndex && Director->IsOnMissingFrame() && Film->Butler->IsHidden(),
                TEXT("Playback stops at the missing frame with butler hidden"));
            const bool bPlayBlocked = !Flow->CanUseVerbs();
            Director->RecordFact(HeritageTags::Fact_FireDoused, NSLOCTEXT("Heritage", "IgnoredFact", "Ignored playback fact"));
            const bool bIgnored = Director->GetTakeFacts().IsEmpty();
            Director->Pause();
            Check(bPlayBlocked && Flow->CanUseVerbs() && Film->Kettle->Mesh->IsSimulatingPhysics(), TEXT("Play blocks verbs; Pause enables verbs and kettle physics"));
            Film->Kettle->UseOn(Film->Fireplace, Player, Reason);
            const bool bCool = Film->Key->Object->CanBeGrabbed(Reason);
            Film->Key->OnGrabbed(Player); Film->Key->OnGrabbed(Player);
            Check(bIgnored && bCool && Director->IsImaged() && Director->GetTakeFacts().Num() == 2,
                TEXT("Play ignores facts; water and cool-key grab image the frame, deduped per session"));
            Director->ResumePlay(1); const bool bCanonicalFire = Film->Fireplace->Object->HasState(HeritageTags::State_Lit);
            Director->Pause();
            Check(bCanonicalFire && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit), TEXT("Resume restores canonical fire; re-pause restores working layer"));
            Director->Rollback();
            const bool bRolledBack = !Director->IsImaged() && Director->GetTakeFacts().IsEmpty() && Film->Fireplace->Object->HasState(HeritageTags::State_Lit);
            Film->Kettle->UseOn(Film->Basin, Player, Reason);
            Film->Kettle->UseOn(Film->Fireplace, Player, Reason); Film->Key->OnGrabbed(Player);
            Check(bRolledBack && Director->IsImaged(), TEXT("Rollback removes restored session evidence and state; actions can image again"));
            Director->ToggleFact(true, HeritageTags::Fact_FireDoused);
            const bool bMissingFactRefused = !Director->Upload(true, Reason) && !Director->GetEdit(true).bUploaded;
            Director->ToggleFact(true, HeritageTags::Fact_KeyTaken); Director->ToggleFact(true, HeritageTags::Fact_KettleFilled);
            Check(bMissingFactRefused && !Director->Upload(true, Reason) && !Director->GetEdit(true).bUploaded, TEXT("Incomplete and extra-fact key compositions refused without locking"));
            Director->ToggleFact(true, HeritageTags::Fact_KettleFilled);
            Director->CycleAudio(true, 1); Director->CycleLight(true, 1);
            const bool bUploaded = Director->Upload(true, Reason);
            const bool bSecondRefused = !Director->Upload(true, Reason);
            Director->ToggleFact(true, HeritageTags::Fact_KeyTaken); Director->CycleLight(true, 1);
            Director->ResumePlay(1); Director->SetScene(4);
            Check(bUploaded && bSecondRefused && Flow->bKeySceneUploaded && Director->LastPlayableScene() == 7 && !Film->Fireplace->Object->HasState(HeritageTags::State_Lit) && Director->GetEdit(true).ChosenFacts.Num() == 2 && Director->GetEdit(true).LightChoice == 1,
                TEXT("Exact answer uploads, unlocks later scenes, restores fire and makes edit immutable"));
            Director->SetScene(2); Director->Pause();
            FHeritageStimulus Impact; Impact.Type = HeritageTags::Stimulus_Impact; Impact.Magnitude = 1000.f; Impact.Source = Player; Impact.Instigator = Player;
            Film->Vase->Object->ReceiveStimulus(Impact);
            Director->ToggleFact(false, HeritageTags::Fact_ReelFound);
            const auto Distinct = Director->GetDistinctFacts();
            Check(Distinct.ContainsByPredicate([](const auto& Fact) { return Fact.Fact == HeritageTags::Fact_VaseBroken; }) && Distinct.ContainsByPredicate([](const auto& Fact) { return Fact.Fact == HeritageTags::Fact_ReelFound; }) && !Film->Vase->FilmReel->IsHidden() && Director->Upload(false, Reason) && Flow->bLeafRewardUnlocked,
                TEXT("Breaking vase records both facts; ReelFound leaf unlocks Reality reward"));
            Flow->LeaveFilm();
            const FVector RealityLocal = Reality->GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
            Check(Flow->Era == EHeritageEra::Reality && FMath::Abs(RealityLocal.X) < Reality->RoomSize.X / 2 && FMath::Abs(RealityLocal.Y) < Reality->RoomSize.Y / 2 && !Reality->Fireplace->Object->HasState(HeritageTags::State_Lit) && Reality->Shards->GetActorTransform().Equals(ShardsBefore) && !Reality->Shards->IsHidden(),
                TEXT("Return to Reality preserves cold fireplace and static shards"));
            Archive->ScanMemento(Reality->Portrait->Object); Flow->CloseScreen();
            Reality->Portrait->Object->ReceiveStimulus(Use); Reality->Portrait->Tick(Reality->Portrait->SlideDuration);
            const bool bRevealed = !Reality->Key->IsHidden();
            const bool bKeyGated = !Reality->Key->Object->CanBeGrabbed(Reason);
            Archive->ScanMemento(Reality->Key->Object); Flow->CloseScreen();
            const bool bKeyReady = Reality->Key->Object->CanBeGrabbed(Reason);
            Reality->Key->UseOn(Reality->Door, Player, Reason);
            Check(bRevealed && bKeyGated && bKeyReady && Flow->bSliceComplete && Reality->Door->Object->HasState(HeritageTags::State_Open),
                TEXT("Knowledge opens portrait; revealed key requires scan and opens Reality door"));
            const bool bRewardVisible = !Reality->Reel2->IsHidden(); Reality->Reel2->Object->ReceiveStimulus(Use);
            Check(bRewardVisible && Flow->OwnsReel(TEXT("Reel2")) && !Flow->EnterFilm(TEXT("Reel2")) && Flow->Era == EHeritageEra::Reality,
                TEXT("Reel 2 reward collectable but unreadable"));
            Check(Flow->ObjectiveStage >= 8 && !Archive->ActionLog.IsEmpty(), TEXT("Final objective and action log present"));

            // Additional checks target transport, presentation choices, UI suspension and reversible side props.
            const bool bRealityNoBlur = Player->Camera->PostProcessSettings.bOverride_MotionBlurAmount && Player->Camera->PostProcessSettings.MotionBlurAmount == 0.f;
            Flow->EnterFilm(TEXT("Reel1"));
            Check(bRealityNoBlur && Player->Camera->PostProcessSettings.bOverride_MotionBlurAmount && Player->Camera->PostProcessSettings.MotionBlurAmount == 0.f && Player->Camera->PostProcessBlendWeight == 1.f,
                TEXT("Both eras explicitly override motion blur to zero"));
            Director->SetScene(4); Director->SetTransport(true, false);
            const bool bForward = Director->GetDirection() == 1 && Director->GetRate() == Director->FastRate;
            Director->SetTransport(false, true);
            const bool bRewind = Director->GetDirection() == -1 && Director->GetRate() == Director->FastRate;
            Director->SetTransport(false, false);
            Check(bForward && bRewind && Director->GetDirection() == 1 && Director->GetRate() == 1.f && Director->GetAudioCaption().EqualTo(AHeritageFilmDirector::AudioName(1)) && FMath::IsNearlyEqual(Film->GetMainLight()->Intensity, Film->RoomLightIntensity * Director->ColdLightMultiplier),
                TEXT("Transport direction/rate and uploaded audio/light choices reflected in playback"));
            Director->Pause(); Flow->ToggleScreen(EHeritageScreen::Archive);
            const int32 BeforeScreenFacts = Director->GetTakeFacts().Num();
            Director->RecordFact(HeritageTags::Fact_ButlerShoved, NSLOCTEXT("Heritage", "IgnoredUIFact", "Ignored screen fact"));
            const bool bSuspended = !Flow->CanMove() && !Flow->CanUseVerbs() && Film->Kettle->IsFrozen() && Director->GetTakeFacts().Num() == BeforeScreenFacts;
            Flow->CloseScreen();
            Check(bSuspended && Flow->CanMove() && Film->Kettle->Mesh->IsSimulatingPhysics(), TEXT("Screens freeze pause physics and reject facts; close restores interaction"));
            Director->ResumePlay(1); Director->SetScene(0); Director->Pause();
            Film->Butler->Object->ReceiveStimulus(Impact); Film->Vase->Object->ReceiveStimulus(Impact);
            const bool bDropped = Film->Butler->Letter->Object->HasState(HeritageTags::State_Dropped) && !Film->Butler->Letter->GetAttachParentActor();
            Director->Rollback();
            Check(bDropped && Film->Butler->Letter->GetAttachParentActor() == Film->Butler && !Film->Butler->Letter->Object->HasState(HeritageTags::State_Dropped) && !Film->Vase->Object->HasState(HeritageTags::State_Broken) && Film->Vase->FilmReel->IsHidden(),
                TEXT("Rollback restores letter attachment, intact vase and hidden vase reel"));
            Flow->LeaveFilm();
        }
    };
}
void RunHeritageSelfTest(UWorld* World)
{
    if (!World || !World->IsGameWorld()) { UE_LOG(LogHeritage, Display, TEXT("Heritage.SelfTest requires a running game/PIE world.")); return; }
    if (ActiveTests.Contains(World)) { UE_LOG(LogHeritage, Display, TEXT("Heritage.SelfTest is already running.")); return; }
    ActiveTests.Add(World);
    FHeritageChecks Checks; Checks.Exercise(World);
    const bool bSuccess = Checks.Checked == 20 && Checks.Passed == 20;
    UE_LOG(LogHeritage, Display, TEXT("HERITAGE SELFTEST: %s (%d/%d)"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Checks.Passed, Checks.Checked);
    ActiveTests.Remove(World);
    if (FParse::Param(FCommandLine::Get(), TEXT("HeritageSelfTest"))) { FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1); }
}
static FAutoConsoleCommandWithWorld HeritageSelfTestCommand(TEXT("Heritage.SelfTest"), TEXT("Run 20 game-loop checks on a fresh level with default scan gating (1). This consumes the slice state."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&RunHeritageSelfTest));
