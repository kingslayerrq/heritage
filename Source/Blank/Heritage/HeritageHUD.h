#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HeritageHUD.generated.h"

UCLASS()
class BLANK_API AHeritageHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    void ResetNavigation();
    void Navigate(int32 X, int32 Y);
    void Confirm();
    void Back();
    void Click();
    int32 GetSelectedFrame() const { return SelectedFrame; }
    UPROPERTY(EditAnywhere, Category="Heritage") float CutFlashDuration = 0.09f;
    UPROPERTY(EditAnywhere, Category="Heritage") float CutFlashOpacity = 0.22f;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ScratchCount = 4;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScratchOpacity = 0.08f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ReadableColor = FLinearColor(0.25f, 0.32f, 0.38f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor RestoredColor = FLinearColor(0.1f, 0.5f, 0.25f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor MissingColor = FLinearColor(0.65f, 0.07f, 0.06f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ImagedColor = FLinearColor(0.65f, 0.4f, 0.06f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor LockedColor = FLinearColor(0.045f, 0.05f, 0.065f);
    UPROPERTY(EditAnywhere, Category="Heritage") float Margin = 24.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float LineHeight = 28.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ToastFadeDuration = 0.5f;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ActionLines = 3;
private:
    // Value-only hit targets rebuilt each draw, in logical Canvas coordinates.
    struct FButton
    {
        float X, Y, W, H;
        int32 Id;
        bool bEnabled;
        bool Contains(float PX, float PY) const { return PX >= X && PY >= Y && PX < X + W && PY < Y + H; }
    };
    TArray<FButton> Buttons;
    int32 FocusId = 1;
    int32 SelectedFrame = INDEX_NONE;
    int32 SelectedNote = 0;
    int32 NoteFirst = 0;
    bool bNotebook = false;
    bool bConfirmUpload = false;
    FText ScreenMessage;
    float Scale = 1.f;
    float Width = 1280.f;
    float Height = 800.f;
    void Activate(int32 Id);
    bool MousePosition(float& X, float& Y) const;
    void Rect(FLinearColor Color, float X, float Y, float W, float H);
    void Text(const FText& Value, float X, float Y, float W, FLinearColor Color = FLinearColor::White, bool bHeading = false);
    void Plate(const FText& Value, float X, float Y, float W, FLinearColor Color = FLinearColor::White, bool bHeading = false);
    void WrappedText(const FText& Value, float X, float Y, float W, int32 MaxLines);
    void Button(const FText& Value, int32 Id, float X, float Y, float W, float H, bool bEnabled = true, FLinearColor Color = FLinearColor(0.12f, 0.17f, 0.21f));
    void DrawFilmstrip(float X, float Y, float W, bool bClickable);
    void DrawArchive();
    void DrawControls();
};
