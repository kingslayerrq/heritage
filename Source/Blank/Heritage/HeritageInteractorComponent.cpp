#include "HeritageInteractorComponent.h"
#include "HeritageCharacter.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageObjectComponent.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "HeritageInterfaces.h"
#include "HeritageArchiveSubsystem.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UHeritageInteractorComponent::UHeritageInteractorComponent() { PrimaryComponentTick.bCanEverTick = true; }
UHeritageObjectComponent* UHeritageInteractorComponent::GetFocused() const { return Focused.Get(); }
AActor* UHeritageInteractorComponent::GetHeld() const { return Held.Get(); }
void UHeritageInteractorComponent::BeginPlay() { Super::BeginPlay(); Character = Cast<AHeritageCharacter>(GetOwner()); }
void UHeritageInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Release(false, false);
    for (auto& Pair : CollisionRestores)
    {
        GetWorld()->GetTimerManager().ClearTimer(Pair.Value.Timer);
        if (auto* Body = Pair.Key.Get()) { Body->SetCollisionResponseToChannel(ECC_Pawn, Pair.Value.Response); }
    }
    CollisionRestores.Empty(); Super::EndPlay(EndPlayReason);
}
FVector UHeritageInteractorComponent::HoldTarget() const
{
    return Character->Camera->GetComponentLocation() + Character->Camera->GetForwardVector() * HoldDistance + FVector::UpVector * HoldVerticalOffset;
}
void UHeritageInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Character) { return; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow->CanUseCamera() && bCameraMode) { ExitCameraMode(); }
    if (!Flow->CanMove()) { Focused.Reset(); ScanProgress = 0.f; return; }
    if (Held.IsStale() || HeldBody.IsStale()) { Release(false, false); }
    if (HeldBody.IsValid())
    {
        if (!Held.IsValid() || Held->IsHidden()) { Release(false, false); }
        else if (FVector::Dist(HeldBody->GetCenterOfMass(), HoldTarget()) > BreakDistance) { Drop(); }
        else { Character->PhysicsHandle->SetTargetLocationAndRotation(HoldTarget(), FRotator(0, Character->Camera->GetComponentRotation().Yaw, 0)); }
    }
    else if (Character->PhysicsHandle->GetGrabbedComponent() || Held.IsValid()) { Release(false, false); }
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(HeritageFocus), false, Character);
    if (Held.IsValid()) { Params.AddIgnoredActor(Held.Get()); }
    const FVector Start = Character->Camera->GetComponentLocation();
    GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Character->Camera->GetForwardVector() * (bCameraMode ? ScanDistance : FocusDistance), ECC_Visibility, Params);
    auto* NewFocus = UHeritageObjectComponent::Find(Hit.GetActor());
    if (NewFocus != Focused.Get()) { ScanProgress = 0.f; }
    Focused = NewFocus; FocusHitDistance = Hit.Distance;
    Character->Camera->SetFieldOfView(FMath::FInterpTo(Character->Camera->FieldOfView, bCameraMode ? CameraFOV : NormalFOV, DeltaTime, FOVInterpSpeed));
    if (bCameraMode && NewFocus && NewFocus->bMemento && !NewFocus->HasState(HeritageTags::State_Scanned))
    {
        ScanProgress += DeltaTime;
        if (ScanProgress >= ScanHoldTime)
        {
            auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
            if (Archive->ScanMemento(NewFocus))
            {
                FHeritageStimulus Photo; Photo.Type = HeritageTags::Stimulus_Photo; Photo.Instigator = Character; Photo.Source = Character; Photo.Location = Hit.ImpactPoint;
                UHeritageStatics::SendStimulus(NewFocus->GetOwner(), Photo);
                if (NewFocus->GetOwner()->IsHidden()) { Focused.Reset(); ScanProgress = 0.f; }
                FlashEndTime = GetWorld()->GetTimeSeconds() + FlashDuration;
            }
        }
    }
    else { ScanProgress = 0.f; }
}
bool UHeritageInteractorComponent::CanReach(AActor* Actor, FText& OutReason) const
{
    if (!Character || !IsValid(Actor)) { OutReason = NSLOCTEXT("Heritage", "NoTarget", "Aim at an object."); return false; }
    FVector Centre, Extent; Actor->GetActorBounds(false, Centre, Extent);
    const float Feet = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if (Centre.Z > Feet + MaxReachHeight) { OutReason = NSLOCTEXT("Heritage", "TooHigh", "Too high to reach."); return false; }
    const FVector CameraLocation = Character->Camera->GetComponentLocation();
    const float Distance = Focused.IsValid() && Focused->GetOwner() == Actor ? FocusHitDistance : FVector::Dist(CameraLocation, FBox(Centre - Extent, Centre + Extent).GetClosestPointTo(CameraLocation));
    if (Distance > ReachDistance) { OutReason = NSLOCTEXT("Heritage", "TooFar", "Too far."); return false; }
    if (!Character->GetCharacterMovement()->IsMovingOnGround()) { OutReason = NSLOCTEXT("Heritage", "GroundedOnly", "Stand on the ground to interact."); return false; }
    OutReason = FText::GetEmpty(); return true;
}
void UHeritageInteractorComponent::GrabOrDrop()
{
    if (!Character || bCameraMode || !GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseVerbs()) { return; }
    if (Held.IsValid()) { Drop(); return; }
    auto* Object = Focused.Get();
    FText Reason; AActor* Actor = Object ? Object->GetOwner() : nullptr;
    if (!CanReach(Actor, Reason) || !Object || !Object->CanBeGrabbed(Reason))
    {
        UHeritageStatics::Toast(this, Reason);
        UHeritageStatics::LogAction(this, HeritageTags::Verb_Grab, UHeritageStatics::NameOf(Actor), FText::GetEmpty(), Reason); return;
    }
    auto* Body = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
    if (!Body || !Body->IsSimulatingPhysics())
    {
        Reason = NSLOCTEXT("Heritage", "FixedInPlace", "It is fixed in place."); UHeritageStatics::Toast(this, Reason);
        UHeritageStatics::LogAction(this, HeritageTags::Verb_Grab, Object->DisplayName, FText::GetEmpty(), Reason); return;
    }
    FCollisionRestore* Existing = CollisionRestores.Find(Body);
    if (Existing) { GetWorld()->GetTimerManager().ClearTimer(Existing->Timer); }
    else { FCollisionRestore Restore; Restore.Response = Body->GetCollisionResponseToChannel(ECC_Pawn); CollisionRestores.Add(Body, Restore); }
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Character->PhysicsHandle->GrabComponentAtLocationWithRotation(Body, NAME_None, Body->GetCenterOfMass(), Body->GetComponentRotation());
    Held = Actor; HeldBody = Body; Object->bHeld = true; Object->LastThrower.Reset();
    if (auto* Reactive = Cast<IHeritageReactive>(Actor)) { Reactive->OnGrabbed(Character); }
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Grab, Object->DisplayName, FText::GetEmpty(), NSLOCTEXT("Heritage", "Grabbed", "Picked up."));
}
void UHeritageInteractorComponent::Release(bool bThrow, bool bLog)
{
    if (!Character) { return; }
    AActor* Actor = Held.Get(); UPrimitiveComponent* Body = HeldBody.Get();
    const FText Name = UHeritageStatics::NameOf(Actor);
    if (auto* Object = UHeritageObjectComponent::Find(Actor)) { Object->bHeld = false; Object->LastThrower = bThrow ? Character.Get() : nullptr; }
    Character->PhysicsHandle->ReleaseComponent(); Held.Reset(); HeldBody.Reset();
    if (Body)
    {
        if (bThrow) { Body->AddImpulse(Character->Camera->GetForwardVector() * ThrowSpeed, NAME_None, true); }
        const TWeakObjectPtr<UPrimitiveComponent> WeakBody(Body);
        if (auto* Restore = CollisionRestores.Find(WeakBody))
        {
            GetWorld()->GetTimerManager().SetTimer(Restore->Timer, FTimerDelegate::CreateWeakLambda(this, [this, WeakBody]()
            {
                if (const auto* Pending = CollisionRestores.Find(WeakBody))
                {
                    if (auto* Primitive = WeakBody.Get()) { Primitive->SetCollisionResponseToChannel(ECC_Pawn, Pending->Response); }
                }
                CollisionRestores.Remove(WeakBody);
            }), PawnCollisionRestoreDelay, false);
        }
    }
    if (bLog && Actor)
    {
        UHeritageStatics::LogAction(this, bThrow ? HeritageTags::Verb_Throw : HeritageTags::Verb_Drop, Name, FText::GetEmpty(),
            bThrow ? NSLOCTEXT("Heritage", "Thrown", "Thrown.") : NSLOCTEXT("Heritage", "Dropped", "Put down."));
    }
}
void UHeritageInteractorComponent::Drop() { Release(false, true); }
void UHeritageInteractorComponent::Throw() { if (!bCameraMode && Held.IsValid() && GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseVerbs()) { Release(true, true); } }
FText UHeritageInteractorComponent::GetPromptText() const
{
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow->CanMove() || (!Flow->CanUseVerbs() && !bCameraMode)) { return FText::GetEmpty(); }
    if (bCameraMode)
    {
        if (const auto* Object = Focused.Get(); Object && Object->bMemento)
        {
            return Object->HasState(HeritageTags::State_Scanned) ? NSLOCTEXT("Heritage", "AlreadyCaptured", "Already captured.") : NSLOCTEXT("Heritage", "HoldCapture", "Hold to capture...");
        }
        return NSLOCTEXT("Heritage", "FindMemento", "Aim at a memento. [Tab] Put camera away");
    }
    if (Held.IsValid())
    {
        if (Cast<IHeritageUsable>(Held.Get()))
        {
            if (AActor* Target = UseTarget())
            {
                FText Reason;
                if (!CanReach(Target, Reason)) { return Reason; }
                return FText::Format(NSLOCTEXT("Heritage", "UsePrompt", "[F] Use {0} on {1}  |  [LMB] Drop  [RMB] Throw"), UHeritageStatics::NameOf(Held.Get()), UHeritageStatics::NameOf(Target));
            }
        }
        return NSLOCTEXT("Heritage", "CarryPrompt", "[LMB] Drop  [RMB] Throw");
    }
    if (const auto* Object = Focused.Get())
    {
        FText Reason;
        if (!CanReach(Object->GetOwner(), Reason)) { return Reason; }
        if (Flow->RequiresScanHere() && Object->bMemento && !Object->HasState(HeritageTags::State_Scanned)) { return NSLOCTEXT("Heritage", "ScanFirst", "Scan this first with the camera."); }
        if (!Object->bGrabbable) { return FText::Format(NSLOCTEXT("Heritage", "UseEmptyPrompt", "[F] Use {0}"), Object->DisplayName); }
        if (!Object->CanBeGrabbed(Reason)) { return Reason; }
        return FText::Format(NSLOCTEXT("Heritage", "GrabPrompt", "[LMB] Grab {0}"), Object->DisplayName);
    }
    return FText::GetEmpty();
}
AActor* UHeritageInteractorComponent::UseTarget() const
{
    if (Focused.IsValid()) { return Focused->GetOwner(); }
    if (!Character) { return nullptr; }
    const FVector Point = Character->Camera->GetComponentLocation() + Character->Camera->GetForwardVector() * UseAssistDistance;
    AActor* Closest = nullptr; float BestDistance = FMath::Square(UseAssistRadius);
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (Candidate == Held.Get() || Candidate == Character || Candidate->IsHidden() || !UHeritageObjectComponent::Find(Candidate)) { continue; }
        const float Distance = FVector::DistSquared(Point, Candidate->GetActorLocation());
        if (Distance < BestDistance)
        {
            FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(HeritageUseAssist), false, Character);
            if (Held.IsValid()) { Params.AddIgnoredActor(Held.Get()); }
            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Character->Camera->GetComponentLocation(), Candidate->GetActorLocation(), ECC_Visibility, Params);
            if (!bBlocked || Hit.GetActor() == Candidate) { BestDistance = Distance; Closest = Candidate; }
        }
    }
    return Closest;
}
void UHeritageInteractorComponent::Use()
{
    if (!Character || bCameraMode || !GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseVerbs()) { return; }
    AActor* Source = Held.Get();
    AActor* Target = Source ? UseTarget() : (Focused.IsValid() ? Focused->GetOwner() : nullptr);
    const FText SourceName = UHeritageStatics::NameOf(Source);
    const FText TargetName = UHeritageStatics::NameOf(Target);
    FText Result;
    auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
    const int32 PreviousLogCount = Archive->ActionLog.Num();
    const auto NeedsScan = [this](AActor* Actor)
    {
        const auto* Object = UHeritageObjectComponent::Find(Actor);
        return Object && Object->bMemento && !Object->HasState(HeritageTags::State_Scanned) && GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->RequiresScanHere();
    };
    if (NeedsScan(Source) || NeedsScan(Target))
    {
        Result = NSLOCTEXT("Heritage", "ScanFirst", "Scan this first with the camera.");
    }
    else if (Source && !Cast<IHeritageUsable>(Source))
    {
        Result = NSLOCTEXT("Heritage", "NothingHappens", "Nothing happens.");
    }
    else if (CanReach(Target, Result))
    {
        if (Source)
        {
            if (auto* Usable = Cast<IHeritageUsable>(Source)) { Usable->UseOn(Target, Character, Result); }
            else { Result = NSLOCTEXT("Heritage", "NothingHappens", "Nothing happens."); }
        }
        else
        {
            FHeritageStimulus Stimulus; Stimulus.Type = HeritageTags::Stimulus_Use; Stimulus.Source = Character; Stimulus.Instigator = Character;
            if (UHeritageStatics::SendStimulus(Target, Stimulus)) { Result = NSLOCTEXT("Heritage", "Used", "Used."); }
            else { Result = NSLOCTEXT("Heritage", "NothingHappens", "Nothing happens."); }
        }
    }
    // Preserve the scene's specific reaction instead of hiding it behind a generic verb toast.
    if (Archive->ActionLog.Num() > PreviousLogCount)
    {
        const FText Reaction = Archive->ActionLog.Last().Result;
        if (!Source) { Result = Reaction; }
        else if (!Result.EqualTo(Reaction)) { Result = FText::Format(NSLOCTEXT("Heritage", "UseAndReaction", "{0} {1}"), Result, Reaction); }
    }
    UHeritageStatics::Toast(this, Result);
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Use, SourceName, TargetName, Result);
}
void UHeritageInteractorComponent::ToggleCameraMode()
{
    if (!Character || !GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseCamera()) { return; }
    bCameraMode = !bCameraMode; ScanProgress = 0.f;
    if (bCameraMode) { Character->EndDash(); Drop(); }
}
float UHeritageInteractorComponent::GetScanProgress01() const { return FMath::Clamp(ScanProgress / FMath::Max(ScanHoldTime, UE_SMALL_NUMBER), 0.f, 1.f); }
float UHeritageInteractorComponent::GetFlashAlpha() const
{
    return GetWorld() ? FMath::Clamp((FlashEndTime - GetWorld()->GetTimeSeconds()) / FMath::Max(FlashDuration, UE_SMALL_NUMBER), 0.f, 1.f) : 0.f;
}

void UHeritageInteractorComponent::ExitCameraMode()
{
    bCameraMode = false; ScanProgress = 0.f;
    if (Character && Character->Camera) { Character->Camera->SetFieldOfView(NormalFOV); }
}
