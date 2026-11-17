#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HeritageTypes.h"
#include "HeritageInterfaces.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UHeritageReactive : public UInterface { GENERATED_BODY() };
class BLANK_API IHeritageReactive
{
    GENERATED_BODY()
public:
    virtual bool HandleStimulus(const FHeritageStimulus& Stimulus) = 0;
    virtual bool CanBeGrabbed(FText& OutReason) const { return true; }
    virtual void OnGrabbed(AActor* By) {}
};

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UHeritageUsable : public UInterface { GENERATED_BODY() };
class BLANK_API IHeritageUsable
{
    GENERATED_BODY()
public:
    virtual bool UseOn(AActor* Target, AActor* User, FText& OutResult) = 0;
};
