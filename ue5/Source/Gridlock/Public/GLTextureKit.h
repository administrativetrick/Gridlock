#pragma once
#include "CoreMinimal.h"

// Procedural, tileable PBR textures for the city. All deterministic; pixels are BGRA8.
struct FGLTex { int32 W = 0, H = 0; TArray<uint8> BGRA; bool bSRGB = true; bool bNormal = false; };

class FGLTextureKit
{
public:
	static FGLTex ConcreteAlbedo(int32 Size);  // sRGB colour: grey concrete, panel drift, stains, rust streaks
	static FGLTex ConcreteMasks(int32 Size);   // r ambient occlusion, g roughness, b height
	static FGLTex ConcreteNormal(int32 Size);  // tangent-space normal of the concrete height (seams, bolts, cracks)
	static FGLTex MetalMasks(int32 Size);      // r ambient occlusion, g roughness, b metal mask (painted stripes = 0)
	static FGLTex MetalNormal(int32 Size);     // plate bevels and rivet rows
	static FGLTex Grime(int32 Size);           // r dirt and drip streaks, g puddle mask
	static FGLTex Windows(int32 Size);         // r lit pane, g brightness, b frame/spandrel, a hue family
	static FGLTex Signage(int32 Size);         // r neon glyph bars, g palette index
	static FGLTex Traces(int32 Size);          // r circuit traces for the hex tile top (UV-centred)

	struct FEntry { const TCHAR* Name; FGLTex (*Build)(int32); int32 Size; };
	static TArray<FEntry> Library();

	// noise (tileable over Period lattice cells)
	static float Hash(int32 X, int32 Y, int32 Seed);
	static float Hash1(int32 X, int32 Seed) { return Hash(X, 7, Seed); }
	static float Perlin(float X, float Y, int32 Period, int32 Seed);            // -1..1
	static float Fbm(float X, float Y, int32 Period, int32 Octaves, int32 Seed); // 0..1
	static float Ridged(float X, float Y, int32 Period, int32 Octaves, int32 Seed);
	static void  Worley(float X, float Y, int32 Period, int32 Seed, float& F1, float& F2);
};
