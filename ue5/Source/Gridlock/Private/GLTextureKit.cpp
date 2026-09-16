#include "GLTextureKit.h"

// ---------------------------------------------------------------- noise
static uint32 HashU(uint32 X) { X ^= X >> 16; X *= 0x7feb352dU; X ^= X >> 15; X *= 0x846ca68bU; X ^= X >> 16; return X; }
float FGLTextureKit::Hash(int32 X, int32 Y, int32 Seed) { return (HashU((uint32)X * 73856093U ^ (uint32)Y * 19349663U ^ (uint32)Seed * 83492791U) & 0xffffffU) / 16777215.f; }
static int32 Wrap(int32 A, int32 P) { return ((A % P) + P) % P; }
static float Quintic(float T) { return T * T * T * (T * (T * 6.f - 15.f) + 10.f); }

float FGLTextureKit::Perlin(float X, float Y, int32 Period, int32 Seed)
{
	const int32 Xi = FMath::FloorToInt(X), Yi = FMath::FloorToInt(Y);
	const float Fx = X - Xi, Fy = Y - Yi;
	auto Grad = [&](int32 A, int32 B, float Dx, float Dy) { const float Ang = Hash(Wrap(A, Period), Wrap(B, Period), Seed) * 6.2831853f; return FMath::Cos(Ang) * Dx + FMath::Sin(Ang) * Dy; };
	const float N00 = Grad(Xi, Yi, Fx, Fy), N10 = Grad(Xi + 1, Yi, Fx - 1, Fy), N01 = Grad(Xi, Yi + 1, Fx, Fy - 1), N11 = Grad(Xi + 1, Yi + 1, Fx - 1, Fy - 1);
	const float U = Quintic(Fx), V = Quintic(Fy);
	return FMath::Lerp(FMath::Lerp(N00, N10, U), FMath::Lerp(N01, N11, U), V) * 1.4142f;
}
float FGLTextureKit::Fbm(float X, float Y, int32 Period, int32 Octaves, int32 Seed)
{
	float Sum = 0, Amp = 0.5f, Norm = 0; int32 P = Period; float Fx = X, Fy = Y;
	for (int32 i = 0; i < Octaves; ++i) { Sum += Amp * Perlin(Fx, Fy, P, Seed + i * 101); Norm += Amp; Amp *= 0.5f; Fx *= 2; Fy *= 2; P *= 2; }
	return FMath::Clamp(0.5f + 0.5f * Sum / Norm, 0.f, 1.f);
}
float FGLTextureKit::Ridged(float X, float Y, int32 Period, int32 Octaves, int32 Seed)
{
	float Sum = 0, Amp = 0.5f, Norm = 0; int32 P = Period; float Fx = X, Fy = Y;
	for (int32 i = 0; i < Octaves; ++i) { const float R = 1.f - FMath::Abs(Perlin(Fx, Fy, P, Seed + i * 77)); Sum += Amp * R * R; Norm += Amp; Amp *= 0.55f; Fx *= 2; Fy *= 2; P *= 2; }
	return FMath::Clamp(Sum / Norm, 0.f, 1.f);
}
void FGLTextureKit::Worley(float X, float Y, int32 Period, int32 Seed, float& F1, float& F2)
{
	const int32 Xi = FMath::FloorToInt(X), Yi = FMath::FloorToInt(Y);
	F1 = F2 = 1e9f;
	for (int32 dy = -1; dy <= 1; ++dy) for (int32 dx = -1; dx <= 1; ++dx)
	{
		const int32 Cx = Xi + dx, Cy = Yi + dy;
		const float Px = Cx + Hash(Wrap(Cx, Period), Wrap(Cy, Period), Seed), Py = Cy + Hash(Wrap(Cx, Period), Wrap(Cy, Period), Seed + 1);
		const float D = FMath::Sqrt((Px - X) * (Px - X) + (Py - Y) * (Py - Y));
		if (D < F1) { F2 = F1; F1 = D; } else if (D < F2) F2 = D;
	}
}

// ---------------------------------------------------------------- helpers
static void Put(FGLTex& T, int32 X, int32 Y, float R, float G, float B, float A = 1.f)
{
	uint8* P = &T.BGRA[(Y * T.W + X) * 4];
	P[0] = (uint8)FMath::Clamp(FMath::RoundToInt(B * 255.f), 0, 255); P[1] = (uint8)FMath::Clamp(FMath::RoundToInt(G * 255.f), 0, 255);
	P[2] = (uint8)FMath::Clamp(FMath::RoundToInt(R * 255.f), 0, 255); P[3] = (uint8)FMath::Clamp(FMath::RoundToInt(A * 255.f), 0, 255);
}
static FGLTex Blank(int32 S, bool bSRGB) { FGLTex T; T.W = T.H = S; T.BGRA.SetNumZeroed(S * S * 4); T.bSRGB = bSRGB; return T; }
static void NormalFromHeight(FGLTex& T, TFunctionRef<float(int32, int32)> H, float Strength)
{
	const int32 S = T.W;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const float Gx = (H((X + 1) % S, Y) - H((X - 1 + S) % S, Y)) * Strength, Gy = (H(X, (Y + 1) % S) - H(X, (Y - 1 + S) % S)) * Strength;
		FVector3f N(-Gx, -Gy, 1.f); N.Normalize();
		Put(T, X, Y, N.X * 0.5f + 0.5f, N.Y * 0.5f + 0.5f, N.Z * 0.5f + 0.5f);
	}
	T.bSRGB = false; T.bNormal = true;
}

// ---------------------------------------------------------------- concrete
struct FConcreteSample { float Height, Seam, Bolt, Crack, Stain, Rust, Drift; };
static FConcreteSample SampleConcrete(int32 X, int32 Y, int32 S)
{
	using K = FGLTextureKit;
	const float U = (float)X / S, V = (float)Y / S;
	// domain-warped base surface
	const float Wx = K::Fbm(U * 4.f + 3.1f, V * 4.f, 4, 3, 41) - 0.5f, Wy = K::Fbm(U * 4.f, V * 4.f + 7.3f, 4, 3, 43) - 0.5f;
	const float Base = K::Fbm((U + Wx * 0.08f) * 12.f, (V + Wy * 0.08f) * 12.f, 12, 5, 3);
	const float Fine = K::Ridged(U * 48.f, V * 48.f, 48, 3, 9);
	// two-level panels: big grid, some panels split in half (per-panel hash), all with slight seam wobble
	const int32 Big = S / 4; const int32 Bx = X / Big, By = Y / Big;
	const bool SplitV = K::Hash(Bx, By, 51) < 0.35f, SplitH = K::Hash(Bx, By, 53) < 0.25f;
	int32 Px = X % Big, Py = Y % Big;
	const float Wob = (K::Fbm(U * 30.f, V * 30.f, 30, 2, 61) - 0.5f) * 1.6f;
	auto EdgeDist = [&](int32 P, int32 Len, bool Split) { float D = (float)FMath::Min(P, Len - P); if (Split) D = FMath::Min(D, FMath::Abs(P - Len / 2.f)); return D + Wob; };
	const float Dx = EdgeDist(Px, Big, SplitV), Dy = EdgeDist(Py, Big, SplitH);
	const float SeamW = 2.2f + K::Fbm(U * 60.f, V * 60.f, 60, 2, 63) * 1.4f;             // chipped edges
	const float Seam = FMath::Clamp(1.f - FMath::Min(Dx, Dy) / SeamW, 0.f, 1.f);
	// bolts near the corners of each big panel
	float Bolt = 0.f;
	for (int32 cy = 0; cy < 2; ++cy) for (int32 cx = 0; cx < 2; ++cx)
	{
		const float Cx = cx ? Big - 14.f : 14.f, Cy = cy ? Big - 14.f : 14.f;
		const float D = FMath::Sqrt((Px - Cx) * (Px - Cx) + (Py - Cy) * (Py - Cy));
		Bolt = FMath::Max(Bolt, FMath::Clamp(1.f - (D - 2.5f) / 2.f, 0.f, 1.f));
	}
	// cracks: cellular edges where the crack-density field allows
	float F1, F2; K::Worley(U * 10.f, V * 10.f, 10, 71, F1, F2);
	const float Density = K::Fbm(U * 5.f, V * 5.f, 5, 3, 73);
	const float Crack = FMath::Clamp(1.f - (F2 - F1) / 0.035f, 0.f, 1.f) * FMath::Clamp((Density - 0.55f) * 6.f, 0.f, 1.f);
	// stains & rust
	const float Stain = K::Fbm(U * 3.f, V * 3.f, 3, 4, 21);
	const float Streak = K::Fbm(U * 40.f, V * 2.f, 40, 3, 23);
	const float BoltRow = FMath::Clamp(1.f - FMath::Abs(Px - 14.f) / 5.f, 0.f, 1.f) + FMath::Clamp(1.f - FMath::Abs(Px - (Big - 14.f)) / 5.f, 0.f, 1.f);
	const float Below = FMath::Clamp((Py - 18.f) / 60.f, 0.f, 1.f) * (1.f - FMath::Clamp((Py - 18.f) / 140.f, 0.f, 1.f));
	const float Rust = FMath::Clamp(BoltRow * Below * Streak * 2.2f, 0.f, 1.f);
	const float Drift = K::Fbm(U * 2.f, V * 2.f, 2, 2, 27);
	FConcreteSample R;
	R.Height = FMath::Clamp(0.55f + 0.25f * (Base - 0.5f) * 2.f + 0.08f * Fine - Seam * 0.5f - Bolt * 0.45f - Crack * 0.3f, 0.f, 1.f);
	R.Seam = Seam; R.Bolt = Bolt; R.Crack = Crack; R.Stain = Stain; R.Rust = Rust; R.Drift = Drift;
	return R;
}

FGLTex FGLTextureKit::ConcreteAlbedo(int32 S)
{
	FGLTex T = Blank(S, true);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const FConcreteSample C = SampleConcrete(X, Y, S);
		FVector3f Col(0.56f, 0.555f, 0.535f);
		Col += FVector3f(0.05f, 0.02f, -0.04f) * (C.Drift - 0.5f) * 2.f;          // warm / cool drift between pours
		Col *= 0.72f + 0.45f * C.Height;
		Col *= FMath::Lerp(0.72f, 1.05f, C.Stain);
		Col = FMath::Lerp(Col, FVector3f(0.38f, 0.24f, 0.12f), C.Rust * 0.8f);
		Col *= 1.f - C.Seam * 0.55f - C.Crack * 0.45f - C.Bolt * 0.5f;
		Put(T, X, Y, Col.X, Col.Y, Col.Z);
	}
	return T;
}
FGLTex FGLTextureKit::ConcreteMasks(int32 S)
{
	FGLTex T = Blank(S, false);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const FConcreteSample C = SampleConcrete(X, Y, S);
		const float AO = FMath::Clamp(1.f - C.Seam * 0.6f - C.Crack * 0.5f - C.Bolt * 0.6f - (1.f - C.Height) * 0.15f, 0.f, 1.f);
		const float Rough = FMath::Clamp(0.58f + 0.3f * (0.6f - C.Height) + C.Seam * 0.15f + C.Crack * 0.1f + (1.f - C.Stain) * 0.12f - C.Rust * 0.1f, 0.f, 1.f);
		Put(T, X, Y, AO, Rough, C.Height);
	}
	return T;
}
FGLTex FGLTextureKit::ConcreteNormal(int32 S)
{
	FGLTex T = Blank(S, false);
	NormalFromHeight(T, [S](int32 X, int32 Y) { return SampleConcrete(X, Y, S).Height; }, 5.5f);
	return T;
}

// ---------------------------------------------------------------- metal
static float MetalHeight(int32 X, int32 Y, int32 S, float& OutRivet, float& OutBevel, float& OutScratch)
{
	using K = FGLTextureKit;
	const int32 Plate = S / 4; const int32 Px = X % Plate, Py = Y % Plate;
	const float Edge = (float)FMath::Min(FMath::Min(Px, Plate - Px), FMath::Min(Py, Plate - Py));
	OutBevel = FMath::Clamp(Edge / 6.f, 0.f, 1.f);                                     // 0 at the seam, 1 inside
	float Rivet = 0.f;
	const int32 Step = Plate / 4;
	for (int32 i = 0; i < 4; ++i)
	{
		const float Off = Step * 0.5f + Step * i;
		auto Dot = [&](float Cx, float Cy) { const float D = FMath::Sqrt((Px - Cx) * (Px - Cx) + (Py - Cy) * (Py - Cy)); return FMath::Clamp(1.f - D / 3.f, 0.f, 1.f); };
		Rivet = FMath::Max(Rivet, FMath::Max(FMath::Max(Dot(Off, 9.f), Dot(Off, Plate - 9.f)), FMath::Max(Dot(9.f, Off), Dot(Plate - 9.f, Off))));
	}
	OutRivet = Rivet;
	const float U = (float)X / S, V = (float)Y / S;
	float F1, F2; K::Worley(U * 6.f, V * 60.f, 6, 91, F1, F2);                          // anisotropic cells → scratches
	OutScratch = FMath::Clamp(1.f - (F2 - F1) / 0.03f, 0.f, 1.f) * FMath::Clamp((K::Fbm(U * 3.f, V * 3.f, 3, 2, 93) - 0.45f) * 5.f, 0.f, 1.f);
	const float Brush = K::Fbm(U * 90.f, V * 3.f, 90, 2, 95);
	return 0.5f + 0.35f * OutBevel * 0.3f + Rivet * 0.5f - OutScratch * 0.12f + (Brush - 0.5f) * 0.03f;
}
FGLTex FGLTextureKit::MetalMasks(int32 S)
{
	FGLTex T = Blank(S, false);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		float Rivet, Bevel, Scratch; MetalHeight(X, Y, S, Rivet, Bevel, Scratch);
		const float U = (float)X / S, V = (float)Y / S;
		const float Brush = Fbm(U * 90.f, V * 3.f, 90, 2, 95);
		const int32 Plate = S / 4; const bool Stripe = ((X / Plate) + (Y / Plate)) % 5 == 0 && (Y % Plate) < Plate / 5;   // painted hazard band on some plates
		const float AO = FMath::Clamp(0.55f + 0.45f * Bevel - Scratch * 0.1f, 0.f, 1.f);
		const float Rough = FMath::Clamp(0.32f + 0.25f * Brush + Scratch * 0.3f + Rivet * 0.1f + (Stripe ? 0.3f : 0.f), 0.f, 1.f);
		Put(T, X, Y, AO, Rough, Stripe ? 0.f : 1.f);
	}
	return T;
}
FGLTex FGLTextureKit::MetalNormal(int32 S)
{
	FGLTex T = Blank(S, false);
	NormalFromHeight(T, [S](int32 X, int32 Y) { float R, B, Sc; return MetalHeight(X, Y, S, R, B, Sc); }, 4.f);
	return T;
}

// ---------------------------------------------------------------- grime / puddles
FGLTex FGLTextureKit::Grime(int32 S)
{
	FGLTex T = Blank(S, false);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const float U = (float)X / S, V = (float)Y / S;
		const float Broad = Fbm(U * 3.f, V * 3.f, 3, 4, 5);
		const float Streak = Fbm(U * 28.f, V * 1.5f, 28, 3, 9);
		const float Drip = FMath::Pow(FMath::Clamp(1.f - V * 1.2f, 0.f, 1.f), 0.6f);
		const float G = FMath::Clamp(0.45f + 0.35f * Broad + 0.3f * (Streak - 0.5f) * Drip, 0.f, 1.f);
		float F1, F2; Worley(U * 5.f, V * 5.f, 5, 15, F1, F2);
		const float Puddle = FMath::Clamp((0.55f - F1) * 3.f + (Fbm(U * 8.f, V * 8.f, 8, 3, 17) - 0.5f) * 0.8f, 0.f, 1.f);
		Put(T, X, Y, G, Puddle, G);
	}
	return T;
}

// ---------------------------------------------------------------- windows
FGLTex FGLTextureKit::Windows(int32 S)
{
	FGLTex T = Blank(S, false);
	const int32 Cell = S / 8;
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const int32 Cx = X / Cell, Cy = Y / Cell, Px = X % Cell, Py = Y % Cell;
		const float Fx = (float)Px / Cell, Fy = (float)Py / Cell;
		const bool On = Hash(Cx, Cy, 7) < 0.66f;
		const float Bright = 0.3f + 0.7f * Hash(Cx, Cy, 11);
		const int32 Style = (int32)(Hash(Cx, Cy, 13) * 3.f);                          // 0 open, 1 blinds, 2 curtains
		const float HueSel = Hash(Cx, Cy, 17); const float Hue = HueSel < 0.55f ? 0.f : HueSel < 0.85f ? 0.5f : 1.f;
		const bool Pane = Fx > 0.09f && Fx < 0.91f && Fy > 0.12f && Fy < 0.78f;      // spandrel below 0.78
		const bool Mullion = FMath::Abs(Fx - 0.5f) < 0.018f;
		float Lit = 0.f;
		if (On && Pane && !Mullion)
		{
			Lit = FMath::Lerp(1.f, 0.55f, (Fy - 0.12f) / 0.66f);                            // interior light falls off toward the floor
			if (Style == 1 && (Py % 5) < 2) Lit *= 0.35f;                                   // blinds
			if (Style == 2) { const float E = FMath::Min(Fx - 0.09f, 0.91f - Fx); Lit *= FMath::Clamp(E / 0.12f, 0.15f, 1.f); }   // curtains at the edges
			Lit *= 0.85f + 0.3f * Hash(X / 3, Y / 3, 19);                                   // interior clutter
		}
		const bool Frame = (!Pane && Fy < 0.78f) || Mullion;
		const float Spandrel = Fy >= 0.78f ? 1.f : 0.f;
		Put(T, X, Y, Lit, Bright, FMath::Clamp((Frame ? 0.7f : 0.f) + Spandrel, 0.f, 1.f), Hue);
	}
	return T;
}

// ---------------------------------------------------------------- neon signage
FGLTex FGLTextureKit::Signage(int32 S)
{
	FGLTex T = Blank(S, false);
	const int32 GW = S / 8, GH = S / 16;                                                  // slot grid
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
	{
		const int32 Gx = X / GW, Gy = Y / GH;
		const bool Has = Hash(Gx, Gy, 31) < 0.28f;
		float R = 0.f; const float Pal = Hash(Gx, Gy, 37);
		if (Has)
		{
			const float W = 0.35f + 0.6f * Hash(Gx, Gy, 33), Hh = 0.25f + 0.5f * Hash(Gx, Gy, 35);
			const float Fx = (float)(X % GW) / GW, Fy = (float)(Y % GH) / GH;
			if (Fx > 0.05f && Fx < 0.05f + W && Fy > 0.15f && Fy < 0.15f + Hh)
			{
				// glyph-like vertical strokes
				const int32 Col = (X % GW) / 3; const bool Ink = Hash(Col, Gy, 39) < 0.55f;
				const bool Border = Fx < 0.08f || Fx > 0.02f + W || Fy < 0.2f || Fy > 0.1f + Hh;
				R = Border ? 1.f : (Ink ? 0.9f : 0.15f);
			}
			// underline bar
			if (Fx > 0.05f && Fx < 0.05f + W && Fy > 0.2f + Hh && Fy < 0.26f + Hh) R = 0.8f;
		}
		Put(T, X, Y, R, Pal, 0.f);
	}
	return T;
}

// ---------------------------------------------------------------- tile: datacenter floor plan as a circuit board
namespace
{
	struct FPlanCanvas
	{
		int32 S; TArray<float> V; TArray<float> O; float Order = 0.f;      // O = build order (0 first .. 1 last) of the element drawn at each pixel
		explicit FPlanCanvas(int32 InS) : S(InS) { V.SetNumZeroed(InS * InS); O.SetNumZeroed(InS * InS); }
		bool InHex(float X, float Y) const { const float Px = X - S * 0.5f, Py = Y - S * 0.5f; return FMath::Max(FMath::Abs(Px) * 0.8660254f + FMath::Abs(Py) * 0.5f, FMath::Abs(Py)) / (S * 0.5f) < 0.9f; }
		void Write(int32 x, int32 y, float Val) { const int32 i = y * S + x; if (Val > V[i]) { V[i] = Val; O[i] = Order; } }
		void Stamp(float X, float Y, float Radius, float Val)
		{
			const int32 X0 = FMath::Max(0, FMath::FloorToInt(X - Radius)), X1 = FMath::Min(S - 1, FMath::CeilToInt(X + Radius));
			const int32 Y0 = FMath::Max(0, FMath::FloorToInt(Y - Radius)), Y1 = FMath::Min(S - 1, FMath::CeilToInt(Y + Radius));
			for (int32 y = Y0; y <= Y1; ++y) for (int32 x = X0; x <= X1; ++x)
			{
				const float D = FMath::Sqrt((x + 0.5f - X) * (x + 0.5f - X) + (y + 0.5f - Y) * (y + 0.5f - Y));
				const float A = FMath::Clamp(Radius + 0.5f - D, 0.f, 1.f);
				if (A > 0 && InHex(x, y)) Write(x, y, Val * A);
			}
		}
		void Line(float X0, float Y0, float X1, float Y1, float Width, float Val)
		{
			const float L = FMath::Sqrt((X1 - X0) * (X1 - X0) + (Y1 - Y0) * (Y1 - Y0)); const int32 N = FMath::Max(1, FMath::CeilToInt(L / 0.5f));
			for (int32 i = 0; i <= N; ++i) { const float t = (float)i / N; Stamp(FMath::Lerp(X0, X1, t), FMath::Lerp(Y0, Y1, t), Width * 0.5f, Val); }
		}
		void Rect(float X0, float Y0, float X1, float Y1, float Val)
		{
			for (int32 y = FMath::Max(0, (int32)Y0); y < FMath::Min(S, (int32)Y1); ++y) for (int32 x = FMath::Max(0, (int32)X0); x < FMath::Min(S, (int32)X1); ++x) if (InHex(x, y)) Write(x, y, Val);
		}
		void Ring(float X, float Y, float R, float W, float Val) { const int32 N = FMath::Max(8, (int32)(R * 6)); for (int32 i = 0; i < N; ++i) { const float A = 2 * PI * i / N; Stamp(X + R * FMath::Cos(A), Y + R * FMath::Sin(A), W * 0.5f, Val); } }
		// PCB-style route: 45-degree diagonal until aligned, then straight to the target
		void Route(float X0, float Y0, float X1, float Y1, float Width, float Val)
		{
			const float Dx = X1 - X0, Dy = Y1 - Y0; const float D = FMath::Min(FMath::Abs(Dx), FMath::Abs(Dy));
			const float Mx = X0 + FMath::Sign(Dx) * D, My = Y0 + FMath::Sign(Dy) * D;
			Line(X0, Y0, Mx, My, Width, Val); Line(Mx, My, X1, Y1, Width, Val);
		}
	};
}

FGLTex FGLTextureKit::Traces(int32 S)
{
	FPlanCanvas Cv(S);
	const float C = S * 0.5f, K = S / 1024.f;
	// faint ground-plane via grid (always present)
	Cv.Order = 0.f;
	for (int32 y = 24; y < S; y += 32 * K) for (int32 x = 24; x < S; x += 32 * K) if (Cv.InHex(x, y)) Cv.Stamp(x, y, 1.2f * K, 0.14f);
	// core switch pad (first thing a sector gets)
	Cv.Order = 0.02f;
	const float Core = 92.f * K;
	Cv.Rect(C - Core * 0.5f, C - Core * 0.5f, C + Core * 0.5f, C + Core * 0.5f, 0.35f);
	for (int32 e = 0; e < 4; ++e) { const float o = Core * 0.5f - 1.5f * K; Cv.Line(C - o, C - o, C + o, C - o, 3.f * K, 1.f); Cv.Line(C + o, C - o, C + o, C + o, 3.f * K, 1.f); Cv.Line(C + o, C + o, C - o, C + o, 3.f * K, 1.f); Cv.Line(C - o, C + o, C - o, C - o, 3.f * K, 1.f); }
	for (int32 py = 0; py < 4; ++py) for (int32 px = 0; px < 4; ++px) Cv.Rect(C - 30 * K + px * 20 * K, C - 30 * K + py * 20 * K, C - 30 * K + px * 20 * K + 12 * K, C - 30 * K + py * 20 * K + 12 * K, 0.95f);
	// rack rows: hot/cold aisle pairs
	const float RackW = 44.f * K, RackH = 24.f * K, Pitch = 56.f * K, RowPitch = 76.f * K;
	int32 Row = 0;
	for (float Ry = C - 5.5f * RowPitch; Ry <= C + 5.5f * RowPitch + 1; Ry += RowPitch, ++Row)
	{
		const bool Upper = Ry < C;
		const float TrayY = Upper ? Ry - RackH * 0.5f - 10.f * K : Ry + RackH * 0.5f + 10.f * K;     // cable tray on the aisle side
		float TrayMin = 1e9f, TrayMax = -1e9f;
		const float Shift = (Row % 2) * Pitch * 0.5f;
		for (float Rx = C - 8 * Pitch + Shift; Rx <= C + 8 * Pitch; Rx += Pitch)
		{
			if (FMath::Abs(Rx - C) < Core * 0.5f + 34 * K && FMath::Abs(Ry - C) < Core * 0.5f + 40 * K) continue;   // keep the core zone clear
			if (!Cv.InHex(Rx - RackW * 0.55f, Ry - RackH) || !Cv.InHex(Rx + RackW * 0.55f, Ry + RackH)) continue;
			const int32 Cx = (int32)(Rx / Pitch), Cy = Row;
			if (Hash(Cx, Cy, 101) < 0.18f) continue;                                                      // empty slot
			Cv.Order = 0.15f + 0.85f * Hash(Cx, Cy, 113);                                                // racks fill in as the sector develops
			// rack: filled body, bright outline, a few unit dashes
			Cv.Rect(Rx - RackW * 0.5f, Ry - RackH * 0.5f, Rx + RackW * 0.5f, Ry + RackH * 0.5f, 0.28f);
			Cv.Line(Rx - RackW * 0.5f, Ry - RackH * 0.5f, Rx + RackW * 0.5f, Ry - RackH * 0.5f, 2.f * K, 0.9f);
			Cv.Line(Rx - RackW * 0.5f, Ry + RackH * 0.5f, Rx + RackW * 0.5f, Ry + RackH * 0.5f, 2.f * K, 0.9f);
			Cv.Line(Rx - RackW * 0.5f, Ry - RackH * 0.5f, Rx - RackW * 0.5f, Ry + RackH * 0.5f, 2.f * K, 0.9f);
			Cv.Line(Rx + RackW * 0.5f, Ry - RackH * 0.5f, Rx + RackW * 0.5f, Ry + RackH * 0.5f, 2.f * K, 0.9f);
			const int32 Units = 3 + (int32)(Hash(Cx, Cy, 103) * 4);
			for (int32 u = 0; u < Units; ++u) { const float ux = Rx - RackW * 0.4f + u * (RackW * 0.8f / Units); Cv.Line(ux, Ry - RackH * 0.25f, ux, Ry + RackH * 0.25f, 1.6f * K, 0.55f); }
			// stub to the tray + via
			const float StubY0 = Upper ? Ry - RackH * 0.5f : Ry + RackH * 0.5f;
			Cv.Line(Rx, StubY0, Rx, TrayY, 2.2f * K, 0.85f);
			Cv.Ring(Rx, TrayY, 3.2f * K, 1.8f * K, 1.f);
			// silkscreen label dashes on the far side
			const float LabY = Upper ? Ry + RackH * 0.5f + 7 * K : Ry - RackH * 0.5f - 7 * K;
			const int32 Dashes = 2 + (int32)(Hash(Cx, Cy, 107) * 3);
			for (int32 d = 0; d < Dashes; ++d) Cv.Line(Rx - RackW * 0.4f + d * 9 * K, LabY, Rx - RackW * 0.4f + d * 9 * K + 5 * K, LabY, 1.4f * K, 0.5f);
			TrayMin = FMath::Min(TrayMin, Rx); TrayMax = FMath::Max(TrayMax, Rx);
		}
		if (TrayMin > TrayMax) continue;
		// tray along the row, then fan into the core with a 45-degree route (trays come early)
		Cv.Order = 0.1f;
		Cv.Line(TrayMin, TrayY, TrayMax, TrayY, 3.f * K, 0.9f);
		const float Side = (Hash(Row, 3, 109) < 0.5f) ? -1.f : 1.f;
		const float EndX = Side < 0 ? TrayMin : TrayMax;
		const float TargetX = C + Side * (Core * 0.5f + 2 * K);
		const float TargetY = FMath::Clamp(Ry, C - Core * 0.4f, C + Core * 0.4f);
		if (FMath::Abs(Ry - C) > Core * 0.5f + 20 * K)
		{
			// leave the tray at a mid point, route to the core face
			const float LeaveX = FMath::Clamp(C + Side * (Core * 0.5f + 60 * K + FMath::Abs(Ry - C) * 0.35f), FMath::Min(TrayMin, TrayMax), FMath::Max(TrayMin, TrayMax));
			Cv.Ring(LeaveX, TrayY, 3.5f * K, 2.f * K, 1.f);
			Cv.Route(LeaveX, TrayY, TargetX, TargetY, 3.f * K, 0.9f);
			Cv.Stamp(TargetX, TargetY, 3.5f * K, 1.f);
		}
		else { Cv.Line(EndX, TrayY, TargetX, TrayY, 3.f * K, 0.9f); Cv.Stamp(TargetX, TrayY, 3.5f * K, 1.f); }
	}
	// a few decoupling-capacitor pairs and test points for texture (late additions)
	Cv.Order = 0.7f;
	for (int32 i = 0; i < 40; ++i)
	{
		const float x = Hash(i, 1, 131) * S, y = Hash(i, 2, 131) * S;
		if (!Cv.InHex(x, y) || FMath::Abs(x - C) < Core || FMath::Abs(y - C) < Core * 0.6f) continue;
		if (Cv.V[(int32)y * S + (int32)x] > 0.2f) continue;
		Cv.Stamp(x, y, 2.5f * K, 0.8f); Cv.Stamp(x + 7 * K, y, 2.5f * K, 0.8f); Cv.Line(x, y, x + 7 * K, y, 1.5f * K, 0.6f);
	}
	FGLTex T = Blank(S, false);
	for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X) { const float v = FMath::Clamp(Cv.V[Y * S + X], 0.f, 1.f); Put(T, X, Y, v, Cv.O[Y * S + X], v); }
	return T;
}

TArray<FGLTextureKit::FEntry> FGLTextureKit::Library()
{
	return {
		{ TEXT("T_ConcreteA"), &ConcreteAlbedo, 1024 }, { TEXT("T_ConcreteM"), &ConcreteMasks, 1024 }, { TEXT("T_ConcreteN"), &ConcreteNormal, 1024 },
		{ TEXT("T_MetalM"), &MetalMasks, 512 }, { TEXT("T_MetalN"), &MetalNormal, 512 },
		{ TEXT("T_Grime"), &Grime, 512 }, { TEXT("T_Windows"), &Windows, 512 }, { TEXT("T_Signage"), &Signage, 512 }, { TEXT("T_Traces"), &Traces, 1024 },
	};
}
