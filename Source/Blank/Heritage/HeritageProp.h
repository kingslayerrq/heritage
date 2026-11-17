#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeritageTypes.h"
#include "HeritageProp.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
class UHeritageObjectComponent;
class UMaterialInstanceDynamic;

UENUM()
enum class EHeritageShape : uint8 { Cube, Sphere, Cylinder, Cone, Plane };

UCLASS()
class BLANK_API AHeritageProp : public AActor
{
    GENERATED_BODY()
public:
    AHeritageProp();
    virtual void BeginPlay() override;
    void InitShape(EHeritageShape Shape, FVector SizeCm, FLinearColor Color, float MassKg);
    void SetColor(FLinearColor Color);
    void MakeStatic();
    UPROPERTY() FName FilmId;
    bool bPhysicsProp = false;
    bool IsFrozen() const { return bFrozen; }
    void SetFrozen(bool bFreeze);
    virtual void RefreshFromState();
    FHeritageActorSnapshot Capture(const FTransform& Room) const;
    void Restore(const FHeritageActorSnapshot& Snapshot, const FTransform& Room);
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, Category="Heritage") TObjectPtr<UHeritageObjectComponent> Object;
    UPROPERTY(EditAnywhere, Category="Heritage") float MinImpactSpeed = 300.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ImpactReportInterval = 0.1f;
    UPROPERTY(EditAnywhere, Category="Heritage") float PropMass = 0.6f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector ShapeSize = FVector(25, 18, 5);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ShapeColor = FLinearColor(0.22f, 0.025f, 0.025f);
protected:
    UStaticMeshComponent* AddShape(FName Name, EHeritageShape Shape, FVector Position, FVector Size, FLinearColor Color);
    static UStaticMesh* LoadShape(EHeritageShape Shape);
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
    UFUNCTION() void OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    float LastImpactTime = -BIG_NUMBER;
    bool bFrozen = false;
};
