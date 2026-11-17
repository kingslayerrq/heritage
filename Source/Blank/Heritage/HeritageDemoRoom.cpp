#include "HeritageDemoRoom.h"
#include "HeritageProp.h"
#include "HeritageFilmDirector.h"
#include "HeritageProps.h"
#include "HeritageObjectComponent.h"
#include "HeritageArchiveSubsystem.h"
#include "HeritageTags.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

AHeritageDemoRoom::AHeritageDemoRoom()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
UStaticMeshComponent* AHeritageDemoRoom::Box(FName Name, FVector Position, FVector Size, FLinearColor Color)
{
    auto* Mesh = NewObject<UStaticMeshComponent>(this, Name);
    AddInstanceComponent(Mesh); Mesh->SetupAttachment(RootComponent);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetRelativeLocation(Position); Mesh->SetRelativeScale3D(Size / 100.f);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    auto* Material = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), Mesh);
    Material->SetVectorParameterValue(TEXT("Color"), Color); Mesh->SetMaterial(0, Material);
    Mesh->RegisterComponent(); return Mesh;
}
void AHeritageDemoRoom::Build()
{
    if (bBuilt) { return; } bBuilt = true;
    Box(TEXT("Floor"), FVector(0, 0, -WallThickness / 2), FVector(RoomSize.X, RoomSize.Y, WallThickness), FloorColor);
    Box(TEXT("Ceiling"), FVector(0, 0, RoomSize.Z), FVector(RoomSize.X, RoomSize.Y, WallThickness), WallColor);
    Box(TEXT("EastWall"), FVector(RoomSize.X / 2, 0, RoomSize.Z / 2), FVector(WallThickness, RoomSize.Y, RoomSize.Z), WallColor);
    Box(TEXT("WestWall"), FVector(-RoomSize.X / 2, 0, RoomSize.Z / 2), FVector(WallThickness, RoomSize.Y, RoomSize.Z), WallColor);
    Box(TEXT("NorthWall"), FVector(0, RoomSize.Y / 2, RoomSize.Z / 2), FVector(RoomSize.X, WallThickness, RoomSize.Z), WallColor);
    const float SideWidth = (RoomSize.X - DoorOpeningWidth) / 2;
    for (int32 Sign : {-1, 1})
    {
        Box(FName(*FString::Printf(TEXT("SouthWall%d"), Sign)), FVector(Sign * (DoorOpeningWidth + SideWidth) / 2, -RoomSize.Y / 2, RoomSize.Z / 2), FVector(SideWidth, WallThickness, RoomSize.Z), WallColor);
    }
    Box(TEXT("DoorLintel"), FVector(0, -RoomSize.Y / 2, (DoorOpeningHeight + RoomSize.Z) / 2), FVector(DoorOpeningWidth, WallThickness, RoomSize.Z - DoorOpeningHeight), WallColor);
    auto* Light = NewObject<UPointLightComponent>(this, TEXT("RoomLight"));
    MainLight = Light;
    AddInstanceComponent(Light); Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(RoomLightPosition); Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(RoomLightIntensity); Light->SetAttenuationRadius(RoomLightRadius);
    Light->SetLightColor(RoomLightColor); Light->RegisterComponent();
    Box(TEXT("TableTop"), TableBase + FVector(0, 0, TableTopBottom + TableTopSize.Z / 2), TableTopSize, FloorColor);
    for (int32 XSign : {-1, 1})
    {
        for (int32 YSign : {-1, 1})
        {
            Box(FName(*FString::Printf(TEXT("TableLeg%d%d"), XSign, YSign)),
                TableBase + FVector(XSign * (TableTopSize.X - TableLegWidth) / 2, YSign * (TableTopSize.Y - TableLegWidth) / 2, TableTopBottom / 2),
                FVector(TableLegWidth, TableLegWidth, TableTopBottom), FloorColor);
        }
    }
    for (int32 Index = 0; Index < (Era == EHeritageEra::Reality ? 1 : BookCount); ++Index)
    {
        const FVector Position = TableBase + FVector(0, 0, TableTopBottom + TableTopSize.Z + BookSize.Z / 2) + BookStackOffset * Index;
        if (auto* Book = GetWorld()->SpawnActor<AHeritageProp>(GetActorTransform().TransformPosition(Position), GetActorRotation()))
        {
            Book->FilmId = FName(*FString::Printf(TEXT("Book%d"), Index)); Props.Add(Book);
            Book->InitShape(EHeritageShape::Cube, BookSize, Book->ShapeColor, Book->PropMass);
        }
    }
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Each prop root is its mesh centre; the layout table describes base centres.
    const auto WorldPoint = [this](FVector Local) { return GetActorTransform().TransformPosition(Local); };
    const auto* CabinetDefault = GetDefault<AHeritageCabinet>();
    Cabinet = GetWorld()->SpawnActor<AHeritageCabinet>(WorldPoint(CabinetBase + FVector(0, 0, CabinetDefault->ShapeSize.Z / 2)), GetActorRotation(), Params);
    const auto* KettleDefault = GetDefault<AHeritageKettle>();
    Kettle = GetWorld()->SpawnActor<AHeritageKettle>(WorldPoint((Era == EHeritageEra::Film ? TableBase + FVector(40, -15, TableTopBottom + TableTopSize.Z) : BasinBase + FVector(-60, -40, 0)) + FVector(0, 0, KettleDefault->ShapeSize.Z / 2 + SpawnClearance)), GetActorRotation(), Params);
    const auto* FireDefault = GetDefault<AHeritageFireplace>();
    Fireplace = GetWorld()->SpawnActor<AHeritageFireplace>(WorldPoint(FireplaceBase + FVector(-FireDefault->ShapeSize.X / 2, 0, FireDefault->ShapeSize.Z / 2)), GetActorRotation(), Params);
    const auto* KeyDefault = GetDefault<AHeritageKey>();
    Key = GetWorld()->SpawnActor<AHeritageKey>(WorldPoint(FireplaceBase + FVector(-FireDefault->ShapeSize.X / 2, 0, FireDefault->ShapeSize.Z + KeyDefault->ShapeSize.Z / 2 + SpawnClearance)), GetActorRotation(), Params);
    const auto* BasinDefault = GetDefault<AHeritageBasin>();
    Basin = GetWorld()->SpawnActor<AHeritageBasin>(WorldPoint(BasinBase + FVector(0, 0, BasinDefault->ShapeSize.Z / 2)), GetActorRotation(), Params);
    const auto* DoorDefault = GetDefault<AHeritageDoor>();
    Door = GetWorld()->SpawnActor<AHeritageDoor>(WorldPoint(DoorBase + FVector(0, 0, DoorDefault->PanelSize.Z + DoorDefault->FrameThickness / 2)), GetActorRotation(), Params);
    Box(TEXT("Pedestal"), PedestalBase + FVector(0, 0, PedestalSize.Z / 2), PedestalSize, FloorColor);
    const auto* VaseDefault = GetDefault<AHeritageBreakable>();
    if (Era == EHeritageEra::Film) { Vase = GetWorld()->SpawnActor<AHeritageBreakable>(WorldPoint(PedestalBase + FVector(0, 0, PedestalSize.Z + VaseDefault->ShapeSize.Z / 2 + SpawnClearance)), GetActorRotation(), Params); }
    const auto* ButlerDefault = GetDefault<AHeritageNPC>();
    if (Era == EHeritageEra::Film) { Butler = GetWorld()->SpawnActor<AHeritageNPC>(WorldPoint(ButlerBase + FVector(0, 0, ButlerDefault->ShapeSize.Z / 2)), GetActorRotation(), Params); }
    Portrait = GetWorld()->SpawnActor<AHeritagePortrait>(WorldPoint(PortraitCentre), GetActorRotation() + FRotator(0, 90, 0), Params);
    if (Vase)
    {
        for (const auto& Shard : Vase->Fragments) { Props.Add(Shard); }
        auto* Reel = GetWorld()->SpawnActor<AHeritageProp>(Vase->GetActorLocation(), GetActorRotation(), Params);
        if (Reel)
        {
            Reel->InitShape(EHeritageShape::Cylinder, ReelSize, ReelColor, ReelMass);
            Reel->Object->DisplayName = NSLOCTEXT("Heritage", "FilmReel", "Film reel"); Reel->Object->bMemento = false; Reel->FilmId = TEXT("VaseReel"); Props.Add(Reel);
            Reel->Object->MementoNote = NSLOCTEXT("Heritage", "ReelNote", "A small film reel found inside the vase.");
            Reel->Mesh->SetSimulatePhysics(false); Reel->SetActorEnableCollision(false); Reel->SetActorHiddenInGame(true);
            Reel->AttachToActor(Vase, FAttachmentTransformRules::KeepWorldTransform); Vase->FilmReel = Reel;
        }
    }
    if (Butler)
    {
        auto* Letter = GetWorld()->SpawnActor<AHeritageProp>(Butler->GetActorLocation() + GetActorRotation().RotateVector(Butler->LetterOffset), GetActorRotation(), Params);
        if (Letter)
        {
            Letter->InitShape(EHeritageShape::Cube, LetterSize, FLinearColor::White, LetterMass);
            Letter->Object->DisplayName = NSLOCTEXT("Heritage", "Letter", "Letter"); Letter->Object->bMemento = false; Letter->FilmId = TEXT("Letter"); Props.Add(Letter); Letter->Object->bGrabbable = false;
            Letter->Object->MementoNote = NSLOCTEXT("Heritage", "LetterNote", "A folded letter carried by the butler.");
            Letter->Mesh->SetSimulatePhysics(false); Letter->AttachToActor(Butler, FAttachmentTransformRules::KeepWorldTransform); Butler->Letter = Letter;
        }
    }
    const auto Register = [this](AHeritageProp* Prop, FName Id)
    {
        if (!Prop) { return; }
        Prop->FilmId = Id; Props.Add(Prop);
        if (Era == EHeritageEra::Film) { Prop->Object->bMemento = false; }
    };
    Register(Kettle, TEXT("Kettle")); Register(Fireplace, TEXT("Fireplace")); Register(Key, TEXT("Key"));
    Register(Door, TEXT("Door")); Register(Vase, TEXT("Vase")); Register(Butler, TEXT("Butler"));
    Register(Portrait, TEXT("Portrait")); Register(Cabinet, TEXT("Cabinet")); Register(Basin, TEXT("Basin"));
    if (Era == EHeritageEra::Reality)
    {
        Fireplace->Object->RemoveState(HeritageTags::State_Lit); Fireplace->RefreshFromState();
        Kettle->Object->DisplayName = NSLOCTEXT("Heritage", "RustedKettle", "Rusted kettle");
        Kettle->SetColor(FLinearColor(0.25f, 0.13f, 0.07f));
        Shards = GetWorld()->SpawnActor<AHeritageShards>(WorldPoint(PedestalBase + FVector(0, 0, PedestalSize.Z + 3)), GetActorRotation(), Params);
        Reel1 = GetWorld()->SpawnActor<AHeritageReelCanister>(WorldPoint(TableBase + FVector(40, -15, TableTopBottom + TableTopSize.Z + 3)), GetActorRotation(), Params);
        Reel2 = GetWorld()->SpawnActor<AHeritageReelCanister>(WorldPoint(PedestalBase + FVector(0, 0, PedestalSize.Z + 6)), GetActorRotation(), Params);
        Reel2->ReelId = TEXT("Reel2"); Reel2->Object->bMemento = false;
        Reel2->Object->DisplayName = NSLOCTEXT("Heritage", "Reel2", "Reel 2");
        Reel2->SetActorHiddenInGame(true); Reel2->SetActorEnableCollision(false);
        Key->SetFrozen(true);
        Key->SetActorLocation(WorldPoint(PortraitCentre + FVector(-10, 0, 0)), false, nullptr, ETeleportType::TeleportPhysics);
        Key->SetActorRotation(GetActorRotation() + FRotator(0, 90, 0));
        Key->Object->RemoveState(HeritageTags::State_Hot);
        Key->SetActorHiddenInGame(true); Key->SetActorEnableCollision(false); Portrait->HiddenKey = Key;
        // A small shelf keeps the revealed key at reachable height until picked up.
        Box(TEXT("KeyShelf"), PortraitCentre + FVector(-10, 0, -4), FVector(22, 24, 4), FloorColor);
    }
    else
    {
        auto* Book = GetWorld()->SpawnActor<AHeritageProp>(WorldPoint(CabinetBase + FVector(0, 0, CabinetDefault->ShapeSize.Z + BookSize.Z / 2 + SpawnClearance)), GetActorRotation(), Params);
        Register(Book, TEXT("CabinetBook"));
        if (Butler) { Butler->RefreshFromState(); }
    }
    Fireplace->RefreshFromState();
    GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()->GetTotalMementos();
}
FTransform AHeritageDemoRoom::GetPlayerSpawnTransform() const
{
    return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(PlayerSpawn));
}
void AHeritageDemoRoom::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Kettle)
    {
        bKettleTaken |= Kettle->Object->bHeld;
        bKettleFilled |= Kettle->Object->HasState(HeritageTags::State_Filled);
    }
    if (IsValid(Key)) { bKeyTaken |= Key->Object->bHeld; }
    if (Door && Door->Object->HasState(HeritageTags::State_Open)) { bKeyTaken = true; }
}

void AHeritageDemoRoom::RegisterFilmActors(AHeritageFilmDirector* Director)
{
    if (Era != EHeritageEra::Film || !Director) { return; }
    for (const auto& Prop : Props) { if (Prop) { Director->RegisterActor(Prop); } }
}
