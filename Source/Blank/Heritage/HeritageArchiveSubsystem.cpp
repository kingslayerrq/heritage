#include "HeritageArchiveSubsystem.h"
#include "HeritageObjectComponent.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageTags.h"
#include "HeritageProps.h"
#include "HeritageLog.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UHeritageArchiveSubsystem::LogAction(FGameplayTag Verb, const FText& Object, const FText& Target, const FText& Result)
{
    FHeritageActionRecord Record;
    Record.Time = GetWorld()->GetTimeSeconds(); Record.Verb = Verb;
    Record.Object = Object; Record.Target = Target; Record.Result = Result;
    ActionLog.Add(Record);
    UE_LOG(LogHeritage, Log, TEXT("%.2f %s | %s -> %s | %s"), Record.Time, *Verb.ToString(), *Object.ToString(), *Target.ToString(), *Result.ToString());
    OnActionLogged.Broadcast(Record);
}
bool UHeritageArchiveSubsystem::ScanMemento(UHeritageObjectComponent* Object)
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow->CanUseCamera() || !IsValid(Object) || Object->GetOwner()->IsHidden() || !Object->bMemento || Object->HasState(HeritageTags::State_Scanned)) { return false; }
    FHeritageArchiveEntry Entry;
    Entry.Name = Object->DisplayName; Entry.Note = Object->MementoNote;
    Entry.StateAtCapture = Object->StateTags; Entry.Time = GetWorld()->GetTimeSeconds();
    Entries.Add(Entry);
    Object->AddState(HeritageTags::State_Scanned);
    LogAction(HeritageTags::Verb_Photo, Entry.Name, FText::GetEmpty(), NSLOCTEXT("Heritage", "Captured", "Memento captured."));
    OnMementoScanned.Broadcast(Entry);
    Flow->NotifyScan();
    if (auto* Reel = Cast<AHeritageReelCanister>(Object->GetOwner()))
    {
        Flow->CollectReel(Reel->ReelId);
        Reel->SetActorHiddenInGame(true);
        Reel->SetActorEnableCollision(false);
    }
    else if (Entries.Num() == 1)
    {
        Toast(NSLOCTEXT("Heritage", "FirstNotebookEntry", "Added to the Notebook. [N] opens the Archive."));
    }
    else
    {
        Toast(FText::Format(NSLOCTEXT("Heritage", "CaptureToast", "Captured: {0}"), Entry.Name));
    }
    return true;
}
int32 UHeritageArchiveSubsystem::GetTotalMementos()
{
    int32 Count = 0;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        TInlineComponentArray<UHeritageObjectComponent*> Objects(*It);
        for (const auto* Object : Objects) { if (Object->bMemento) { ++Count; } }
    }
    TotalMementos = FMath::Max(TotalMementos, Count);
    return TotalMementos;
}
void UHeritageArchiveSubsystem::Toast(const FText& Text, float Duration)
{
    CurrentToast = Text;
    ToastExpireTime = GetWorld()->GetTimeSeconds() + (Duration < 0.f ? DefaultToastDuration : Duration);
}
