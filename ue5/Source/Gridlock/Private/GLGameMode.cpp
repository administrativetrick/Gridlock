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
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
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

	// Neon-noir lighting: a dim cool key so the emissive network carries the frame, faint violet sky, dense low fog,
	// and a film-style post stack (bloom, fringe, grain, vignette, cool grade). No static lighting anywhere.
	if (ADirectionalLight* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 3000), FRotator(-52.f, 35.f, 0.f)))
	{
		Key->SetMobility(EComponentMobility::Movable);
		if (ULightComponent* LC = Key->GetLightComponent()) { LC->SetIntensity(1.1f); LC->SetLightColor(FLinearColor(0.55f, 0.65f, 1.0f)); LC->SetVolumetricScatteringIntensity(1.5f); }
	}
	if (ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector(0, 0, 3000), FRotator::ZeroRotator))
	{
		if (USkyLightComponent* SC = Sky->GetLightComponent()) { SC->SetMobility(EComponentMobility::Movable); SC->SetIntensity(0.25f); SC->SetLightColor(FLinearColor(0.3f, 0.25f, 0.6f)); SC->RecaptureSky(); }
	}
	if (AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50.f), FRotator::ZeroRotator))
	{
		if (UExponentialHeightFogComponent* FC = Fog->GetComponent())
		{
			FC->SetFogDensity(0.012f); FC->SetFogHeightFalloff(0.6f); FC->SetFogInscatteringColor(FLinearColor(0.02f, 0.03f, 0.09f));
			FC->SetStartDistance(600.f); FC->SetVolumetricFog(true); FC->SetVolumetricFogScatteringDistribution(0.4f); FC->SetVolumetricFogExtinctionScale(1.5f);
		}
	}
	if (APostProcessVolume* PP = GetWorld()->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator))
	{
		PP->bUnbound = true;
		FPostProcessSettings& P = PP->Settings;
		P.bOverride_BloomIntensity = true; P.BloomIntensity = 1.6f;
		P.bOverride_BloomThreshold = true; P.BloomThreshold = 0.5f;
		P.bOverride_VignetteIntensity = true; P.VignetteIntensity = 0.5f;
		P.bOverride_SceneFringeIntensity = true; P.SceneFringeIntensity = 0.45f;
		P.bOverride_FilmGrainIntensity = true; P.FilmGrainIntensity = 0.28f;
		P.bOverride_ColorSaturation = true; P.ColorSaturation = FVector4(1.12f, 1.12f, 1.12f, 1.f);
		P.bOverride_ColorContrast = true; P.ColorContrast = FVector4(1.18f, 1.18f, 1.18f, 1.f);
		P.bOverride_ColorGain = true; P.ColorGain = FVector4(0.9f, 0.95f, 1.12f, 1.f);
		P.bOverride_AutoExposureBias = true; P.AutoExposureBias = 0.4f;
		P.bOverride_AmbientOcclusionIntensity = true; P.AmbientOcclusionIntensity = 0.7f;
	}
	SetupDevFlags();
}

// Dev harness: -autoshot takes a screenshot once the scene has settled; -autoquit=N exits after N seconds.
void AGLGameMode::SetupDevFlags()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("autoshot")))
	{
		FTimerHandle H1;
		GetWorldTimerManager().SetTimer(H1, FTimerDelegate::CreateLambda([]() { FScreenshotRequest::RequestScreenshot(TEXT("gridlock_autoshot"), true, false); }), 12.f, false);
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
	if (FParse::Param(FCommandLine::Get(), TEXT("autoselect")))
	{
		FTimerHandle H5; TWeakObjectPtr<AGLGameMode> Self(this);
		GetWorldTimerManager().SetTimer(H5, FTimerDelegate::CreateLambda([Self]()
		{
			if (!Self.IsValid()) return;
			if (AGLPlayerController* PC = Cast<AGLPlayerController>(Self->GetWorld()->GetFirstPlayerController()))
			{
				PC->Sel = Self->Sim().S().synds[Self->Me()].crown; PC->bBoard = FParse::Param(FCommandLine::Get(), TEXT("autoboard"));
				if (Self->Map) Self->Map->Selected = PC->Sel;
				Self->MarkDirty();
			}
		}), 4.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("autoresearch")))
	{
		// flips only the panel flag, exactly like the T key, so the shot proves the panel populates without any other input
		FTimerHandle H7; TWeakObjectPtr<AGLGameMode> Self(this);
		GetWorldTimerManager().SetTimer(H7, FTimerDelegate::CreateLambda([Self]() { if (Self.IsValid()) if (AGLPlayerController* PC = Cast<AGLPlayerController>(Self->GetWorld()->GetFirstPlayerController())) PC->ToggleResearch(); }), 6.f, false);
	}
	float Zoom = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("autozoom="), Zoom) && Zoom > 0.f)
	{
		FTimerHandle H6; TWeakObjectPtr<AGLGameMode> Self(this);
		GetWorldTimerManager().SetTimer(H6, FTimerDelegate::CreateLambda([Self, Zoom]()
		{
			if (!Self.IsValid()) return;
			if (APlayerController* PC = Self->GetWorld()->GetFirstPlayerController()) if (AGLCameraPawn* P = Cast<AGLCameraPawn>(PC->GetPawn())) P->SetZoom(Zoom);
		}), 3.f, false);
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
	MarkDirty();
}
