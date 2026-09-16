#include "GLTextureKit.h"

static uint32 HashU(uint32 X) { X ^= X >> 16; X *= 0x7feb352dU; X ^= X >> 15; X *= 0x846ca68bU; X ^= X >> 16; return X; }
float FGLTextureKit::Hash(int32 X, int32 Y, int32 Seed) { return (HashU((uint32)X * 73856093U ^ (uint32)Y * 19349663U ^ (uint32)Seed * 83492791U) & 0xffffffU) / 16777215.f; }

static float Smooth(float T) { return T * T * (3.f - 2.f * T); }
float FGLTextureKit::ValueNoise(float X, float Y, int32 Period, int32 Seed)
{
	const int32 Xi = FMath::FloorToInt(X), Yi = FMath::FloorToInt(Y);
	const float Fx = Smooth(X - Xi), Fy = Smooth(Y - Yi);
	auto L = [&](int32 A, int32 B) { return Hash(((A % Period) + Period) % Period, ((B % Period) + Period) % Period, Seed); };
	const float A = L(Xi, Yi), B = L(Xi + 1, Yi), C = L(Xi, Yi + 1), D = L(Xi + 1, Yi + 1);
	return FMath::Lerp(FMath::Lerp(A, B, Fx), FMath::Lerp(C, D, Fx), Fy);
}
float FGLTextureKit::Fbm(float X, float Y, int32 Period, int32 Octaves, int32 Seed)
{
	float Sum = 0, Amp = 0.5f, Norm = 0; int32 P = Period; float Fx = X, Fy = Y;
	for (int32 i = 0; i < Octaves; ++i) { Sum += Amp * ValueNoise(Fx, Fy, P, Seed + i * 101); Norm += Amp; Amp *= 0.5f; Fx *= 2; Fy *= 2; P *= 2; }
	return Sum / Norm;
}

static void Put(FGLTex& T, int32 X, int32 Y, float R, float G, float B, float A = 1.f)
{
	uint8* P = &T.BGRA[(Y * T.W + X) * 4];
	P[0] = (uint8)FMath::Clamp(FMath::RoundToInt(B * 255.f), 0, 255); P[1] = (uint8)FMath::Clamp(FMath::RoundToInt(G * 255.f), 0, 255);
	P[2] = (uint8)FMath::Clamp(FMath::RoundToInt(R * 255.f), 0, 255); P[3] = (uint8)FMath::Clamp(FMath::RoundToInt(A * 255.f), 0, 255);
}
static FGLTex Blank(int32 S) { FGLTex T; T.W = T.H = S; T.BGRA.SetNumZeroed(S * S * 4); return T; }

// height field shared by Concrete and ConcreteNormal
static float ConcreteHeight(int32 X, int32 Y, int32 S)
{
	const float U = (float)X / S, V = (float)Y / S;
	const float H = FGLTextureKit::Fbm(U * 8.f, V * 8.f, 8, 5, 3);
	const int32 Panel = S / 4;
	const int32 Px = X % Panel, Py = Y % Panel;
	const float SeamX = FMath::Clamp(1.f - FMath::Abs(Px - 1.5f) / 2.5f, 0.f, 1.f), SeamY = FMath::Clamp(1.f - FMath::Abs(Py - 1.5f) / 2.5f, 0.f, 1.f);
	const float Seam = FMath::Max(SeamX, SeamY);
	const float Speck = (FGLTextureKit::Hash(X, Y, 77) - 0.5f) * 0.06f;
	return H * 0.7f - Seam * 0.45f + Speck;
}

FGLTex FGLTextureKit::Concrete(int32 S)
{
	FGLTex T = Blank(S);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const float H = ConcreteHeight(X, Y, S);
		const int32 Panel = S / 4; const int32 Px = X % Panel, Py = Y % Panel;
		const float Seam = FMath::Max(FMath::Clamp(1.f - FMath::Abs(Px - 1.5f) / 2.5f, 0.f, 1.f), FMath::Clamp(1.f - FMath::Abs(Py - 1.5f) / 2.5f, 0.f, 1.f));
		const float Stain = Fbm((float)X / S * 3.f, (float)Y / S * 3.f, 3, 3, 21);
		const float Albedo = FMath::Clamp(0.42f + 0.5f * (H + 0.2f) - Seam * 0.3f - (1.f - Stain) * 0.12f, 0.f, 1.f);
		const float AO = 1.f - Seam * 0.55f;
		const float Rough = FMath::Clamp(0.55f + 0.35f * (0.6f - H) + Seam * 0.15f, 0.f, 1.f);
		Put(T, X, Y, Albedo, AO, Rough);
	}
	T.bSRGB = false; return T;
}

FGLTex FGLTextureKit::ConcreteNormal(int32 S)
{
	FGLTex T = Blank(S); T.bSRGB = false; T.bNormal = true;
	const float Str = 6.f;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		auto Hh = [&](int32 Dx, int32 Dy) { return ConcreteHeight((X + Dx + S) % S, (Y + Dy + S) % S, S); };
		const float Gx = (Hh(1, 0) - Hh(-1, 0)) * Str, Gy = (Hh(0, 1) - Hh(0, -1)) * Str;
		FVector3f N(-Gx, -Gy, 1.f); N.Normalize();
		Put(T, X, Y, N.X * 0.5f + 0.5f, N.Y * 0.5f + 0.5f, N.Z * 0.5f + 0.5f);
	}
	return T;
}

FGLTex FGLTextureKit::Grime(int32 S)
{
	FGLTex T = Blank(S); T.bSRGB = false;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const float U = (float)X / S, V = (float)Y / S;
		const float Broad = Fbm(U * 3.f, V * 3.f, 3, 4, 5);
		const float Streak = Fbm(U * 24.f, V * 1.5f, 24, 3, 9);           // stretched vertically
		const float Drip = FMath::Pow(FMath::Clamp(1.f - V * 1.2f, 0.f, 1.f), 0.6f);
		const float G = FMath::Clamp(0.45f + 0.35f * Broad + 0.3f * (Streak - 0.5f) * Drip, 0.f, 1.f);
		Put(T, X, Y, G, G, G);
	}
	return T;
}

FGLTex FGLTextureKit::Windows(int32 S)
{
	FGLTex T = Blank(S); T.bSRGB = false;
	const int32 Cell = S / 8;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const int32 Cx = X / Cell, Cy = Y / Cell, Px = X % Cell, Py = Y % Cell;
		const bool On = Hash(Cx, Cy, 7) < 0.7f;
		const float Bright = 0.25f + 0.75f * Hash(Cx, Cy, 11);
		const bool Blind = Hash(Cx, Cy, 13) < 0.28f;
		const float Hue = Hash(Cx, Cy, 17) < 0.35f ? 1.f : 0.f;
		const int32 InX0 = Cell * 5 / 32, InX1 = Cell * 27 / 32, InY0 = Cell * 6 / 32, InY1 = Cell * 26 / 32;
		bool Inside = Px >= InX0 && Px < InX1 && Py >= InY0 && Py < InY1;
		if (Blind && Py < (InY0 + InY1) / 2) Inside = false;
		const float Mask = (On && Inside) ? 1.f : 0.f;
		const float Frame = (Px < 2 || Py < 2) ? 1.f : 0.f;
		const float Mullion = (Inside && (FMath::Abs(Px - Cell / 2) < 1)) ? 0.5f : 0.f;
		Put(T, X, Y, FMath::Max(0.f, Mask - Mullion), Bright, Frame, Hue);
	}
	return T;
}

FGLTex FGLTextureKit::Traces(int32 S)
{
	FGLTex T = Blank(S); T.bSRGB = false;
	const float C = S * 0.5f, R = S * 0.5f;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const float Px = X - C + 0.5f, Py = Y - C + 0.5f;
		const float HexD = FMath::Max(FMath::Abs(Px) * 0.8660254f + FMath::Abs(Py) * 0.5f, FMath::Abs(Py)) / R;   // 1 at the hexagon edge
		float Tr = 0.f;
		// concentric hexagon rings
		const float RingF = FMath::Frac(HexD * 4.5f);
		if (HexD > 0.12f && HexD < 0.92f && (RingF < 0.035f || RingF > 0.965f)) Tr = 1.f;
		// radial spokes
		const float Ang = FMath::Atan2(Py, Px);
		for (int32 k = 0; k < 12; ++k)
		{
			const float A = k * PI / 6.f; float Da = FMath::Abs(FMath::Fmod(Ang - A + 3 * PI, 2 * PI) - PI);
			const float Dist = FMath::Sqrt(Px * Px + Py * Py) * FMath::Sin(FMath::Min(Da, 1.f));
			if (Dist < 1.2f && HexD > 0.2f && HexD < 0.85f && (k % 2 == 0 || HexD > 0.5f)) Tr = FMath::Max(Tr, 0.8f);
		}
		// pads
		const int32 G = 26; const int32 Gx = X / G, Gy = Y / G;
		if (Hash(Gx, Gy, 31) > 0.86f && HexD < 0.85f) { const int32 Ox = X % G, Oy = Y % G; if (Ox > 8 && Ox < 18 && Oy > 8 && Oy < 18) Tr = FMath::Max(Tr, 0.9f); }
		// centre glyph
		if (HexD < 0.09f && HexD > 0.06f) Tr = 1.f;
		Put(T, X, Y, Tr, Tr, Tr);
	}
	return T;
}

TArray<FGLTextureKit::FEntry> FGLTextureKit::Library()
{
	return {
		{ TEXT("T_Concrete"), &Concrete, 512 }, { TEXT("T_ConcreteN"), &ConcreteNormal, 512 }, { TEXT("T_Grime"), &Grime, 256 },
		{ TEXT("T_Windows"), &Windows, 256 }, { TEXT("T_Traces"), &Traces, 512 },
	};
}
