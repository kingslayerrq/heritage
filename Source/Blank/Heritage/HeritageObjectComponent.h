#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeritageTypes.h"
#include "HeritageObjectComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeritageStimulus, const FHeritageStimulus&, Stimulus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeritageStateChanged);

UCLASS(ClassGroup=(Heritage), meta=(BlueprintSpawnableComponent))
class BLANK_API UHeritageObjectComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Heritage") FText DisplayName;
    UPROPERTY(EditAnywhere, Category="Heritage") bool bGrabbable = false;
    UPROPERTY(EditAnywhere, Category="Heritage") bool bMemento = false;
    UPROPERTY(EditAnywhere, Category="Heritage") FText MementoNote;
    UPROPERTY(EditAnywhere, Category="Heritage") FGameplayTagContainer StateTags;
    UPROPERTY(BlueprintAssignable) FOnHeritageStimulus OnStimulus;
    UPROPERTY(BlueprintAssignable) FOnHeritageStateChanged OnStateChanged;

    // Generic carry metadata: verbs never need to know a concrete prop class.
    bool bHeld = false;
    TWeakObjectPtr<AActor> LastThrower;
    void AddState(FGameplayTag State);
    void RemoveState(FGameplayTag State);
    bool HasState(FGameplayTag State) const;
    bool ReceiveStimulus(const FHeritageStimulus& Stimulus);
    bool CanBeGrabbed(FText& OutReason) const;
    static UHeritageObjectComponent* Find(const AActor* Actor);
};
