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
    UPROPERTY(EditAnywhere, Category="Heritage") float CutFlashDuration = 0.09f;
    UPROPERTY(EditAnywhere, Category="Heritage") float CutFlashOpacity = 0.22f;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ScratchCount = 4;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScratchOpacity = 0.08f;
    UPROPERTY(EditAnywhere, Category="Heritage") float CellHeight = 32.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScreenFooterBottom = 210.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float MapMaxHeight = 220.f;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector2D DamagePlateSize = FVector2D(100, 100);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor SelectionColor = FLinearColor(0.18f, 0.25f, 0.3f, 0.9f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ReadableColor = FLinearColor(0.25f, 0.32f, 0.38f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor LockedColor = FLinearColor(0.06f, 0.06f, 0.07f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor RestoredColor = FLinearColor(0.1f, 0.5f, 0.25f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor MissingColor = FLinearColor(0.65f, 0.07f, 0.06f);
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ScreenDimColor = FLinearColor(0, 0, 0, 0.9f);

    UPROPERTY(EditAnywhere, Category="Heritage") float Margin = 24.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float LineHeight = 24.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float LegendWidth = 340.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float CrosshairSize = 4.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ToastFadeDuration = 0.5f;
    UPROPERTY(EditAnywhere, Category="Heritage") int32 ActionLines = 5;
    UPROPERTY(EditAnywhere, Category="Heritage") FVector2D ViewfinderFraction = FVector2D(0.3f, 0.3f);
    UPROPERTY(EditAnywhere, Category="Heritage") float BracketLength = 40.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float BracketThickness = 2.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScanBarWidth = 180.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ScanBarHeight = 8.f;
    UPROPERTY(EditAnywhere, Category="Heritage") float ToastScreenY = 0.7f;
    UPROPERTY(EditAnywhere, Category="Heritage") float BannerScreenY = 0.25f;
    UPROPERTY(EditAnywhere, Category="Heritage") FLinearColor ToastColor = FLinearColor(1.f, 1.f, 0.6f);
private:
    // All screen navigation state lives here. Row -1 is the Archive tab bar.
    int32 SelectedRow = -1;
    int32 ArchiveTab = 0;
    bool bEditing = false;
    bool bKeyFrame = true;
    bool bConfirmUpload = false;
    FText ScreenMessage;
    void DrawFilmstrip(float X, float Y, float Width);
    void DrawMap();
    void DrawArchive();
    void DrawScreenRow(const FText& Text, int32 Row, float X, float Y, float Width);
    void Text(const FText& Value, float X, float Y, float Width, FLinearColor Color = FLinearColor::White);
    void WrappedText(const FText& Value, float X, float Y, float Width, int32 MaxLines);
};
