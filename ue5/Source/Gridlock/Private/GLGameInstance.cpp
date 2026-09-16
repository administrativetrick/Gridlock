#include "GLGameInstance.h"
#include "MoviePlayer.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	const FLinearColor kCyan(0.f, 0.86f, 1.f);
	const FLinearColor kMagenta(1.f, 0.16f, 0.78f);
	const FLinearColor kInk(0.024f, 0.035f, 0.063f);

	// Full-screen loading card. It is ticked by the Slate loading thread, so every animated value is a
	// function of wall time rather than of game ticks.
	class SGLLoadingScreen : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGLLoadingScreen) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&)
		{
			Start = FPlatformTime::Seconds();
			const FSlateFontInfo Title = FCoreStyle::GetDefaultFontStyle("Bold", 72);
			const FSlateFontInfo Sub   = FCoreStyle::GetDefaultFontStyle("Regular", 22);
			const FSlateFontInfo Body  = FCoreStyle::GetDefaultFontStyle("Regular", 14);
			const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle("Regular", 12);

			const double StartCopy = Start;
			auto Elapsed = [StartCopy]() { return (float)(FPlatformTime::Seconds() - StartCopy); };

			ChildSlot
			[
				SNew(SOverlay)
				// backdrop
				+ SOverlay::Slot()
				[ SNew(SImage).Image(&Backdrop) ]
				// accent rails top and bottom
				+ SOverlay::Slot().VAlign(VAlign_Top)
				[ SNew(SBox).HeightOverride(3.f)[ SNew(SImage).Image(&CyanBrush) ] ]
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[ SNew(SBox).HeightOverride(3.f)[ SNew(SImage).Image(&MagentaBrush) ] ]
				// title block
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[ SNew(STextBlock).Text(FText::FromString(TEXT("GRIDLOCK"))).Font(Title).ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.98f, 1.f))) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
					[ SNew(SBox).WidthOverride(360.f).HeightOverride(1.f)[ SNew(SImage).Image(&CyanBrush) ] ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f, 0.f, 0.f)
					[ SNew(STextBlock).Text(FText::FromString(TEXT("S I L I C O N   S Y N D I C A T E"))).Font(Sub).ColorAndOpacity(FSlateColor(kMagenta)) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 26.f, 0.f, 0.f)
					[ SNew(STextBlock).Text(FText::FromString(TEXT("Own the wire. Own the city."))).Font(Body).ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.75f, 0.85f))) ]
					// progress rail: a packet sweeping the rail, eased, looping so a long load never looks stuck
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 40.f, 0.f, 0.f)
					[
						SNew(SBox).WidthOverride(420.f).HeightOverride(6.f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[ SNew(SImage).Image(&RailBrush) ]
							+ SOverlay::Slot().HAlign(HAlign_Left)
							[
								SNew(SBox).WidthOverride_Lambda([Elapsed]() {
									const float T = FMath::Fmod(Elapsed(), 2.4f) / 2.4f;
									return 60.f + 360.f * (1.f - FMath::Pow(1.f - T, 3.f));
								})
								[ SNew(SImage).Image(&CyanBrush) ]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)
					[
						SNew(STextBlock).Font(Small).ColorAndOpacity(FSlateColor(kCyan))
						.Text_Lambda([Elapsed]() {
							static const TCHAR* Steps[] = {
								TEXT("Surveying the city grid"), TEXT("Pouring concrete"), TEXT("Lighting the Exchanges"),
								TEXT("Seating the rival syndicates"), TEXT("Routing first light down the fiber"), TEXT("Waking the Board Room") };
							const int32 N = UE_ARRAY_COUNT(Steps);
							const int32 I = FMath::Min(N - 1, (int32)(Elapsed() / 0.9f));
							const int32 Dots = 1 + ((int32)(Elapsed() * 3.f) % 3);
							return FText::FromString(FString(Steps[I]) + FString::ChrN(Dots, TEXT('.')));
						})
					]
				]
				// footer
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24.f, 0.f, 0.f, 18.f)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("A dual-layer 4X. Route data, buy the council, breach the rivals."))).Font(Small).ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.5f, 0.6f))) ]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 24.f, 18.f)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("v0.1"))).Font(Small).ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.5f, 0.6f))) ]
			];
		}

	private:
		double Start = 0.0;
		FSlateColorBrush Backdrop { kInk };
		FSlateColorBrush CyanBrush { kCyan };
		FSlateColorBrush MagentaBrush { kMagenta };
		FSlateColorBrush RailBrush { FLinearColor(0.08f, 0.16f, 0.24f) };
	};
}

void UGLGameInstance::Init()
{
	Super::Init();
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGLGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGLGameInstance::EndLoadingScreen);
}

void UGLGameInstance::BeginLoadingScreen(const FString&)
{
	if (IsRunningDedicatedServer() || !IsMoviePlayerEnabled()) return;
	FLoadingScreenAttributes A;
	A.bAutoCompleteWhenLoadingCompletes = true;
	A.bMoviesAreSkippable = false;
	A.bWaitForManualStop = false;
	A.MinimumLoadingScreenDisplayTime = FParse::Param(FCommandLine::Get(), TEXT("autoshot")) ? 0.5f : 3.f;
	A.WidgetLoadingScreen = SNew(SGLLoadingScreen);
	GetMoviePlayer()->SetupLoadingScreen(A);
}

void UGLGameInstance::EndLoadingScreen(UWorld*)
{
	// nothing to tear down: the movie player stops itself once the world is in and the minimum time has passed
}
