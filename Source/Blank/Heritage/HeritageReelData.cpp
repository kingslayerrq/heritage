#include "HeritageReelData.h"
#include "HeritageTags.h"

namespace
{
    FHeritageActorSnapshot Key(FName Id, FVector Position, FVector Size, bool bHidden = false, FRotator Rotation = FRotator::ZeroRotator)
    {
        FHeritageActorSnapshot Result;
        Result.Id = Id; Result.RoomLocal = FTransform(Rotation, Position, Size / 100.f); Result.bHidden = bHidden;
        return Result;
    }
    FHeritageActorSnapshot Butler(FVector Position, bool bHidden = false) { return Key(TEXT("Butler"), Position, FVector(45, 45, 150), bHidden); }
    FHeritageActorSnapshot Kettle(FVector Position, bool bFilled)
    {
        auto Result = Key(TEXT("Kettle"), Position, FVector(22, 22, 25));
        if (bFilled) { Result.State.AddTag(HeritageTags::State_Filled); } return Result;
    }
    FHeritageActorSnapshot Portrait(bool bOpen)
    {
        auto Result = Key(TEXT("Portrait"), FVector(-375, 150, 150), FVector(80, 4, 110), false, FRotator(0, 90, 0));
        if (bOpen) { Result.State.AddTag(HeritageTags::State_Open); } return Result;
    }
}
FHeritageSegmentDef HeritageReelData::BuildReel1()
{
    FHeritageSegmentDef Reel;
    Reel.ReelId = TEXT("Reel1"); Reel.Title = NSLOCTEXT("Heritage", "StudyEvening", "The Study, an evening");
    Reel.KeyAnswer.AddTag(HeritageTags::Fact_FireDoused); Reel.KeyAnswer.AddTag(HeritageTags::Fact_KeyTaken);
    Reel.Scenes.SetNum(8);
    auto Fire = Key(TEXT("Fireplace"), FVector(345, 0, 5), FVector(70, 120, 10)); Fire.State.AddTag(HeritageTags::State_Lit);
    auto HearthKey = Key(TEXT("Key"), FVector(345, 0, 12), FVector(12, 4, 2)); HearthKey.State.AddTag(HeritageTags::State_Hot);
    auto Door = Key(TEXT("Door"), FVector(0, -295, 215), FVector(120, 14, 10)); Door.State.AddTag(HeritageTags::State_Locked);
    Reel.Scenes[0].Keys = { Butler(FVector(-100, -150, 75)), Kettle(FVector(10, 225, 94.5), false), Fire, HearthKey, Door,
        Portrait(false), Key(TEXT("Vase"), FVector(200, -200, 113.5), FVector(25, 25, 45)),
        Key(TEXT("VaseReel"), FVector(200, -200, 113.5), FVector(20, 20, 4), true),
        Key(TEXT("Letter"), FVector(-65, -150, 100), FVector(20, 14, 1)) };
    Reel.Scenes[0].Caption = NSLOCTEXT("Heritage", "Scene1", "A fire. A key in the hearth. The butler arrives.");
    Reel.Scenes[1].Keys = { Butler(FVector(-30, 165, 75)), Kettle(FVector(5, 165, 100), false) };
    Reel.Scenes[1].Caption = NSLOCTEXT("Heritage", "Scene2", "The butler takes the kettle.");
    Reel.Scenes[2].Keys = { Butler(FVector(200, 190, 75)), Kettle(FVector(235, 190, 100), true) };
    Reel.Scenes[2].bHasDamagedArea = true;
    Reel.Scenes[2].Caption = NSLOCTEXT("Heritage", "Scene3", "Water fills the kettle. The pedestal is obscured.");
    Reel.Scenes[3].Keys = { Butler(FVector(230, 0, 75)), Kettle(FVector(270, 0, 95), true) };
    Reel.Scenes[3].Caption = NSLOCTEXT("Heritage", "Scene4", "The full kettle is brought to the fire.");
    Reel.Scenes[4].Kind = EHeritageSceneKind::MissingKey;
    Reel.Scenes[4].Caption = NSLOCTEXT("Heritage", "Scene5", "FRAME MISSING");
    Fire.State.Reset();
    Reel.RestoredKeyScene = { Fire, Kettle(FVector(330, 35, 23.5), false), Butler(FVector(230, 0, 75)), Key(TEXT("Key"), FVector(265, 0, 100), FVector(12, 4, 2)) };
    Reel.Scenes[5].Keys = { Butler(FVector(-180, 80, 75)), Key(TEXT("Key"), FVector(-145, 80, 100), FVector(12, 4, 2)) };
    Reel.Scenes[5].Caption = NSLOCTEXT("Heritage", "Scene6", "The butler carries the key to the portrait.");
    Reel.Scenes[6].Keys = { Butler(FVector(-300, 150, 75)), Portrait(true), Key(TEXT("Key"), FVector(-385, 150, 150), FVector(12, 4, 2), false, FRotator(0, 90, 0)) };
    Reel.Scenes[6].Caption = NSLOCTEXT("Heritage", "Scene7", "The key is hidden behind the portrait.");
    Reel.Scenes[7].Keys = { Butler(FVector(-300, 150, 75), true), Portrait(false), Door, Key(TEXT("Key"), FVector(-385, 150, 150), FVector(12, 4, 2), true, FRotator(0, 90, 0)) };
    Reel.Scenes[7].Caption = NSLOCTEXT("Heritage", "Scene8", "The portrait is back. The door is closed.");
    for (int32 Index = 5; Index < Reel.Scenes.Num(); ++Index) { Reel.Scenes[Index].bLockedUntilKeyUploaded = true; }
    return Reel;
}
