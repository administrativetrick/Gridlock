#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GLMaterialCommandlet.generated.h"

// Generates the game's materials as assets so the runtime never depends on hand-authored content:
//   /Game/Materials/M_Neon      lit, params Color / Emissive / EmissiveStrength / Metallic / Roughness / RimWidth; neon rim on every face
//   /Game/Materials/M_NeonInst  same rim, colour + strength from per-instance custom data 0..3 (instanced buildings, links, packets)
//   /Game/Materials/M_Holo      unlit translucent, colour from custom data 0..2, opacity from 3 (presence discs)
// Run:  UnrealEditor-Cmd.exe Gridlock.uproject -run=GLMaterial
UCLASS()
class UGLMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UGLMaterialCommandlet();
	virtual int32 Main(const FString& Params) override;
};
