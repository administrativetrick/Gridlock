#include "GLCameraPawn.h"
#include "Camera/CameraComponent.h"

AGLCameraPawn::AGLCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	Focus = CreateDefaultSubobject<USceneComponent>(TEXT("Focus"));
	RootComponent = Focus;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Focus);
	Camera->FieldOfView = 36.f;
	Camera->PostProcessBlendWeight = 0.f;
	ApplyZoom();
}

void AGLCameraPawn::SetZoom(float NewZoom)
{
	Zoom = FMath::Clamp(NewZoom, 700.f, 7000.f);
	ApplyZoom();
}

void AGLCameraPawn::ApplyZoom()
{
	// 62 degrees down, looking toward +Y so the hex rows read left to right
	const float Pitch = 54.f;
	const float Back = Zoom * FMath::Cos(FMath::DegreesToRadians(Pitch));
	const float Up = Zoom * FMath::Sin(FMath::DegreesToRadians(Pitch));
	Camera->SetRelativeLocation(FVector(0.f, -Back, Up));
	Camera->SetRelativeRotation(FRotator(-Pitch, 90.f, 0.f));
}
