#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeritageTypes.h"
#include "HeritageFilmDirector.generated.h"
class AHeritageProp;
class AHeritageDemoRoom;

UCLASS()
class BLANK_API AHeritageFilmDirector : public AActor
{
    GENERATED_BODY()
public:
    AHeritageFilmDirector();
    void RegisterActor(AHeritageProp* Prop);
    void Initialize(AHeritageDemoRoom* InRoom);
    virtual void Tick(float DeltaSeconds) override;
    void BeginVisit();
    void EndVisit();
    // All frame arguments are zero-based scene indices.
    void AdvanceTime(float DeltaSeconds);
    int32 GetFirstUnrestoredFrame() const;
    int32 GetLastReachableFrame() const;
    bool IsLocked(int32 Frame) const;
    int32 GetTargetFrame() const;
    bool IsMissing(int32 Frame) const;
    bool IsRestored(int32 Frame) const;
    bool HasShownLastFrame() const { return bShownLastFrame; }
    float GetGapProgress01() const;
    void SetScene(int32 Index);
    void StepScene(int32 Delta);
    void SetTransport(bool bForwardHeld, bool bRewindHeld);
    void Pause();
    void ResumePlay(int32 Direction);
    void Rollback();
    void RecordFact(FGameplayTag Fact, const FText& Text);
    void SetScreenSuspended(bool bSuspend);
    TArray<FHeritageFactRecord> GetFactsFor(int32 Frame) const;
    void ToggleFact(int32 Frame, FGameplayTag Fact);
    void CycleAudio(int32 Frame, int32 Delta);
    void CycleLight(int32 Frame, int32 Delta);
    bool CanEdit(int32 Frame, FText& Why) const;
    bool Upload(int32 Frame, FText& OutResult);
    bool IsOnMissingFrame() const;
    bool IsImaged(int32 Frame) const;
    bool CanUpload(int32 Frame, FText& Why) const;
    bool IsRecording() const { return bVisiting && Mode == EHeritageFilmMode::Pause && !bScreenSuspended; }
    int32 GetSceneIndex() const { return SceneIndex; }
    int32 GetSceneCount() const { return Segment.Scenes.Num(); }
    int32 GetDirection() const { return PlayDirection; }
    float GetRate() const { return Rate; }
    EHeritageFilmMode GetMode() const { return Mode; }
    const FHeritageSegmentDef& GetSegment() const { return Segment; }
    const FHeritageFrameEdit& GetEdit(int32 Frame) const;
    const TMap<FName, TObjectPtr<AHeritageProp>>& GetActors() const { return Actors; }
    const TArray<FHeritageFactRecord>& GetTakeFacts() const { return TakeFacts; }
    FText GetCaption() const;
    FText GetSceneStatus(int32 Index) const;
    FText GetAudioCaption() const;
    static FText AudioName(int32 Choice);
    static FText LightName(int32 Choice);
    float GetLastCutTime() const { return LastCutTime; }
    UPROPERTY(EditAnywhere, Category="Heritage") float GapSecondsPerAction = 2.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FrameInterval = 1.5f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FastRate = 4.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ColdLightMultiplier = 0.35f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DaylightMultiplier = 1.2f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ColdLightColor = FLinearColor(0.35f, 0.5f, 1.f);
private:
    FHeritageSegmentDef Segment;
    UPROPERTY() TMap<FName, TObjectPtr<AHeritageProp>> Actors;
    UPROPERTY() TObjectPtr<AHeritageDemoRoom> Room;
    TArray<FHeritageActorSnapshot> Baseline;
    TArray<FHeritageActorSnapshot> Layer;
    TArray<FHeritageFactRecord> TakeFacts;
    TMap<int32, FHeritageFrameEdit> Edits;
    EHeritageFilmMode Mode = EHeritageFilmMode::Play;
    int32 SceneIndex = 0;
    int32 PlayDirection = 1;
    float Rate = 1.f;
    float FrameTimer = 0.f;
    float LastCutTime = -BIG_NUMBER;
    int32 PauseSession = 0;
    int32 PendingImagedFrame = INDEX_NONE;
    int32 LayerScene = INDEX_NONE;
    int32 LayerPauseSession = 0;
    bool bHasLayer = false;
    bool bShownLastFrame = false;
    bool bVisiting = false;
    bool bScreenSuspended = false;
    bool bPauseToastShown = false;
    void CaptureLayer();
    void DiscardLayer();
    void RestoreSnapshots(const TArray<FHeritageActorSnapshot>& Snapshots);
    void FreezeAll(bool bFreeze);
    void DropHeld();
    float GetFrameDuration() const;
    void ApplyLight();
};
