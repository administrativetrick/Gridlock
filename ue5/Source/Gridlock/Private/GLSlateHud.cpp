#include "GLSlateHud.h"
#include "GLPlayerController.h"
#include "GLGameMode.h"
#include "GLMapActor.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"

static FString S2F(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }
static const FLinearColor kInk(0.82f, 0.92f, 1.0f), kDim(0.48f, 0.56f, 0.68f), kCyan(0.0f, 0.9f, 1.0f), kWarn(1.0f, 0.6f, 0.2f), kBad(1.0f, 0.3f, 0.3f), kGood(0.3f, 1.0f, 0.6f);
static FText T(const FString& S) { return FText::FromString(S); }
template <class TVal, class F> static TAttribute<TVal> Attr(F&& Fn) { return TAttribute<TVal>::Create(TAttribute<TVal>::FGetter::CreateLambda(Forward<F>(Fn))); }
static FText Tf(const TCHAR* Fmt, ...) { TCHAR Buf[512]; va_list Ap; va_start(Ap, Fmt); FCString::GetVarArgs(Buf, UE_ARRAY_COUNT(Buf), Fmt, Ap); va_end(Ap); return FText::FromString(Buf); }

AGLGameMode* SGLHud::GM() const { return PC ? PC->GM() : nullptr; }

void SGLHud::Construct(const FArguments& InArgs)
{
	PC = InArgs._PC;
	F9 = FCoreStyle::GetDefaultFontStyle("Regular", 9); F10 = FCoreStyle::GetDefaultFontStyle("Regular", 10); F11 = FCoreStyle::GetDefaultFontStyle("Regular", 11);
	F12 = FCoreStyle::GetDefaultFontStyle("Bold", 12); F13 = FCoreStyle::GetDefaultFontStyle("Bold", 13); F18 = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	BtnStyle.SetNormal(BtnN).SetHovered(BtnH).SetPressed(BtnP).SetNormalPadding(FMargin(8, 3)).SetPressedPadding(FMargin(8, 4, 8, 2));
	BarStyle.SetBackgroundImage(BarBg).SetFillImage(BarFill).SetEnableFillAnimation(false);
	EndStyle.SetNormal(EndN).SetHovered(EndH).SetPressed(EndP).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));

	const TAttribute<EVisibility> VisBoard = Attr<EVisibility>([this]() { return (PC && PC->bBoard) ? EVisibility::Visible : EVisibility::Collapsed; });
	const TAttribute<EVisibility> VisResearch = Attr<EVisibility>([this]() { return (PC && PC->bResearch) ? EVisibility::Visible : EVisibility::Collapsed; });
	const TAttribute<EVisibility> VisDoctrine = Attr<EVisibility>([this]() { return (PC && PC->bDoctrine) ? EVisibility::Visible : EVisibility::Collapsed; });
	const TAttribute<EVisibility> VisHelp = Attr<EVisibility>([this]() { return (PC && PC->bHelp) ? EVisibility::Visible : EVisibility::Collapsed; });

	ChildSlot
	[
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot().Anchors(FAnchors(0, 0, 1, 0)).Offset(FMargin(0, 0, 0, 74)).Alignment(FVector2D(0, 0)) [ TopBar() ]
		+ SConstraintCanvas::Slot().Anchors(FAnchors(0, 0)).Offset(FMargin(8, 82, 250, 0)).Alignment(FVector2D(0, 0)).AutoSize(true) [ Toolbar() ]
		+ SConstraintCanvas::Slot().Anchors(FAnchors(1, 0)).Offset(FMargin(-8, 82, 470, 0)).Alignment(FVector2D(1, 0)).AutoSize(true)
		[
			SNew(SBox).WidthOverride(470) [ Panel(SAssignNew(InspectorBox, SVerticalBox)) ]
		]
		+ SConstraintCanvas::Slot().Anchors(FAnchors(0, 1)).Offset(FMargin(8, -8, 640, 190)).Alignment(FVector2D(0, 1))
		[
			Panel(SNew(SScrollBox) + SScrollBox::Slot() [ SAssignNew(LogBox, SVerticalBox) ], 8.f)
		]
		+ SConstraintCanvas::Slot().Anchors(FAnchors(1, 1)).Offset(FMargin(-28, -28, 112, 112)).Alignment(FVector2D(1, 1))
		[
			SNew(SButton).ButtonStyle(&EndStyle).IsFocusable(false).ContentPadding(FMargin(0)).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.OnClicked_Lambda([this]() { if (PC) PC->OnEndCycle(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center) [ Txt(TEXT("END"), F13, kCyan) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center) [ Txt(TEXT("CYCLE"), F13, kCyan) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0) [ Txt(Attr<FText>([this]() { AGLGameMode* G = GM(); return G ? Tf(TEXT("%d"), G->Sim().S().cycle) : FText::GetEmpty(); }), F10, FSlateColor(kDim)) ]
			]
		]
		+ SConstraintCanvas::Slot().Anchors(FAnchors(0.5f, 0.5f)).Alignment(FVector2D(0.5f, 0.5f)).AutoSize(true)
		[
			SNew(SOverlay)
			+ SOverlay::Slot() [ SNew(SBox).Visibility(VisBoard).WidthOverride(760) [ Panel(SNew(SScrollBox) + SScrollBox::Slot() [ SAssignNew(BoardBox, SVerticalBox) ]) ] ]
			+ SOverlay::Slot() [ SNew(SBox).Visibility(VisResearch).WidthOverride(820).MaxDesiredHeight(640) [ Panel(SNew(SScrollBox) + SScrollBox::Slot() [ SAssignNew(ResearchBox, SVerticalBox) ]) ] ]
			+ SOverlay::Slot() [ SNew(SBox).Visibility(VisDoctrine).WidthOverride(700) [ Panel(SAssignNew(DoctrineBox, SVerticalBox)) ] ]
			+ SOverlay::Slot() [ SNew(SBox).Visibility(VisHelp).WidthOverride(720) [ HelpPanel() ] ]
			+ SOverlay::Slot() [ GameOverBanner() ]
		]
	];
}

void SGLHud::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	AGLGameMode* G = GM(); if (!G || !PC) return;
	if (G->Version != SeenVersion || PC->Sel != SeenSel)
	{
		SeenVersion = G->Version; SeenSel = PC->Sel;
		RebuildInspector(); RebuildLog(); RebuildBoard(); RebuildResearch(); RebuildDoctrine();
	}
}

// ---------------------------------------------------------------- primitives
TSharedRef<SWidget> SGLHud::Panel(TSharedRef<SWidget> Content, float Pad) { return SNew(SBorder).BorderImage(&PanelBrush).Padding(Pad) [ Content ]; }
TSharedRef<SWidget> SGLHud::Head(const FString& S) { return SNew(SBorder).BorderImage(&HeadBrush).Padding(FMargin(6, 2)) [ SNew(STextBlock).Text(T(S)).Font(F12).ColorAndOpacity(FSlateColor(kCyan)) ]; }
TSharedRef<SWidget> SGLHud::Txt(TAttribute<FText> Tx, const FSlateFontInfo& F, TAttribute<FSlateColor> C, bool Wrap) { return SNew(STextBlock).Text(Tx).Font(F).ColorAndOpacity(C).AutoWrapText(Wrap); }
TSharedRef<SWidget> SGLHud::Txt(const FString& S, const FSlateFontInfo& F, const FLinearColor& C, bool Wrap) { return SNew(STextBlock).Text(T(S)).Font(F).ColorAndOpacity(FSlateColor(C)).AutoWrapText(Wrap); }
TSharedRef<SWidget> SGLHud::Btn(const FString& Label, TFunction<void()> Fn, const FSlateFontInfo* Font, TAttribute<FSlateColor> Fg)
{
	if (!Fg.IsSet()) Fg = FSlateColor(kInk);
	return SNew(SButton).ButtonStyle(&BtnStyle).IsFocusable(false).OnClicked_Lambda([Fn]() { Fn(); return FReply::Handled(); })
		[ SNew(STextBlock).Text(T(Label)).Font(Font ? *Font : F10).ColorAndOpacity(Fg) ];
}
TSharedRef<SWidget> SGLHud::Gauge(TAttribute<TOptional<float>> Pct, TAttribute<FSlateColor> Fill, float Height)
{
	return SNew(SBox).HeightOverride(Height) [ SNew(SProgressBar).Style(&BarStyle).Percent(Pct).FillColorAndOpacity(Fill).BorderPadding(FVector2D(0, 0)) ];
}
TSharedRef<SWidget> SGLHud::LabelledGauge(const FString& Label, TAttribute<TOptional<float>> Pct, TAttribute<FSlateColor> Fill, TAttribute<FText> Value)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight() [ SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1) [ Txt(Label, F9, kDim) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Txt(Value, F9, FSlateColor(kInk)) ] ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 0) [ Gauge(Pct, Fill) ];
}

// ---------------------------------------------------------------- top bar
TSharedRef<SWidget> SGLHud::TopBar()
{
	auto Me = [this]() -> const gl::Syndicate* { AGLGameMode* G = GM(); return G ? &G->Sim().S().synds[G->Me()] : nullptr; };
	auto Stat = [this, Me](const FString& Label, TFunction<FString(const gl::Syndicate&)> Fn, FLinearColor C = kInk)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight() [ Txt(Label, F9, kDim) ]
			+ SVerticalBox::Slot().AutoHeight() [ Txt(Attr<FText>([Me, Fn]() { const gl::Syndicate* S = Me(); return S ? T(Fn(*S)) : FText::GetEmpty(); }), F12, FSlateColor(C)) ];
	};
	auto Pad = [](TSharedRef<SWidget> W) { return SNew(SBox).Padding(FMargin(10, 0)) [ W ]; };
	return SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(12, 6))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Txt(TEXT("GRIDLOCK"), F18, kCyan) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0) [ Txt(TEXT("SILICON SYNDICATE"), F11, kDim) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18, 0) [ Txt(Attr<FText>([this]() { AGLGameMode* G = GM(); return G ? Tf(TEXT("CYCLE %d / %d"), G->Sim().S().cycle, gl::K::GameEnd) : FText::GetEmpty(); }), F13, FSlateColor(kInk)) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18, 0) [ Txt(Attr<FText>([Me]() { const gl::Syndicate* S = Me(); return S ? T(S2F(S->name) + TEXT("  -  ") + UTF8_TO_TCHAR(gl::archDef(S->arch).name)) : FText::GetEmpty(); }), F12, Attr<FSlateColor>([Me]() { const gl::Syndicate* S = Me(); return FSlateColor(S ? AGLMapActor::SyndColor(S->id) : kInk); })) ]
			+ SHorizontalBox::Slot().FillWidth(1) [ SNew(SSpacer) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Txt(Attr<FText>([this]() { return PC ? T(PC->ModeText()) : FText::GetEmpty(); }), F11, FSlateColor(kCyan)) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14, 0, 0, 0) [ Txt(Attr<FText>([this]() { return PC ? T(PC->LastMsg) : FText::GetEmpty(); }), F11, FSlateColor(kWarn)) ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("CAPITAL"), [this](const gl::Syndicate& S) { return FString::Printf(TEXT("%.0f  (-%.0f)"), S.capital, GM()->Sim().UpkeepOf(S.id)); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("BANDWIDTH  produced / delivered / lost"), [](const gl::Syndicate& S) { return FString::Printf(TEXT("%.1f / %.1f / %.1f"), S.flow.produced, S.flow.delivered, S.flow.lost); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("OPS  used / need"), [](const gl::Syndicate& S) { return FString::Printf(TEXT("%.1f / %.1f"), S.flow.opsPool, S.flow.opsNeed); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("COMPUTE"), [](const gl::Syndicate& S) { return FString::Printf(TEXT("%.1f"), S.flow.compute); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("SOLD / BUFFER"), [](const gl::Syndicate& S) { return FString::Printf(TEXT("%.1f / %.1f"), S.flow.sold, S.buffer); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("SECTORS"), [this](const gl::Syndicate& S) { int N = 0; for (auto& H : GM()->Sim().S().hexes) if (H.owner == S.id) ++N; return FString::FromInt(N); })) ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("MANDATE / SLOTS"), [this](const gl::Syndicate& S) { return FString::Printf(TEXT("%d  /  %d of %d"), S.mandate, S.slotsUsed, GM()->Sim().Slots(S.id)); })) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0) [ SNew(SBox).WidthOverride(170) [ LabelledGauge(TEXT("EXPOSURE"),
				Attr<TOptional<float>>([Me]() { const gl::Syndicate* S = Me(); return TOptional<float>(S ? (float)S->exposure / 100.f : 0.f); }),
				Attr<FSlateColor>([Me]() { const gl::Syndicate* S = Me(); const float X = S ? (float)S->exposure : 0; return FSlateColor(X >= 60 ? kBad : X >= 40 ? kWarn : kCyan); }),
				Attr<FText>([Me]() { const gl::Syndicate* S = Me(); return S ? Tf(TEXT("%.0f%s"), S->exposure, S->exposure >= 60 ? TEXT("  AUDIT RISK") : S->exposure >= 40 ? TEXT("  noticed") : TEXT("")) : FText::GetEmpty(); })) ] ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0) [ SNew(SBox).WidthOverride(170) [ LabelledGauge(TEXT("VALUATION SHARE"),
				Attr<TOptional<float>>([Me]() { const gl::Syndicate* S = Me(); return TOptional<float>(S ? (float)S->share : 0.f); }),
				FSlateColor(kGood),
				Attr<FText>([Me]() { const gl::Syndicate* S = Me(); return S ? Tf(TEXT("%.1f%%"), S->share * 100.0) : FText::GetEmpty(); })) ] ]
			+ SHorizontalBox::Slot().AutoWidth() [ Pad(Stat(TEXT("MODEL M"), [](const gl::Syndicate& S) { return S.arch == gl::Arch::Hive ? FString::Printf(TEXT("%.0f"), S.M) : FString(TEXT("-")); }, kGood)) ]
			+ SHorizontalBox::Slot().FillWidth(1) [ SNew(SSpacer) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Txt(Attr<FText>([this]() {
				AGLGameMode* G = GM(); if (!G) return FText::GetEmpty(); const gl::GameState& S = G->Sim().S(); FString Out;
				for (auto& O : S.synds) if (O.alive && O.hasPhase) Out += FString::Printf(TEXT("%s%s: %s, %d left"), Out.IsEmpty() ? TEXT("") : TEXT("   "), *S2F(O.name), UTF8_TO_TCHAR(gl::pathName(O.phase.type)), O.phase.cyclesLeft);
				return T(Out); }), F11, FSlateColor(kWarn)) ]
		]
	];
}

// ---------------------------------------------------------------- toolbar
TSharedRef<SWidget> SGLHud::Toolbar()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	auto Add = [&](TSharedRef<SWidget> W) { Box->AddSlot().AutoHeight().Padding(0, 2) [ W ]; };
	Add(Btn(TEXT("Lay fiber  [L]"), [this]() { PC->SetLay(); }));
	Add(Btn(TEXT("Build  >"), [this]() { bBuildMenu = !bBuildMenu; bOpsMenu = false; }));
	TSharedRef<SVerticalBox> BuildMenu = SNew(SVerticalBox);
	BuildMenu->SetVisibility(Attr<EVisibility>([this]() { return bBuildMenu ? EVisibility::Visible : EVisibility::Collapsed; }));
	struct FB { const TCHAR* Label; int32 Kind; int32 Tier; };
	const FB Builds[] = {
		{ TEXT("Edge Node  T1  [1]"), (int32)gl::StructKind::Node, 1 }, { TEXT("Core Node  T2  [2]"), (int32)gl::StructKind::Node, 2 }, { TEXT("Hyperscale  T3  [3]"), (int32)gl::StructKind::Node, 3 },
		{ TEXT("Substation  [4]"), (int32)gl::StructKind::Substation, 0 }, { TEXT("Repeater  [5]"), (int32)gl::StructKind::Repeater, 0 }, { TEXT("Peering Rack  [6]"), (int32)gl::StructKind::Rack, 0 },
		{ TEXT("Security Outpost  [7]"), (int32)gl::StructKind::Outpost, 0 }, { TEXT("Surveillance Array  [8]"), (int32)gl::StructKind::Array, 0 }, { TEXT("Lab  [9]"), (int32)gl::StructKind::Lab, 0 },
		{ TEXT("Honeypot  [0]"), (int32)gl::StructKind::Honeypot, 0 }, { TEXT("Private Grid"), (int32)gl::StructKind::PrivateGrid, 0 }, { TEXT("Tap"), (int32)gl::StructKind::Tap, 0 }, { TEXT("Cutout"), (int32)gl::StructKind::Cutout, 0 }, { TEXT("Model Fork"), (int32)gl::StructKind::Fork, 0 } };
	for (const FB& B : Builds) { const int32 K = B.Kind, Tr = B.Tier; const double Cost = K == (int32)gl::StructKind::Node ? gl::nodeDef(Tr).cost : gl::structDef((gl::StructKind)K).cost; BuildMenu->AddSlot().AutoHeight().Padding(12, 1, 0, 1) [ Btn(FString::Printf(TEXT("%s   %.0f"), B.Label, Cost), [this, K, Tr]() { PC->SetBuild(K, Tr); bBuildMenu = false; }, &F9) ]; }
	Add(BuildMenu);
	Add(Btn(TEXT("Operations  >"), [this]() { bOpsMenu = !bOpsMenu; bBuildMenu = false; }));
	TSharedRef<SVerticalBox> OpsMenu = SNew(SVerticalBox);
	OpsMenu->SetVisibility(Attr<EVisibility>([this]() { return bOpsMenu ? EVisibility::Visible : EVisibility::Collapsed; }));
	for (int32 i = 0; i < (int32)gl::OpKind::COUNT; ++i)
	{
		const gl::OpDef& D = gl::opDef((gl::OpKind)i);
		if ((gl::OpKind)i == gl::OpKind::Repair || (gl::OpKind)i == gl::OpKind::Oracle || (gl::OpKind)i == gl::OpKind::Retrain) continue;
		OpsMenu->AddSlot().AutoHeight().Padding(12, 1, 0, 1) [ Btn(FString::Printf(TEXT("%s   %s%.0f  X%.0f"), UTF8_TO_TCHAR(D.name), D.bw > 0 ? TEXT("BW ") : TEXT("C "), D.bw > 0 ? D.bw : D.capital, D.x), [this, i]() { PC->SetOp(i); bOpsMenu = false; }, &F9) ];
	}
	Add(OpsMenu);
	Add(Btn(TEXT("Research  [T]"), [this]() { PC->ToggleResearch(); }));
	Add(Btn(TEXT("Doctrine  [M]"), [this]() { PC->ToggleDoctrine(); }));
	Add(Btn(TEXT("Board Room  [V]"), [this]() { PC->ToggleBoard(); }));
	Add(Btn(TEXT("Buy 10 BW  [B]"), [this]() { PC->BuyBW(); }));
	Add(Btn(TEXT("Recentre  [Home]"), [this]() { PC->Recenter(); }));
	Add(Btn(TEXT("Help  [F1]"), [this]() { PC->ToggleHelp(); }));
	Add(Btn(TEXT("Cancel  [Esc]"), [this]() { PC->OnCancel(); }, nullptr, FSlateColor(kDim)));
	return Panel(Box, 6.f);
}

TSharedRef<SWidget> SGLHud::HelpPanel()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	auto L = [&](const FString& S, const FLinearColor& C = kInk) { B->AddSlot().AutoHeight().Padding(0, 1) [ Txt(S, F10, C, true) ]; };
	B->AddSlot().AutoHeight().Padding(0, 0, 0, 6) [ Head(TEXT("HOW TO PLAY")) ];
	L(TEXT("Territory is what your network can feed. Every sector draws bandwidth each cycle; unfed sectors brown out and go neutral in four cycles."));
	L(TEXT("Lay fiber (L): click a hex in your network, then the destination. Conduit rights are bought along the route and the projected survival is shown."));
	L(TEXT("Loss grows with every hop from a node or repeater. Watch the packets fall off long links. Build an Edge Node (1) where a sector browns out. Nodes need Substations (4) in range."));
	L(TEXT("Cyber layer: Scan (C) to see, Intrusion (I) to push presence into a rival subnet, Harden (H) and Purge (P) your own. At 40 presence you can Siphon (K) or Jam (J); at 70, Root (R)."));
	L(TEXT("Every op adds Exposure. At 40 the Bureau notices you, at 60 it audits, at 80 it cuts your grid power."));
	L(TEXT("Peer at an Exchange (lay fiber into it, build a Rack (6)) to sell surplus bandwidth and reach distant targets."));
	L(TEXT("Research (T) is funded by compute allocated at nodes. Doctrine (M) needs Mandate from the Board every 12 cycles."));
	L(TEXT("Endings (V): Hostile Takeover, Singularity, Blackout, The Charter, or highest valuation at cycle 200."), kWarn);
	B->AddSlot().AutoHeight().Padding(0, 6, 0, 0) [ Head(TEXT("READING THE MAP")) ];
	L(TEXT("Colour = who holds the sector, brightness = integrity, dim = stale data, near-black = never seen. Tall buildings = rich sectors. The white spires are Exchanges."), kDim);
	L(TEXT("Fiber: thickness = capacity, colour shifts red with loss, orange flicker = sabotaged. Discs: cyan = your presence, other colours = rivals you can read, red = rooted. Grey hexagon = single point of failure."), kDim);
	L(TEXT("Camera: arrows / W A S E pan, wheel zooms, Home recentres. Right-click or Esc cancels any mode."), kDim);
	return Panel(B, 12.f);
}

TSharedRef<SWidget> SGLHud::GameOverBanner()
{
	return SNew(SBox).Visibility_Lambda([this]() { AGLGameMode* G = GM(); return (G && G->Sim().S().over) ? EVisibility::Visible : EVisibility::Collapsed; })
	[
		Panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight() [ Txt(Attr<FText>([this]() { AGLGameMode* G = GM(); return G ? Tf(TEXT("GAME OVER  -  CYCLE %d"), G->Sim().S().victory.cycle) : FText::GetEmpty(); }), F18, FSlateColor(kCyan)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6) [ Txt(Attr<FText>([this]() { AGLGameMode* G = GM(); if (!G) return FText::GetEmpty(); const gl::GameState& S = G->Sim().S(); return T(S2F(S.synds[S.victory.winner].name) + TEXT(" wins by ") + UTF8_TO_TCHAR(gl::pathName(S.victory.path))); }), F13,
				Attr<FSlateColor>([this]() { AGLGameMode* G = GM(); return FSlateColor(G ? AGLMapActor::SyndColor(G->Sim().S().victory.winner) : kInk); })) ], 20.f)
	];
}

// ---------------------------------------------------------------- inspector
void SGLHud::RebuildInspector()
{
	InspectorBox->ClearChildren();
	AGLGameMode* G = GM(); const gl::Game& Gm = G->Sim(); const gl::GameState& S = Gm.S(); const int32 Me = G->Me();
	auto Row = [&](TSharedRef<SWidget> W, float Top = 2.f) { InspectorBox->AddSlot().AutoHeight().Padding(0, Top, 0, 0) [ W ]; };
	if (PC->Sel < 0) { Row(Head(TEXT("INSPECTOR"))); Row(Txt(TEXT("Click a hex to read both layers."), F10, kDim)); return; }
	const gl::Hex& H = S.hexes[PC->Sel]; const bool Vis = Gm.CanSee(Me, H.id);
	Row(Head(FString::Printf(TEXT("#%d  %s%s"), H.id, UTF8_TO_TCHAR(gl::sectorDef(H.type).name), Vis ? TEXT("") : TEXT("   [STALE]"))));
	if (H.type == gl::Sector::Barrier) { Row(Txt(TEXT("Barrier: conduit only, +15 crossing premium into the next hex."), F10, kDim, true)); return; }
	const gl::SectorDef& D = gl::sectorDef(H.type);
	Row(Txt(FString::Printf(TEXT("yield %.0f   demand %.0f   conduit %.0f   pop %d   district %d   rights %s"), D.cap, D.demand, D.conduit, D.pop, H.district + 1, (H.rights.count(Me) || H.owner == Me) ? TEXT("yes") : TEXT("no")), F9, kDim, true));
	int32 OwnerSid = H.owner; double C = H.C; auto Seen = H.seen.find(Me);
	if (!Vis) { if (Seen == H.seen.end()) { Row(Txt(TEXT("Never seen. Scan it or move adjacent."), F10, kDim)); return; } OwnerSid = Seen->second.owner; C = Seen->second.C; Row(Txt(FString::Printf(TEXT("last seen cycle %d"), Seen->second.cycle), F9, kDim)); }
	Row(Head(TEXT("PHYSICAL")), 8.f);
	const FLinearColor OC = OwnerSid >= 0 ? AGLMapActor::SyndColor(OwnerSid) : kDim;
	Row(Txt(FString::Printf(TEXT("%s%s"), OwnerSid >= 0 ? *S2F(S.synds[OwnerSid].name) : TEXT("neutral"), (Vis && H.brownout) ? TEXT("   BROWNOUT") : TEXT("")), F11, OC));
	Row(LabelledGauge(TEXT("control integrity"), TOptional<float>((float)(C / 100.0)), FSlateColor(OC), T(FString::Printf(TEXT("%.0f"), C))));
	if (Vis && H.delivered.count(Me))
	{
		const double R = H.ratio.count(Me) ? H.ratio.at(Me) : 0.0;
		Row(LabelledGauge(TEXT("your delivery / demand"), TOptional<float>((float)FMath::Min(1.0, R)), FSlateColor(R >= 1 ? kGood : R >= 0.6 ? kWarn : kBad), T(FString::Printf(TEXT("%.1f / %.1f   r %.2f%s"), H.delivered.at(Me), H.demand.count(Me) ? H.demand.at(Me) : 0.0, R, H.dual.count(Me) ? TEXT("  dual-path") : TEXT("  SINGLE PATH")))));
		if (H.owner == Me)
		{
			auto Pri = H.priority.find(Me); const gl::Priority Cur = Pri == H.priority.end() ? gl::Priority::Normal : Pri->second;
			Row(SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Txt(TEXT("priority"), F9, kDim) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0) [ Btn(TEXT("Critical"), [this]() { PC->SetSelPriority(gl::Priority::Critical); }, &F9, FSlateColor(Cur == gl::Priority::Critical ? kCyan : kDim)) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0) [ Btn(TEXT("Normal"), [this]() { PC->SetSelPriority(gl::Priority::Normal); }, &F9, FSlateColor(Cur == gl::Priority::Normal ? kCyan : kDim)) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0) [ Btn(TEXT("Low"), [this]() { PC->SetSelPriority(gl::Priority::Low); }, &F9, FSlateColor(Cur == gl::Priority::Low ? kCyan : kDim)) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0) [ Btn(H.rowDenial ? TEXT("Right-of-way: DENIED") : TEXT("Deny right-of-way"), [this]() { PC->ToggleDeny(); }, &F9) ], 4.f);
		}
	}
	if (Vis) for (const gl::Structure* St : Gm.StructsIn(H.id))
	{
		if (!Gm.AssetVisible(Me, *St)) continue;
		FString L = FString::Printf(TEXT("%s%s  -  %s"), St->kind == gl::StructKind::Node ? UTF8_TO_TCHAR(gl::nodeDef(St->tier).name) : UTF8_TO_TCHAR(gl::structDef(St->kind).name), St->crown ? TEXT(" (CROWN)") : TEXT(""), *S2F(S.synds[St->sid].name));
		if (!St->built) L += FString::Printf(TEXT("   building %d"), St->buildLeft);
		if (St->darkUntil > S.cycle) L += TEXT("   DARK");
		if (St->orphanedAt >= 0) L += TEXT("   ORPHANED");
		Row(Txt(L, F10, AGLMapActor::SyndColor(St->sid)));
		if (St->kind == gl::StructKind::Node && St->built) Row(LabelledGauge(TEXT("power / output / utilisation"), TOptional<float>((float)St->util), FSlateColor(kCyan), T(FString::Printf(TEXT("%.0f%%  %.1f BW  %.0f%%"), St->powerFrac * 100, St->out, St->util * 100))));
	}
	if (Vis) for (const gl::Link* L : Gm.LinksThrough(H.id))
	{
		if (!Gm.LinkVisible(Me, *L)) continue;
		FString Tx = FString::Printf(TEXT("%s  %s  %d>%d%s"), UTF8_TO_TCHAR(gl::linkDef(L->type).name), *S2F(S.synds[L->sid].name), L->path.front(), L->path.back(), L->built ? TEXT("") : TEXT("  laying"));
		for (const gl::Segment& Sg : L->segs) if (Sg.a == H.id || Sg.b == H.id) Tx += FString::Printf(TEXT("\n   seg %d-%d  flow %.1f / %.0f  loss %.0f%%%s"), Sg.a, Sg.b, Sg.flow, Sg.cap, Sg.lastLoss * 100, Sg.sabotagedUntil > S.cycle ? TEXT("  SABOTAGED") : TEXT(""));
		Row(Txt(Tx, F9, AGLMapActor::SyndColor(L->sid)));
	}
	Row(Head(TEXT("CYBER")), 8.f);
	if (Vis) Row(LabelledGauge(TEXT("firewall"), TOptional<float>((float)(H.fw / 100.0)), FSlateColor(kWarn), T(FString::Printf(TEXT("%.0f"), H.fw))));
	const double MyP = H.P.count(Me) ? H.P.at(Me) : 0.0;
	if (H.owner != Me) Row(LabelledGauge(TEXT("your presence"), TOptional<float>((float)(MyP / 100.0)), FSlateColor(AGLMapActor::SyndColor(Me)), T(FString::Printf(TEXT("%.0f"), MyP))));
	if (Vis && Gm.PresenceVisible(Me, H.id)) { for (auto& KV : H.P) if (KV.first != Me) Row(LabelledGauge(KV.first == gl::RogueSid ? TEXT("ROGUE AI presence") : S2F(S.synds[KV.first].name) + TEXT(" presence"), TOptional<float>((float)(KV.second / 100.0)), FSlateColor(AGLMapActor::SyndColor(KV.first)), T(FString::Printf(TEXT("%.0f"), KV.second)))); }
	else if (Vis && H.owner == Me) { bool Contested = false; for (auto& KV : H.P) if (KV.first != Me && KV.second >= 40) Contested = true; Row(Txt(Contested ? TEXT("CONTESTED (+2 demand). Build an Array or Scan to read who.") : TEXT("Rival presence unknown - an Array or Scan reveals values."), F9, Contested ? kWarn : kDim, true)); }
	if (Vis && !H.roots.empty()) { FString R = TEXT("ROOTED by:"); for (int Rt : H.roots) R += TEXT(" ") + S2F(S.synds[Rt].name); Row(Txt(R, F11, kBad)); }
	if (Vis && H.daemons.count(Me)) Row(Txt(FString::Printf(TEXT("your daemons: %d"), H.daemons.at(Me)), F10, kGood));
}

void SGLHud::RebuildLog()
{
	LogBox->ClearChildren();
	AGLGameMode* G = GM();
	for (const std::string& L : G->Sim().RecentLog(G->Me(), 14)) LogBox->AddSlot().AutoHeight() [ Txt(S2F(L), F9, kDim, true) ];
}

// ---------------------------------------------------------------- board room
void SGLHud::RebuildBoard()
{
	if (!PC->bBoard) return;
	BoardBox->ClearChildren();
	AGLGameMode* G = GM(); const gl::Game& Gm = G->Sim(); const gl::GameState& S = Gm.S(); const int32 Me = G->Me();
	auto Row = [&](TSharedRef<SWidget> W, float Top = 3.f) { BoardBox->AddSlot().AutoHeight().Padding(0, Top, 0, 0) [ W ]; };
	Row(Head(TEXT("BOARD ROOM  -  VALUATIONS ARE PUBLIC")));
	for (auto& O : S.synds)
	{
		const FLinearColor C = AGLMapActor::SyndColor(O.id);
		Row(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.42f) [ Txt(FString::Printf(TEXT("%s  -  %s%s%s"), *S2F(O.name), UTF8_TO_TCHAR(gl::archDef(O.arch).name), O.shadowOf >= 0 ? TEXT("  (subsidiary)") : TEXT(""), O.alive ? TEXT("") : TEXT("  DELISTED")), F10, C) ]
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center).Padding(8, 0) [ Gauge(TOptional<float>((float)O.share), FSlateColor(C), 9.f) ]
			+ SHorizontalBox::Slot().FillWidth(0.23f) [ Txt(FString::Printf(TEXT("%5.1f%%   val %.0f   seats %d   X %.0f%s"), O.share * 100.0, O.valuation, Gm.SeatsOf(O.id), O.exposure, O.hasPhase ? *FString::Printf(TEXT("   %s"), UTF8_TO_TCHAR(gl::pathName(O.phase.type))) : TEXT("")), F9, kInk) ]);
	}
	Row(Head(TEXT("YOUR PATHS")), 10.f);
	for (const gl::PathProgress& P : Gm.Progress(Me))
	{
		Row(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.3f) [ Txt(UTF8_TO_TCHAR(gl::pathName(P.path)), F11, P.available ? kWarn : kInk) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center).Padding(8, 0) [ Gauge(TOptional<float>((float)(P.proximity / 100.0)), FSlateColor(P.available ? kWarn : kCyan), 9.f) ]
			+ SHorizontalBox::Slot().FillWidth(0.25f) [ Txt(FString::Printf(TEXT("%.0f%%  %s%s"), P.proximity, P.available ? TEXT("READY ") : TEXT(""), *S2F(P.status)), F9, kInk) ]);
		for (const std::string& U : P.unmet) Row(Txt(TEXT("      needs: ") + S2F(U), F9, kDim), 0.f);
	}
	Row(Head(TEXT("VERBS")), 10.f);
	Row(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Tender Offer"), [this]() { PC->Verb(0); }, &F9) ] + SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Buy 1% shares"), [this]() { PC->Verb(1); }, &F9) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Poison pill"), [this]() { PC->Verb(2); }, &F9) ] + SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Complaint"), [this]() { PC->Verb(8); }, &F9) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Kill switch"), [this]() { PC->Verb(7); }, &F9) ]);
	Row(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Train Seed"), [this]() { PC->Verb(3); }, &F9) ] + SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Launch"), [this]() { PC->Verb(4); }, &F9) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Blackout"), [this]() { PC->Verb(5); }, &F9) ] + SHorizontalBox::Slot().AutoWidth().Padding(2) [ Btn(TEXT("Charter motion"), [this]() { PC->Verb(6); }, &F9) ], 0.f);
	FString Dist = TEXT("Districts:"); for (int i = 0; i < (int)S.districts.size(); ++i) { const gl::District& D = S.districts[i]; Dist += FString::Printf(TEXT("   D%d %s"), i + 1, D.seat >= 0 ? (D.bought ? *(S2F(S.synds[D.seat].name) + TEXT(" (bought)")) : *S2F(S.synds[D.seat].name)) : TEXT("open")); }
	Row(Txt(Dist, F9, kDim, true), 8.f);
	for (auto& C : S.cartels) { FString Tx = TEXT("CARTEL against ") + S2F(S.synds[C.target].name) + TEXT(":"); for (int Mb : C.members) Tx += TEXT(" ") + S2F(S.synds[Mb].name); Row(Txt(Tx, F10, kBad)); }
	if (S.emergent.active) Row(Txt(TEXT("AN EMERGENT INTELLIGENCE IS LOOSE IN THE BACKBONE."), F11, kBad));
}

// ---------------------------------------------------------------- research / doctrine
void SGLHud::RebuildResearch()
{
	if (!PC->bResearch) return;
	ResearchBox->ClearChildren();
	AGLGameMode* G = GM(); const gl::Game& Gm = G->Sim(); const gl::Syndicate& Me = Gm.S().synds[G->Me()]; const gl::Mods M = Gm.ModsOf(Me.id);
	auto Row = [&](TSharedRef<SWidget> W, float Top = 2.f) { ResearchBox->AddSlot().AutoHeight().Padding(0, Top, 0, 0) [ W ]; };
	FString Q = TEXT("Queue:"); for (gl::Tech Tq : Me.researchQueue) Q += TEXT("  ") + FString(UTF8_TO_TCHAR(gl::techDef(Tq).name)); if (Me.researchQueue.empty()) Q += TEXT("  (empty)");
	Row(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center) [ Head(FString::Printf(TEXT("RESEARCH  -  %.1f compute/cycle, %d slot(s)"), Me.flow.compute, M.researchSlots)) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0) [ Btn(TEXT("Auto-queue"), [this]() { PC->AutoQueueTech(); }, &F9) ]
		+ SHorizontalBox::Slot().AutoWidth() [ Btn(TEXT("Clear queue"), [this]() { PC->ClearResearch(); }, &F9) ]);
	Row(Txt(Q, F9, kCyan, true));
	for (int b = 0; b < 4; ++b)
	{
		Row(Txt(UTF8_TO_TCHAR(gl::branchName((gl::Branch)b)), F12, kInk), 8.f);
		for (int i = 0; i < gl::TechCount; ++i)
		{
			const gl::TechDef& D = gl::techDef((gl::Tech)i); if ((int)D.branch != b) continue;
			const bool Done = Me.techs[i]; const gl::Result Can = Gm.CanResearch(Me.id, (gl::Tech)i);
			auto Pr = Me.researchProgress.find(i); const double Cost = D.cost * M.research[b];
			const FLinearColor C = Done ? kGood : Can ? kInk : kDim;
			TSharedRef<SHorizontalBox> R = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.30f).VAlign(VAlign_Center) [ Txt(FString::Printf(TEXT("T%d  %s"), D.tier, UTF8_TO_TCHAR(D.name)), F10, C) ]
				+ SHorizontalBox::Slot().FillWidth(0.14f).VAlign(VAlign_Center) [ Gauge(TOptional<float>(Done ? 1.f : Pr != Me.researchProgress.end() ? (float)(Pr->second / Cost) : 0.f), FSlateColor(Done ? kGood : kCyan), 6.f) ]
				+ SHorizontalBox::Slot().FillWidth(0.44f).VAlign(VAlign_Center).Padding(8, 0) [ Txt(FString::Printf(TEXT("%.0f   %s"), Cost, UTF8_TO_TCHAR(D.desc)), F9, kDim, true) ]
				+ SHorizontalBox::Slot().FillWidth(0.12f).HAlign(HAlign_Right) [ Done ? Txt(TEXT("done"), F9, kGood) : Can ? Btn(TEXT("Queue"), [this, i]() { PC->QueueTechAt(i); }, &F9) : Txt(TEXT("locked"), F9, kDim) ];
			Row(R);
		}
	}
}

void SGLHud::RebuildDoctrine()
{
	if (!PC->bDoctrine) return;
	DoctrineBox->ClearChildren();
	AGLGameMode* G = GM(); const gl::Game& Gm = G->Sim(); const gl::Syndicate& Me = Gm.S().synds[G->Me()];
	auto Row = [&](TSharedRef<SWidget> W, float Top = 3.f) { DoctrineBox->AddSlot().AutoHeight().Padding(0, Top, 0, 0) [ W ]; };
	Row(Head(FString::Printf(TEXT("DOCTRINE  -  %s  -  Mandate available: %d"), UTF8_TO_TCHAR(gl::archDef(Me.arch).name), Me.mandate)));
	Row(Txt(FString::Printf(TEXT("Board Review every %d cycles grants 1 Mandate, +1 if the KPI is met: %s"), gl::K::BoardReview, UTF8_TO_TCHAR(gl::archDef(Me.arch).kpi)), F9, kDim, true));
	for (int i = 0; i < gl::DoctrineCount; ++i)
	{
		const gl::DoctrineDef& D = gl::doctrineDef((gl::Doctrine)i); if (D.arch != Me.arch) continue;
		const bool Has = Me.doctrines[i]; const gl::Result Can = Gm.CanTakeDoctrine(Me.id, (gl::Doctrine)i);
		Row(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.34f).VAlign(VAlign_Center) [ Txt(FString::Printf(TEXT("Tier %d   %s"), D.tier, UTF8_TO_TCHAR(D.name)), F10, Has ? kGood : Can ? kInk : kDim) ]
			+ SHorizontalBox::Slot().FillWidth(0.52f).VAlign(VAlign_Center).Padding(8, 0) [ Txt(UTF8_TO_TCHAR(D.desc), F9, kDim, true) ]
			+ SHorizontalBox::Slot().FillWidth(0.14f).HAlign(HAlign_Right) [ Has ? Txt(TEXT("adopted"), F9, kGood) : Can ? Btn(TEXT("Adopt"), [this, i]() { PC->TakeDoctrineAt(i); }, &F9) : Txt(TEXT("locked"), F9, kDim) ]);
	}
}
