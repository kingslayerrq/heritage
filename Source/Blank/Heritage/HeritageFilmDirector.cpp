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
void AHeritageFilmDirector::EndVisit()
{
    if (Mode == EHeritageFilmMode::Pause) { CaptureLayer(); }
    bVisiting = false; FreezeAll(true);
}
int32 AHeritageFilmDirector::LastPlayableScene() const { return KeyEdit.bUploaded ? Segment.Scenes.Num() - 1 : Segment.KeySceneIndex; }
bool AHeritageFilmDirector::IsOnMissingFrame() const { return SceneIndex == Segment.KeySceneIndex && !KeyEdit.bUploaded; }
void AHeritageFilmDirector::SetScene(int32 Index)
{
    if (!Room || Segment.Scenes.IsEmpty()) { return; }
    DropHeld(); SceneIndex = FMath::Clamp(Index, 0, LastPlayableScene()); FrameTimer = 0.f;
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
        Merge(Segment.Scenes[Frame].Keys);
        if (Frame == Segment.KeySceneIndex && KeyEdit.bUploaded) { Merge(Segment.RestoredKeyScene); }
    }
    if (IsOnMissingFrame())
    {
        for (auto& Snapshot : Effective) { if (Snapshot.Id == FName(TEXT("Butler"))) { Snapshot.bHidden = true; } }
    }
    RestoreSnapshots(Effective); FreezeAll(true); ApplyLight(); LastCutTime = GetWorld()->GetTimeSeconds();
    if (IsOnMissingFrame() && bVisiting) { GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->NotifyMissingFrame(); }
}
void AHeritageFilmDirector::StepScene(int32 Delta) { SetScene(SceneIndex + Delta); }
void AHeritageFilmDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!bVisiting || bScreenSuspended || Mode != EHeritageFilmMode::Play || !Flow || !Flow->CanMove()) { return; }
    if ((PlayDirection > 0 && SceneIndex >= LastPlayableScene()) || (PlayDirection < 0 && SceneIndex <= 0)) { FrameTimer = 0.f; return; }
    FrameTimer += DeltaSeconds;
    const float Interval = FMath::Max(FrameInterval / Rate, UE_SMALL_NUMBER);
    while (FrameTimer >= Interval)
    {
        const float Remainder = FrameTimer - Interval;
        StepScene(PlayDirection); FrameTimer = Remainder;
        if ((PlayDirection > 0 && SceneIndex >= LastPlayableScene()) || (PlayDirection < 0 && SceneIndex <= 0)) { FrameTimer = 0.f; break; }
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
    DropHeld(); bHasLayer = false; Layer.Empty(); LayerScene = INDEX_NONE;
    TakeFacts.RemoveAll([this](const auto& Fact) { return Fact.PauseSession == PauseSession; });
    // Unuploaded compositions cannot retain evidence removed by rollback.
    const auto Facts = GetDistinctFacts();
    for (auto* Edit : { &KeyEdit, &LeafEdit })
    {
        if (!Edit->bUploaded) { Edit->ChosenFacts.RemoveAll([&Facts](FGameplayTag Tag) { return !Facts.ContainsByPredicate([Tag](const auto& Fact) { return Fact.Fact == Tag; }); }); }
    }
    RecomputeImaged(); SetScene(SceneIndex); FreezeAll(false);
    UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "RolledBack", "Rolled back."));
}
void AHeritageFilmDirector::SetScreenSuspended(bool bSuspend)
{
    bScreenSuspended = bSuspend;
    FreezeAll(bSuspend || Mode == EHeritageFilmMode::Play || !bVisiting);
}
void AHeritageFilmDirector::RecomputeImaged()
{
    FGameplayTagContainer Facts; for (const auto& Record : TakeFacts) { Facts.AddTag(Record.Fact); }
    const bool bWasImaged = bImaged; bImaged = Facts.HasAllExact(Segment.KeyAnswer);
    if (bImaged && !bWasImaged)
    {
        UHeritageStatics::Toast(this, NSLOCTEXT("Heritage", "Imaged", "Imaging successful."));
        GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->NotifyImaged();
    }
    else if (!bImaged && bWasImaged)
    {
        auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
        if (!Flow->bKeySceneUploaded && Flow->ObjectiveStage == 6) { Flow->ObjectiveStage = 5; }
    }
}
void AHeritageFilmDirector::RecordFact(FGameplayTag Fact, const FText& Text)
{
    if (!IsRecording() || !Fact.IsValid()) { return; }
    if (TakeFacts.ContainsByPredicate([this, Fact](const auto& Record) { return Record.Fact == Fact && Record.PauseSession == PauseSession; })) { return; }
    FHeritageFactRecord Record; Record.Fact = Fact; Record.Text = Text; Record.SceneIndex = SceneIndex; Record.PauseSession = PauseSession;
    TakeFacts.Add(Record); RecomputeImaged();
}
TArray<FHeritageFactRecord> AHeritageFilmDirector::GetDistinctFacts() const
{
    TArray<FHeritageFactRecord> Result;
    for (const auto& Record : TakeFacts)
    {
        if (!Result.ContainsByPredicate([&Record](const auto& Other) { return Other.Fact == Record.Fact; })) { Result.Add(Record); }
    }
    return Result;
}
bool AHeritageFilmDirector::CanEdit(bool bKeyFrame, FText& Why) const
{
    if (GetEdit(bKeyFrame).bUploaded) { Why = NSLOCTEXT("Heritage", "EditLocked", "Uploaded frames cannot be changed."); return false; }
    if (bKeyFrame && !bImaged) { Why = NSLOCTEXT("Heritage", "NeedImaging", "Act out the missing moment to image it first."); return false; }
    if (!bKeyFrame && TakeFacts.IsEmpty()) { Why = NSLOCTEXT("Heritage", "NeedFact", "Record an action in Pause Mode first."); return false; }
    Why = FText::GetEmpty(); return true;
}
void AHeritageFilmDirector::ToggleFact(bool bKeyFrame, FGameplayTag Fact)
{
    FText Why; if (!CanEdit(bKeyFrame, Why)) { return; }
    if (!TakeFacts.ContainsByPredicate([Fact](const auto& Record) { return Record.Fact == Fact; })) { return; }
    auto& Chosen = (bKeyFrame ? KeyEdit : LeafEdit).ChosenFacts;
    if (Chosen.Contains(Fact)) { Chosen.Remove(Fact); } else { Chosen.Add(Fact); }
}
void AHeritageFilmDirector::CycleAudio(bool bKeyFrame, int32 Delta)
{
    FText Why; if (!CanEdit(bKeyFrame, Why)) { return; }
    auto& Edit = bKeyFrame ? KeyEdit : LeafEdit; Edit.AudioChoice = ((Edit.AudioChoice + Delta) % 4 + 4) % 4;
}
void AHeritageFilmDirector::CycleLight(bool bKeyFrame, int32 Delta)
{
    FText Why; if (!CanEdit(bKeyFrame, Why)) { return; }
    auto& Edit = bKeyFrame ? KeyEdit : LeafEdit; Edit.LightChoice = ((Edit.LightChoice + Delta) % 3 + 3) % 3;
}
bool AHeritageFilmDirector::Upload(bool bKeyFrame, FText& OutResult)
{
    if (!CanEdit(bKeyFrame, OutResult)) { return false; }
    auto& Edit = bKeyFrame ? KeyEdit : LeafEdit;
    FGameplayTagContainer Chosen;
    for (FGameplayTag Fact : Edit.ChosenFacts)
    {
        if (!TakeFacts.ContainsByPredicate([Fact](const auto& Record) { return Record.Fact == Fact; })) { return false; }
        Chosen.AddTag(Fact);
    }
    if (bKeyFrame ? !(Chosen.HasAllExact(Segment.KeyAnswer) && Segment.KeyAnswer.HasAllExact(Chosen)) : Chosen.Num() != 1)
    {
        OutResult = NSLOCTEXT("Heritage", "WrongFrames", "The frames do not fit together."); return false;
    }
    Edit.bUploaded = true;
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (bKeyFrame)
    {
        Flow->NotifyKeyUploaded(); OutResult = NSLOCTEXT("Heritage", "Restored", "Frame restored. The rest of the reel is readable.");
    }
    else if (Chosen.HasTagExact(HeritageTags::Fact_ReelFound))
    {
        Flow->UnlockLeafReward(); OutResult = NSLOCTEXT("Heritage", "KnowWhere", "You know where to look now.");
    }
    else { OutResult = NSLOCTEXT("Heritage", "AccountRecorded", "Your account is on the reel."); }
    if (Mode == EHeritageFilmMode::Play) { SetScene(SceneIndex); }
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Film, Segment.Title, FText::GetEmpty(), OutResult);
    return true;
}
FText AHeritageFilmDirector::GetCaption() const
{
    if (SceneIndex == Segment.KeySceneIndex && KeyEdit.bUploaded) { return NSLOCTEXT("Heritage", "RestoredCaption", "The fire is out. The butler holds the key."); }
    return Segment.Scenes.IsValidIndex(SceneIndex) ? Segment.Scenes[SceneIndex].Caption : FText::GetEmpty();
}
FText AHeritageFilmDirector::GetSceneStatus(int32 Index) const
{
    if (!Segment.Scenes.IsValidIndex(Index)) { return FText::GetEmpty(); }
    if (Index > LastPlayableScene()) { return NSLOCTEXT("Heritage", "Locked", "Locked"); }
    if (Index == Segment.KeySceneIndex) { return KeyEdit.bUploaded ? NSLOCTEXT("Heritage", "RestoredStatus", "Restored") : NSLOCTEXT("Heritage", "MissingStatus", "Missing"); }
    return NSLOCTEXT("Heritage", "Readable", "Readable");
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
    if (SceneIndex == Segment.KeySceneIndex && KeyEdit.bUploaded) { return AudioName(KeyEdit.AudioChoice); }
    if (SceneIndex == Segment.LeafSceneIndex && LeafEdit.bUploaded) { return AudioName(LeafEdit.AudioChoice); }
    return FText::GetEmpty();
}
void AHeritageFilmDirector::ApplyLight()
{
    if (!Room || !Room->GetMainLight()) { return; }
    int32 Choice = 0;
    if (SceneIndex == Segment.KeySceneIndex && KeyEdit.bUploaded) { Choice = KeyEdit.LightChoice; }
    if (SceneIndex == Segment.LeafSceneIndex && LeafEdit.bUploaded) { Choice = LeafEdit.LightChoice; }
    Room->GetMainLight()->SetLightColor(Choice == 1 ? ColdLightColor : Choice == 2 ? FLinearColor::White : Room->RoomLightColor);
    Room->GetMainLight()->SetIntensity(Room->RoomLightIntensity * (Choice == 1 ? ColdLightMultiplier : Choice == 2 ? DaylightMultiplier : 1.f));
}
