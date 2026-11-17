#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeritageTypes.h"
#include "HeritageStatics.generated.h"

UCLASS()
class BLANK_API UHeritageStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    static bool SendStimulus(AActor* Target, const FHeritageStimulus& Stimulus);
    static void LogAction(const UObject* WorldContext, FGameplayTag Verb, const FText& Object, const FText& Target, const FText& Result);
    static void Toast(const UObject* WorldContext, const FText& Text);
    static bool RequiresScan();
    static void RecordFact(const UObject* WorldContext, FGameplayTag Fact, const FText& Text);
    static FText NameOf(const AActor* Actor);
};
