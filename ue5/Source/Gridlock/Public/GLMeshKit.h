#pragma once
#include "CoreMinimal.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

// Procedural kitbash: composes brutalist architecture and infrastructure props from primitives.
// Every face carries unit UVs so the rim material outlines every edge (the wireframe-on-concrete look).
class FGLMeshKit
{
public:
	FGLMeshKit();
	FMeshDescription& Mesh() { return MD; }

	// primitives (positions in centimetres, Z up)
	void Quad(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector3f& D, const FVector3f& Normal);
	void Box(const FVector3f& Center, const FVector3f& Size, float YawDeg = 0.f);
	void BoxMinMax(const FVector3f& Min, const FVector3f& Max) { Box((Min + Max) * 0.5f, Max - Min); }
	void Prism(const FVector3f& Base, float Radius, float Height, int32 Sides, float RotDeg = 0.f);
	void Frustum(const FVector3f& Base, float R0, float R1, float Height, int32 Sides, float RotDeg = 0.f);
	void CylinderX(float Length, float Radius, int32 Sides);       // axis along +X, centred
	void HexTile(float Radius, float Height, float Bevel);

	// architecture archetypes (footprint roughly within a 60 cm radius; heights in cm)
	static FMeshDescription TowerA(); static FMeshDescription TowerB(); static FMeshDescription Slab();
	static FMeshDescription Arcology(); static FMeshDescription Factory(); static FMeshDescription Dock();
	static FMeshDescription Sprawl(); static FMeshDescription Undercity(); static FMeshDescription Spire();
	// infrastructure props
	static FMeshDescription Node(); static FMeshDescription Substation(); static FMeshDescription Repeater();
	static FMeshDescription Outpost(); static FMeshDescription Array(); static FMeshDescription Rack();
	static FMeshDescription Lab(); static FMeshDescription Honeypot(); static FMeshDescription Tap();
	static FMeshDescription Cutout(); static FMeshDescription Fork(); static FMeshDescription Cable();
	static FMeshDescription Tile();

	struct FEntry { const TCHAR* Name; FMeshDescription (*Build)(); };
	static TArray<FEntry> Library();

private:
	FMeshDescription MD;
	FStaticMeshAttributes Attr;
	FPolygonGroupID Group;
};
