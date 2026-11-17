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
    int32 LastPlayableScene() const;
    void SetScene(int32 Index);
    void StepScene(int32 Delta);
    void SetTransport(bool bForwardHeld, bool bRewindHeld);
    void Pause();
    void ResumePlay(int32 Direction);
    void Rollback();
    void RecordFact(FGameplayTag Fact, const FText& Text);
    void SetScreenSuspended(bool bSuspend);
    TArray<FHeritageFactRecord> GetDistinctFacts() const;
    void ToggleFact(bool bKeyFrame, FGameplayTag Fact);
    void CycleAudio(bool bKeyFrame, int32 Delta);
    void CycleLight(bool bKeyFrame, int32 Delta);
    bool CanEdit(bool bKeyFrame, FText& Why) const;
    bool Upload(bool bKeyFrame, FText& OutResult);
    bool IsOnMissingFrame() const;
    bool IsImaged() const { return bImaged; }
    bool IsRecording() const { return bVisiting && Mode == EHeritageFilmMode::Pause && !bScreenSuspended; }
    int32 GetSceneIndex() const { return SceneIndex; }
    int32 GetSceneCount() const { return Segment.Scenes.Num(); }
    int32 GetDirection() const { return PlayDirection; }
    float GetRate() const { return Rate; }
    EHeritageFilmMode GetMode() const { return Mode; }
    const FHeritageSegmentDef& GetSegment() const { return Segment; }
    const FHeritageFrameEdit& GetEdit(bool bKeyFrame) const { return bKeyFrame ? KeyEdit : LeafEdit; }
    const TMap<FName, TObjectPtr<AHeritageProp>>& GetActors() const { return Actors; }
    const TArray<FHeritageFactRecord>& GetTakeFacts() const { return TakeFacts; }
    FText GetCaption() const;
    FText GetSceneStatus(int32 Index) const;
    FText GetAudioCaption() const;
    static FText AudioName(int32 Choice);
    static FText LightName(int32 Choice);
    float GetLastCutTime() const { return LastCutTime; }
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
    FHeritageFrameEdit KeyEdit, LeafEdit;
    EHeritageFilmMode Mode = EHeritageFilmMode::Play;
    int32 SceneIndex = 0;
    int32 PlayDirection = 1;
    float Rate = 1.f;
    float FrameTimer = 0.f;
    float LastCutTime = -BIG_NUMBER;
    int32 PauseSession = 0;
    int32 LayerScene = INDEX_NONE;
    int32 LayerPauseSession = 0;
    bool bHasLayer = false;
    bool bImaged = false;
    bool bVisiting = false;
    bool bScreenSuspended = false;
    bool bPauseToastShown = false;
    void CaptureLayer();
    void RestoreSnapshots(const TArray<FHeritageActorSnapshot>& Snapshots);
    void FreezeAll(bool bFreeze);
    void DropHeld();
    void RecomputeImaged();
    void ApplyLight();
};
