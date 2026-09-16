#pragma once
#include "CoreMinimal.h"

// Procedural, tileable textures for the city. All deterministic; pixels are BGRA8.
struct FGLTex { int32 W = 0, H = 0; TArray<uint8> BGRA; bool bSRGB = true; bool bNormal = false; };

class FGLTextureKit
{
public:
	static FGLTex Concrete(int32 Size);      // r albedo, g ambient occlusion, b roughness
	static FGLTex ConcreteNormal(int32 Size);// tangent-space normal of the concrete height
	static FGLTex Grime(int32 Size);         // r large-scale dirt and vertical streaks
	static FGLTex Windows(int32 Size);       // r window mask, g brightness, b frame, a hue selector
	static FGLTex Traces(int32 Size);        // r circuit traces for the hex tile top (UV-centred)

	struct FEntry { const TCHAR* Name; FGLTex (*Build)(int32); int32 Size; };
	static TArray<FEntry> Library();

	// noise helpers (tileable over Period lattice cells)
	static float Hash(int32 X, int32 Y, int32 Seed);
	static float ValueNoise(float X, float Y, int32 Period, int32 Seed);
	static float Fbm(float X, float Y, int32 Period, int32 Octaves, int32 Seed);
};
