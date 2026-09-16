#include "GLMapActor.h"
#include "GLGameMode.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

static constexpr float kHexSize = 100.f;

FLinearColor AGLMapActor::SyndColor(int32 Sid)
{
	static const FLinearColor P[6] = {
		FLinearColor(0.00f, 0.90f, 1.00f), FLinearColor(1.00f, 0.17f, 0.84f), FLinearColor(1.00f, 0.69f, 0.00f),
		FLinearColor(0.22f, 1.00f, 0.53f), FLinearColor(0.62f, 0.31f, 0.87f), FLinearColor(1.00f, 0.36f, 0.36f) };
	if (Sid == gl::RogueSid) return FLinearColor(1.f, 1.f, 1.f);
	return (Sid >= 0 && Sid < 6) ? P[Sid] : FLinearColor(0.35f, 0.38f, 0.45f);
}

AGLMapActor::AGLMapActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AGLMapActor::HexWorld(int32 Hex) const
{
	const gl::Hex& H = GM->Sim().S().hexes[Hex];
	return FVector(kHexSize * FMath::Sqrt(3.f) * (H.q + H.r * 0.5f), kHexSize * 1.5f * H.r, 0.f);
}

float AGLMapActor::HexHeight(int32 Hex) const
{
	switch (GM->Sim().S().hexes[Hex].type)
	{
	case gl::Sector::Financial: return 70.f; case gl::Sector::Campus: return 52.f; case gl::Sector::Port: return 30.f;
	case gl::Sector::Industrial: return 36.f; case gl::Sector::Arcology: return 60.f; case gl::Sector::Sprawl: return 18.f;
	case gl::Sector::Undercity: return 10.f; case gl::Sector::Exchange: return 26.f; default: return 3.f;
	}
}

FVector AGLMapActor::MapCenter() const
{
	FVector C(0); int N = 0;
	for (const gl::Hex& H : GM->Sim().S().hexes) { C += HexWorld(H.id); ++N; }
	return N ? C / N : C;
}

void AGLMapActor::BuildHexMesh(UProceduralMeshComponent* Mesh, float Radius, float Height) const
{
	TArray<FVector> V; TArray<int32> T; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	// top: centre + ring, bottom ring
	V.Add(FVector(0, 0, Height)); Nrm.Add(FVector::UpVector); UV.Add(FVector2D(0.5f, 0.5f));
	for (int i = 0; i < 6; ++i) { const float A = FMath::DegreesToRadians(30.f + 60.f * i); V.Add(FVector(Radius * FMath::Cos(A), Radius * FMath::Sin(A), Height)); Nrm.Add(FVector::UpVector); UV.Add(FVector2D(0.5f + 0.5f * FMath::Cos(A), 0.5f + 0.5f * FMath::Sin(A))); }
	for (int i = 0; i < 6; ++i) { T.Add(0); T.Add(1 + (i + 1) % 6); T.Add(1 + i); }
	// sides: separate verts per face for hard normals
	for (int i = 0; i < 6; ++i)
	{
		const float A0 = FMath::DegreesToRadians(30.f + 60.f * i), A1 = FMath::DegreesToRadians(30.f + 60.f * (i + 1));
		const FVector P0(Radius * FMath::Cos(A0), Radius * FMath::Sin(A0), 0), P1(Radius * FMath::Cos(A1), Radius * FMath::Sin(A1), 0);
		const FVector N = ((P0 + P1) * 0.5f).GetSafeNormal();
		const int32 B = V.Num();
		V.Add(P0); V.Add(P1); V.Add(P1 + FVector(0, 0, Height)); V.Add(P0 + FVector(0, 0, Height));
		for (int k = 0; k < 4; ++k) { Nrm.Add(N); UV.Add(FVector2D(k & 1, k >> 1)); }
		T.Add(B); T.Add(B + 2); T.Add(B + 1); T.Add(B); T.Add(B + 3); T.Add(B + 2);
	}
	Col.Init(FLinearColor::White, V.Num()); Tan.Init(FProcMeshTangent(1, 0, 0), V.Num());
	Mesh->CreateMeshSection_LinearColor(0, V, T, Nrm, UV, Col, Tan, true);
}

void AGLMapActor::Init(AGLGameMode* InGM)
{
	GM = InGM;
	BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	checkf(BaseMat && Cube && Cylinder && Cone && Sphere, TEXT("Engine basic shapes missing"));

	const gl::GameState& S = GM->Sim().S();
	HexMeshes.SetNum(S.hexes.size()); HexMats.SetNum(S.hexes.size());
	for (const gl::Hex& H : S.hexes)
	{
		UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(this, *FString::Printf(TEXT("Hex%d"), H.id));
		M->SetupAttachment(RootComponent);
		M->RegisterComponent();
		M->bUseComplexAsSimpleCollision = true;
		M->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		M->SetCollisionResponseToAllChannels(ECR_Block);
		BuildHexMesh(M, kHexSize * 0.92f, HexHeight(H.id));
		M->SetRelativeLocation(HexWorld(H.id));
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
		M->SetMaterial(0, MID);
		HexMeshes[H.id] = M; HexMats[H.id] = MID; CompToHex.Add(M, H.id);
	}
	Refresh();
}

int32 AGLMapActor::HexFromComponent(const UPrimitiveComponent* Comp) const
{
	if (!Comp) return -1;
	if (const int32* Id = CompToHex.Find(Comp)) return *Id;
	return -1;
}

void AGLMapActor::Refresh()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me();
	for (const gl::Hex& H : S.hexes)
	{
		FLinearColor Base;
		switch (H.type)
		{
		case gl::Sector::Barrier: Base = FLinearColor(0.02f, 0.03f, 0.05f); break;
		case gl::Sector::Exchange: Base = FLinearColor(0.85f, 0.88f, 0.95f); break;
		case gl::Sector::Financial: Base = FLinearColor(0.16f, 0.15f, 0.10f); break;
		case gl::Sector::Campus: Base = FLinearColor(0.10f, 0.13f, 0.17f); break;
		case gl::Sector::Arcology: Base = FLinearColor(0.13f, 0.10f, 0.16f); break;
		case gl::Sector::Industrial: Base = FLinearColor(0.14f, 0.12f, 0.09f); break;
		case gl::Sector::Port: Base = FLinearColor(0.09f, 0.14f, 0.14f); break;
		default: Base = FLinearColor(0.09f, 0.10f, 0.12f); break;
		}
		const bool Vis = G.CanSee(Me, H.id);
		auto Seen = H.seen.find(Me);
		const bool Ever = Vis || Seen != H.seen.end();
		FLinearColor C = Base;
		if (H.type != gl::Sector::Barrier)
		{
			if (!Ever) C = FLinearColor(0.015f, 0.015f, 0.02f);
			else
			{
				const int32 OwnerSid = Vis ? H.owner : Seen->second.owner;
				const double Integ = Vis ? H.C : Seen->second.C;
				if (OwnerSid >= 0) C = FMath::Lerp(Base, SyndColor(OwnerSid) * 0.85f, 0.3f + 0.55f * (float)(Integ / 100.0));
				if (!Vis) C *= 0.35f;
				if (Vis && H.brownout && H.owner == Me) C = FMath::Lerp(C, FLinearColor(1.f, 0.2f, 0.1f), 0.5f * (0.5f + 0.5f * FMath::Sin(Time * 6.f)));
			}
		}
		if (H.id == Selected) C += FLinearColor(0.35f, 0.35f, 0.35f);
		if (H.id == FromHex) C += FLinearColor(0.4f, 0.35f, 0.0f);
		HexMats[H.id]->SetVectorParameterValue(TEXT("Color"), C);
	}
	RefreshStructures();
}

void AGLMapActor::RefreshStructures()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me();
	TSet<int32> Live;
	for (const gl::Structure& St : S.structs)
	{
		if (!St.alive || !G.AssetVisible(Me, St)) continue;
		Live.Add(St.id);
		TObjectPtr<UStaticMeshComponent>* Existing = StructComps.Find(St.id);
		UStaticMeshComponent* C = Existing ? Existing->Get() : nullptr;
		if (!C)
		{
			C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Struct%d"), St.id));
			C->SetupAttachment(RootComponent);
			C->RegisterComponent();
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			C->SetMaterial(0, MID);
			StructComps.Add(St.id, C); StructMats.Add(St.id, MID);
		}
		UStaticMesh* Mesh = Cube; FVector Scale(0.3f); FVector Offset(0); FLinearColor Col = SyndColor(St.sid);
		switch (St.kind)
		{
		case gl::StructKind::Node: Mesh = Cube; Scale = FVector(0.45f, 0.45f, 0.45f + 0.18f * St.tier); Col = SyndColor(St.sid) * 1.2f; if (St.crown) Scale *= 1.25f; break;
		case gl::StructKind::Substation: case gl::StructKind::PrivateGrid: Mesh = Cylinder; Scale = FVector(0.28f, 0.28f, 0.35f); Offset = FVector(38, 22, 0); Col = FLinearColor(1.f, 0.85f, 0.2f); break;
		case gl::StructKind::Repeater: Mesh = Cone; Scale = FVector(0.3f, 0.3f, 0.5f); Col = FLinearColor(0.7f, 0.9f, 1.f); break;
		case gl::StructKind::Outpost: Mesh = Cube; Scale = FVector(0.3f, 0.3f, 0.25f); Offset = FVector(-38, 22, 0); Col = FLinearColor(0.9f, 0.3f, 0.25f); break;
		case gl::StructKind::Array: Mesh = Sphere; Scale = FVector(0.22f); Offset = FVector(0, -42, 0); Col = FLinearColor(0.6f, 1.f, 0.9f); break;
		case gl::StructKind::Honeypot: Mesh = Sphere; Scale = FVector(0.18f); Offset = FVector(-30, -30, 0); Col = FLinearColor(1.f, 0.55f, 0.1f); break;
		case gl::StructKind::Lab: Mesh = Cylinder; Scale = FVector(0.22f, 0.22f, 0.5f); Offset = FVector(30, -30, 0); Col = FLinearColor(0.75f, 0.4f, 1.f); break;
		case gl::StructKind::Rack: Mesh = Cube; Scale = FVector(0.35f, 0.35f, 0.7f); Col = FLinearColor(1.f, 1.f, 1.f); break;
		case gl::StructKind::Tap: Mesh = Cone; Scale = FVector(0.18f, 0.18f, 0.3f); Offset = FVector(0, 40, 0); Col = FLinearColor(0.8f, 0.1f, 0.6f); break;
		case gl::StructKind::Cutout: Mesh = Cube; Scale = FVector(0.25f, 0.25f, 0.1f); Col = FLinearColor(0.3f, 0.3f, 0.35f); break;
		case gl::StructKind::Fork: Mesh = Sphere; Scale = FVector(0.2f); Offset = FVector(38, -22, 0); Col = FLinearColor(0.3f, 1.f, 0.6f); break;
		default: break;
		}
		if (!St.built) Col *= 0.35f;
		if (St.darkUntil > S.cycle) Col = FLinearColor(0.15f, 0.15f, 0.18f);
		C->SetStaticMesh(Mesh);
		C->SetRelativeScale3D(Scale);
		C->SetRelativeLocation(HexWorld(St.hex) + Offset + FVector(0, 0, HexHeight(St.hex) + Scale.Z * 50.f));
		if (TObjectPtr<UMaterialInstanceDynamic>* M = StructMats.Find(St.id)) (*M)->SetVectorParameterValue(TEXT("Color"), Col);
	}
	for (auto It = StructComps.CreateIterator(); It; ++It)
	{
		if (Live.Contains(It.Key())) continue;
		if (It.Value()) It.Value()->DestroyComponent();
		StructMats.Remove(It.Key());
		It.RemoveCurrent();
	}
}

void AGLMapActor::DrawLinks()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me(); UWorld* W = GetWorld();
	for (const gl::Link& L : S.links)
	{
		if (!L.alive || !G.LinkVisible(Me, L)) continue;
		const FLinearColor Own = SyndColor(L.sid);
		for (const gl::Segment& Sg : L.segs)
		{
			const FVector A = HexWorld(Sg.a) + FVector(0, 0, HexHeight(Sg.a) + 22.f), B = HexWorld(Sg.b) + FVector(0, 0, HexHeight(Sg.b) + 22.f);
			FLinearColor C = L.built ? FMath::Lerp(Own, FLinearColor(1.f, 0.1f, 0.1f), FMath::Clamp((float)Sg.lastLoss / 0.30f, 0.f, 1.f)) : FLinearColor(0.3f, 0.3f, 0.35f);
			if (Sg.sabotagedUntil > S.cycle) C = FLinearColor(1.f, 0.2f, 0.f) * (0.5f + 0.5f * FMath::Sin(Time * 8.f));
			if (Sg.cap <= 0) C = FLinearColor(0.2f, 0.2f, 0.2f);
			const float Thick = L.wireless ? 1.5f : 2.f + (float)Sg.cap / 12.f;
			DrawDebugLine(W, A, B, C.ToFColor(true), false, -1.f, 0, Thick);
			if (Sg.flow > 0.05 && L.built)
			{
				const int32 N = FMath::Clamp((int32)(Sg.flow / 4.0) + 1, 1, 8);
				const FVector From = Sg.lastDir >= 0 ? A : B, To = Sg.lastDir >= 0 ? B : A;
				for (int32 i = 0; i < N; ++i)
				{
					const float T = FMath::Fmod(Time * 0.7f + (float)i / N, 1.f);
					// packets that will be lost drop away from the line as they travel
					const float Drop = (float)Sg.lastLoss * 120.f * T * T;
					DrawDebugPoint(W, FMath::Lerp(From, To, T) + FVector(0, 0, 6.f - Drop), 3.5f, FColor(230, 245, 255), false, -1.f, 0);
				}
			}
		}
	}
}

void AGLMapActor::DrawOverlays()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me(); UWorld* W = GetWorld();
	const FVector Y(1, 0, 0), Z(0, 1, 0);
	for (const gl::Hex& H : S.hexes)
	{
		if (H.type == gl::Sector::Barrier) continue;
		const FVector C = HexWorld(H.id) + FVector(0, 0, HexHeight(H.id) + 2.f);
		if (H.owner != Me) { auto P = H.P.find(Me); if (P != H.P.end() && P->second > 0) DrawDebugCircle(W, C, 40.f + 0.3f * (float)P->second, 24, SyndColor(Me).ToFColor(true), false, -1.f, 0, 1.5f + (float)P->second / 40.f, Y, Z, false); }
		if (G.PresenceVisible(Me, H.id)) { int32 k = 0; for (auto& KV : H.P) { if (KV.first == Me || KV.second <= 0) continue; DrawDebugCircle(W, C, 30.f + 8.f * k + 0.25f * (float)KV.second, 18, SyndColor(KV.first).ToFColor(true), false, -1.f, 0, 1.f, Y, Z, false); ++k; } }
		if (!H.roots.empty() && G.CanSee(Me, H.id)) DrawDebugCircle(W, C + FVector(0, 0, 30), 55.f, 6, FColor::Red, false, -1.f, 0, 3.f, Y, Z, false);
		if (H.owner == Me && !H.dual.count(Me) && H.id != S.synds[Me].crown && H.delivered.count(Me)) DrawDebugCircle(W, C, 88.f, 6, FColor(90, 90, 110), false, -1.f, 0, 0.8f, Y, Z, false);
	}
	if (Selected >= 0) DrawDebugCircle(W, HexWorld(Selected) + FVector(0, 0, HexHeight(Selected) + 3.f), 96.f, 6, FColor::White, false, -1.f, 0, 3.f, Y, Z, false);
	if (FromHex >= 0) DrawDebugCircle(W, HexWorld(FromHex) + FVector(0, 0, HexHeight(FromHex) + 3.f), 96.f, 6, FColor::Yellow, false, -1.f, 0, 3.f, Y, Z, false);
	for (int32 Ex : S.exchanges) DrawDebugCircle(W, HexWorld(Ex) + FVector(0, 0, HexHeight(Ex) + 3.f), 70.f + 10.f * FMath::Sin(Time * 2.f), 6, FColor::White, false, -1.f, 0, 1.5f, Y, Z, false);
}

void AGLMapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (!GM) return;
	if (GM->ConsumeDirty()) Refresh();
	DrawLinks();
	DrawOverlays();
}
