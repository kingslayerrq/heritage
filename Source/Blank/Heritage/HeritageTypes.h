#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HeritageTypes.generated.h"

class AActor;

UENUM()
enum class EHeritageEra : uint8 { Reality, Film };
UENUM()
enum class EHeritageFilmMode : uint8 { Play, Pause };
UENUM()
enum class EHeritageSceneKind : uint8 { Readable, MissingKey };
UENUM()
enum class EHeritageScreen : uint8 { None, Map, Archive };

// Value-only records: these contain no UObject references and need no GC traversal.
USTRUCT()
struct BLANK_API FHeritageActorSnapshot
{
    GENERATED_BODY()
    FName Id;
    FTransform RoomLocal = FTransform::Identity;
    FGameplayTagContainer State;
    bool bHidden = false;
};
USTRUCT()
struct BLANK_API FHeritageSceneDef
{
    GENERATED_BODY()
    EHeritageSceneKind Kind = EHeritageSceneKind::Readable;
    bool bLockedUntilKeyUploaded = false;
    bool bHasDamagedArea = false;
    FText Caption;
    TArray<FHeritageActorSnapshot> Keys;
};
USTRUCT()
struct BLANK_API FHeritageFactRecord
{
    GENERATED_BODY()
    FGameplayTag Fact;
    FText Text;
    int32 SceneIndex = 0;
    int32 PauseSession = 0;
};
USTRUCT()
struct BLANK_API FHeritageFrameEdit
{
    GENERATED_BODY()
    TArray<FGameplayTag> ChosenFacts;
    int32 AudioChoice = 0;
    int32 LightChoice = 0;
    bool bUploaded = false;
};
USTRUCT()
struct BLANK_API FHeritageSegmentDef
{
    GENERATED_BODY()
    FName ReelId;
    FText Title;
    TArray<FHeritageSceneDef> Scenes;
    int32 KeySceneIndex = 4;
    int32 LeafSceneIndex = 2;
    FGameplayTagContainer KeyAnswer;
    TArray<FHeritageActorSnapshot> RestoredKeyScene;
};

USTRUCT(BlueprintType)
struct BLANK_API FHeritageStimulus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FGameplayTag Type;
    UPROPERTY(BlueprintReadWrite) float Magnitude = 0.f;
    UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Instigator = nullptr;
    UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Source = nullptr;
    UPROPERTY(BlueprintReadWrite) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite) FVector Direction = FVector::ZeroVector;
};

USTRUCT()
struct BLANK_API FHeritageActionRecord
{
    GENERATED_BODY()
    UPROPERTY() float Time = 0.f;
    UPROPERTY() FGameplayTag Verb;
    UPROPERTY() FText Object;
    UPROPERTY() FText Target;
    UPROPERTY() FText Result;
};

USTRUCT()
struct BLANK_API FHeritageArchiveEntry
{
    GENERATED_BODY()
    UPROPERTY() FText Name;
    UPROPERTY() FText Note;
    UPROPERTY() FGameplayTagContainer StateAtCapture;
    UPROPERTY() float Time = 0.f;
};
