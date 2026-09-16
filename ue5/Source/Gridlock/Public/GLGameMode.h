#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "gridlock/Game.h"
#include "GLGameMode.generated.h"

class AGLMapActor;

// Owns the simulation. Everything in the client reads gl::Game through here.
UCLASS()
class AGLGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AGLGameMode();
	virtual void BeginPlay() override;

	gl::Game& Sim() { return *Game; }
	const gl::Game& Sim() const { return *Game; }
	int32 Me() const { return 0; }
	void EndCycle();
	void MarkDirty() { bDirty = true; ++Version; }
	bool ConsumeDirty() { const bool D = bDirty; bDirty = false; return D; }
	int32 Version = 0;               // bumps on every state change; the UI compares against it

	UPROPERTY() TObjectPtr<AGLMapActor> Map = nullptr;

private:
	void SetupDevFlags();
	TUniquePtr<gl::Game> Game;
	bool bDirty = true;
};
