#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "gridlock/Game.h"
#include "GLPlayerController.generated.h"

class AGLGameMode;

UENUM()
enum class EGLMode : uint8 { Select, Lay, Build, Op };

// Input and order issuing. Every order goes through gl::Game validation; the UI shows the reply.
// The Slate HUD drives the same public methods the keyboard does.
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
	bool bHelp = false; bool bBoard = false; bool bResearch = false; bool bDoctrine = false; bool bGuide = true;
	void ToggleGuide() { bGuide = !bGuide; }
	FString LastMsg;
	FString ModeText() const;

	AGLGameMode* GM() const;
	void OnEndCycle();
	void OnCancel();
	void ToggleHelp() { bHelp = !bHelp; }
	void ToggleBoard() { bBoard = !bBoard; bResearch = bDoctrine = false; }
	void ToggleResearch() { bResearch = !bResearch; bBoard = bDoctrine = false; }
	void ToggleDoctrine() { bDoctrine = !bDoctrine; bBoard = bResearch = false; }
	void SetLay();
	void SetBuild(int32 Kind, int32 Tier);
	void SetOp(int32 Kind);
	void AutoQueueTech();
	void QueueTechAt(int32 TechIndex);
	void ClearResearch();
	void TakeDoctrineAt(int32 DoctrineIndex);
	void AutoDoctrine();
	void SetSelPriority(gl::Priority P);
	void BuyBW();
	void ToggleDeny();
	void Recenter();
	void FocusExchange();          // fly the camera to the Exchange nearest your Crown Node
	void Verb(int32 Which);
	void ZoomIn(); void ZoomOut();
	void Report(const gl::Result& R);

private:
	void Sync();
	void HandleHexClick(int32 Hex);
	void OnClick();
	void TogglePriorityKey();

	template <int32 K, int32 T> void BuildKey() { SetBuild(K, T); }
	template <int32 K> void OpKey() { SetOp(K); }
	template <int32 V> void VerbKey() { Verb(V); }
	template <int32 Dir, bool On> void Pan() { PanState[Dir] = On; }
	template <bool On> void Drag() { bDragging = On; }

	bool PanState[4] = { false, false, false, false };
	bool bDragging = false;        // middle mouse held: drag pans the map
};
