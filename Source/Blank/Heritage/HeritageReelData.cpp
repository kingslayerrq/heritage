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
    FHeritageActorSnapshot Portrait()
    {
        auto Result = Key(TEXT("Portrait"), FVector(-375, 150, 150), FVector(80, 4, 110), false, FRotator(0, 90, 0));
        return Result;
    }
}
FHeritageSegmentDef HeritageReelData::BuildReel1()
{
    FHeritageSegmentDef Reel;
    Reel.ReelId = TEXT("Reel1"); Reel.Title = NSLOCTEXT("Heritage", "StudyEvening", "The Study, an evening");
    Reel.Scenes.SetNum(8);
    auto Fire = Key(TEXT("Fireplace"), FVector(345, 0, 5), FVector(70, 120, 10)); Fire.State.AddTag(HeritageTags::State_Lit);
    auto HearthKey = Key(TEXT("Key"), FVector(345, 0, 12), FVector(12, 4, 2)); HearthKey.State.AddTag(HeritageTags::State_Hot);
    auto Door = Key(TEXT("Door"), FVector(0, -295, 215), FVector(120, 14, 10)); Door.State.AddTag(HeritageTags::State_Locked);
    Reel.Scenes[0].Keys = { Butler(FVector(-100, -150, 75)), Kettle(FVector(10, 225, 94.5), false), Fire, HearthKey, Door,
        Portrait(), Key(TEXT("Vase"), FVector(200, -200, 113.5), FVector(25, 25, 45)),
        Key(TEXT("VaseReel"), FVector(200, -200, 113.5), FVector(20, 20, 4), true),
        Key(TEXT("Letter"), FVector(-65, -150, 100), FVector(20, 14, 1)) };
    Reel.Scenes[0].Caption = NSLOCTEXT("Heritage", "Scene1", "A fire. A key in the hearth. The butler arrives.");
    Reel.Scenes[1].Keys = { Butler(FVector(-30, 165, 75)), Kettle(FVector(5, 165, 100), false) };
    Reel.Scenes[1].Caption = NSLOCTEXT("Heritage", "Scene2", "The butler takes the kettle.");
    Reel.Scenes[2].Restored = { Butler(FVector(200, 190, 75)), Kettle(FVector(235, 190, 100), true) };
    Reel.Scenes[2].Kind = EHeritageSceneKind::Missing;
    Reel.Scenes[2].Answer.AddTag(HeritageTags::Fact_KettleFilled);
    Reel.Scenes[2].Caption = NSLOCTEXT("Heritage", "Scene3", "FRAME MISSING");
    Reel.Scenes[3].Keys = { Butler(FVector(230, 0, 75)), Kettle(FVector(270, 0, 95), true) };
    Reel.Scenes[3].Caption = NSLOCTEXT("Heritage", "Scene4", "The full kettle is brought to the fire.");
    Reel.Scenes[4].Kind = EHeritageSceneKind::Missing;
    Reel.Scenes[4].Caption = NSLOCTEXT("Heritage", "Scene5", "FRAME MISSING");
    Reel.Scenes[4].Answer.AddTag(HeritageTags::Fact_FireDoused);
    Reel.Scenes[4].Answer.AddTag(HeritageTags::Fact_KeyTaken);
    Fire.State.Reset();
    Reel.Scenes[4].Restored = { Fire, Kettle(FVector(330, 35, 23.5), false), Butler(FVector(230, 0, 75)), Key(TEXT("Key"), FVector(265, 0, 100), FVector(12, 4, 2)) };
    // Complete post-gap state: neither missing frame is required to contribute keys.
    Reel.Scenes[5].Keys = { Fire, Kettle(FVector(330, 35, 23.5), false), Butler(FVector(180, 30, 75)),
        Key(TEXT("Key"), FVector(145, 30, 100), FVector(12, 4, 2)) };
    Reel.Scenes[5].Caption = NSLOCTEXT("Heritage", "V2Scene6", "The butler leaves the fireplace carrying the key.");
    Reel.Scenes[6].Keys = { Butler(FVector(-100, 90, 75)), Key(TEXT("Key"), FVector(-135, 90, 100), FVector(12, 4, 2)) };
    Reel.Scenes[6].Caption = NSLOCTEXT("Heritage", "V2Scene7", "The butler crosses the room.");
    Reel.Scenes[7].Keys = { Butler(FVector(-300, 150, 75)), Portrait(), Door,
        Key(TEXT("Key"), FVector(-335, 150, 100), FVector(12, 4, 2)) };
    Reel.Scenes[7].Caption = NSLOCTEXT("Heritage", "V2Scene8", "The butler stands before the closed portrait.");
    return Reel;
}
