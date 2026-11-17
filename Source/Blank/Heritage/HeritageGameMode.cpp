#include "HeritageGameMode.h"
#include "HeritageCharacter.h"
#include "HeritageDemoRoom.h"
#include "HeritageHUD.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

void RunHeritageSelfTest(UWorld* World);

AHeritageGameMode::AHeritageGameMode()
{
    DefaultPawnClass = AHeritageCharacter::StaticClass(); HUDClass = AHeritageHUD::StaticClass();
}
void AHeritageGameMode::StartPlay()
{
    Super::StartPlay();
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    const auto SpawnRoom = [this](EHeritageEra Era, FVector Origin)
    {
        const FTransform Transform(FRotator::ZeroRotator, Origin);
        auto* NewRoom = GetWorld()->SpawnActorDeferred<AHeritageDemoRoom>(AHeritageDemoRoom::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (NewRoom) { NewRoom->Era = Era; NewRoom->FinishSpawning(Transform); NewRoom->Build(); }
        return NewRoom;
    };
    Room = SpawnRoom(EHeritageEra::Reality, RoomOrigin);
    Flow->RealityRoom = Room; Flow->FilmRoom = SpawnRoom(EHeritageEra::Film, FilmRoomOrigin);
    Flow->Director = GetWorld()->SpawnActor<AHeritageFilmDirector>();
    if (Flow->FilmRoom && Flow->Director)
    {
        Flow->FilmRoom->RegisterFilmActors(Flow->Director);
        Flow->Director->Initialize(Flow->FilmRoom);
    }
    if (!Room) { return; }
    const FTransform Spawn = Room->GetPlayerSpawnTransform();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                Pawn->SetActorLocationAndRotation(Spawn.GetLocation(), Spawn.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
                PC->SetControlRotation(Spawn.Rotator());
            }
        }
    }
    Flow->ApplyPostProcess();
    if (FParse::Param(FCommandLine::Get(), TEXT("HeritageSelfTest")))
    {
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { RunHeritageSelfTest(GetWorld()); }), SelfTestStartDelay, false);
    }
}
