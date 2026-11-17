#include "HeritageHUD.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageArchiveSubsystem.h"
#include "HeritageObjectComponent.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "HeritageDemoRoom.h"
#include "GameFramework/PlayerController.h"
#include "HeritageTags.h"
#include "HeritageStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void AHeritageHUD::ResetNavigation()
{
    SelectedRow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->OpenScreen == EHeritageScreen::Map ? 0 : -1;
    ArchiveTab = 0; bEditing = false; bConfirmUpload = false; ScreenMessage = FText::GetEmpty();
}
void AHeritageHUD::Navigate(int32 X, int32 Y)
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->OpenScreen == EHeritageScreen::None || bConfirmUpload) { return; }
    ScreenMessage = FText::GetEmpty();
    if (Flow->OpenScreen == EHeritageScreen::Map)
    {
        const int32 Count = Flow->OwnedReels.Num() + (Flow->Era == EHeritageEra::Film ? 1 : 0);
        SelectedRow = FMath::Clamp(SelectedRow + Y, 0, FMath::Max(0, Count - 1)); return;
    }
    auto* Director = Flow->Director.Get();
    if (bEditing && Director)
    {
        const int32 Count = Director->GetDistinctFacts().Num();
        SelectedRow = FMath::Clamp(SelectedRow + Y, 0, Count + 2);
        if (X != 0 && SelectedRow == Count) { Director->CycleAudio(bKeyFrame, X); }
        if (X != 0 && SelectedRow == Count + 1) { Director->CycleLight(bKeyFrame, X); }
        return;
    }
    if (SelectedRow == -1 && X != 0) { ArchiveTab = 1 - ArchiveTab; }
    const int32 Count = ArchiveTab == 0 ? GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()->Entries.Num() : 2;
    SelectedRow = FMath::Clamp(SelectedRow + Y, -1, Count - 1);
}
void AHeritageHUD::Confirm()
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->OpenScreen == EHeritageScreen::None) { return; }
    auto* Director = Flow->Director.Get();
    if (Flow->OpenScreen == EHeritageScreen::Map)
    {
        const bool bFilm = Flow->Era == EHeritageEra::Film;
        if (bFilm && SelectedRow == 0) { Flow->LeaveFilm(); return; }
        const int32 ReelIndex = SelectedRow - (bFilm ? 1 : 0);
        if (Flow->OwnedReels.IsValidIndex(ReelIndex))
        {
            const FName ReelId = Flow->OwnedReels[ReelIndex];
            if (bFilm && ReelId == FName(TEXT("Reel1"))) { Flow->CloseScreen(); } else { Flow->EnterFilm(ReelId); }
        }
        return;
    }
    if (!Director) { return; }
    if (bConfirmUpload)
    {
        bConfirmUpload = false;
        FText Result; const bool bUploaded = Director->Upload(bKeyFrame, Result);
        ScreenMessage = Result; UHeritageStatics::Toast(this, Result);
        if (bUploaded) { bEditing = false; SelectedRow = bKeyFrame ? 0 : 1; }
        return;
    }
    if (SelectedRow == -1) { SelectedRow = 0; return; }
    if (ArchiveTab == 0) { return; }
    if (!bEditing)
    {
        bKeyFrame = SelectedRow == 0;
        if (Director->CanEdit(bKeyFrame, ScreenMessage)) { bEditing = true; SelectedRow = 0; }
        return;
    }
    const auto Facts = Director->GetDistinctFacts();
    if (Facts.IsValidIndex(SelectedRow)) { Director->ToggleFact(bKeyFrame, Facts[SelectedRow].Fact); }
    else if (SelectedRow == Facts.Num()) { Director->CycleAudio(bKeyFrame, 1); }
    else if (SelectedRow == Facts.Num() + 1) { Director->CycleLight(bKeyFrame, 1); }
    else if (Director->CanEdit(bKeyFrame, ScreenMessage)) { bConfirmUpload = true; }
}
void AHeritageHUD::Back()
{
    if (bConfirmUpload) { bConfirmUpload = false; return; }
    if (bEditing) { bEditing = false; SelectedRow = bKeyFrame ? 0 : 1; return; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->OpenScreen == EHeritageScreen::Archive && SelectedRow != -1) { SelectedRow = -1; return; }
    Flow->CloseScreen();
}
void AHeritageHUD::Text(const FText& Value, float X, float Y, float Width, FLinearColor Color)
{
    float W = 0.f, H = 0.f; GetTextSize(Value.ToString(), W, H, GEngine->GetMediumFont());
    DrawText(Value.ToString(), Color, X, Y, GEngine->GetMediumFont(), FMath::Min(1.f, Width / FMath::Max(1.f, W)));
}
void AHeritageHUD::WrappedText(const FText& Value, float X, float Y, float Width, int32 MaxLines)
{
    TArray<FString> Words; Value.ToString().ParseIntoArrayWS(Words);
    FString Line; int32 Lines = 0;
    for (const FString& Word : Words)
    {
        const FString Next = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float W = 0.f, H = 0.f; GetTextSize(Next, W, H, GEngine->GetMediumFont());
        if (W > Width && !Line.IsEmpty())
        {
            Text(FText::FromString(Line), X, Y + Lines * LineHeight, Width); Line = Word;
            if (++Lines >= MaxLines) { return; }
        }
        else { Line = Next; }
    }
    if (Lines < MaxLines) { Text(FText::FromString(Line), X, Y + Lines * LineHeight, Width); }
}
void AHeritageHUD::DrawScreenRow(const FText& Value, int32 Row, float X, float Y, float Width)
{
    if (SelectedRow == Row) { DrawRect(SelectionColor, X - 4, Y - 2, Width + 8, LineHeight); }
    Text(Value, X, Y, Width, SelectedRow == Row ? FLinearColor::Yellow : FLinearColor::White);
}
void AHeritageHUD::DrawFilmstrip(float X, float Y, float Width)
{
    const auto* Director = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->Director.Get();
    if (!Director || Director->GetSceneCount() == 0) { return; }
    const auto& Segment = Director->GetSegment(); const float CellWidth = Width / Director->GetSceneCount();
    for (int32 Index = 0; Index < Director->GetSceneCount(); ++Index)
    {
        FLinearColor Color = ReadableColor;
        if (Index > Director->LastPlayableScene()) { Color = LockedColor; }
        if (Index == Segment.KeySceneIndex) { Color = Director->GetEdit(true).bUploaded ? RestoredColor : MissingColor; }
        const float CellX = X + CellWidth * Index;
        if (Index == Director->GetSceneIndex()) { DrawRect(FLinearColor::Yellow, CellX - 2, Y - 2, CellWidth - 2, CellHeight + 4); }
        DrawRect(Color, CellX, Y, CellWidth - 6, CellHeight);
        Text(FText::AsNumber(Index + 1), CellX + 6, Y + 2, CellWidth - 12);
        if (Segment.Scenes[Index].bHasDamagedArea)
        {
            DrawRect(Director->GetEdit(false).bUploaded ? FLinearColor::Green : FLinearColor::Yellow, CellX + CellWidth / 2, Y - 5, 8, 8);
        }
    }
}
void AHeritageHUD::DrawMap()
{
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    const float Width = Canvas->ClipX - Margin * 2, LeftWidth = Width * 0.42f, Top = Margin + LineHeight * 4;
    Text(NSLOCTEXT("Heritage", "StudyMap", "MAP / THE STUDY"), Margin, Top, LeftWidth);
    const float MapY = Top + LineHeight * 2, MapH = FMath::Min(MapMaxHeight, Canvas->ClipY * 0.3f);
    DrawRect(FLinearColor(0.2f, 0.23f, 0.25f), Margin, MapY, LeftWidth, MapH);
    DrawRect(FLinearColor(0.06f, 0.08f, 0.09f), Margin + 4, MapY + 4, LeftWidth - 8, MapH - 8);
    Text(NSLOCTEXT("Heritage", "MapTable", "TABLE"), Margin + LeftWidth * 0.35f, MapY + 12, LeftWidth * 0.3f);
    Text(NSLOCTEXT("Heritage", "MapFireplace", "FIREPLACE"), Margin + LeftWidth * 0.7f, MapY + MapH * 0.45f, LeftWidth * 0.28f);
    Text(NSLOCTEXT("Heritage", "MapDoor", "DOOR"), Margin + LeftWidth * 0.45f, MapY + MapH - LineHeight, LeftWidth * 0.3f);
    Text(Flow->Era == EHeritageEra::Reality ? NSLOCTEXT("Heritage", "RealityBadge", "REALITY") : NSLOCTEXT("Heritage", "FilmBadge", "FILM / Reel 1"), Margin, MapY + MapH + 8, LeftWidth);
    const float RackX = Margin + Width * 0.48f, RackWidth = Width * 0.52f;
    Text(NSLOCTEXT("Heritage", "FilmRack", "FILM RACK"), RackX, Top, RackWidth);
    int32 Row = 0;
    if (Flow->Era == EHeritageEra::Film)
    {
        DrawScreenRow(NSLOCTEXT("Heritage", "LeaveReel", "Leave the reel (return to Reality)"), Row, RackX, Top + LineHeight * (Row + 2), RackWidth); ++Row;
    }
    for (FName Reel : Flow->OwnedReels)
    {
        const FText Label = Reel == FName(TEXT("Reel1")) ? (Flow->IsKeySceneUploaded() ?
            NSLOCTEXT("Heritage", "RackRestored", "Reel 1 - 8 of 8 frames readable") : NSLOCTEXT("Heritage", "RackMissing", "Reel 1 - 4 of 8 frames readable, 1 missing")) : NSLOCTEXT("Heritage", "RackUnreadable", "Reel 2 - unreadable");
        DrawScreenRow(Label, Row, RackX, Top + LineHeight * (Row + 2), RackWidth); ++Row;
    }
    if (Row == 0) { Text(NSLOCTEXT("Heritage", "EmptyRack", "No reels collected."), RackX, Top + LineHeight * 2, RackWidth); }
}
void AHeritageHUD::DrawArchive()
{
    const auto* Director = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->Director.Get();
    const auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
    const float Width = Canvas->ClipX - Margin * 2, Top = Margin + LineHeight * 4, ContentY = Top + LineHeight * 2;
    DrawScreenRow(ArchiveTab == 0 ? NSLOCTEXT("Heritage", "NotebookTab", "[NOTEBOOK]    Workbench") : NSLOCTEXT("Heritage", "WorkbenchTab", "Notebook    [WORKBENCH]"), -1, Margin, Top, Width);
    if (ArchiveTab == 0)
    {
        if (Archive->Entries.IsEmpty()) { Text(NSLOCTEXT("Heritage", "NotebookEmpty", "Scan a memento to add it here."), Margin, ContentY, Width); return; }
        const int32 VisibleRows = FMath::Max(1, FMath::FloorToInt((Canvas->ClipY - Top - ScreenFooterBottom - LineHeight * 2) / LineHeight));
        const int32 First = FMath::Max(0, SelectedRow - VisibleRows + 1);
        for (int32 Index = First; Index < FMath::Min(Archive->Entries.Num(), First + VisibleRows); ++Index)
        {
            DrawScreenRow(Archive->Entries[Index].Name, Index, Margin, ContentY + LineHeight * (Index - First), Width * 0.3f);
        }
        const auto& Entry = Archive->Entries[FMath::Clamp(SelectedRow, 0, Archive->Entries.Num() - 1)];
        WrappedText(Entry.Note, Margin + Width * 0.36f, ContentY, Width * 0.63f, 3);
        Text(NSLOCTEXT("Heritage", "CapturedState", "State at capture:"), Margin + Width * 0.36f, ContentY + LineHeight * 4, Width * 0.63f);
        WrappedText(Entry.StateAtCapture.IsEmpty() ? NSLOCTEXT("Heritage", "NoCapturedState", "No marked state.") : FText::FromString(Entry.StateAtCapture.ToStringSimple()), Margin + Width * 0.36f, ContentY + LineHeight * 5, Width * 0.63f, 2);
        return;
    }
    if (!Director) { return; }
    if (!bEditing)
    {
        DrawFilmstrip(Margin, ContentY, Width);
        DrawScreenRow(Director->GetEdit(true).bUploaded ? NSLOCTEXT("Heritage", "KeyUploadedRow", "Frame 5 (restored, locked)") : NSLOCTEXT("Heritage", "KeyMissingRow", "Frame 5 (missing)"), 0, Margin, ContentY + LineHeight * 2, Width);
        DrawScreenRow(Director->GetEdit(false).bUploaded ? NSLOCTEXT("Heritage", "LeafUploadedRow", "Frame 3 (damaged area, uploaded)") : NSLOCTEXT("Heritage", "LeafRow", "Frame 3 (damaged area)"), 1, Margin, ContentY + LineHeight * 3, Width);
        if (SelectedRow >= 0)
        {
            FText Why; if (!Director->CanEdit(SelectedRow == 0, Why)) { Text(Why, Margin, ContentY + LineHeight * 5, Width, ToastColor); }
        }
    }
    else
    {
        const auto Facts = Director->GetDistinctFacts(); const auto& Edit = Director->GetEdit(bKeyFrame);
        Text(bKeyFrame ? NSLOCTEXT("Heritage", "EditKey", "FRAME 5 / SCENE") : NSLOCTEXT("Heritage", "EditLeaf", "FRAME 3 / SCENE (choose one fact)"), Margin, ContentY, Width);
        for (int32 Index = 0; Index < Facts.Num(); ++Index)
        {
            DrawScreenRow(FText::Format(Edit.ChosenFacts.Contains(Facts[Index].Fact) ? NSLOCTEXT("Heritage", "ChosenFact", "[x] {0}") : NSLOCTEXT("Heritage", "UnchosenFact", "[ ] {0}"), Facts[Index].Text), Index, Margin, ContentY + LineHeight * (Index + 1), Width);
        }
        DrawScreenRow(FText::Format(NSLOCTEXT("Heritage", "AudioRow", "Audio: < {0} >"), AHeritageFilmDirector::AudioName(Edit.AudioChoice)), Facts.Num(), Margin, ContentY + LineHeight * (Facts.Num() + 1), Width);
        DrawScreenRow(FText::Format(NSLOCTEXT("Heritage", "LightRow", "Light: < {0} >"), AHeritageFilmDirector::LightName(Edit.LightChoice)), Facts.Num() + 1, Margin, ContentY + LineHeight * (Facts.Num() + 2), Width);
        DrawScreenRow(NSLOCTEXT("Heritage", "UploadButton", "UPLOAD"), Facts.Num() + 2, Margin, ContentY + LineHeight * (Facts.Num() + 3), Width);
    }
    if (bConfirmUpload)
    {
        DrawRect(FLinearColor(0.04f, 0.04f, 0.05f, 0.98f), Margin, Canvas->ClipY * 0.45f, Width, LineHeight * 3);
        Text(NSLOCTEXT("Heritage", "ConfirmUpload", "Uploaded frames cannot be changed. Upload? [Enter] yes  [Backspace] no"), Margin + 8, Canvas->ClipY * 0.45f + LineHeight, Width - 16, FLinearColor::Yellow);
    }
}
void AHeritageHUD::DrawHUD()
{
    Super::DrawHUD(); if (!Canvas || !GEngine) { return; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
    auto* Character = Cast<AHeritageCharacter>(GetOwningPawn());
    if (!Flow || !Archive || !Character) { return; }
    const auto* Interactor = Character->Interactor.Get(); const auto* Director = Flow->Director.Get();
    const float Width = Canvas->ClipX - Margin * 2;
    const bool bScreen = Flow->OpenScreen != EHeritageScreen::None, bFilm = Flow->Era == EHeritageEra::Film;
    if (bFilm && Director && !bScreen)
    {
        const float CutAlpha = FMath::Clamp(1.f - (GetWorld()->GetTimeSeconds() - Director->GetLastCutTime()) / FMath::Max(CutFlashDuration, UE_SMALL_NUMBER), 0.f, 1.f);
        DrawRect(FLinearColor(0, 0, 0, CutAlpha * CutFlashOpacity), 0, 0, Canvas->ClipX, Canvas->ClipY);
        for (int32 Index = 0; Index < ScratchCount; ++Index)
        {
            const float X = FMath::FRand() * Canvas->ClipX;
            DrawLine(X, 0, X, Canvas->ClipY, FLinearColor(1, 1, 1, ScratchOpacity));
        }
        if (Director->GetMode() == EHeritageFilmMode::Play && Director->GetSceneIndex() == Director->GetSegment().LeafSceneIndex && !Director->GetEdit(false).bUploaded && Flow->FilmRoom && PlayerOwner)
        {
            FVector2D Position;
            const FVector WorldPoint = Flow->FilmRoom->GetActorTransform().TransformPosition(Flow->FilmRoom->PedestalBase + FVector(0, 0, Flow->FilmRoom->PedestalSize.Z));
            if (PlayerOwner->ProjectWorldLocationToScreen(WorldPoint, Position))
            {
                DrawRect(LockedColor, Position.X - DamagePlateSize.X / 2, Position.Y - DamagePlateSize.Y, DamagePlateSize.X, DamagePlateSize.Y);
                Text(NSLOCTEXT("Heritage", "DamagedArea", "DAMAGED AREA"), Position.X - DamagePlateSize.X / 2, Position.Y - DamagePlateSize.Y / 2, DamagePlateSize.X, FLinearColor::Yellow);
            }
        }
        if (Director->IsOnMissingFrame())
        {
            DrawRect(FLinearColor(0.08f, 0.01f, 0.01f, 0.85f), Canvas->ClipX * 0.3f, Canvas->ClipY * 0.3f, Canvas->ClipX * 0.4f, LineHeight * 2);
            Text(NSLOCTEXT("Heritage", "MissingPlate", "FRAME MISSING"), Canvas->ClipX * 0.32f, Canvas->ClipY * 0.3f + 8, Canvas->ClipX * 0.36f, FLinearColor::Red);
        }
    }
    if (!bScreen)
    {
        DrawRect(FLinearColor::White, Canvas->ClipX / 2 - CrosshairSize / 2, Canvas->ClipY / 2 - CrosshairSize / 2, CrosshairSize, CrosshairSize);
        Text(Interactor->GetPromptText(), Margin, Canvas->ClipY * 0.6f, Width);
        if (auto* Held = UHeritageObjectComponent::Find(Interactor->GetHeld()))
        {
            Text(FText::Format(NSLOCTEXT("Heritage", "HeldName", "Holding: {0}"), Held->DisplayName), Margin, Canvas->ClipY * 0.6f + LineHeight, Width);
        }
        Text(bFilm ? NSLOCTEXT("Heritage", "FilmControls", "WASD / Mouse | Space Pause or Jump | E/Q Transport | Backspace Rollback | M Map | N Archive | R Reset") :
            NSLOCTEXT("Heritage", "RealityControls", "WASD / Mouse | Space Jump | Ctrl Crouch | Shift Dash | Tab Camera | LMB Grab/Drop | RMB Throw | F Use | M Map | N Archive | R Reset"), Margin, Margin + LineHeight * 2, Width);
        if (Interactor->IsCameraMode())
        {
            for (int32 XSign : {-1, 1})
            {
                for (int32 YSign : {-1, 1})
                {
                    const float X = Canvas->ClipX * (0.5f + XSign * ViewfinderFraction.X);
                    const float Y = Canvas->ClipY * (0.5f + YSign * ViewfinderFraction.Y);
                    DrawLine(X, Y, X - XSign * BracketLength, Y, FLinearColor::White, BracketThickness);
                    DrawLine(X, Y, X, Y - YSign * BracketLength, FLinearColor::White, BracketThickness);
                }
            }
            const float X = (Canvas->ClipX - ScanBarWidth) / 2;
            const float Y = Canvas->ClipY / 2 + LineHeight * 3;
            DrawRect(FLinearColor::Black, X, Y, ScanBarWidth, ScanBarHeight);
            DrawRect(FLinearColor::White, X, Y, ScanBarWidth * Interactor->GetScanProgress01(), ScanBarHeight);
        }
        if (Interactor->GetFlashAlpha() > 0.f) { DrawRect(FLinearColor(1, 1, 1, Interactor->GetFlashAlpha()), 0, 0, Canvas->ClipX, Canvas->ClipY); }
    }
    else
    {
        DrawRect(ScreenDimColor, 0, 0, Canvas->ClipX, Canvas->ClipY);
        if (Flow->OpenScreen == EHeritageScreen::Map) { DrawMap(); } else { DrawArchive(); }
        Text(Flow->OpenScreen == EHeritageScreen::Map ? NSLOCTEXT("Heritage", "MapFooter", "Up/Down select | Enter confirm | M/N close | Backspace back") :
            NSLOCTEXT("Heritage", "ArchiveFooter", "Tab bar: Left/Right tabs, Down enter | Up/Down rows | Enter toggle | Audio/Light: Left/Right | Backspace back | N/M close"), Margin, Canvas->ClipY - ScreenFooterBottom, Width);
        Text(ScreenMessage, Margin, Canvas->ClipY - ScreenFooterBottom - LineHeight, Width, ToastColor);
    }
    Text(bFilm ? NSLOCTEXT("Heritage", "FilmBadge", "FILM / Reel 1") : NSLOCTEXT("Heritage", "RealityBadge", "REALITY"), Margin, Margin, Width);
    Text(Flow->GetObjectiveText(), Margin, Margin + LineHeight, Width, ToastColor);
    FLinearColor FadedToast = ToastColor;
    FadedToast.A = FMath::Clamp((Archive->ToastExpireTime - GetWorld()->GetTimeSeconds()) / FMath::Max(ToastFadeDuration, UE_SMALL_NUMBER), 0.f, 1.f);
    Text(Archive->CurrentToast, Margin, Canvas->ClipY - ScreenFooterBottom + LineHeight, Width, FadedToast);
    const int32 Start = FMath::Max(0, Archive->ActionLog.Num() - ActionLines);
    for (int32 Index = Start; Index < Archive->ActionLog.Num(); ++Index)
    {
        const auto& Record = Archive->ActionLog[Index];
        Text(FText::Format(NSLOCTEXT("Heritage", "ActionLine", "{0} > {1}: {2}"), Record.Object, Record.Target, Record.Result), Margin,
            Canvas->ClipY - 55 - (Archive->ActionLog.Num() - Index) * (LineHeight * 0.65f), Width * (bFilm ? 0.55f : 1.f), FLinearColor(0.7f, 0.7f, 0.7f));
    }
    if (bFilm && Director)
    {
        DrawFilmstrip(Margin, Canvas->ClipY - CellHeight - 8, Width);
        const FText ModeText = Director->GetMode() == EHeritageFilmMode::Pause ? (bScreen ? NSLOCTEXT("Heritage", "PauseSuspended", "PAUSE / SCREEN") : NSLOCTEXT("Heritage", "PauseMode", "PAUSE *REC")) :
            Director->GetDirection() < 0 ? NSLOCTEXT("Heritage", "RewindMode", "<< REWIND x4") : Director->GetRate() > 1 ? NSLOCTEXT("Heritage", "FastMode", "PLAY >> x4") : NSLOCTEXT("Heritage", "PlayMode", "PLAY >");
        Text(ModeText, Margin + Width * 0.58f, Canvas->ClipY - 145, Width * 0.42f);
        Text(Director->GetCaption(), Margin + Width * 0.58f, Canvas->ClipY - 120, Width * 0.42f);
        Text(Director->GetAudioCaption(), Margin + Width * 0.58f, Canvas->ClipY - 95, Width * 0.42f, ToastColor);
    }
    if (Flow->bSliceComplete) { Text(NSLOCTEXT("Heritage", "SliceComplete", "SLICE COMPLETE"), Canvas->ClipX * 0.35f, Canvas->ClipY * BannerScreenY, Canvas->ClipX * 0.4f, FLinearColor::Yellow); }
}
