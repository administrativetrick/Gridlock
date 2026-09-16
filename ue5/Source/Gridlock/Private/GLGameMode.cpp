#include "GLGameMode.h"
#include "GLMapActor.h"
#include "GLCameraPawn.h"
#include "GLPlayerController.h"
#include "GLHUD.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

AGLGameMode::AGLGameMode()
{
	DefaultPawnClass = AGLCameraPawn::StaticClass();
	PlayerControllerClass = AGLPlayerController::StaticClass();
	HUDClass = AGLHUD::StaticClass();
}

void AGLGameMode::BeginPlay()
{
	Super::BeginPlay();

	gl::Config Cfg;
	FString Seed;
	if (FParse::Value(FCommandLine::Get(), TEXT("seed="), Seed) && !Seed.IsEmpty()) Cfg.seed = TCHAR_TO_UTF8(*Seed);
	FString Arch;
	if (FParse::Value(FCommandLine::Get(), TEXT("arch="), Arch))
	{
		Arch = Arch.ToLower();
		if (Arch == TEXT("ghost")) Cfg.player = gl::Arch::Ghost;
		else if (Arch == TEXT("hive")) Cfg.player = gl::Arch::Hive;
		else Cfg.player = gl::Arch::Hegemony;
	}
	Game = MakeUnique<gl::Game>(Cfg);

	Map = GetWorld()->SpawnActor<AGLMapActor>(AGLMapActor::StaticClass(), FTransform::Identity);
	Map->Init(this);

	// Neon-noir lighting: one cool key light, faint sky. No static lighting anywhere.
	if (ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 3000), FRotator(-58.f, 35.f, 0.f)))
	{
		Key->SetMobility(EComponentMobility::Movable);
		if (ULightComponent* LC = Key->GetLightComponent()) { LC->SetIntensity(2.5f); LC->SetLightColor(FLinearColor(0.72f, 0.78f, 1.0f)); }
	}
	if (ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector(0, 0, 3000), FRotator::ZeroRotator))
	{
		if (USkyLightComponent* SC = Sky->GetLightComponent()) { SC->SetMobility(EComponentMobility::Movable); SC->SetIntensity(0.35f); SC->SetLightColor(FLinearColor(0.35f, 0.3f, 0.6f)); SC->RecaptureSky(); }
	}
	SetupDevFlags();
}

// Dev harness: -autoshot takes a screenshot once the scene has settled; -autoquit=N exits after N seconds.
void AGLGameMode::SetupDevFlags()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("autoshot")))
	{
		FTimerHandle H1;
		GetWorldTimerManager().SetTimer(H1, FTimerDelegate::CreateLambda([]() { FScreenshotRequest::RequestScreenshot(TEXT("gridlock_autoshot"), false, false); }), 12.f, false);
	}
	int32 Cycles = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("autocycles="), Cycles) && Cycles > 0)
	{
		FTimerHandle H3;
		TWeakObjectPtr<AGLGameMode> Self(this);
		GetWorldTimerManager().SetTimer(H3, FTimerDelegate::CreateLambda([Self]() { if (Self.IsValid()) Self->EndCycle(); }), 1.0f, true, 2.0f);
		FTimerHandle H4;
		GetWorldTimerManager().SetTimer(H4, FTimerDelegate::CreateLambda([Self, H3]() mutable { if (Self.IsValid()) Self->GetWorldTimerManager().ClearTimer(H3); }), 2.0f + Cycles * 1.0f + 0.5f, false);
	}
	float Quit = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("autoquit="), Quit) && Quit > 0.f)
	{
		FTimerHandle H2;
		GetWorldTimerManager().SetTimer(H2, FTimerDelegate::CreateLambda([]() { FPlatformMisc::RequestExit(false); }), Quit, false);
	}
}

void AGLGameMode::EndCycle()
{
	if (!Game || Game->S().over) return;
	Game->EndCycle();
	bDirty = true;
}
