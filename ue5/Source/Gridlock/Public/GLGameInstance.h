#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GLGameInstance.generated.h"

// Owns the custom loading screen: a Slate widget shown by the movie player while the world loads and the
// city is generated, in place of the engine's default black frame. The startup splash bitmap (before the
// window exists) is Content/Splash/Splash.bmp; this covers everything after it.
UCLASS()
class UGLGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;

private:
	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* World);
};
