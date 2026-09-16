#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GLHUD.generated.h"

class SGLHud;

// Hosts the Slate interface (SGLHud) in the game viewport.
UCLASS()
class AGLHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	TSharedPtr<SGLHud> Widget;
};
