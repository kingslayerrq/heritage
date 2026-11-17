#include "HeritageStatics.h"
#include "HeritageObjectComponent.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "HeritageProp.h"
#include "HeritageArchiveSubsystem.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarRequireScan(TEXT("heritage.RequireScanToInteract"), 1, TEXT("Require scanning mementos before interaction in Reality."));
bool UHeritageStatics::RequiresScan() { return CVarRequireScan.GetValueOnGameThread() != 0; }
bool UHeritageStatics::SendStimulus(AActor* Target, const FHeritageStimulus& Stimulus)
{
    if (UHeritageObjectComponent* Object = UHeritageObjectComponent::Find(Target)) { return Object->ReceiveStimulus(Stimulus); }
    return false;
}
void UHeritageStatics::LogAction(const UObject* WorldContext, FGameplayTag Verb, const FText& Object, const FText& Target, const FText& Result)
{
    if (WorldContext && WorldContext->GetWorld())
    {
        if (auto* Archive = WorldContext->GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()) { Archive->LogAction(Verb, Object, Target, Result); }
    }
}
void UHeritageStatics::Toast(const UObject* WorldContext, const FText& Text)
{
    if (WorldContext && WorldContext->GetWorld())
    {
        if (auto* Archive = WorldContext->GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()) { Archive->Toast(Text); }
    }
}
FText UHeritageStatics::NameOf(const AActor* Actor)
{
    if (const auto* Object = UHeritageObjectComponent::Find(Actor)) { return Object->DisplayName; }
    return NSLOCTEXT("Heritage", "Nothing", "nothing");
}

void UHeritageStatics::RecordFact(const UObject* WorldContext, FGameplayTag Fact, const FText& Text)
{
    if (!WorldContext || !WorldContext->GetWorld()) { return; }
    auto* Flow = WorldContext->GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow || Flow->Era != EHeritageEra::Film || !Flow->Director) { return; }
    const auto* Prop = Cast<AHeritageProp>(WorldContext);
    if (!Prop) { return; }
    const auto* Registered = Flow->Director->GetActors().Find(Prop->FilmId);
    if (Registered && Registered->Get() == Prop) { Flow->Director->RecordFact(Fact, Text); }
}
