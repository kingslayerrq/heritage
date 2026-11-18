#include "HeritageHUD.h"
#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageArchiveSubsystem.h"
#include "HeritageObjectComponent.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "HeritageStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
    enum EButtonId : int32
    {
        ReelTab = 1, NotebookTab, Close, ToggleReel, Reel2,
        AudioPrevious = 10, AudioNext, LightPrevious, LightNext, Upload, Accept, Cancel,
        NotesPrevious, NotesNext,
        FrameBase = 100, FactBase = 200, NoteBase = 300
    };
    const FLinearColor Backing(0.025f, 0.035f, 0.045f, 0.88f);
    const FLinearColor Amber(1.f, 0.8f, 0.35f);
}
void AHeritageHUD::ResetNavigation()
{
    Buttons.Reset(); bNotebook = false; bConfirmUpload = false; ScreenMessage = FText::GetEmpty();
    SelectedNote = 0; NoteFirst = 0; SelectedFrame = INDEX_NONE;
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->Director)
    {
        SelectedFrame = Flow->Era == EHeritageEra::Film ? Flow->Director->GetTargetFrame() : INDEX_NONE;
        if (SelectedFrame == INDEX_NONE) { SelectedFrame = Flow->Director->GetFirstUnrestoredFrame(); }
        if (SelectedFrame == INDEX_NONE) { SelectedFrame = Flow->Director->GetSceneIndex(); }
    }
    FocusId = !Flow->OwnsReel(TEXT("Reel1")) ? ReelTab :
        Flow->Era == EHeritageEra::Reality ? ToggleReel : FrameBase + SelectedFrame;
}
bool AHeritageHUD::MousePosition(float& X, float& Y) const
{
    if (!PlayerOwner || !PlayerOwner->GetMousePosition(X, Y)) { return false; }
    X /= Scale; Y /= Scale; return true;
}
void AHeritageHUD::Click()
{
    if (GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->OpenScreen == EHeritageScreen::None) { return; }
    float X = 0.f, Y = 0.f; if (!MousePosition(X, Y)) { return; }
    for (int32 Index = Buttons.Num() - 1; Index >= 0; --Index)
    {
        // Copy: activation may invalidate the hit-target list (close, tab switch, modal).
        const FButton Hit = Buttons[Index];
        if (Hit.Contains(X, Y)) { FocusId = Hit.Id; if (Hit.bEnabled) { Activate(Hit.Id); } return; }
    }
}
void AHeritageHUD::Navigate(int32 X, int32 Y)
{
    if (GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->OpenScreen == EHeritageScreen::None || Buttons.IsEmpty()) { return; }
    if (X != 0)
    {
        auto* Director = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->Director.Get();
        if (Director && (FocusId == AudioPrevious || FocusId == AudioNext)) { Director->CycleAudio(SelectedFrame, X); return; }
        if (Director && (FocusId == LightPrevious || FocusId == LightNext)) { Director->CycleLight(SelectedFrame, X); return; }
    }
    const auto* Current = Buttons.FindByPredicate([this](const auto& B) { return B.Id == FocusId; });
    if (!Current) { FocusId = Buttons[0].Id; return; }
    const float CX = Current->X + Current->W / 2, CY = Current->Y + Current->H / 2;
    float Best = BIG_NUMBER; int32 Next = FocusId;
    for (const auto& B : Buttons)
    {
        const float DX = B.X + B.W / 2 - CX, DY = B.Y + B.H / 2 - CY;
        const float Along = X != 0 ? DX * X : DY * Y;
        const float Across = X != 0 ? FMath::Abs(DY) : FMath::Abs(DX);
        if (Along <= 1.f) { continue; }
        const float Score = Along + Across * 4.f;
        if (Score < Best) { Best = Score; Next = B.Id; }
    }
    FocusId = Next;
    if (FocusId >= FrameBase && FocusId < FactBase) { SelectedFrame = FocusId - FrameBase; ScreenMessage = FText::GetEmpty(); Buttons.Reset(); }
    if (FocusId >= NoteBase) { SelectedNote = FocusId - NoteBase; }
}
void AHeritageHUD::Confirm()
{
    if (GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->OpenScreen == EHeritageScreen::None) { return; }
    const auto* ButtonToUse = Buttons.FindByPredicate([this](const auto& B) { return B.Id == FocusId; });
    if (ButtonToUse && ButtonToUse->bEnabled) { Activate(FocusId); }
}
void AHeritageHUD::Back()
{
    Buttons.Reset();
    if (bConfirmUpload) { bConfirmUpload = false; FocusId = Upload; return; }
    GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CloseScreen();
}
void AHeritageHUD::Activate(int32 Id)
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    auto* Director = Flow->Director.Get();
    if (Flow->OpenScreen == EHeritageScreen::None) { return; }
    Buttons.Reset(); ScreenMessage = FText::GetEmpty();
    if (bConfirmUpload)
    {
        if (Id == Accept && Director)
        {
            bConfirmUpload = false; FocusId = Upload;
            FText UploadResult;
            if (!Director->Upload(SelectedFrame, UploadResult))
            {
                ScreenMessage = UploadResult;
                UHeritageStatics::Toast(this, UploadResult);
            }
        }
        else if (Id == Cancel) { bConfirmUpload = false; FocusId = Upload; }
        return;
    }
    if (Id == Close) { Flow->CloseScreen(); return; }
    if (Id == ToggleReel) { Flow->ToggleReel(); return; }
    if (Id == Reel2)
    {
        Flow->EnterFilm(TEXT("Reel2"));
        ScreenMessage = NSLOCTEXT("Heritage", "UnreadableReel", "This reel cannot be read yet."); return;
    }
    if (Id == ReelTab || Id == NotebookTab) { bNotebook = Id == NotebookTab; return; }
    if (Id == NotesPrevious || Id == NotesNext)
    {
        const int32 Count = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>()->Entries.Num();
        NoteFirst = FMath::Clamp(NoteFirst + (Id == NotesNext ? 8 : -8), 0, FMath::Max(0, Count - 1));
        SelectedNote = NoteFirst; return;
    }
    if (Id >= NoteBase) { SelectedNote = Id - NoteBase; return; }
    if (!Director) { return; }
    if (Id >= FactBase)
    {
        const auto Facts = Director->GetFactsFor(SelectedFrame);
        if (Facts.IsValidIndex(Id - FactBase)) { Director->ToggleFact(SelectedFrame, Facts[Id - FactBase].Fact); }
        return;
    }
    if (Id >= FrameBase) { SelectedFrame = Id - FrameBase; return; }
    if (Id == AudioPrevious || Id == AudioNext) { Director->CycleAudio(SelectedFrame, Id == AudioNext ? 1 : -1); }
    if (Id == LightPrevious || Id == LightNext) { Director->CycleLight(SelectedFrame, Id == LightNext ? 1 : -1); }
    if (Id == Upload && Director->CanUpload(SelectedFrame, ScreenMessage)) { bConfirmUpload = true; FocusId = Accept; }
}
void AHeritageHUD::Rect(FLinearColor Color, float X, float Y, float W, float H)
{
    DrawRect(Color, X * Scale, Y * Scale, W * Scale, H * Scale);
}
void AHeritageHUD::Text(const FText& Value, float X, float Y, float W, FLinearColor Color, bool bHeading)
{
    auto* Font = bHeading ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
    float TW = 0.f, TH = 0.f; GetTextSize(Value.ToString(), TW, TH, Font);
    DrawText(Value.ToString(), Color, X * Scale, Y * Scale, Font, Scale * FMath::Min(1.f, W / FMath::Max(1.f, TW)));
}
void AHeritageHUD::Plate(const FText& Value, float X, float Y, float W, FLinearColor Color, bool bHeading)
{
    if (Value.IsEmpty()) { return; }
    Rect(Backing, X, Y, W, bHeading ? 42.f : LineHeight);
    Text(Value, X + 8, Y + 4, W - 16, Color, bHeading);
}
void AHeritageHUD::WrappedText(const FText& Value, float X, float Y, float W, int32 MaxLines)
{
    TArray<FString> Words; Value.ToString().ParseIntoArrayWS(Words);
    FString Line; int32 Lines = 0;
    for (const FString& Word : Words)
    {
        const FString Next = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float TW = 0.f, TH = 0.f; GetTextSize(Next, TW, TH, GEngine->GetMediumFont());
        if (TW > W && !Line.IsEmpty())
        {
            Text(FText::FromString(Line), X, Y + Lines * LineHeight, W); Line = Word;
            if (++Lines >= MaxLines) { return; }
        }
        else { Line = Next; }
    }
    if (Lines < MaxLines) { Text(FText::FromString(Line), X, Y + Lines * LineHeight, W); }
}
void AHeritageHUD::Button(const FText& Value, int32 Id, float X, float Y, float W, float H, bool bEnabled, FLinearColor Color)
{
    const FButton B{X, Y, W, H, Id, bEnabled}; Buttons.Add(B);
    float MX = 0.f, MY = 0.f;
    const bool bHover = MousePosition(MX, MY) && B.Contains(MX, MY);
    Rect((bHover || FocusId == Id) ? Amber : FLinearColor(0.4f, 0.45f, 0.5f), X, Y, W, H);
    Rect(bEnabled ? Color : FLinearColor(0.09f, 0.1f, 0.11f), X + 2, Y + 2, W - 4, H - 4);
    Text(Value, X + 10, Y + 9, W - 20, bEnabled ? FLinearColor::White : FLinearColor(0.55f, 0.55f, 0.55f));
}
void AHeritageHUD::DrawFilmstrip(float X, float Y, float W, bool bClickable)
{
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    const auto* Director = Flow->Director.Get(); if (!Director || Director->GetSceneCount() == 0) { return; }
    const float CellWidth = W / Director->GetSceneCount();
    for (int32 Frame = 0; Frame < Director->GetSceneCount(); ++Frame)
    {
        const FLinearColor Color = Director->IsLocked(Frame) ? LockedColor :
            Director->IsRestored(Frame) ? RestoredColor : Director->IsImaged(Frame) ? ImagedColor : Director->IsMissing(Frame) ? MissingColor : ReadableColor;
        const float CX = X + Frame * CellWidth;
        if (Flow->Era == EHeritageEra::Film && Frame == Director->GetSceneIndex()) { Rect(Amber, CX - 3, Y - 3, CellWidth - 2, 64); }
        const FText Label = FText::Format(NSLOCTEXT("Heritage", "FilmCell", "{0}  {1}"), FText::AsNumber(Frame + 1), Director->GetSceneStatus(Frame));
        if (bClickable) { Button(Label, FrameBase + Frame, CX, Y, CellWidth - 8, 58, true, Color); }
        else { Rect(Color, CX, Y, CellWidth - 8, 58); Text(Label, CX + 6, Y + 16, CellWidth - 20); }
        if (bClickable && Frame == SelectedFrame) { Rect(FLinearColor::White, CX + 6, Y + 50, CellWidth - 20, 3); }
    }
}
void AHeritageHUD::DrawArchive()
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    const auto* Director = Flow->Director.Get();
    const auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
    const float W = Width - Margin * 2;
    Button(NSLOCTEXT("Heritage", "ReelTab", "REEL"), ReelTab, Margin, 104, 150, 42);
    Button(NSLOCTEXT("Heritage", "V2NotebookTab", "NOTEBOOK"), NotebookTab, Margin + 160, 104, 170, 42);
    Button(NSLOCTEXT("Heritage", "CloseButton", "CLOSE [N]"), Close, Width - Margin - 140, 104, 140, 42);
    if (Flow->OwnsReel(TEXT("Reel1")))
    {
        Button(Flow->Era == EHeritageEra::Film ? NSLOCTEXT("Heritage", "LeaveReelButton", "LEAVE REEL") : NSLOCTEXT("Heritage", "EnterReelButton", "ENTER REEL"), ToggleReel, Width - Margin - 370, 104, 220, 42);
    }
    if (bNotebook)
    {
        Text(NSLOCTEXT("Heritage", "NotebookHeading", "NOTEBOOK"), Margin, 170, W, FLinearColor::White, true);
        if (Archive->Entries.IsEmpty()) { Text(NSLOCTEXT("Heritage", "NotebookEmpty", "Scan a memento to add it here."), Margin, 220, W); }
        else
        {
            for (int32 Index = NoteFirst; Index < FMath::Min(NoteFirst + 8, Archive->Entries.Num()); ++Index)
            {
                Button(Archive->Entries[Index].Name, NoteBase + Index, Margin, 220 + (Index - NoteFirst) * 48, W * 0.3f, 42);
            }
            const auto& Entry = Archive->Entries[FMath::Clamp(SelectedNote, 0, Archive->Entries.Num() - 1)];
            Text(Entry.Name, Margin + W * 0.35f, 220, W * 0.65f, Amber, true);
            WrappedText(Entry.Note, Margin + W * 0.35f, 275, W * 0.65f, 5);
            Text(NSLOCTEXT("Heritage", "CapturedState", "State at capture:"), Margin + W * 0.35f, 440, W * 0.65f);
            WrappedText(Entry.StateAtCapture.IsEmpty() ? NSLOCTEXT("Heritage", "NoCapturedState", "No marked state.") : FText::FromString(Entry.StateAtCapture.ToStringSimple()), Margin + W * 0.35f, 475, W * 0.65f, 5);
            if (Archive->Entries.Num() > 8)
            {
                Button(FText::FromString(TEXT("<")), NotesPrevious, Margin, 620, 90, 40, NoteFirst > 0);
                Button(FText::FromString(TEXT(">")), NotesNext, Margin + 100, 620, 90, 40, NoteFirst + 8 < Archive->Entries.Num());
            }
        }
    }
    else if (!Flow->OwnsReel(TEXT("Reel1")))
    {
        Plate(NSLOCTEXT("Heritage", "NoReel", "Scan the film reel on the table to add it here."), Margin, 180, W, Amber, true);
    }
    else if (Director)
    {
        DrawFilmstrip(Margin, 170, W, true);
        Text(FText::Format(NSLOCTEXT("Heritage", "FrameTitle", "Frame {0} - {1}"), FText::AsNumber(SelectedFrame + 1), Director->GetSceneStatus(SelectedFrame)), Margin, 250, W, FLinearColor::White, true);
        FText Why; const bool bFrameEditable = Director->CanEdit(SelectedFrame, Why);
        if (Director->IsLocked(SelectedFrame))
        {
            Text(FText::Format(NSLOCTEXT("Heritage", "LockedFrameHelp", "Locked. Restore frame {0} first."), FText::AsNumber(Director->GetFirstUnrestoredFrame() + 1)), Margin, 296, W, Amber);
        }
        else
        {
            Text(bFrameEditable ? NSLOCTEXT("Heritage", "ChecklistHelp", "Tick the actions that belong in this frame.") : Why, Margin, 296, W, Amber);
        }
        if (Director->IsMissing(SelectedFrame))
        {
            const auto Facts = Director->GetFactsFor(SelectedFrame); const auto& Edit = Director->GetEdit(SelectedFrame);
            for (int32 Index = 0; Index < Facts.Num(); ++Index)
            {
                Button(FText::Format(Edit.ChosenFacts.Contains(Facts[Index].Fact) ? NSLOCTEXT("Heritage", "ChosenFact", "[x] {0}") : NSLOCTEXT("Heritage", "UnchosenFact", "[ ] {0}"), Facts[Index].Text), FactBase + Index, Margin, 342 + Index * 46, W * 0.6f, 40, bFrameEditable);
            }
            if (Facts.IsEmpty()) { Text(NSLOCTEXT("Heritage", "NoEvidence", "No actions recorded for this frame yet."), Margin, 350, W * 0.6f); }
            const float RX = Margin + W * 0.65f, RW = W * 0.35f;
            Text(FText::Format(NSLOCTEXT("Heritage", "AudioLabel", "Audio: {0}"), AHeritageFilmDirector::AudioName(Edit.AudioChoice)), RX, 342, RW);
            Button(FText::FromString(TEXT("<")), AudioPrevious, RX, 378, 70, 40, bFrameEditable);
            Button(FText::FromString(TEXT(">")), AudioNext, RX + 80, 378, 70, 40, bFrameEditable);
            Text(FText::Format(NSLOCTEXT("Heritage", "LightLabel", "Light: {0}"), AHeritageFilmDirector::LightName(Edit.LightChoice)), RX, 440, RW);
            Button(FText::FromString(TEXT("<")), LightPrevious, RX, 476, 70, 40, bFrameEditable);
            Button(FText::FromString(TEXT(">")), LightNext, RX + 80, 476, 70, 40, bFrameEditable);
            FText UploadWhy; const bool bUploadable = Director->CanUpload(SelectedFrame, UploadWhy);
            Button(FText::Format(NSLOCTEXT("Heritage", "UploadFrame", "UPLOAD FRAME {0}"), FText::AsNumber(SelectedFrame + 1)), Upload, RX, 552, RW, 52, bUploadable, RestoredColor);
            if (!bUploadable) { WrappedText(UploadWhy, RX, 618, RW, 2); }
        }
        if (Flow->OwnsReel(TEXT("Reel2"))) { Button(NSLOCTEXT("Heritage", "Reel2Button", "REEL 2 / UNREADABLE"), Reel2, Margin, 660, 300, 40); }
    }
    Text(ScreenMessage, Margin, Height - 80, W, Amber);
    Text(NSLOCTEXT("Heritage", "V2ArchiveFooter", "Click or arrows / Enter | Audio / Light: Left / Right | Backspace: cancel / close | N: close"), Margin, Height - 42, W);
    if (bConfirmUpload)
    {
        // Modal hit list contains only these two buttons; underlying controls cannot receive a click.
        Buttons.Reset(); Rect(FLinearColor(0, 0, 0, 0.8f), 0, 0, Width, Height);
        const float X = (Width - 640) / 2, Y = (Height - 210) / 2;
        Rect(FLinearColor(0.12f, 0.16f, 0.2f), X, Y, 640, 210);
        Text(NSLOCTEXT("Heritage", "UploadHeading", "UPLOAD FRAME?"), X + 24, Y + 20, 592, Amber, true);
        Text(NSLOCTEXT("Heritage", "UploadWarning", "Uploaded frames cannot be changed."), X + 24, Y + 76, 592);
        Button(NSLOCTEXT("Heritage", "UploadButton", "UPLOAD"), Accept, X + 24, Y + 132, 280, 50);
        Button(NSLOCTEXT("Heritage", "CancelButton", "CANCEL"), Cancel, X + 336, Y + 132, 280, 50);
    }
}
void AHeritageHUD::DrawControls()
{
    const auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    const auto* Director = Flow->Director.Get();
    float X = Margin;
    const auto Chip = [this, &X](const TCHAR* Key, const FText& Action, float W)
    {
        Rect(Backing, X, 86, W, 36); Rect(FLinearColor(0.7f, 0.75f, 0.8f), X, 86, 84, 36);
        Rect(Backing, X + 2, 88, 80, 32); Text(FText::FromString(Key), X + 8, 94, 68);
        Text(Action, X + 92, 94, W - 100); X += W + 10;
    };
    if (Flow->Era == EHeritageEra::Reality)
    {
        Chip(TEXT("Tab"), NSLOCTEXT("Heritage", "CameraChip", "Camera"), 200);
        Chip(TEXT("N"), NSLOCTEXT("Heritage", "ArchiveChip", "Archive"), 200);
        if (Flow->OwnsReel(TEXT("Reel1"))) { Chip(TEXT("Enter"), NSLOCTEXT("Heritage", "EnterChip", "Enter reel"), 260); }
    }
    else if (Director && Director->GetMode() == EHeritageFilmMode::Play)
    {
        Chip(TEXT("Space"), NSLOCTEXT("Heritage", "PauseChip", "Pause"), 190);
        Chip(TEXT("E"), FText::FromString(TEXT(">>")), 140); Chip(TEXT("Q"), FText::FromString(TEXT("<<")), 140);
        Chip(TEXT("Enter"), NSLOCTEXT("Heritage", "LeaveChip", "Leave"), 190);
        Chip(TEXT("N"), NSLOCTEXT("Heritage", "ArchiveChip", "Archive"), 200);
    }
    else if (Director)
    {
        Chip(TEXT("E"), NSLOCTEXT("Heritage", "ResumeChip", "Resume"), 200);
        Chip(TEXT("Backspace"), NSLOCTEXT("Heritage", "RollbackChip", "Roll back"), 250);
        const int32 Frame = Director->GetTargetFrame();
        Chip(TEXT("N"), Frame != INDEX_NONE && !Director->IsLocked(Frame) && Director->IsImaged(Frame) ? FText::Format(NSLOCTEXT("Heritage", "UploadChip", "Upload frame {0}"), FText::AsNumber(Frame + 1)) : NSLOCTEXT("Heritage", "ArchiveChip", "Archive"), 290);
        Chip(TEXT("Enter"), NSLOCTEXT("Heritage", "LeaveChip", "Leave"), 190);
    }
}
void AHeritageHUD::DrawHUD()
{
    Super::DrawHUD(); Buttons.Reset(); if (!Canvas || !GEngine) { return; }
    Scale = FMath::Max(0.01f, FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 800.f));
    Width = Canvas->ClipX / Scale; Height = Canvas->ClipY / Scale;
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    auto* Archive = GetWorld()->GetSubsystem<UHeritageArchiveSubsystem>();
    auto* Character = Cast<AHeritageCharacter>(GetOwningPawn());
    if (!Flow || !Archive || !Character) { return; }
    const auto* Director = Flow->Director.Get(); const auto* Interactor = Character->Interactor.Get();
    const bool bFilm = Flow->Era == EHeritageEra::Film, bScreen = Flow->OpenScreen != EHeritageScreen::None;
    const float W = Width - Margin * 2;
    if (bScreen) { Rect(FLinearColor(0.015f, 0.02f, 0.03f, 0.97f), 0, 0, Width, Height); }
    else
    {
        if (bFilm && Director)
        {
            const float Alpha = FMath::Clamp(1.f - (GetWorld()->GetTimeSeconds() - Director->GetLastCutTime()) / FMath::Max(CutFlashDuration, UE_SMALL_NUMBER), 0.f, 1.f);
            Rect(FLinearColor(0, 0, 0, Alpha * CutFlashOpacity), 0, 0, Width, Height);
            for (int32 Index = 0; Index < ScratchCount; ++Index) { Rect(FLinearColor(1, 1, 1, ScratchOpacity), FMath::FRand() * Width, 0, 1, Height); }
            const FText Mode = Director->GetMode() == EHeritageFilmMode::Pause ? NSLOCTEXT("Heritage", "V2PauseMode", "PAUSE / REC") : Director->GetDirection() < 0 ? NSLOCTEXT("Heritage", "RewindMode", "<< REWIND x4") : Director->GetRate() > 1 ? NSLOCTEXT("Heritage", "FastMode", "PLAY >> x4") : NSLOCTEXT("Heritage", "PlayMode", "PLAY >");
            Plate(Mode, Margin, 138, 230, Amber, true);
            if (Director->GetMode() == EHeritageFilmMode::Play && Director->IsOnMissingFrame())
            {
                const float X = Width * 0.3f, Y = Height * 0.3f, PW = Width * 0.4f;
                Plate(NSLOCTEXT("Heritage", "MissingPlate", "FRAME MISSING"), X, Y, PW, Amber, true);
                Rect(Backing, X, Y + 42, PW, 18);
                Rect(MissingColor, X + 4, Y + 46, (PW - 8) * (1.f - Director->GetGapProgress01()), 10);
            }
            if (Director->GetMode() == EHeritageFilmMode::Play && Director->GetDirection() > 0 && Director->IsLocked(Director->GetSceneIndex() + 1))
            {
                Plate(FText::Format(NSLOCTEXT("Heritage", "RestoreToSeeMore", "Restore frame {0} to see more."), FText::AsNumber(Director->GetFirstUnrestoredFrame() + 1)), Width * 0.3f, Height * 0.3f + 64, Width * 0.4f, Amber);
            }
            if (Director->GetMode() == EHeritageFilmMode::Pause)
            {
                const int32 Frame = Director->GetTargetFrame();
                if (Frame != INDEX_NONE)
                {
                    int32 Count = 0; const auto& Answer = Director->GetSegment().Scenes[Frame].Answer;
                    for (const auto& Fact : Director->GetFactsFor(Frame)) { if (Answer.HasTagExact(Fact.Fact)) { ++Count; } }
                    const bool bFrameImaged = Director->IsImaged(Frame);
                    const FText ImagedFormat = Director->IsLocked(Frame) ? NSLOCTEXT("Heritage", "RecordingLockedImaged", "FRAME {0} IMAGED - [N] to review") :
                        NSLOCTEXT("Heritage", "RecordingImaged", "FRAME {0} IMAGED - [N] to upload");
                    const FText Recording = bFrameImaged ? FText::Format(ImagedFormat, FText::AsNumber(Frame + 1)) :
                        FText::Format(NSLOCTEXT("Heritage", "RecordingProgress", "REC > FRAME {0}  {1}/{2} actions"), FText::AsNumber(Frame + 1), FText::AsNumber(Count), FText::AsNumber(Answer.Num()));
                    Plate(Recording, Width * 0.28f, 142, Width * 0.55f, bFrameImaged ? FLinearColor::Green : Amber, true);
                }
            }
            DrawFilmstrip(Margin, Height - 70, W, false);
            Plate(Director->GetCaption(), Width * 0.57f, Height - 144, Width * 0.4f);
            Plate(Director->GetAudioCaption(), Width * 0.57f, Height - 110, Width * 0.4f, Amber);
        }
        DrawControls();
        Rect(FLinearColor::White, Width / 2 - 2, Height / 2 - 2, 4, 4);
        Plate(Interactor->GetPromptText(), Margin, Height * 0.6f, W * 0.65f);
        if (const auto* Held = UHeritageObjectComponent::Find(Interactor->GetHeld()))
        {
            Plate(FText::Format(NSLOCTEXT("Heritage", "HeldName", "Holding: {0}"), Held->DisplayName), Margin, Height * 0.6f + 34, W * 0.65f);
        }
        if (Interactor->IsCameraMode())
        {
            for (int32 XS : {-1, 1})
            {
                for (int32 YS : {-1, 1})
                {
                    const float X = Width * (0.5f + XS * 0.3f), Y = Height * (0.5f + YS * 0.3f);
                    Rect(FLinearColor::White, XS > 0 ? X - 40 : X, Y, 40, 2);
                    Rect(FLinearColor::White, X, YS > 0 ? Y - 40 : Y, 2, 40);
                }
            }
            Rect(Backing, Width / 2 - 92, Height / 2 + 70, 184, 12);
            Rect(FLinearColor::White, Width / 2 - 90, Height / 2 + 72, 180 * Interactor->GetScanProgress01(), 8);
        }
        if (Interactor->GetFlashAlpha() > 0.f) { Rect(FLinearColor(1, 1, 1, Interactor->GetFlashAlpha()), 0, 0, Width, Height); }
        const int32 First = FMath::Max(0, Archive->ActionLog.Num() - ActionLines);
        for (int32 Index = First; Index < Archive->ActionLog.Num(); ++Index)
        {
            const auto& Record = Archive->ActionLog[Index];
            Plate(FText::Format(NSLOCTEXT("Heritage", "ActionLine", "{0} > {1}: {2}"), Record.Object, Record.Target, Record.Result), Margin, Height - 100 - (Archive->ActionLog.Num() - Index) * 30, W * 0.53f, FLinearColor(0.75f, 0.75f, 0.75f));
        }
        FLinearColor Toast = Amber;
        Toast.A = FMath::Clamp((Archive->ToastExpireTime - GetWorld()->GetTimeSeconds()) / FMath::Max(ToastFadeDuration, UE_SMALL_NUMBER), 0.f, 1.f);
        if (Toast.A > 0.f) { Plate(Archive->CurrentToast, Margin, Height - 240, W, Toast); }
    }
    Plate(bFilm ? NSLOCTEXT("Heritage", "FilmBadge", "FILM / Reel 1") : NSLOCTEXT("Heritage", "RealityBadge", "REALITY"), Margin, 24, 220, FLinearColor::White, true);
    Plate(Flow->GetObjectiveText(), Width * 0.23f, 24, Width * 0.74f, Amber, true);
    if (bScreen) { DrawArchive(); }
    else if (Flow->bSliceComplete) { Plate(NSLOCTEXT("Heritage", "SliceComplete", "SLICE COMPLETE"), Width * 0.3f, Height * 0.25f, Width * 0.4f, Amber, true); }
}
