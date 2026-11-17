#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeritageGameMode.generated.h"
class AHeritageDemoRoom;
UCLASS()
class BLANK_API AHeritageGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHeritageGameMode();
    virtual void StartPlay() override;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector RoomOrigin = FVector(0, 0, 50);
    UPROPERTY() TObjectPtr<AHeritageDemoRoom> Room;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector FilmRoomOrigin = FVector(0, 4000, 50);
    UPROPERTY(EditAnywhere, Category="Heritage") float SelfTestStartDelay = 1.f;
};
