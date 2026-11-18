#include "HeritageProps.h"
#include "HeritageObjectComponent.h"
#include "HeritageArchiveSubsystem.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

namespace
{
    void Reaction(AActor* Actor, const FHeritageStimulus& Stimulus, const FText& Text)
    {
        UHeritageStatics::Toast(Actor, Text);
        const FGameplayTag Verb = Stimulus.Type == HeritageTags::Stimulus_Photo ? HeritageTags::Verb_Photo :
            Stimulus.Type == HeritageTags::Stimulus_Impact ? (Stimulus.Source == Stimulus.Instigator ? HeritageTags::Verb_Dash : HeritageTags::Verb_Throw) : HeritageTags::Verb_Use;
        UHeritageStatics::LogAction(Actor, Verb, UHeritageStatics::NameOf(Stimulus.Source), UHeritageStatics::NameOf(Actor), Text);
    }
}
AHeritageBreakable::AHeritageBreakable()
{
    InitShape(EHeritageShape::Cylinder, FVector(25, 25, 45), FLinearColor(0.05f, 0.5f, 0.45f), 2.f);
    Object->DisplayName = NSLOCTEXT("Heritage", "Vase", "Vase");
}
void AHeritageBreakable::BeginPlay()
{
    Super::BeginPlay();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 Index = 0; Index < ShardCount; ++Index)
    {
        auto* Shard = GetWorld()->SpawnActor<AHeritageProp>(GetActorLocation(), GetActorRotation(), Params);
        if (!Shard) { continue; }
        Shard->InitShape(EHeritageShape::Cube, ShardSize, ShapeColor, ShardMass);
        Shard->FilmId = FName(*FString::Printf(TEXT("VaseShard%d"), Index));
        Shard->Object->DisplayName = NSLOCTEXT("Heritage", "Shard", "Vase shard");
        Shard->Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        Shard->SetActorHiddenInGame(true); Shard->SetActorEnableCollision(false); Shard->SetFrozen(true);
        Fragments.Add(Shard);
    }
}
bool AHeritageBreakable::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type != HeritageTags::Stimulus_Impact || Stimulus.Magnitude < BreakThreshold || Object->HasState(HeritageTags::State_Broken)) { return false; }
    Object->AddState(HeritageTags::State_Broken);
    for (const auto& Shard : Fragments)
    {
        FVector Direction = FMath::VRand(); Direction.Z = FMath::Abs(Direction.Z);
        Shard->SetActorLocation(GetActorLocation() + Direction * ShapeSize.Z / 2, false, nullptr, ETeleportType::TeleportPhysics);
    }
    RefreshFromState();
    for (const auto& Shard : Fragments)
    {
        FVector Direction = (Shard->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (Shard->Mesh->IsSimulatingPhysics()) { Shard->Mesh->AddImpulse(Direction * ShardSpeed, NAME_None, true); }
    }
    UHeritageStatics::RecordFact(this, HeritageTags::Fact_VaseBroken, NSLOCTEXT("Heritage", "FactVase", "Broke the vase."));
    if (FilmReel) { UHeritageStatics::RecordFact(this, HeritageTags::Fact_ReelFound, NSLOCTEXT("Heritage", "FactReel", "A film reel was hidden in the vase.")); }
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "Shattered", "Vase shattered."));
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (FilmReel && Flow && Flow->Era == EHeritageEra::Film && Flow->Director && Flow->Director->IsRecording()) { Flow->UnlockLeafReward(); }
    return true;
}
void AHeritageBreakable::RefreshFromState()
{
    const bool bBroken = Object->HasState(HeritageTags::State_Broken);
    SetActorHiddenInGame(bBroken); SetActorEnableCollision(!bBroken);
    if (bBroken) { Mesh->SetSimulatePhysics(false); }
    for (const auto& Shard : Fragments)
    {
        Shard->SetActorHiddenInGame(!bBroken); Shard->SetActorEnableCollision(bBroken); Shard->SetFrozen(IsFrozen());
    }
    if (FilmReel)
    {
        FilmReel->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        FilmReel->SetActorHiddenInGame(!bBroken); FilmReel->SetActorEnableCollision(bBroken);
        if (bBroken && !IsFrozen()) { FilmReel->SetActorLocation(GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics); }
        FilmReel->SetFrozen(IsFrozen());
    }
}
AHeritageBasin::AHeritageBasin()
{
    InitShape(EHeritageShape::Cube, FVector(80, 50, 85), FLinearColor(0.4f, 0.65f, 0.75f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Basin", "Basin");
}
void AHeritageBasin::BeginPlay()
{
    Super::BeginPlay(); AddShape(TEXT("WaterTop"), EHeritageShape::Cube, FVector(0, 0, ShapeSize.Z / 2), WaterTopSize, WaterColor);
}
AHeritageKettle::AHeritageKettle()
{
    InitShape(EHeritageShape::Cylinder, FVector(22, 22, 25), EmptyColor, 1.5f);
    Object->DisplayName = NSLOCTEXT("Heritage", "Kettle", "Kettle"); Object->bMemento = true;
    Object->MementoNote = NSLOCTEXT("Heritage", "KettleNote", "A small kettle that can carry water.");
}
void AHeritageKettle::BeginPlay()
{
    Super::BeginPlay();
    auto* Spout = AddShape(TEXT("Spout"), EHeritageShape::Cone, SpoutPosition, SpoutSize, EmptyColor);
    Spout->SetRelativeRotation(SpoutRotation);
}
bool AHeritageKettle::UseOn(AActor* Target, AActor* User, FText& OutResult)
{
    if (Cast<AHeritageBasin>(Target))
    {
        Object->AddState(HeritageTags::State_Filled); RefreshFromState();
        UHeritageStatics::RecordFact(this, HeritageTags::Fact_KettleFilled, NSLOCTEXT("Heritage", "FactFilled", "Filled the kettle at the basin."));
        OutResult = NSLOCTEXT("Heritage", "KettleFilled", "Filled the kettle."); return true;
    }
    if (!Object->HasState(HeritageTags::State_Filled)) { OutResult = NSLOCTEXT("Heritage", "EmptyKettle", "The kettle is empty."); return false; }
    if (!IsValid(Target)) { OutResult = NSLOCTEXT("Heritage", "NoTarget", "Aim at an object."); return false; }
    FHeritageStimulus Water; Water.Type = HeritageTags::Stimulus_Water; Water.Source = this; Water.Instigator = User; Water.Location = Target->GetActorLocation();
    UHeritageStatics::SendStimulus(Target, Water);
    Object->RemoveState(HeritageTags::State_Filled); RefreshFromState();
    OutResult = FText::Format(NSLOCTEXT("Heritage", "PouredWater", "Poured water on {0}."), UHeritageStatics::NameOf(Target)); return true;
}
void AHeritageKettle::RefreshFromState()
{
    SetColor(Object->HasState(HeritageTags::State_Filled) ? FilledColor : EmptyColor);
}
AHeritageFireplace::AHeritageFireplace()
{
    PrimaryActorTick.bCanEverTick = true;
    InitShape(EHeritageShape::Cube, FVector(70, 120, 10), FLinearColor(0.25f, 0.23f, 0.22f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Fireplace", "Fireplace"); Object->AddState(HeritageTags::State_Lit);
    Hearth = CreateDefaultSubobject<UBoxComponent>(TEXT("Hearth")); Hearth->SetupAttachment(Mesh);
    Hearth->SetAbsolute(false, false, true); Hearth->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Hearth->SetGenerateOverlapEvents(true);
    FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight")); FireLight->SetupAttachment(Mesh);
}
void AHeritageFireplace::BeginPlay()
{
    Super::BeginPlay();
    Hearth->SetBoxExtent(HearthSize / 2); Hearth->SetRelativeLocation(HearthOffset / Mesh->GetComponentScale());
    Flame = AddShape(TEXT("Flame"), EHeritageShape::Cube, FlameOffset, FlameSize, FlameColor);
    FireLight->SetRelativeLocation(FlameOffset / Mesh->GetComponentScale()); FireLight->SetIntensityUnits(ELightUnits::Candelas);
    FireLight->SetIntensity(FireIntensity); FireLight->SetAttenuationRadius(FireRadius); FireLight->SetLightColor(FireLightColor);
    auto* Back = AddShape(TEXT("FireBack"), EHeritageShape::Cube, FVector(ShapeSize.X / 2, 0, FrameHeight / 2), FVector(FrameThickness, ShapeSize.Y, FrameHeight), ShapeColor);
    Back->SetCollisionProfileName(TEXT("BlockAll"));
    for (int32 Sign : {-1, 1})
    {
        auto* Side = AddShape(FName(*FString::Printf(TEXT("FireSide%d"), Sign)), EHeritageShape::Cube,
            FVector(0, Sign * ShapeSize.Y / 2, FrameHeight / 2), FVector(ShapeSize.X, FrameThickness, FrameHeight), ShapeColor);
        Side->SetCollisionProfileName(TEXT("BlockAll"));
    }
    auto* Top = AddShape(TEXT("FireMantel"), EHeritageShape::Cube, FVector(0, 0, FrameHeight), FVector(ShapeSize.X, ShapeSize.Y + FrameThickness, FrameThickness), ShapeColor);
    Top->SetCollisionProfileName(TEXT("BlockAll")); RefreshHeat();
}
void AHeritageFireplace::RefreshHeat()
{
    TSet<TWeakObjectPtr<UHeritageObjectComponent>> Current;
    if (Object->HasState(HeritageTags::State_Lit))
    {
        // Geometric query is synchronous after snapshot teleports, unlike overlap caches.
        for (TActorIterator<AHeritageProp> It(GetWorld()); It; ++It)
        {
            AActor* Actor = *It;
            const FVector Local = Hearth->GetComponentTransform().InverseTransformPosition(Actor->GetActorLocation());
            if (Actor == this || Actor->IsHidden() || !FBox(-Hearth->GetUnscaledBoxExtent(), Hearth->GetUnscaledBoxExtent()).IsInsideOrOn(Local)) { continue; }
            if (auto* Item = UHeritageObjectComponent::Find(Actor)) { Current.Add(Item); Item->AddState(HeritageTags::State_Hot); }
        }
    }
    for (const auto& Item : HeatedObjects)
    {
        if (Item.IsValid() && !Current.Contains(Item)) { Item->RemoveState(HeritageTags::State_Hot); }
    }
    HeatedObjects = MoveTemp(Current);
}
void AHeritageFireplace::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); RefreshHeat();
    if (Object->HasState(HeritageTags::State_Lit)) { FireLight->SetIntensity(FireIntensity * (1.f + FlickerAmount * FMath::Sin(GetWorld()->GetTimeSeconds() * FlickerSpeed))); }
}
bool AHeritageFireplace::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type != HeritageTags::Stimulus_Water) { return false; }
    if (Object->HasState(HeritageTags::State_Lit))
    {
        Object->RemoveState(HeritageTags::State_Lit); RefreshFromState();
        UHeritageStatics::RecordFact(this, HeritageTags::Fact_FireDoused, NSLOCTEXT("Heritage", "FactDoused", "Doused the fire with the kettle."));
        Reaction(this, Stimulus, NSLOCTEXT("Heritage", "FireOut", "The fire hisses out."));
    }
    else { Reaction(this, Stimulus, NSLOCTEXT("Heritage", "WetAshes", "The ashes are already wet.")); }
    return true;
}
void AHeritageFireplace::RefreshFromState()
{
    const bool bLit = Object->HasState(HeritageTags::State_Lit);
    if (Flame) { Flame->SetVisibility(bLit); }
    FireLight->SetIntensity(bLit ? FireIntensity : 0.f); RefreshHeat();
}
void AHeritageKey::OnGrabbed(AActor* By)
{
    if (!Object->HasState(HeritageTags::State_Hot))
    {
        UHeritageStatics::RecordFact(this, HeritageTags::Fact_KeyTaken, NSLOCTEXT("Heritage", "FactKey", "Took the key from the hearth."));
    }
}
AHeritageKey::AHeritageKey()
{
    InitShape(EHeritageShape::Cube, FVector(12, 4, 2), FLinearColor(0.95f, 0.65f, 0.05f), 0.2f);
    Object->DisplayName = NSLOCTEXT("Heritage", "Key", "Key"); Object->bMemento = true;
    Object->MementoNote = NSLOCTEXT("Heritage", "KeyNote", "A small metal key.");
}
bool AHeritageKey::UseOn(AActor* Target, AActor* User, FText& OutResult)
{
    if (!Cast<AHeritageDoor>(Target)) { OutResult = NSLOCTEXT("Heritage", "KeyNoFit", "The key does not fit here."); return false; }
    FHeritageStimulus Use; Use.Type = HeritageTags::Stimulus_Use; Use.Source = this; Use.Instigator = User;
    const bool bOpened = UHeritageStatics::SendStimulus(Target, Use);
    OutResult = bOpened ? NSLOCTEXT("Heritage", "DoorOpened", "The key turns. The door opens.") : NSLOCTEXT("Heritage", "AlreadyOpen", "The door is already open.");
    return bOpened;
}
AHeritageDoor::AHeritageDoor()
{
    InitShape(EHeritageShape::Cube, FVector(120, 14, 10), FLinearColor(0.25f, 0.12f, 0.045f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Door", "Door"); Object->AddState(HeritageTags::State_Locked);
}
void AHeritageDoor::BeginPlay()
{
    Super::BeginPlay();
    Panel = AddShape(TEXT("DoorPanel"), EHeritageShape::Cube, FVector(0, 0, -(PanelSize.Z + FrameThickness) / 2), PanelSize, ShapeColor);
    Panel->SetCollisionProfileName(TEXT("BlockAll"));
    for (int32 Sign : {-1, 1})
    {
        auto* Side = AddShape(FName(*FString::Printf(TEXT("DoorPost%d"), Sign)), EHeritageShape::Cube,
            FVector(Sign * (PanelSize.X + FrameThickness) / 2, 0, -(PanelSize.Z + FrameThickness) / 2), FVector(FrameThickness, ShapeSize.Y, PanelSize.Z), ShapeColor);
        Side->SetCollisionProfileName(TEXT("BlockAll"));
    }
}
void AHeritageDoor::RefreshFromState()
{
    if (!Panel) { return; }
    const FRotator Swing(0, Object->HasState(HeritageTags::State_Open) ? OpenAngle : 0.f, 0);
    const FVector Pivot(-PanelSize.X / 2, 0, -(PanelSize.Z + FrameThickness) / 2);
    const FVector Offset = Pivot + Swing.RotateVector(FVector(PanelSize.X / 2, 0, 0));
    Panel->SetWorldLocation(GetActorLocation() + GetActorRotation().RotateVector(Offset));
    Panel->SetWorldRotation(GetActorRotation() + Swing);
}
bool AHeritageDoor::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type == HeritageTags::Stimulus_Impact) { Reaction(this, Stimulus, NSLOCTEXT("Heritage", "DoorRattle", "The door rattles.")); return true; }
    if (Stimulus.Type != HeritageTags::Stimulus_Use) { return false; }
    if (Object->HasState(HeritageTags::State_Open)) { Reaction(this, Stimulus, NSLOCTEXT("Heritage", "AlreadyOpen", "The door is already open.")); return false; }
    if (!Cast<AHeritageKey>(Stimulus.Source)) { Reaction(this, Stimulus, NSLOCTEXT("Heritage", "LockedDoor", "Locked. There is a keyhole.")); return false; }
    Object->RemoveState(HeritageTags::State_Locked); Object->AddState(HeritageTags::State_Open);
    RefreshFromState();
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->Era == EHeritageEra::Reality)
    {
        Flow->CompleteSlice(); GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()->bDemoComplete = true;
    }
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "DoorOpened", "The key turns. The door opens."));
    // Film actors must survive so the director can restore them.
    Stimulus.Source->SetActorHiddenInGame(true); Stimulus.Source->SetActorEnableCollision(false);
    if (auto* Prop = Cast<AHeritageProp>(Stimulus.Source)) { Prop->SetFrozen(true); }
    return true;
}
AHeritageCabinet::AHeritageCabinet()
{
    InitShape(EHeritageShape::Cube, FVector(90, 45, 230), FLinearColor(0.18f, 0.075f, 0.025f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Cabinet", "Cabinet");
    TopVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TopVolume")); TopVolume->SetupAttachment(Mesh);
    TopVolume->SetAbsolute(false, false, true); TopVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic")); TopVolume->SetGenerateOverlapEvents(true);
}
void AHeritageCabinet::BeginPlay()
{
    Super::BeginPlay(); TopVolume->SetBoxExtent(FVector(ShapeSize.X / 2, ShapeSize.Y / 2, TopVolumeHeight / 2));
    TopVolume->SetRelativeLocation(FVector(0, 0, (ShapeSize.Z + TopVolumeHeight) / 2) / Mesh->GetComponentScale());
}
bool AHeritageCabinet::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type != HeritageTags::Stimulus_Impact || Stimulus.Magnitude < ShakeThreshold) { return false; }
    TArray<AActor*> Contents; TopVolume->GetOverlappingActors(Contents);
    for (AActor* Actor : Contents)
    {
        if (auto* Item = UHeritageObjectComponent::Find(Actor))
        {
            if (auto* Body = Cast<UPrimitiveComponent>(Actor->GetRootComponent()); Body && Body->IsSimulatingPhysics() && !Item->bHeld)
            {
                Body->AddImpulse(ShakeDirection.GetSafeNormal() * ShakeOutSpeed + FVector::UpVector * ShakeUpSpeed, NAME_None, true);
            }
        }
    }
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "Shudder", "The cabinet shudders.")); return true;
}
AHeritageNPC::AHeritageNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    InitShape(EHeritageShape::Cylinder, FVector(45, 45, 150), FLinearColor(0.025f, 0.025f, 0.035f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Butler", "Butler");
}
void AHeritageNPC::BeginPlay()
{
    Super::BeginPlay(); RestRotation = GetActorRotation();
    AddShape(TEXT("Head"), EHeritageShape::Sphere, FVector(0, 0, (ShapeSize.Z + HeadSize.Z) / 2), HeadSize, HeadColor);
}
void AHeritageNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (StaggerRemaining > 0.f)
    {
        StaggerRemaining = FMath::Max(0.f, StaggerRemaining - DeltaSeconds);
        SetActorRotation(RestRotation + FRotator(StaggerAngle * StaggerRemaining / FMath::Max(StaggerDuration, UE_SMALL_NUMBER), 0, 0));
    }
}
bool AHeritageNPC::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type == HeritageTags::Stimulus_Water) { Reaction(this, Stimulus, NSLOCTEXT("Heritage", "ButlerWet", "The butler glares at you, dripping.")); return true; }
    if (Stimulus.Type != HeritageTags::Stimulus_Impact || Stimulus.Magnitude < StaggerThreshold) { return false; }
    if (Letter) { Letter->Object->AddState(HeritageTags::State_Dropped); }
    RefreshFromState(); StaggerRemaining = StaggerDuration;
    UHeritageStatics::RecordFact(this, HeritageTags::Fact_ButlerShoved, NSLOCTEXT("Heritage", "FactShoved", "Shoved the butler."));
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "LetterDropped", "The butler stumbles and drops a letter."));
    return true;
}
void AHeritageNPC::RefreshFromState()
{
    StaggerRemaining = 0.f; RestRotation = GetActorRotation();
    if (!Letter) { return; }
    const bool bDropped = Letter->Object->HasState(HeritageTags::State_Dropped);
    Letter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Letter->Object->bGrabbable = bDropped;
    if (!bDropped)
    {
        Letter->Mesh->SetSimulatePhysics(false);
        Letter->SetActorLocation(GetActorLocation() + GetActorRotation().RotateVector(LetterOffset));
        Letter->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        Letter->SetActorHiddenInGame(IsHidden()); Letter->SetActorEnableCollision(!IsHidden());
    }
    else { Letter->SetFrozen(IsFrozen()); }
}
AHeritagePortrait::AHeritagePortrait()
{
    PrimaryActorTick.bCanEverTick = true;
    InitShape(EHeritageShape::Cube, FVector(80, 4, 110), FLinearColor(0.2f, 0.1f, 0.04f), 1.f);
    MakeStatic(); Object->DisplayName = NSLOCTEXT("Heritage", "Portrait", "Portrait"); Object->bMemento = true;
    Object->MementoNote = NSLOCTEXT("Heritage", "PortraitNote", "A framed portrait on the study wall.");
}
void AHeritagePortrait::BeginPlay()
{
    Super::BeginPlay(); ClosedPosition = GetActorLocation(); AddShape(TEXT("PortraitInset"), EHeritageShape::Cube, FVector(0, -(ShapeSize.Y + InnerSize.Y) / 2, 0), InnerSize, InnerColor);
}
bool AHeritagePortrait::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (Stimulus.Type != HeritageTags::Stimulus_Use) { return false; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->Era == EHeritageEra::Reality)
    {
        if (Flow->RequiresScanHere() && Object->bMemento && !Object->HasState(HeritageTags::State_Scanned))
        {
            Reaction(this, Stimulus, NSLOCTEXT("Heritage", "ScanFirst", "Scan this first with the camera.")); return false;
        }
        if (!Object->HasState(HeritageTags::State_Open))
        {
            Object->AddState(HeritageTags::State_Open);
            // Refresh establishes the end state; Tick presents the slide from the closed position.
            RefreshFromState(); SetActorLocation(ClosedPosition); SlideElapsed = 0.f; bSliding = true;
        }
        Reaction(this, Stimulus, NSLOCTEXT("Heritage", "PortraitRevealed", "A key, behind the portrait.")); return true;
    }
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "PortraitUse", "A stern family, painted long ago.")); return true;
}

void AHeritagePortrait::RefreshFromState()
{
    bSliding = false;
    const bool bOpen = Object->HasState(HeritageTags::State_Open);
    SetActorLocation(ClosedPosition + GetActorRotation().RotateVector(FVector(bOpen ? SlideDistance : 0.f, 0, 0)));
    if (HiddenKey)
    {
        HiddenKey->SetActorHiddenInGame(!bOpen); HiddenKey->SetActorEnableCollision(bOpen);
        HiddenKey->SetFrozen(!bOpen);
    }
}
void AHeritagePortrait::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bSliding) { return; }
    SlideElapsed = FMath::Min(SlideElapsed + DeltaSeconds, SlideDuration);
    SetActorLocation(ClosedPosition + GetActorRotation().RotateVector(FVector(SlideDistance * SlideElapsed / FMath::Max(SlideDuration, UE_SMALL_NUMBER), 0, 0)));
    if (SlideElapsed >= SlideDuration) { RefreshFromState(); }
}

AHeritageReelCanister::AHeritageReelCanister()
{
    InitShape(EHeritageShape::Cylinder, FVector(22, 22, 6), FLinearColor(0.08f, 0.09f, 0.1f), 1.f);
    MakeStatic(); Object->bMemento = true;
    Object->DisplayName = NSLOCTEXT("Heritage", "Reel1", "Reel 1");
    Object->MementoNote = NSLOCTEXT("Heritage", "CanisterNote", "A film canister from the study.");
}
void AHeritageReelCanister::BeginPlay()
{
    Super::BeginPlay(); AddShape(TEXT("Disc"), EHeritageShape::Cylinder, DiscOffset, DiscSize, DiscColor);
}
bool AHeritageReelCanister::HandleStimulus(const FHeritageStimulus& Stimulus)
{
    if (IsHidden() || Stimulus.Type != HeritageTags::Stimulus_Use) { return false; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow || !Flow->CanUseVerbs() || Flow->Era != EHeritageEra::Reality) { return false; }
    Reaction(this, Stimulus, NSLOCTEXT("Heritage", "ScanFirst", "Scan this first with the camera."));
    return false;
}
AHeritageShards::AHeritageShards()
{
    InitShape(EHeritageShape::Cube, FVector(7, 6, 4), FLinearColor(0.08f, 0.25f, 0.22f), 1.f);
    MakeStatic(); Object->bMemento = true;
    Object->DisplayName = NSLOCTEXT("Heritage", "Shards", "Vase shards");
    Object->MementoNote = NSLOCTEXT("Heritage", "ShardsNote", "Fragments of a vase on its pedestal.");
}
void AHeritageShards::BeginPlay()
{
    Super::BeginPlay();
    for (int32 Index = 1; Index < ShardCount; ++Index)
    {
        const float Angle = 2.f * PI * Index / ShardCount;
        auto* Shard = AddShape(FName(*FString::Printf(TEXT("Shard%d"), Index)), EHeritageShape::Cube,
            FVector(FMath::Cos(Angle) * ShardRadius, FMath::Sin(Angle) * ShardRadius, 0), ShapeSize, ShapeColor);
        Shard->SetCollisionProfileName(TEXT("BlockAll"));
    }
}
