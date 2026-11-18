#include "HeritageFilmDirector.h"
#include "HeritageReelData.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageDemoRoom.h"
#include "HeritageProps.h"
#include "HeritageObjectComponent.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AHeritageFilmDirector::AHeritageFilmDirector() { PrimaryActorTick.bCanEverTick = true; }
void AHeritageFilmDirector::RegisterActor(AHeritageProp* Prop)
{
    if (Prop && !Prop->FilmId.IsNone()) { Actors.Add(Prop->FilmId, Prop); }
}
void AHeritageFilmDirector::Initialize(AHeritageDemoRoom* InRoom)
{
    Room = InRoom; Segment = HeritageReelData::BuildReel1(); Baseline.Empty();
    Edits.Empty();
    for (int32 Frame = 0; Frame < Segment.Scenes.Num(); ++Frame)
    {
        if (IsMissing(Frame)) { Edits.Add(Frame, FHeritageFrameEdit()); }
    }
    if (!Room) { return; }
    for (const auto& Pair : Actors) { if (Pair.Value) { Baseline.Add(Pair.Value->Capture(Room->GetActorTransform())); } }
    SetScene(0);
}
void AHeritageFilmDirector::DropHeld()
{
    if (auto* Player = Cast<AHeritageCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Player->EndDash(); Player->Interactor->Drop();
    }
}
void AHeritageFilmDirector::FreezeAll(bool bFreeze)
{
    for (const auto& Pair : Actors) { if (Pair.Value) { Pair.Value->SetFrozen(bFreeze); } }
}
void AHeritageFilmDirector::RestoreSnapshots(const TArray<FHeritageActorSnapshot>& Snapshots)
{
    if (!Room) { return; }
    // Restore every value before resolving cross-actor derived state (letter, reel, heat).
    for (const auto& Snapshot : Snapshots)
    {
        if (auto* Found = Actors.Find(Snapshot.Id); Found && IsValid(Found->Get())) { (*Found)->Restore(Snapshot, Room->GetActorTransform()); }
    }
    if (Room->Vase) { Room->Vase->RefreshFromState(); }
    if (Room->Butler) { Room->Butler->RefreshFromState(); }
    if (Room->Fireplace) { Room->Fireplace->RefreshFromState(); }
}
void AHeritageFilmDirector::BeginVisit()
{
    bVisiting = true; bScreenSuspended = false; bPauseToastShown = false;
    Mode = EHeritageFilmMode::Play; PlayDirection = 1; Rate = 1.f; SetScene(0);
}
void AHeritageFilmDirector::CaptureLayer()
{
    if (!Room) { return; }
    DropHeld(); Layer.Empty();
    for (const auto& Pair : Actors) { if (Pair.Value) { Layer.Add(Pair.Value->Capture(Room->GetActorTransform())); } }
    bHasLayer = true; LayerScene = SceneIndex; LayerPauseSession = PauseSession;
}
void AHeritageFilmDirector::DiscardLayer()
{
    Layer.Empty(); bHasLayer = false; LayerScene = INDEX_NONE; LayerPauseSession = 0;
}
void AHeritageFilmDirector::EndVisit()
{
    if (Mode == EHeritageFilmMode::Pause) { CaptureLayer(); }
    bVisiting = false; FreezeAll(true);
}
bool AHeritageFilmDirector::IsMissing(int32 Frame) const
{
    return Segment.Scenes.IsValidIndex(Frame) && Segment.Scenes[Frame].Kind == EHeritageSceneKind::Missing;
}
const FHeritageFrameEdit& AHeritageFilmDirector::GetEdit(int32 Frame) const
{
    static const FHeritageFrameEdit Empty;
    const auto* Edit = Edits.Find(Frame); return Edit ? *Edit : Empty;
}
bool AHeritageFilmDirector::IsRestored(int32 Frame) const { return IsMissing(Frame) && GetEdit(Frame).bUploaded; }
int32 AHeritageFilmDirector::GetFirstUnrestoredFrame() const
{
    for (int32 Index = 0; Index < Segment.Scenes.Num(); ++Index)
    {
        if (IsMissing(Index) && !IsRestored(Index)) { return Index; }
    }
    return INDEX_NONE;
}
int32 AHeritageFilmDirector::GetLastReachableFrame() const
{
    const int32 LastFrame = Segment.Scenes.Num() - 1;
    const int32 FirstUnrestored = GetFirstUnrestoredFrame();
    return FirstUnrestored == INDEX_NONE ? LastFrame : FMath::Min(FirstUnrestored + 1, LastFrame);
}
bool AHeritageFilmDirector::IsLocked(int32 Frame) const
{
    return Segment.Scenes.IsValidIndex(Frame) && Frame > GetLastReachableFrame();
}
int32 AHeritageFilmDirector::GetTargetFrame() const
{
    for (int32 Frame = SceneIndex; Frame < Segment.Scenes.Num(); ++Frame)
    {
        if (IsMissing(Frame) && !IsRestored(Frame)) { return Frame; }
    }
    return INDEX_NONE;
}
bool AHeritageFilmDirector::IsOnMissingFrame() const { return IsMissing(SceneIndex) && !IsRestored(SceneIndex); }
float AHeritageFilmDirector::GetFrameDuration() const
{
    return FMath::Max(IsOnMissingFrame() ? GapSecondsPerAction * Segment.Scenes[SceneIndex].Answer.Num() : FrameInterval, UE_SMALL_NUMBER);
}
float AHeritageFilmDirector::GetGapProgress01() const
{
    return IsOnMissingFrame() ? FMath::Clamp(FrameTimer / GetFrameDuration(), 0.f, 1.f) : 0.f;
}
void AHeritageFilmDirector::SetScene(int32 Index)
{
    if (!Room || Segment.Scenes.IsEmpty()) { return; }
    DropHeld(); SceneIndex = FMath::Clamp(Index, 0, GetLastReachableFrame()); FrameTimer = 0.f;
    TArray<FHeritageActorSnapshot> Effective = Baseline;
    const auto Merge = [&Effective](const TArray<FHeritageActorSnapshot>& Keys)
    {
        for (const auto& Key : Keys)
        {
            if (auto* Existing = Effective.FindByPredicate([&Key](const auto& Value) { return Value.Id == Key.Id; })) { *Existing = Key; }
            else { Effective.Add(Key); }
        }
    };
    for (int32 Frame = 0; Frame <= SceneIndex; ++Frame)
    {
        if (!IsMissing(Frame)) { Merge(Segment.Scenes[Frame].Keys); }
        else if (IsRestored(Frame)) { Merge(Segment.Scenes[Frame].Restored); }
    }
    if (IsOnMissingFrame())
    {
        for (auto& Snapshot : Effective) { if (Snapshot.Id == FName(TEXT("Butler"))) { Snapshot.bHidden = true; } }
    }
    RestoreSnapshots(Effective); FreezeAll(true); ApplyLight(); LastCutTime = GetWorld()->GetTimeSeconds();
    if (bVisiting && SceneIndex == Segment.Scenes.Num() - 1) { bShownLastFrame = true; }
    if (IsOnMissingFrame() && bVisiting) { GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->NotifyMissingFrame(); }
}
void AHeritageFilmDirector::StepScene(int32 Delta)
{
    if (!Room || Segment.Scenes.IsEmpty()) { return; }
    const int32 Destination = SceneIndex + FMath::Clamp(Delta, -SceneIndex, GetLastReachableFrame() - SceneIndex);
    while (SceneIndex != Destination)
    {
        SetScene(SceneIndex + (Delta > 0 ? 1 : -1));
    }
}
void AHeritageFilmDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); AdvanceTime(DeltaSeconds);
    // Reactions and the interactor can post their own toast after RecordFact in the same input event.
    // Present the imaging transition after that event, without coupling gameplay verbs to facts.
    if (PendingImagedFrame != INDEX_NONE)
    {
        if (bVisiting && IsImaged(PendingImagedFrame) && !IsRestored(PendingImagedFrame))
        {
            UHeritageStatics::Toast(this, FText::Format(NSLOCTEXT("Heritage", "FrameImaged", "Frame {0} imaged."), FText::AsNumber(PendingImagedFrame + 1)));
        }
        PendingImagedFrame = INDEX_NONE;
    }
}
void AHeritageFilmDirector::AdvanceTime(float DeltaSeconds)
{
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!bVisiting || bScreenSuspended || Mode != EHeritageFilmMode::Play || !Flow || !Flow->CanMove() || !Room) { return; }
    const int32 TransportEnd = PlayDirection > 0 ? GetLastReachableFrame() : 0;
    if (TransportEnd == INDEX_NONE || SceneIndex == TransportEnd) { return; }
    // Timer is in reel seconds, so changing transport rate preserves gap progress.
    FrameTimer += FMath::Max(DeltaSeconds, 0.f) * Rate;
    while (FrameTimer >= GetFrameDuration())
    {
        const float Remainder = FrameTimer - GetFrameDuration();
        StepScene(PlayDirection); FrameTimer = Remainder;
        if (SceneIndex == TransportEnd)
        {
            FrameTimer = 0.f; break;
        }
    }
}
void AHeritageFilmDirector::SetTransport(bool bForwardHeld, bool bRewindHeld)
{
    if (Mode != EHeritageFilmMode::Play) { return; }
    PlayDirection = bRewindHeld && !bForwardHeld ? -1 : 1; Rate = bForwardHeld || bRewindHeld ? FastRate : 1.f;
}
void AHeritageFilmDirector::Pause()
{
    if (!bVisiting || bScreenSuspended || Mode == EHeritageFilmMode::Pause) { return; }
    Mode = EHeritageFilmMode::Pause; ++PauseSession;
    if (bHasLayer && LayerScene == SceneIndex)
    {
        RestoreSnapshots(Layer);
        // The restored layer and its evidence now belong to this pause session.
        for (auto& Fact : TakeFacts) { if (Fact.PauseSession == LayerPauseSession) { Fact.PauseSession = PauseSession; } }
        LayerPauseSession = PauseSession;
    }
    FreezeAll(false);
    if (!bPauseToastShown)
    {
        bPauseToastShown = true; UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "ButlerStead", "You act in the butler's stead."));
    }
}
void AHeritageFilmDirector::ResumePlay(int32 Direction)
{
    if (!bVisiting || bScreenSuspended || Mode != EHeritageFilmMode::Pause) { return; }
    CaptureLayer(); Mode = EHeritageFilmMode::Play; PlayDirection = Direction < 0 ? -1 : 1; Rate = 1.f; SetScene(SceneIndex);
}
void AHeritageFilmDirector::Rollback()
{
    if (!IsRecording()) { return; }
    DropHeld(); DiscardLayer();
    const int32 Target = GetTargetFrame();
    TakeFacts.RemoveAll([this, Target](const auto& Fact)
    {
        // The target must become unimaged even if its evidence came from another pause.
        // Uploaded evidence remains immutable, and other frames keep their independent evidence.
        return !IsRestored(Fact.SceneIndex) && (Fact.PauseSession == PauseSession || Fact.SceneIndex == Target);
    });
    // Remove rolled-back evidence from its frame's unuploaded checklist too.
    for (auto& Pair : Edits)
    {
        const auto Facts = GetFactsFor(Pair.Key);
        if (!Pair.Value.bUploaded)
        {
            Pair.Value.ChosenFacts.RemoveAll([&Facts](FGameplayTag Tag)
            {
                return !Facts.ContainsByPredicate([Tag](const auto& Fact) { return Fact.Fact == Tag; });
            });
        }
    }
    SetScene(SceneIndex); FreezeAll(false);
    UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "RolledBack", "Rolled back."));
}
void AHeritageFilmDirector::SetScreenSuspended(bool bSuspend)
{
    bScreenSuspended = bSuspend;
    FreezeAll(bSuspend || Mode == EHeritageFilmMode::Play || !bVisiting);
}
bool AHeritageFilmDirector::IsImaged(int32 Frame) const
{
    if (!IsMissing(Frame)) { return false; }
    FGameplayTagContainer Facts;
    for (const auto& Record : GetFactsFor(Frame)) { Facts.AddTag(Record.Fact); }
    return Facts.HasAllExact(Segment.Scenes[Frame].Answer);
}
void AHeritageFilmDirector::RecordFact(FGameplayTag Fact, const FText& Text)
{
    const int32 Frame = GetTargetFrame();
    if (!IsRecording() || !Fact.IsValid() || Frame == INDEX_NONE) { return; }
    if (TakeFacts.ContainsByPredicate([this, Fact, Frame](const auto& Record)
        { return Record.Fact == Fact && Record.SceneIndex == Frame && Record.PauseSession == PauseSession; })) { return; }
    const bool bWasImaged = IsImaged(Frame);
    FHeritageFactRecord Record; Record.Fact = Fact; Record.Text = Text; Record.SceneIndex = Frame; Record.PauseSession = PauseSession;
    TakeFacts.Add(Record);
    if (!bWasImaged && IsImaged(Frame))
    {
        PendingImagedFrame = Frame;
        UHeritageStatics::Toast(this, FText::Format(NSLOCTEXT("Heritage", "FrameImaged", "Frame {0} imaged."), FText::AsNumber(Frame + 1)));
    }
}
TArray<FHeritageFactRecord> AHeritageFilmDirector::GetFactsFor(int32 Frame) const
{
    TArray<FHeritageFactRecord> Result;
    for (const auto& Record : TakeFacts)
    {
        if (Record.SceneIndex == Frame && !Result.ContainsByPredicate([&Record](const auto& Other) { return Other.Fact == Record.Fact; })) { Result.Add(Record); }
    }
    return Result;
}
bool AHeritageFilmDirector::CanEdit(int32 Frame, FText& Why) const
{
    if (!IsMissing(Frame)) { Why = NSLOCTEXT("Heritage", "ReadableFrame", "This frame is already readable."); return false; }
    if (IsRestored(Frame)) { Why = NSLOCTEXT("Heritage", "EditLocked", "Uploaded frames cannot be changed."); return false; }
    if (!IsImaged(Frame)) { Why = NSLOCTEXT("Heritage", "NeedImaging", "Act out the missing moment to image it first."); return false; }
    Why = FText::GetEmpty(); return true;
}
bool AHeritageFilmDirector::CanUpload(int32 Frame, FText& Why) const
{
    const int32 Earlier = GetFirstUnrestoredFrame();
    if (IsMissing(Frame) && Earlier != INDEX_NONE && Earlier < Frame)
    {
        Why = FText::Format(NSLOCTEXT("Heritage", "RestoreEarlier", "Restore frame {0} first."), FText::AsNumber(Earlier + 1)); return false;
    }
    return CanEdit(Frame, Why);
}
void AHeritageFilmDirector::ToggleFact(int32 Frame, FGameplayTag Fact)
{
    FText Why; if (!CanEdit(Frame, Why)) { return; }
    if (!GetFactsFor(Frame).ContainsByPredicate([Fact](const auto& Record) { return Record.Fact == Fact; })) { return; }
    auto& Chosen = Edits.FindChecked(Frame).ChosenFacts;
    if (Chosen.Contains(Fact)) { Chosen.Remove(Fact); } else { Chosen.Add(Fact); }
}
void AHeritageFilmDirector::CycleAudio(int32 Frame, int32 Delta)
{
    FText Why; if (!CanEdit(Frame, Why)) { return; }
    auto& Edit = Edits.FindChecked(Frame); Edit.AudioChoice = ((Edit.AudioChoice + Delta) % 4 + 4) % 4;
}
void AHeritageFilmDirector::CycleLight(int32 Frame, int32 Delta)
{
    FText Why; if (!CanEdit(Frame, Why)) { return; }
    auto& Edit = Edits.FindChecked(Frame); Edit.LightChoice = ((Edit.LightChoice + Delta) % 3 + 3) % 3;
}
bool AHeritageFilmDirector::Upload(int32 Frame, FText& OutResult)
{
    if (!CanUpload(Frame, OutResult)) { return false; }
    auto& Edit = Edits.FindChecked(Frame);
    FGameplayTagContainer Chosen;
    const auto Facts = GetFactsFor(Frame);
    for (FGameplayTag Fact : Edit.ChosenFacts)
    {
        if (Facts.ContainsByPredicate([Fact](const auto& Record) { return Record.Fact == Fact; })) { Chosen.AddTag(Fact); }
    }
    const auto& Answer = Segment.Scenes[Frame].Answer;
    if (Chosen.Num() != Edit.ChosenFacts.Num() || !Chosen.HasAllExact(Answer) || !Answer.HasAllExact(Chosen))
    {
        OutResult = NSLOCTEXT("Heritage", "WrongFrames", "The frames do not fit together."); return false;
    }
    Edit.bUploaded = true;
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->Era == EHeritageEra::Film)
    {
        DiscardLayer();
        Mode = EHeritageFilmMode::Play; SetTransport(false, false);
        SetScene(Frame);
    }
    Flow->CloseScreen();
    PendingImagedFrame = INDEX_NONE;
    OutResult = FText::Format(NSLOCTEXT("Heritage", "FrameRestored", "Frame {0} restored."), FText::AsNumber(Frame + 1));
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Film, Segment.Title, FText::GetEmpty(), OutResult);
    UHeritageStatics::Toast(this, OutResult);
    return true;
}
FText AHeritageFilmDirector::GetCaption() const
{
    if (IsRestored(SceneIndex))
    {
        return SceneIndex == 2 ? NSLOCTEXT("Heritage", "FilledCaption", "The butler fills the kettle at the basin.") :
            NSLOCTEXT("Heritage", "RestoredCaption", "The fire is out. The butler holds the key.");
    }
    return Segment.Scenes.IsValidIndex(SceneIndex) ? Segment.Scenes[SceneIndex].Caption : FText::GetEmpty();
}
FText AHeritageFilmDirector::GetSceneStatus(int32 Index) const
{
    if (!Segment.Scenes.IsValidIndex(Index)) { return FText::GetEmpty(); }
    if (IsLocked(Index)) { return NSLOCTEXT("Heritage", "LockedStatus", "LOCKED"); }
    if (IsRestored(Index)) { return NSLOCTEXT("Heritage", "V2RestoredStatus", "RESTORED"); }
    if (IsImaged(Index)) { return NSLOCTEXT("Heritage", "ImagedStatus", "IMAGED"); }
    return IsMissing(Index) ? NSLOCTEXT("Heritage", "V2MissingStatus", "MISSING") : NSLOCTEXT("Heritage", "V2ReadableStatus", "READABLE");
}
FText AHeritageFilmDirector::AudioName(int32 Choice)
{
    switch (Choice)
    {
    case 1: return NSLOCTEXT("Heritage", "SteamAudio", "Hiss of steam");
    case 2: return NSLOCTEXT("Heritage", "CrashAudio", "Crash");
    case 3: return NSLOCTEXT("Heritage", "LaughterAudio", "Low laughter");
    default: return NSLOCTEXT("Heritage", "SilenceAudio", "Silence");
    }
}
FText AHeritageFilmDirector::LightName(int32 Choice)
{
    switch (Choice)
    {
    case 1: return NSLOCTEXT("Heritage", "ColdLight", "Cold dark");
    case 2: return NSLOCTEXT("Heritage", "Daylight", "Daylight");
    default: return NSLOCTEXT("Heritage", "Firelight", "Firelight");
    }
}
FText AHeritageFilmDirector::GetAudioCaption() const
{
    if (Mode != EHeritageFilmMode::Play) { return FText::GetEmpty(); }
    if (IsRestored(SceneIndex)) { return AudioName(GetEdit(SceneIndex).AudioChoice); }
    return FText::GetEmpty();
}
void AHeritageFilmDirector::ApplyLight()
{
    if (!Room || !Room->GetMainLight()) { return; }
    int32 Choice = 0;
    if (IsRestored(SceneIndex)) { Choice = GetEdit(SceneIndex).LightChoice; }
    Room->GetMainLight()->SetLightColor(Choice == 1 ? ColdLightColor : Choice == 2 ? FLinearColor::White : Room->RoomLightColor);
    Room->GetMainLight()->SetIntensity(Room->RoomLightIntensity * (Choice == 1 ? ColdLightMultiplier : Choice == 2 ? DaylightMultiplier : 1.f));
}
