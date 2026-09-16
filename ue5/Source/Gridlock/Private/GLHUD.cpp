#include "GLHUD.h"
#include "GLSlateHud.h"
#include "GLPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void AGLHUD::BeginPlay()
{
	Super::BeginPlay();
	AGLPlayerController* PC = Cast<AGLPlayerController>(GetOwningPlayerController());
	if (!PC || !GEngine || !GEngine->GameViewport) return;
	Widget = SNew(SGLHud).PC(PC);
	GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 5);
}

void AGLHUD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Widget.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	Widget.Reset();
	Super::EndPlay(Reason);
}
