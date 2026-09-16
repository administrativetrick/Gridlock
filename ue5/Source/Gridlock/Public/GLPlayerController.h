#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "gridlock/Game.h"
#include "GLPlayerController.generated.h"

class AGLGameMode;

UENUM()
enum class EGLMode : uint8 { Select, Lay, Build, Op };

// Input and order issuing. Every order goes through gl::Game validation; the HUD shows the reply.
UCLASS()
class AGLPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AGLPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	EGLMode Mode = EGLMode::Select;
	int32 BuildKind = -1; int32 BuildTier = 1; int32 OpKind = -1;
	int32 Sel = -1; int32 FromHex = -1;
	bool bHelp = true; bool bBoard = false;
	FString LastMsg;
	FString ModeText() const;

private:
	AGLGameMode* GM() const;
	void Report(const gl::Result& R);
	void Sync();
	void HandleHexClick(int32 Hex);

	void OnClick();
	void OnEndCycle();
	void OnCancel();
	void ToggleHelp() { bHelp = !bHelp; }
	void ToggleBoard() { bBoard = !bBoard; }
	void SetLay();
	void SetBuild(int32 Kind, int32 Tier);
	void SetOp(int32 Kind);
	void QueueTech();
	void TakeDoctrine();
	void TogglePriority();
	void BuyBW();
	void ToggleDeny();
	void Recenter();
	void Verb(int32 Which);
	void ZoomIn(); void ZoomOut();

	template <int32 K, int32 T> void BuildKey() { SetBuild(K, T); }
	template <int32 K> void OpKey() { SetOp(K); }
	template <int32 V> void VerbKey() { Verb(V); }
	template <int32 Dir, bool On> void Pan() { PanState[Dir] = On; }

	bool PanState[4] = { false, false, false, false };
};
