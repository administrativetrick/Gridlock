#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GLCameraPawn.generated.h"

class UCameraComponent;

// Isometric strategy camera: the pawn is the focus point on the ground; the camera hangs above it at a fixed pitch.
UCLASS()
class AGLCameraPawn : public APawn
{
	GENERATED_BODY()
public:
	AGLCameraPawn();
	void SetZoom(float NewZoom);
	float GetZoom() const { return Zoom; }

private:
	UPROPERTY() TObjectPtr<USceneComponent> Focus;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	float Zoom = 2600.f;
	void ApplyZoom();
};
