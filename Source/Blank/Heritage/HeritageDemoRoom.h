#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeritageTypes.h"
#include "HeritageDemoRoom.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AHeritageProp;
class AHeritageFilmDirector;
class AHeritageReelCanister;
class AHeritageShards;
class AHeritageKettle;
class AHeritageFireplace;
class AHeritageKey;
class AHeritageDoor;
class AHeritageCabinet;
class AHeritageBasin;
class AHeritageBreakable;
class AHeritageNPC;
class AHeritagePortrait;
UCLASS()
class BLANK_API AHeritageDemoRoom : public AActor
{
    GENERATED_BODY()
public:
    AHeritageDemoRoom();
    void Build();
    void RegisterFilmActors(AHeritageFilmDirector* Director);
    UPointLightComponent* GetMainLight() const { return MainLight; }
    UPROPERTY() EHeritageEra Era = EHeritageEra::Reality;
    UPROPERTY() TArray<TObjectPtr<AHeritageProp>> Props;
    UPROPERTY() TObjectPtr<AHeritageReelCanister> Reel1;
    UPROPERTY() TObjectPtr<AHeritageReelCanister> Reel2;
    UPROPERTY() TObjectPtr<AHeritageShards> Shards;
    UPROPERTY() TObjectPtr<UPointLightComponent> MainLight;

    virtual void Tick(float DeltaSeconds) override;
    FTransform GetPlayerSpawnTransform() const;
    UPROPERTY() TObjectPtr<AHeritageKettle> Kettle;
    UPROPERTY() TObjectPtr<AHeritageFireplace> Fireplace;
    UPROPERTY() TObjectPtr<AHeritageKey> Key;
    UPROPERTY() TObjectPtr<AHeritageDoor> Door;
    UPROPERTY() TObjectPtr<AHeritageCabinet> Cabinet;
    UPROPERTY() TObjectPtr<AHeritageBasin> Basin;
    UPROPERTY() TObjectPtr<AHeritageBreakable> Vase;
    UPROPERTY() TObjectPtr<AHeritageNPC> Butler;
    UPROPERTY() TObjectPtr<AHeritagePortrait> Portrait;
    bool bKettleTaken = false;
    bool bKettleFilled = false;
    bool bKeyTaken = false;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector RoomSize = FVector(800, 600, 350);
    UPROPERTY(EditAnywhere, Category="Heritage") float WallThickness = 20.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector PlayerSpawn = FVector(-300, 0, 100);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector RoomLightPosition = FVector(0, 0, 320);
    UPROPERTY(EditAnywhere, Category="Heritage") float RoomLightIntensity = 5000.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float RoomLightRadius = 1200.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor RoomLightColor = FLinearColor(1.f, 0.9f, 0.75f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor WallColor = FLinearColor(0.75f, 0.72f, 0.65f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor FloorColor = FLinearColor(0.3f, 0.32f, 0.34f);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector TableBase = FVector(-30, 240, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector TableTopSize = FVector(160, 80, 6);
    UPROPERTY(EditAnywhere, Category="Heritage") float TableTopBottom = 75.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float TableLegWidth = 10.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector BookSize = FVector(25, 18, 5);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector BookStackOffset = FVector(4, 3, 6);
    UPROPERTY(EditAnywhere, Category="Heritage") int32 BookCount = 3;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector FireplaceBase = FVector(380, 0, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector CabinetBase = FVector(-150, 275, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector BasinBase = FVector(250, 270, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector DoorBase = FVector(0, -295, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector PedestalBase = FVector(200, -200, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector PedestalSize = FVector(40, 40, 90);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector ButlerBase = FVector(-100, -150, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector PortraitCentre = FVector(-375, 150, 150);
    UPROPERTY(EditAnywhere, Category="Heritage") float DoorOpeningWidth = 120.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float DoorOpeningHeight = 220.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector ReelSize = FVector(20, 20, 4);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector LetterSize = FVector(20, 14, 1);
    UPROPERTY(EditAnywhere, Category="Heritage") float ReelMass = 0.3f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ReelColor = FLinearColor(0.9f, 0.7f, 0.1f);
    UPROPERTY(EditAnywhere, Category="Heritage") float LetterMass = 0.1f;
    UPROPERTY(EditAnywhere, Category="Heritage") float SpawnClearance = 1.f;
private:
    UStaticMeshComponent* Box(FName Name, FVector Position, FVector Size, FLinearColor Color);
    bool bBuilt = false;
};
