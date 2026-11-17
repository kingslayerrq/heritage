#pragma once
#include "CoreMinimal.h"
#include "HeritageProp.h"
#include "HeritageInterfaces.h"
#include "HeritageProps.generated.h"
class UBoxComponent;
class UPointLightComponent;

UCLASS()
class BLANK_API AHeritageBreakable : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageBreakable();
    virtual void BeginPlay() override;
    virtual void RefreshFromState() override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY() TObjectPtr<AHeritageProp> FilmReel;
    UPROPERTY() TArray<TObjectPtr<AHeritageProp>> Fragments;
    UPROPERTY(EditAnywhere, Category="Heritage") float BreakThreshold = 650.f;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ShardCount = 6;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector ShardSize = FVector(6, 6, 6);
    UPROPERTY(EditAnywhere, Category="Heritage") float ShardSpeed = 200.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ShardMass = 0.1f;

};

UCLASS()
class BLANK_API AHeritageBasin : public AHeritageProp
{
    GENERATED_BODY()
public:
    AHeritageBasin();
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector WaterTopSize = FVector(70, 40, 2);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor WaterColor = FLinearColor(0.25f, 0.65f, 0.9f);
};

UCLASS()
class BLANK_API AHeritageKettle : public AHeritageProp, public IHeritageUsable
{
    GENERATED_BODY()
public:
    AHeritageKettle();
    virtual void RefreshFromState() override;
    virtual void BeginPlay() override;
    virtual bool UseOn(AActor* Target, AActor* User, FText& OutResult) override;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector SpoutSize = FVector(10, 10, 18);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector SpoutPosition = FVector(14, 0, 5);
    UPROPERTY(EditAnywhere, Category="Heritage") FRotator SpoutRotation = FRotator(45, 0, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor EmptyColor = FLinearColor(0.5f, 0.5f, 0.52f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor FilledColor = FLinearColor(0.1f, 0.3f, 0.85f);
};

UCLASS()
class BLANK_API AHeritageFireplace : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageFireplace();
    virtual void RefreshFromState() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UBoxComponent> Hearth;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UPointLightComponent> FireLight;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector HearthSize = FVector(75, 105, 95);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector HearthOffset = FVector(0, 0, 50);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector FlameSize = FVector(30, 60, 30);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector FlameOffset = FVector(5, 0, 30);
    UPROPERTY(EditAnywhere, Category="Heritage") float FireIntensity = 800.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FireRadius = 250.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FlickerSpeed = 13.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FlickerAmount = 0.15f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor FlameColor = FLinearColor(1.f, 0.16f, 0.01f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor FireLightColor = FLinearColor(1.f, 0.3f, 0.04f);
    UPROPERTY(EditAnywhere, Category="Heritage") float FrameHeight = 120.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float FrameThickness = 12.f;
    void RefreshHeat();
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Flame;
    TSet<TWeakObjectPtr<UHeritageObjectComponent>> HeatedObjects;
};

UCLASS()
class BLANK_API AHeritageKey : public AHeritageProp, public IHeritageUsable, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageKey();
    virtual void OnGrabbed(AActor* By) override;
    virtual bool HandleStimulus(const FHeritageStimulus&) override { return false; }
    virtual bool UseOn(AActor* Target, AActor* User, FText& OutResult) override;
};

UCLASS()
class BLANK_API AHeritageDoor : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageDoor();
    virtual void RefreshFromState() override;
    virtual void BeginPlay() override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY(EditAnywhere, Category="Heritage") float OpenAngle = 100.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector PanelSize = FVector(100, 8, 210);
    UPROPERTY(EditAnywhere, Category="Heritage") float FrameThickness = 10.f;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Panel;

};

UCLASS()
class BLANK_API AHeritageCabinet : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageCabinet();
    virtual void BeginPlay() override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UBoxComponent> TopVolume;
    UPROPERTY(EditAnywhere, Category="Heritage") float ShakeThreshold = 400.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ShakeOutSpeed = 180.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ShakeUpSpeed = 120.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector ShakeDirection = FVector(0, -1, 0);
    UPROPERTY(EditAnywhere, Category="Heritage") float TopVolumeHeight = 70.f;
};

UCLASS()
class BLANK_API AHeritageNPC : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageNPC();
    virtual void RefreshFromState() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY() TObjectPtr<AHeritageProp> Letter;
    UPROPERTY(EditAnywhere, Category="Heritage") float StaggerThreshold = 400.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float StaggerAngle = 12.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float StaggerDuration = 0.5f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector HeadSize = FVector(40, 40, 40);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor HeadColor = FLinearColor(0.6f, 0.5f, 0.4f);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector LetterOffset = FVector(35, 0, 25);
private:
    FRotator RestRotation;
    float StaggerRemaining = 0.f;
};

UCLASS()
class BLANK_API AHeritagePortrait : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritagePortrait();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY() TObjectPtr<AHeritageKey> HiddenKey;
    UPROPERTY(EditAnywhere, Category="Heritage") float SlideDistance = 60.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float SlideDuration = 0.6f;
    virtual void RefreshFromState() override;
    virtual void BeginPlay() override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector InnerSize = FVector(68, 2, 98);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor InnerColor = FLinearColor(0.7f, 0.6f, 0.45f);
private:
    FVector ClosedPosition = FVector::ZeroVector;
    float SlideElapsed = 0.f;
    bool bSliding = false;
};

UCLASS()
class BLANK_API AHeritageReelCanister : public AHeritageProp, public IHeritageReactive
{
    GENERATED_BODY()
public:
    AHeritageReelCanister();
    virtual void BeginPlay() override;
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) override;
    UPROPERTY() FName ReelId = TEXT("Reel1");
    UPROPERTY(EditAnywhere, Category="Heritage") FVector DiscSize = FVector(18, 18, 1);
    UPROPERTY(EditAnywhere, Category="Heritage") FVector DiscOffset = FVector(0, 0, 3.5);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor DiscColor = FLinearColor(0.8f, 0.55f, 0.1f);
};
UCLASS()
class BLANK_API AHeritageShards : public AHeritageProp
{
    GENERATED_BODY()
public:
    AHeritageShards();
    UPROPERTY(EditAnywhere, Category="Heritage") float ShardRadius = 12.f;
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ShardCount = 6;
};
