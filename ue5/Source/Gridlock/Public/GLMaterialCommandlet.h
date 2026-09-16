#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GLMaterialCommandlet.generated.h"

// Generates every asset the client needs so the project stays reproducible from code:
//   /Game/Materials/M_Neon      lit, params Color / Emissive / EmissiveStrength / Metallic / Roughness / RimWidth / Fill; neon rim on every face
//   /Game/Materials/M_NeonInst  same rim, colour + strength from per-instance custom data 0..3 (instanced architecture)
//   /Game/Materials/M_GlowInst  uniform glow from custom data (fiber, packets)
//   /Game/Materials/M_Holo      unlit translucent, colour from custom data 0..2, opacity from 3 (presence discs)
//   /Game/Meshes/SM_*           the procedural kitbash library (FGLMeshKit)
// Run:  UnrealEditor-Cmd.exe Gridlock.uproject -run=GLAssets
UCLASS()
class UGLAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UGLAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
