#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GLHUD.generated.h"

class AGLGameMode;
class AGLPlayerController;
class UFont;

// Wireframe-style text HUD: top bar, dual-layer inspector, board room, help, log, game-over banner.
UCLASS()
class AGLHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

private:
	void Panel(float X, float Y, float W, float H);
	float Line(float X, float& Y, const FString& S, const FLinearColor& C, UFont* F, float Scale = 1.f);
	void DrawTopBar(AGLGameMode* GM, AGLPlayerController* PC, UFont* F);
	void DrawInspector(AGLGameMode* GM, AGLPlayerController* PC, UFont* F);
	void DrawLog(AGLGameMode* GM, UFont* F);
	void DrawHelp(UFont* F);
	void DrawBoard(AGLGameMode* GM, UFont* F);
};
