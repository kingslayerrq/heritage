#include "HeritageProp.h"
#include "HeritageObjectComponent.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AHeritageProp::AHeritageProp()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); SetRootComponent(Mesh);
    Object = CreateDefaultSubobject<UHeritageObjectComponent>(TEXT("Object"));
    Object->DisplayName = NSLOCTEXT("Heritage", "Book", "Book"); Object->bGrabbable = true;
    Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    Mesh->SetNotifyRigidBodyCollision(true); Mesh->SetUseCCD(true);
    Mesh->SetGenerateOverlapEvents(true);
    Mesh->OnComponentHit.AddDynamic(this, &AHeritageProp::OnBodyHit);
    InitShape(EHeritageShape::Cube, ShapeSize, ShapeColor, PropMass);
    Mesh->SetSimulatePhysics(true);
}
UStaticMesh* AHeritageProp::LoadShape(EHeritageShape Shape)
{
    const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
    switch (Shape)
    {
        case EHeritageShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
        case EHeritageShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
        case EHeritageShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
        case EHeritageShape::Plane: Path = TEXT("/Engine/BasicShapes/Plane.Plane"); break;
        default: break;
    }
    return LoadObject<UStaticMesh>(nullptr, Path);
}
void AHeritageProp::InitShape(EHeritageShape Shape, FVector SizeCm, FLinearColor Color, float MassKg)
{
    ShapeSize = SizeCm; ShapeColor = Color; PropMass = MassKg;
    Mesh->SetStaticMesh(LoadShape(Shape)); Mesh->SetWorldScale3D(SizeCm / 100.f);
    // Dynamic materials are created after construction, not on class default objects.
    if (HasActorBegunPlay()) { Mesh->SetMassOverrideInKg(NAME_None, PropMass); SetColor(Color); }
}
void AHeritageProp::BeginPlay()
{
    Super::BeginPlay(); Mesh->SetWorldScale3D(ShapeSize / 100.f);
    Mesh->SetMassOverrideInKg(NAME_None, PropMass); SetColor(ShapeColor);
    bPhysicsProp = Mesh->IsSimulatingPhysics();
}
void AHeritageProp::MakeStatic()
{
    bPhysicsProp = false;
    Mesh->SetSimulatePhysics(false); Mesh->SetCollisionProfileName(TEXT("BlockAll")); Object->bGrabbable = false;
}
void AHeritageProp::SetFrozen(bool bFreeze)
{
    bFrozen = bFreeze;
    if (Mesh->IsSimulatingPhysics())
    {
        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    Mesh->SetSimulatePhysics(!bFreeze && bPhysicsProp && !IsHidden() && !GetAttachParentActor());
    SetActorTickEnabled(!bFreeze);
}
void AHeritageProp::RefreshFromState() {}
FHeritageActorSnapshot AHeritageProp::Capture(const FTransform& Room) const
{
    FHeritageActorSnapshot Result;
    Result.Id = FilmId; Result.RoomLocal = GetActorTransform().GetRelativeTransform(Room);
    Result.State = Object->StateTags; Result.bHidden = IsHidden();
    return Result;
}
void AHeritageProp::Restore(const FHeritageActorSnapshot& Snapshot, const FTransform& Room)
{
    SetFrozen(true);
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorTransform(Snapshot.RoomLocal * Room, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorHiddenInGame(Snapshot.bHidden); SetActorEnableCollision(!Snapshot.bHidden);
    Object->StateTags = Snapshot.State; Object->bHeld = false; Object->LastThrower.Reset();
    LastImpactTime = -BIG_NUMBER;
    RefreshFromState();
}
void AHeritageProp::SetColor(FLinearColor Color)
{
    ShapeColor = Color;
    if (!Material)
    {
        Material = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), this);
        Mesh->SetMaterial(0, Material);
    }
    Material->SetVectorParameterValue(TEXT("Color"), Color);
}
UStaticMeshComponent* AHeritageProp::AddShape(FName Name, EHeritageShape Shape, FVector Position, FVector Size, FLinearColor Color)
{
    auto* Part = NewObject<UStaticMeshComponent>(this, Name); AddInstanceComponent(Part);
    Part->SetupAttachment(Mesh); Part->SetStaticMesh(LoadShape(Shape));
    // Root mesh scaling must not distort child dimensions or local offsets.
    Part->SetAbsolute(false, false, true);
    Part->SetRelativeLocation(Position / Mesh->GetComponentScale()); Part->SetWorldScale3D(Size / 100.f);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* MID = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), Part);
    MID->SetVectorParameterValue(TEXT("Color"), Color); Part->SetMaterial(0, MID); Part->RegisterComponent();
    return Part;
}
void AHeritageProp::OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (Object->bHeld || IsHidden() || !HitComponent->IsSimulatingPhysics()) { return; }
    const float Speed = NormalImpulse.Size() / FMath::Max(HitComponent->GetMass(), UE_SMALL_NUMBER);
    const float Now = GetWorld()->GetTimeSeconds();
    if (Speed < MinImpactSpeed || Now - LastImpactTime < ImpactReportInterval) { return; }
    LastImpactTime = Now;
    FHeritageStimulus Stimulus; Stimulus.Type = HeritageTags::Stimulus_Impact; Stimulus.Magnitude = Speed;
    Stimulus.Source = this; Stimulus.Instigator = Object->LastThrower.Get(); Stimulus.Location = Hit.ImpactPoint;
    Stimulus.Direction = -Hit.ImpactNormal;
    UHeritageStatics::SendStimulus(this, Stimulus);
    if (OtherActor != this) { UHeritageStatics::SendStimulus(OtherActor, Stimulus); }
}
