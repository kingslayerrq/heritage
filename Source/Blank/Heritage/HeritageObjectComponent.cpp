#include "HeritageObjectComponent.h"
#include "HeritageInterfaces.h"
#include "HeritageStatics.h"
#include "HeritageGameFlowSubsystem.h"
#include "Engine/World.h"
#include "HeritageTags.h"
#include "GameFramework/Actor.h"

void UHeritageObjectComponent::AddState(FGameplayTag State)
{
    if (!HasState(State)) { StateTags.AddTag(State); OnStateChanged.Broadcast(); }
}
void UHeritageObjectComponent::RemoveState(FGameplayTag State)
{
    if (StateTags.RemoveTag(State)) { OnStateChanged.Broadcast(); }
}
bool UHeritageObjectComponent::HasState(FGameplayTag State) const { return StateTags.HasTagExact(State); }
UHeritageObjectComponent* UHeritageObjectComponent::Find(const AActor* Actor)
{
    return IsValid(Actor) ? Actor->FindComponentByClass<UHeritageObjectComponent>() : nullptr;
}
bool UHeritageObjectComponent::ReceiveStimulus(const FHeritageStimulus& Stimulus)
{
    OnStimulus.Broadcast(Stimulus);
    if (IHeritageReactive* Reactive = Cast<IHeritageReactive>(GetOwner())) { return Reactive->HandleStimulus(Stimulus); }
    return false;
}
bool UHeritageObjectComponent::CanBeGrabbed(FText& OutReason) const
{
    if (!bGrabbable) { OutReason = NSLOCTEXT("Heritage", "NotGrabbable", "Cannot pick this up."); return false; }
    if (HasState(HeritageTags::State_Hot)) { OutReason = NSLOCTEXT("Heritage", "TooHot", "Too hot to touch."); return false; }
    // Only mementos can be scanned, so only they are gated; books and the vase stay usable.
    if (GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->RequiresScanHere() && bMemento && !HasState(HeritageTags::State_Scanned))
    {
        OutReason = NSLOCTEXT("Heritage", "ScanFirst", "Scan this first with the camera."); return false;
    }
    if (const IHeritageReactive* Reactive = Cast<IHeritageReactive>(GetOwner())) { return Reactive->CanBeGrabbed(OutReason); }
    OutReason = FText::GetEmpty();
    return true;
}
