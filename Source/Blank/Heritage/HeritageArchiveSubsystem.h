#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HeritageTypes.h"
#include "HeritageArchiveSubsystem.generated.h"

class UHeritageObjectComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FHeritageActionLogged, const FHeritageActionRecord&);
DECLARE_MULTICAST_DELEGATE_OneParam(FHeritageMementoScanned, const FHeritageArchiveEntry&);

UCLASS()
class BLANK_API UHeritageArchiveSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<FHeritageActionRecord> ActionLog;
    UPROPERTY() TArray<FHeritageArchiveEntry> Entries;
    UPROPERTY() FText CurrentToast;
    UPROPERTY() float ToastExpireTime = 0.f;
    UPROPERTY() bool bDemoComplete = false;
    UPROPERTY(EditAnywhere, Category="Heritage") float DefaultToastDuration = 2.5f;
    FHeritageActionLogged OnActionLogged;
    FHeritageMementoScanned OnMementoScanned;
    void LogAction(FGameplayTag Verb, const FText& Object, const FText& Target, const FText& Result);
    bool ScanMemento(UHeritageObjectComponent* Object);
    int32 GetTotalMementos();
    void Toast(const FText& Text, float Duration = -1.f);
private:
    // Keep the denominator stable after a photographed key is consumed.
    int32 TotalMementos = 0;
};
