#include "GLMeshKit.h"

FGLMeshKit::FGLMeshKit() : Attr(MD)
{
	Attr.Register();
	Attr.GetVertexInstanceUVs().SetNumChannels(1);
	Group = MD.CreatePolygonGroup();
	Attr.GetPolygonGroupMaterialSlotNames()[Group] = FName(TEXT("Default"));
}

void FGLMeshKit::Quad(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector3f& D, const FVector3f& Normal)
{
	// Unreal front faces wind so that cross(B-A, C-A) points against the outward normal.
	const FVector3f P[4] = { A, B, C, D };
	int32 Order[4] = { 0, 1, 2, 3 };
	if (FVector3f::DotProduct(FVector3f::CrossProduct(B - A, C - A), Normal) > 0.f) { Order[1] = 3; Order[3] = 1; }
	static const FVector2f UV[4] = { FVector2f(0, 0), FVector2f(1, 0), FVector2f(1, 1), FVector2f(0, 1) };
	TArray<FVertexInstanceID> VI;
	for (int32 i = 0; i < 4; ++i)
	{
		const FVertexID V = MD.CreateVertex(); Attr.GetVertexPositions()[V] = P[Order[i]];
		const FVertexInstanceID I = MD.CreateVertexInstance(V);
		Attr.GetVertexInstanceNormals()[I] = Normal.GetSafeNormal();
		Attr.GetVertexInstanceUVs().Set(I, 0, UV[i]);
		VI.Add(I);
	}
	MD.CreatePolygon(Group, VI);
}

void FGLMeshKit::Box(const FVector3f& C, const FVector3f& S, float YawDeg)
{
	const FVector3f H = S * 0.5f;
	const float Cs = FMath::Cos(FMath::DegreesToRadians(YawDeg)), Sn = FMath::Sin(FMath::DegreesToRadians(YawDeg));
	auto R = [&](float x, float y, float z) { return C + FVector3f(x * Cs - y * Sn, x * Sn + y * Cs, z); };
	auto N = [&](float x, float y, float z) { return FVector3f(x * Cs - y * Sn, x * Sn + y * Cs, z); };
	// +X
	Quad(R(H.X, -H.Y, -H.Z), R(H.X, H.Y, -H.Z), R(H.X, H.Y, H.Z), R(H.X, -H.Y, H.Z), N(1, 0, 0));
	Quad(R(-H.X, H.Y, -H.Z), R(-H.X, -H.Y, -H.Z), R(-H.X, -H.Y, H.Z), R(-H.X, H.Y, H.Z), N(-1, 0, 0));
	Quad(R(H.X, H.Y, -H.Z), R(-H.X, H.Y, -H.Z), R(-H.X, H.Y, H.Z), R(H.X, H.Y, H.Z), N(0, 1, 0));
	Quad(R(-H.X, -H.Y, -H.Z), R(H.X, -H.Y, -H.Z), R(H.X, -H.Y, H.Z), R(-H.X, -H.Y, H.Z), N(0, -1, 0));
	Quad(R(-H.X, -H.Y, H.Z), R(H.X, -H.Y, H.Z), R(H.X, H.Y, H.Z), R(-H.X, H.Y, H.Z), FVector3f(0, 0, 1));
	Quad(R(-H.X, H.Y, -H.Z), R(H.X, H.Y, -H.Z), R(H.X, -H.Y, -H.Z), R(-H.X, -H.Y, -H.Z), FVector3f(0, 0, -1));
}

void FGLMeshKit::Frustum(const FVector3f& Base, float R0, float R1, float Height, int32 Sides, float RotDeg)
{
	auto Ring = [&](float R, float Z, int32 i) { const float A = FMath::DegreesToRadians(RotDeg + 360.f * i / Sides); return Base + FVector3f(R * FMath::Cos(A), R * FMath::Sin(A), Z); };
	for (int32 i = 0; i < Sides; ++i)
	{
		const FVector3f A = Ring(R0, 0, i), B = Ring(R0, 0, i + 1), C = Ring(R1, Height, i + 1), D = Ring(R1, Height, i);
		FVector3f N = FVector3f::CrossProduct(B - A, D - A).GetSafeNormal();
		const FVector3f Out = ((A + B) * 0.5f - Base - FVector3f(0, 0, 0)); if (FVector3f::DotProduct(N, FVector3f(Out.X, Out.Y, 0)) < 0) N = -N;
		Quad(A, B, C, D, N);
	}
	if (R1 > 0.01f)
	{
		TArray<FVertexInstanceID> VI;
		for (int32 i = Sides - 1; i >= 0; --i)
		{
			const FVector3f P = Ring(R1, Height, i);
			const FVertexID V = MD.CreateVertex(); Attr.GetVertexPositions()[V] = P;
			const FVertexInstanceID I = MD.CreateVertexInstance(V);
			Attr.GetVertexInstanceNormals()[I] = FVector3f(0, 0, 1);
			const float A = FMath::DegreesToRadians(RotDeg + 360.f * i / Sides);
			Attr.GetVertexInstanceUVs().Set(I, 0, FVector2f(0.5f + 0.5f * FMath::Cos(A), 0.5f + 0.5f * FMath::Sin(A)));
			VI.Add(I);
		}
		MD.CreatePolygon(Group, VI);
	}
}
void FGLMeshKit::Prism(const FVector3f& Base, float Radius, float Height, int32 Sides, float RotDeg) { Frustum(Base, Radius, Radius, Height, Sides, RotDeg); }

void FGLMeshKit::CylinderX(float Length, float Radius, int32 Sides)
{
	auto Ring = [&](float X, int32 i) { const float A = 2.f * PI * i / Sides; return FVector3f(X, Radius * FMath::Cos(A), Radius * FMath::Sin(A)); };
	for (int32 i = 0; i < Sides; ++i)
	{
		const FVector3f A = Ring(-Length * 0.5f, i), B = Ring(-Length * 0.5f, i + 1), C = Ring(Length * 0.5f, i + 1), D = Ring(Length * 0.5f, i);
		const FVector3f Mid = (A + B) * 0.5f; Quad(A, B, C, D, FVector3f(0, Mid.Y, Mid.Z).GetSafeNormal());
	}
}

void FGLMeshKit::HexTile(float Radius, float Height, float Bevel)
{
	auto Ring = [&](float R, float Z, int32 i) { const float A = FMath::DegreesToRadians(30.f + 60.f * i); return FVector3f(R * FMath::Cos(A), R * FMath::Sin(A), Z); };
	// top cap
	TArray<FVertexInstanceID> VI;
	for (int32 i = 5; i >= 0; --i)
	{
		const FVertexID V = MD.CreateVertex(); Attr.GetVertexPositions()[V] = Ring(Radius - Bevel, Height, i);
		const FVertexInstanceID I = MD.CreateVertexInstance(V); Attr.GetVertexInstanceNormals()[I] = FVector3f(0, 0, 1);
		const float A = FMath::DegreesToRadians(30.f + 60.f * i);
		Attr.GetVertexInstanceUVs().Set(I, 0, FVector2f(0.5f + 0.5f * FMath::Cos(A), 0.5f + 0.5f * FMath::Sin(A)));
		VI.Add(I);
	}
	MD.CreatePolygon(Group, VI);
	for (int32 i = 0; i < 6; ++i)
	{
		// bevel
		{ const FVector3f A = Ring(Radius, Height - Bevel, i), B = Ring(Radius, Height - Bevel, i + 1), C = Ring(Radius - Bevel, Height, i + 1), D = Ring(Radius - Bevel, Height, i);
		  const FVector3f Mid = (A + B) * 0.5f; Quad(A, B, C, D, (FVector3f(Mid.X, Mid.Y, 0).GetSafeNormal() + FVector3f(0, 0, 1)).GetSafeNormal()); }
		// wall
		{ const FVector3f A = Ring(Radius, 0, i), B = Ring(Radius, 0, i + 1), C = Ring(Radius, Height - Bevel, i + 1), D = Ring(Radius, Height - Bevel, i);
		  const FVector3f Mid = (A + B) * 0.5f; Quad(A, B, C, D, FVector3f(Mid.X, Mid.Y, 0).GetSafeNormal()); }
	}
}

// ---------------------------------------------------------------- architecture
#define GL_KIT FGLMeshKit K; auto& k = K;
#define GL_DONE return MoveTemp(K.Mesh());
using V3 = FVector3f;

FMeshDescription FGLMeshKit::TowerA()
{
	GL_KIT
	k.Box(V3(0, 0, 4), V3(44, 44, 8)); k.Box(V3(0, 0, 63), V3(26, 26, 110));
	k.Box(V3(0, 0, 48), V3(32, 32, 3)); k.Box(V3(0, 0, 88), V3(32, 32, 3));
	k.Box(V3(0, 0, 138), V3(18, 18, 40)); k.Box(V3(0, 0, 165), V3(10, 10, 14)); k.Prism(V3(0, 0, 172), 2, 30, 6);
	k.Box(V3(16, -18, 12), V3(10, 8, 24)); k.Box(V3(-18, 14, 9), V3(8, 10, 18));
	GL_DONE
}
FMeshDescription FGLMeshKit::TowerB()
{
	GL_KIT
	k.Box(V3(0, 0, 3), V3(50, 40, 6));
	k.Box(V3(-12, -6, 48), V3(16, 22, 90)); k.Box(V3(10, 8, 68), V3(20, 16, 130));
	k.Box(V3(-1, 1, 62), V3(26, 6, 5)); k.Box(V3(-1, 1, 96), V3(26, 6, 5));
	k.Box(V3(-12, -6, 96), V3(10, 14, 6)); k.Box(V3(10, 8, 137), V3(12, 10, 8)); k.Prism(V3(10, 8, 141), 1.5f, 22, 6);
	GL_DONE
}
FMeshDescription FGLMeshKit::Slab()
{
	GL_KIT
	k.Box(V3(0, 0, 2), V3(52, 30, 4)); k.Box(V3(0, 0, 27), V3(44, 22, 46));
	for (int i = 0; i < 5; ++i) k.Box(V3(-16.f + 8.f * i, -13, 27), V3(2, 4, 46));
	k.Box(V3(8, 0, 54), V3(12, 10, 8)); k.Box(V3(-12, 4, 52), V3(6, 6, 4));
	GL_DONE
}
FMeshDescription FGLMeshKit::Arcology()
{
	GL_KIT
	k.Box(V3(0, 0, 20), V3(60, 60, 40)); k.Box(V3(0, 0, 58), V3(46, 46, 36)); k.Box(V3(0, 0, 91), V3(32, 32, 30));
	k.Frustum(V3(0, 0, 106), 14, 6, 14, 12); k.Prism(V3(0, 0, 120), 6, 4, 12);
	for (int i = 0; i < 4; ++i) { const float a = i * PI / 2; k.Box(V3(24 * FMath::Cos(a), 24 * FMath::Sin(a), 44), V3(6, 6, 8)); }
	GL_DONE
}
FMeshDescription FGLMeshKit::Factory()
{
	GL_KIT
	k.Box(V3(-8, 0, 11), V3(50, 34, 22)); k.Box(V3(-8, 0, 24), V3(50, 6, 4)); k.Box(V3(-8, -12, 23), V3(46, 4, 2)); k.Box(V3(-8, 12, 23), V3(46, 4, 2));
	k.Prism(V3(20, -10, 0), 8, 26, 12); k.Prism(V3(20, 10, 0), 8, 26, 12); k.Prism(V3(-26, 12, 0), 3, 48, 8);
	k.Box(V3(6, 0, 20), V3(30, 3, 3)); k.Box(V3(20, 0, 27), V3(3, 20, 3));
	GL_DONE
}
FMeshDescription FGLMeshKit::Dock()
{
	GL_KIT
	k.Box(V3(0, 0, 1), V3(64, 40, 2));
	k.Box(V3(-24, 0, 20), V3(4, 4, 40)); k.Box(V3(24, 0, 20), V3(4, 4, 40)); k.Box(V3(0, 0, 41), V3(60, 4, 4)); k.Box(V3(8, 0, 36), V3(6, 6, 6));
	k.Box(V3(-10, -14, 5.5f), V3(16, 7, 7)); k.Box(V3(-10, -14, 12.5f), V3(16, 7, 7)); k.Box(V3(10, 12, 5.5f), V3(16, 7, 7)); k.Box(V3(24, -12, 5.5f), V3(16, 7, 7), 20);
	GL_DONE
}
FMeshDescription FGLMeshKit::Sprawl()
{
	GL_KIT
	k.Box(V3(-18, -12, 8), V3(14, 14, 16)); k.Box(V3(0, -16, 6), V3(12, 10, 12)); k.Box(V3(16, -8, 10), V3(12, 14, 20), 10);
	k.Box(V3(-10, 12, 5), V3(16, 12, 10)); k.Box(V3(12, 14, 7), V3(10, 10, 14), -15); k.Box(V3(0, 0, 4), V3(8, 8, 8)); k.Prism(V3(-16, 2, 0), 1.5f, 22, 6);
	GL_DONE
}
FMeshDescription FGLMeshKit::Undercity()
{
	GL_KIT
	k.Box(V3(-14, -10, 2), V3(24, 24, 4)); k.Box(V3(12, 8, 2), V3(24, 24, 4)); k.Box(V3(-2, 16, 1.5f), V3(18, 14, 3));
	k.Box(V3(0, -4, 6), V3(44, 3, 3)); k.Box(V3(6, 0, 12), V3(3, 3, 24)); k.Prism(V3(14, -14, 0), 3, 14, 8); k.Box(V3(-20, 6, 3), V3(6, 6, 6));
	GL_DONE
}
FMeshDescription FGLMeshKit::Spire()
{
	GL_KIT
	k.Frustum(V3(0, 0, 0), 18, 6, 180, 6); k.Prism(V3(0, 0, 60), 20, 3, 6); k.Prism(V3(0, 0, 120), 16, 3, 6); k.Prism(V3(0, 0, 170), 10, 3, 6);
	k.Prism(V3(0, 0, 180), 2, 40, 6); k.Box(V3(0, 0, 3), V3(56, 56, 6));
	GL_DONE
}

// ---------------------------------------------------------------- props
FMeshDescription FGLMeshKit::Node()
{
	GL_KIT
	k.Box(V3(0, 0, 20), V3(24, 24, 40));
	k.Box(V3(14, 0, 22), V3(2, 26, 44)); k.Box(V3(-14, 0, 22), V3(2, 26, 44)); k.Box(V3(0, 14, 22), V3(26, 2, 44)); k.Box(V3(0, -14, 22), V3(26, 2, 44));
	k.Box(V3(0, 0, 47), V3(14, 14, 6)); k.Prism(V3(0, 0, 50), 1.5f, 16, 6);
	GL_DONE
}
FMeshDescription FGLMeshKit::Substation()
{
	GL_KIT
	k.Box(V3(0, 0, 3), V3(20, 20, 6)); k.Prism(V3(0, 0, 6), 8, 14, 8);
	k.Prism(V3(0, 0, 20), 10, 2, 8); k.Prism(V3(0, 0, 24), 8, 2, 8); k.Prism(V3(0, 0, 28), 6, 2, 8);
	k.Box(V3(7, 7, 10), V3(2, 2, 20)); k.Box(V3(-7, -7, 10), V3(2, 2, 20));
	GL_DONE
}
FMeshDescription FGLMeshKit::Repeater()
{
	GL_KIT
	k.Box(V3(0, 0, 1), V3(14, 14, 2)); k.Frustum(V3(0, 0, 0), 6, 2, 40, 4, 45); k.Frustum(V3(0, 0, 40), 1, 9, 5, 10);
	GL_DONE
}
FMeshDescription FGLMeshKit::Outpost()
{
	GL_KIT
	k.Box(V3(0, 0, 5), V3(28, 20, 10)); k.Prism(V3(0, 0, 10), 5, 6, 8); k.Box(V3(8, 0, 14), V3(12, 2, 2)); k.Box(V3(0, -13, 3), V3(32, 3, 6));
	GL_DONE
}
FMeshDescription FGLMeshKit::Array()
{
	GL_KIT
	k.Prism(V3(0, 0, 0), 2, 22, 6); k.Frustum(V3(0, 0, 22), 2, 12, 6, 12); k.Prism(V3(0, 0, 27), 13, 1, 12); k.Box(V3(0, 0, 1), V3(10, 10, 2));
	GL_DONE
}
FMeshDescription FGLMeshKit::Rack()
{
	GL_KIT
	k.Box(V3(0, 0, 40), V3(10, 10, 80)); for (int i = 1; i <= 6; ++i) k.Box(V3(0, 0, 12.f * i), V3(14, 14, 1.5f)); k.Prism(V3(0, 0, 80), 3, 12, 6);
	GL_DONE
}
FMeshDescription FGLMeshKit::Lab()
{
	GL_KIT
	k.Prism(V3(0, 0, 0), 10, 10, 12); k.Frustum(V3(0, 0, 10), 10, 4, 10, 12); k.Prism(V3(0, 0, 20), 4, 4, 12);
	GL_DONE
}
FMeshDescription FGLMeshKit::Honeypot() { GL_KIT k.Prism(V3(0, 0, 0), 5, 8, 6); k.Box(V3(0, 0, 10), V3(6, 6, 4)); GL_DONE }
FMeshDescription FGLMeshKit::Tap() { GL_KIT k.Frustum(V3(0, 0, 0), 1.5f, 4, 10, 6); k.Box(V3(0, 0, 11), V3(6, 2, 2)); GL_DONE }
FMeshDescription FGLMeshKit::Cutout() { GL_KIT k.Box(V3(0, 0, 1.5f), V3(20, 20, 3)); k.Box(V3(0, 0, 4), V3(6, 6, 2)); GL_DONE }
FMeshDescription FGLMeshKit::Fork() { GL_KIT k.Prism(V3(0, 0, 0), 3, 10, 6); k.Box(V3(-3, 0, 13), V3(2, 2, 6)); k.Box(V3(3, 0, 13), V3(2, 2, 6)); GL_DONE }
FMeshDescription FGLMeshKit::Cable() { GL_KIT k.CylinderX(100.f, 1.f, 8); GL_DONE }
FMeshDescription FGLMeshKit::Tile() { GL_KIT k.HexTile(93.f, 10.f, 3.f); GL_DONE }

TArray<FGLMeshKit::FEntry> FGLMeshKit::Library()
{
	return {
		{ TEXT("SM_TowerA"), &TowerA }, { TEXT("SM_TowerB"), &TowerB }, { TEXT("SM_Slab"), &Slab }, { TEXT("SM_Arcology"), &Arcology },
		{ TEXT("SM_Factory"), &Factory }, { TEXT("SM_Dock"), &Dock }, { TEXT("SM_Sprawl"), &Sprawl }, { TEXT("SM_Undercity"), &Undercity }, { TEXT("SM_Spire"), &Spire },
		{ TEXT("SM_Node"), &Node }, { TEXT("SM_Substation"), &Substation }, { TEXT("SM_Repeater"), &Repeater }, { TEXT("SM_Outpost"), &Outpost },
		{ TEXT("SM_Array"), &Array }, { TEXT("SM_Rack"), &Rack }, { TEXT("SM_Lab"), &Lab }, { TEXT("SM_Honeypot"), &Honeypot }, { TEXT("SM_Tap"), &Tap },
		{ TEXT("SM_Cutout"), &Cutout }, { TEXT("SM_Fork"), &Fork }, { TEXT("SM_Cable"), &Cable }, { TEXT("SM_Tile"), &Tile },
	};
}
