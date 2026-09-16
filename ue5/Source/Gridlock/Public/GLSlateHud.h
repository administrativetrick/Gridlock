#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GLPlayerController.h"

class AGLGameMode;
class SVerticalBox;

// The wireframe / holographic interface: top bar with gauges, command toolbar with build and ops menus,
// dual-layer inspector, research and doctrine windows, the Board Room, and the log. Pure Slate, no UMG assets.
class SGLHud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGLHud) {}
		SLATE_ARGUMENT(AGLPlayerController*, PC)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	AGLPlayerController* PC = nullptr;
	int32 SeenVersion = -1; int32 SeenSel = -2;
	bool bBuildMenu = false; bool bOpsMenu = false;

	// styles (must outlive the widgets that reference them)
	FSlateRoundedBoxBrush PanelBrush { FLinearColor(0.015f, 0.025f, 0.05f, 0.88f), 3.f, FLinearColor(0.10f, 0.55f, 0.80f, 0.55f), 1.f };
	FSlateRoundedBoxBrush HeadBrush  { FLinearColor(0.0f, 0.35f, 0.5f, 0.35f), 2.f, FLinearColor(0.0f, 0.9f, 1.0f, 0.6f), 1.f };
	FSlateRoundedBoxBrush BtnN { FLinearColor(0.03f, 0.08f, 0.14f, 0.95f), 2.f, FLinearColor(0.0f, 0.7f, 0.9f, 0.7f), 1.f };
	FSlateRoundedBoxBrush BtnH { FLinearColor(0.0f, 0.35f, 0.5f, 0.95f), 2.f, FLinearColor(0.4f, 1.0f, 1.0f, 1.f), 1.f };
	FSlateRoundedBoxBrush BtnP { FLinearColor(0.0f, 0.6f, 0.8f, 1.0f), 2.f, FLinearColor(1.f, 1.f, 1.f, 1.f), 1.f };
	FSlateRoundedBoxBrush BarBg { FLinearColor(0.02f, 0.04f, 0.07f, 1.f), 2.f, FLinearColor(0.2f, 0.4f, 0.55f, 0.6f), 1.f };
	FSlateRoundedBoxBrush BarFill { FLinearColor::White, 2.f, FLinearColor::Transparent, 0.f };
	FSlateRoundedBoxBrush EndN { FLinearColor(0.02f, 0.10f, 0.16f, 0.96f), 56.f, FLinearColor(0.0f, 0.9f, 1.0f, 0.9f), 2.5f };
	FSlateRoundedBoxBrush EndH { FLinearColor(0.0f, 0.45f, 0.6f, 1.0f), 56.f, FLinearColor(0.6f, 1.0f, 1.0f, 1.f), 3.f };
	FSlateRoundedBoxBrush EndP { FLinearColor(0.0f, 0.75f, 0.95f, 1.0f), 56.f, FLinearColor(1.f, 1.f, 1.f, 1.f), 3.f };
	FButtonStyle BtnStyle;
	FButtonStyle EndStyle;
	FProgressBarStyle BarStyle;
	FSlateFontInfo F9, F10, F11, F12, F13, F18;

	TSharedPtr<SVerticalBox> InspectorBox, LogBox, BoardBox, ResearchBox, DoctrineBox, GuideBox;
	void RebuildGuide();
	EGLMode SeenMode = EGLMode::Select; int32 SeenFrom = -2; int32 SeenFlags = -1;

	AGLGameMode* GM() const;
	TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, float Pad = 10.f);
	TSharedRef<SWidget> Head(const FString& S);
	TSharedRef<SWidget> Txt(TAttribute<FText> T, const FSlateFontInfo& F, TAttribute<FSlateColor> C, bool Wrap = false);
	TSharedRef<SWidget> Txt(const FString& S, const FSlateFontInfo& F, const FLinearColor& C, bool Wrap = false);
	TSharedRef<SWidget> Btn(const FString& Label, TFunction<void()> Fn, const FSlateFontInfo* Font = nullptr, TAttribute<FSlateColor> Fg = TAttribute<FSlateColor>(), const FString& Tip = FString());
	TSharedRef<SWidget> ControlsPanel();
	TSharedRef<SWidget> Gauge(TAttribute<TOptional<float>> Pct, TAttribute<FSlateColor> Fill, float Height = 7.f);
	TSharedRef<SWidget> LabelledGauge(const FString& Label, TAttribute<TOptional<float>> Pct, TAttribute<FSlateColor> Fill, TAttribute<FText> Value);

	TSharedRef<SWidget> TopBar();
	TSharedRef<SWidget> Toolbar();
	TSharedRef<SWidget> HelpPanel();
	TSharedRef<SWidget> GameOverBanner();
	void RebuildInspector();
	void RebuildLog();
	void RebuildBoard();
	void RebuildResearch();
	void RebuildDoctrine();
};
