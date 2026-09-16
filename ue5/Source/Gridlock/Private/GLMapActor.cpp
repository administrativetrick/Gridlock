#include "GLMapActor.h"
#include "GLGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

static constexpr float kHexSize = 100.f;
static constexpr float kPuck = 10.f;

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
float AGLMapActor::HexHeight(int32 Hex) const { return GM->Sim().S().hexes[Hex].type == gl::Sector::Barrier ? 2.f : kPuck; }
int32 AGLMapActor::HexFromComponent(const UPrimitiveComponent* Comp) const { if (!Comp) return -1; if (const int32* Id = CompToHex.Find(Comp)) return *Id; return -1; }
FVector AGLMapActor::MapCenter() const { FVector C(0); int N = 0; for (const gl::Hex& H : GM->Sim().S().hexes) { C += HexWorld(H.id); ++N; } return N ? C / N : C; }
UStaticMesh* AGLMapActor::Mesh(const TCHAR* Name) const { const TObjectPtr<UStaticMesh>* M = Meshes.Find(FName(Name)); checkf(M && *M, TEXT("Generated mesh %s missing: run UnrealEditor-Cmd Gridlock.uproject -run=GLAssets"), Name); return M->Get(); }

static UInstancedStaticMeshComponent* MakeISM(AActor* Owner, USceneComponent* Parent, const FName& Name, UStaticMesh* Mesh, UMaterialInterface* Mat, int32 CustomFloats, bool Shadows)
{
	UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
	C->SetupAttachment(Parent);
	C->RegisterComponent();
	C->SetStaticMesh(Mesh);
	C->SetMaterial(0, Mat);
	C->NumCustomDataFloats = CustomFloats;
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(Shadows);
	C->SetMobility(EComponentMobility::Movable);
	return C;
}

void AGLMapActor::Init(AGLGameMode* InGM)
{
	GM = InGM;
	MatNeon = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Neon.M_Neon"));
	MatInst = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_NeonInst.M_NeonInst"));
	MatHolo = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Holo.M_Holo"));
	MatGlow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GlowInst.M_GlowInst"));
	checkf(MatNeon && MatInst && MatHolo && MatGlow, TEXT("Generated materials missing: run UnrealEditor-Cmd Gridlock.uproject -run=GLAssets"));
	static const TCHAR* Names[] = { TEXT("SM_TowerA"), TEXT("SM_TowerB"), TEXT("SM_Slab"), TEXT("SM_Arcology"), TEXT("SM_Factory"), TEXT("SM_Dock"), TEXT("SM_Sprawl"), TEXT("SM_Undercity"), TEXT("SM_Spire"),
		TEXT("SM_Node"), TEXT("SM_Substation"), TEXT("SM_Repeater"), TEXT("SM_Outpost"), TEXT("SM_Array"), TEXT("SM_Rack"), TEXT("SM_Lab"), TEXT("SM_Honeypot"), TEXT("SM_Tap"), TEXT("SM_Cutout"), TEXT("SM_Fork"), TEXT("SM_Cable"), TEXT("SM_Tile") };
	for (const TCHAR* N : Names) { UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Meshes/%s.%s"), N, N)); checkf(M, TEXT("Generated mesh %s missing: run -run=GLAssets"), N); Meshes.Add(FName(N), M); }
	Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	checkf(Sphere && Cylinder && Plane, TEXT("Engine basic shapes missing"));

	const gl::GameState& S = GM->Sim().S();
	Ground = NewObject<UStaticMeshComponent>(this, TEXT("Ground"));
	Ground->SetupAttachment(RootComponent); Ground->RegisterComponent();
	Ground->SetStaticMesh(Plane); Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ground->SetRelativeLocation(MapCenter() + FVector(0, 0, -1.f)); Ground->SetRelativeScale3D(FVector(140.f, 140.f, 1.f));
	{
		UMaterialInstanceDynamic* GMat = UMaterialInstanceDynamic::Create(MatNeon, this);
		GMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.006f, 0.008f, 0.014f));
		GMat->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		GMat->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.f);
		GMat->SetScalarParameterValue(TEXT("Metallic"), 0.9f);
		GMat->SetScalarParameterValue(TEXT("Roughness"), 0.18f);
		Ground->SetMaterial(0, GMat);
	}
	UStaticMesh* Tile = Mesh(TEXT("SM_Tile"));
	HexTiles.SetNum(S.hexes.size()); HexMats.SetNum(S.hexes.size());
	for (const gl::Hex& H : S.hexes)
	{
		UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Hex%d"), H.id));
		M->SetupAttachment(RootComponent); M->RegisterComponent();
		M->SetStaticMesh(Tile);
		M->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		M->SetCollisionResponseToAllChannels(ECR_Block);
		M->SetCastShadow(false);
		M->SetRelativeLocation(HexWorld(H.id));
		if (H.type == gl::Sector::Barrier) M->SetRelativeScale3D(FVector(1.f, 1.f, 0.2f));
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatNeon, this);
		MID->SetScalarParameterValue(TEXT("RimWidth"), 0.045f);
		MID->SetScalarParameterValue(TEXT("Metallic"), 0.3f);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
		M->SetMaterial(0, MID);
		HexTiles[H.id] = M; HexMats[H.id] = MID; CompToHex.Add(M, H.id);
	}
	LinkMeshes = MakeISM(this, RootComponent, TEXT("Links"), Mesh(TEXT("SM_Cable")), MatGlow, 4, false);
	PacketMeshes = MakeISM(this, RootComponent, TEXT("Packets"), Sphere, MatGlow, 4, false);
	Discs = MakeISM(this, RootComponent, TEXT("Discs"), Cylinder, MatHolo, 4, false);
	Discs->SetTranslucentSortPriority(1);
	BuildCity();
	Refresh();
}

// Architecture per sector from the kitbash library, deterministic from the hex id.
void AGLMapActor::BuildCity()
{
	const gl::GameState& S = GM->Sim().S();
	BuildingRefs.Reset();
	auto ISM = [&](const TCHAR* Name) -> UInstancedStaticMeshComponent* {
		const FName Key(Name); if (TObjectPtr<UInstancedStaticMeshComponent>* E = ArchISM.Find(Key)) return E->Get();
		UInstancedStaticMeshComponent* C = MakeISM(this, RootComponent, *FString::Printf(TEXT("Arch_%s"), Name), Mesh(Name), MatInst, 4, true);
		ArchISM.Add(Key, C); return C; };
	auto Place = [&](const gl::Hex& H, const TCHAR* Name, FVector Offset, float Yaw, float Scale) {
		UInstancedStaticMeshComponent* C = ISM(Name);
		const int32 Idx = C->AddInstance(FTransform(FRotator(0, Yaw, 0), HexWorld(H.id) + FVector(0, 0, kPuck) + Offset, FVector(Scale)), true);
		BuildingRefs.Add({ FName(Name), Idx, H.id }); };
	for (const gl::Hex& H : S.hexes)
	{
		if (H.type == gl::Sector::Barrier) continue;
		FRandomStream R(H.id * 7919 + 17);
		const float Yaw = 60.f * R.RandRange(0, 5) + R.FRandRange(-8.f, 8.f);
		const float Sc = R.FRandRange(0.9f, 1.1f);
		auto Second = [&](const TCHAR* Name, float Chance, float Dist) { if (R.FRand() < Chance) { const float A = R.FRandRange(0, 6.283f); Place(H, Name, FVector(Dist * FMath::Cos(A), Dist * FMath::Sin(A), 0), Yaw + 30.f, 0.75f); } };
		switch (H.type)
		{
		case gl::Sector::Financial: Place(H, R.FRand() < 0.6f ? TEXT("SM_TowerA") : TEXT("SM_TowerB"), FVector(R.FRandRange(-8, 8), R.FRandRange(-8, 8), 0), Yaw, Sc); Second(TEXT("SM_Slab"), 0.5f, 46.f); break;
		case gl::Sector::Campus:    Place(H, TEXT("SM_Slab"), FVector(R.FRandRange(-10, 10), R.FRandRange(-10, 10), 0), Yaw, Sc); Second(TEXT("SM_TowerB"), 0.35f, 44.f); break;
		case gl::Sector::Arcology:  Place(H, TEXT("SM_Arcology"), FVector::ZeroVector, Yaw, Sc); break;
		case gl::Sector::Industrial:Place(H, TEXT("SM_Factory"), FVector(R.FRandRange(-6, 6), R.FRandRange(-6, 6), 0), Yaw, Sc); break;
		case gl::Sector::Port:      Place(H, TEXT("SM_Dock"), FVector::ZeroVector, Yaw, Sc); break;
		case gl::Sector::Sprawl:    Place(H, TEXT("SM_Sprawl"), FVector::ZeroVector, Yaw, Sc); Second(TEXT("SM_Sprawl"), 0.3f, 40.f); break;
		case gl::Sector::Undercity: Place(H, TEXT("SM_Undercity"), FVector::ZeroVector, Yaw, Sc); break;
		case gl::Sector::Exchange:  Place(H, TEXT("SM_Spire"), FVector::ZeroVector, 0.f, 1.f); break;
		default: break;
		}
	}
}

void AGLMapActor::GlowOf(int32 HexId, FLinearColor& OutColor, float& OutStrength, bool& OutVisible) const
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me(); const gl::Hex& H = S.hexes[HexId];
	const bool Vis = G.CanSee(Me, HexId);
	auto Seen = H.seen.find(Me);
	OutVisible = Vis;
	if (H.type == gl::Sector::Exchange) { OutColor = FLinearColor(0.85f, 0.9f, 1.f); OutStrength = 1.6f; return; }
	if (!Vis && Seen == H.seen.end()) { OutColor = FLinearColor(0.16f, 0.26f, 0.4f); OutStrength = 0.3f; return; }
	const int32 OwnerSid = Vis ? H.owner : Seen->second.owner;
	const double Integ = Vis ? H.C : Seen->second.C;
	if (OwnerSid >= 0) { OutColor = SyndColor(OwnerSid); OutStrength = 0.4f + 1.0f * (float)(Integ / 100.0); }
	else { OutColor = FLinearColor(0.35f, 0.5f, 0.7f); OutStrength = 0.34f; }
	if (!Vis) OutStrength *= 0.45f;
}

void AGLMapActor::RefreshHexes()
{
	const gl::GameState& S = GM->Sim().S(); const int32 Me = GM->Me();
	for (const gl::Hex& H : S.hexes)
	{
		UMaterialInstanceDynamic* M = HexMats[H.id];
		if (H.type == gl::Sector::Barrier)
		{
			M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.004f, 0.01f, 0.02f));
			M->SetVectorParameterValue(TEXT("Emissive"), FLinearColor(0.05f, 0.15f, 0.35f));
			M->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.25f);
			M->SetScalarParameterValue(TEXT("Metallic"), 0.95f); M->SetScalarParameterValue(TEXT("Roughness"), 0.08f);
			continue;
		}
		FLinearColor Glow; float Str; bool Vis; GlowOf(H.id, Glow, Str, Vis);
		FLinearColor Base(0.035f, 0.04f, 0.05f);
		if (!Vis) Base *= 0.5f;
		if (Vis && H.brownout && H.owner == Me) { Glow = FLinearColor(1.f, 0.25f, 0.05f); Str = 1.2f + 0.8f * FMath::Sin(Time * 6.f); }
		if (H.id == Selected) { Str += 1.2f; Base += FLinearColor(0.05f, 0.05f, 0.06f); }
		if (H.id == FromHex) { Glow = FLinearColor(1.f, 0.85f, 0.2f); Str = 2.f; }
		M->SetVectorParameterValue(TEXT("Color"), Base);
		M->SetVectorParameterValue(TEXT("Emissive"), Glow);
		M->SetScalarParameterValue(TEXT("EmissiveStrength"), Str);
	}
}

void AGLMapActor::RefreshBuildings()
{
	for (const FBuilding& B : BuildingRefs)
	{
		UInstancedStaticMeshComponent* C = ArchISM[B.Mesh];
		FLinearColor Glow; float Str; bool Vis; GlowOf(B.Hex, Glow, Str, Vis);
		Str *= 0.6f;
		C->SetCustomDataValue(B.Index, 0, Glow.R, false); C->SetCustomDataValue(B.Index, 1, Glow.G, false);
		C->SetCustomDataValue(B.Index, 2, Glow.B, false); C->SetCustomDataValue(B.Index, 3, Str, false);
	}
	for (auto& KV : ArchISM) KV.Value->MarkRenderStateDirty();
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
			C->SetupAttachment(RootComponent); C->RegisterComponent();
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MatNeon, this);
			MID->SetScalarParameterValue(TEXT("RimWidth"), 0.09f);
			MID->SetScalarParameterValue(TEXT("Fill"), 0.12f);
			C->SetMaterial(0, MID);
			StructComps.Add(St.id, C); StructMats.Add(St.id, MID);
		}
		const TCHAR* MeshName = TEXT("SM_Node"); float Scale = 1.f; FVector Offset(0); FLinearColor Col = SyndColor(St.sid); float Str = 1.4f;
		switch (St.kind)
		{
		case gl::StructKind::Node: MeshName = TEXT("SM_Node"); Scale = 0.85f + 0.2f * St.tier; if (St.crown) Scale *= 1.25f; Str = 2.2f; break;
		case gl::StructKind::Substation: MeshName = TEXT("SM_Substation"); Offset = FVector(46, 26, 0); Col = FLinearColor(1.f, 0.8f, 0.15f); break;
		case gl::StructKind::PrivateGrid: MeshName = TEXT("SM_Substation"); Scale = 1.4f; Offset = FVector(46, 26, 0); Col = FLinearColor(1.f, 0.6f, 0.1f); break;
		case gl::StructKind::Repeater: MeshName = TEXT("SM_Repeater"); Col = FLinearColor(0.7f, 0.95f, 1.f); break;
		case gl::StructKind::Outpost: MeshName = TEXT("SM_Outpost"); Offset = FVector(-46, 26, 0); Col = FLinearColor(1.f, 0.3f, 0.2f); break;
		case gl::StructKind::Array: MeshName = TEXT("SM_Array"); Offset = FVector(0, -50, 0); Col = FLinearColor(0.5f, 1.f, 0.9f); break;
		case gl::StructKind::Honeypot: MeshName = TEXT("SM_Honeypot"); Offset = FVector(-36, -36, 0); Col = FLinearColor(1.f, 0.5f, 0.05f); break;
		case gl::StructKind::Lab: MeshName = TEXT("SM_Lab"); Offset = FVector(36, -36, 0); Col = FLinearColor(0.75f, 0.35f, 1.f); break;
		case gl::StructKind::Rack: MeshName = TEXT("SM_Rack"); Offset = FVector(0, 0, 222.f); Col = FLinearColor(1.f, 1.f, 1.f); Str = 2.5f; break;
		case gl::StructKind::Tap: MeshName = TEXT("SM_Tap"); Offset = FVector(0, 46, 0); Col = FLinearColor(0.9f, 0.1f, 0.6f); break;
		case gl::StructKind::Cutout: MeshName = TEXT("SM_Cutout"); Col = FLinearColor(0.3f, 0.3f, 0.35f); Str = 0.4f; break;
		case gl::StructKind::Fork: MeshName = TEXT("SM_Fork"); Offset = FVector(46, -26, 0); Col = FLinearColor(0.3f, 1.f, 0.6f); break;
		default: break;
		}
		if (!St.built) Str = 0.25f;
		if (St.darkUntil > S.cycle) { Str = 0.f; Col = FLinearColor(0.1f, 0.1f, 0.12f); }
		C->SetStaticMesh(Mesh(MeshName));
		C->SetRelativeScale3D(FVector(Scale));
		C->SetRelativeLocation(HexWorld(St.hex) + Offset + FVector(0, 0, kPuck));
		if (TObjectPtr<UMaterialInstanceDynamic>* M = StructMats.Find(St.id))
		{
			(*M)->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.03f, 0.04f));
			(*M)->SetVectorParameterValue(TEXT("Emissive"), Col);
			(*M)->SetScalarParameterValue(TEXT("EmissiveStrength"), Str);
		}
	}
	for (auto It = StructComps.CreateIterator(); It; ++It)
	{
		if (Live.Contains(It.Key())) continue;
		if (It.Value()) It.Value()->DestroyComponent();
		StructMats.Remove(It.Key());
		It.RemoveCurrent();
	}
}

void AGLMapActor::RefreshLinks()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me();
	LinkMeshes->ClearInstances(); PacketMeshes->ClearInstances(); Packets.Reset();
	for (int32 li = 0; li < (int32)S.links.size(); ++li)
	{
		const gl::Link& L = S.links[li];
		if (!L.alive || !G.LinkVisible(Me, L)) continue;
		const FLinearColor Own = SyndColor(L.sid);
		for (int32 si = 0; si < (int32)L.segs.size(); ++si)
		{
			const gl::Segment& Sg = L.segs[si];
			const float Z = kPuck + 7.f;
			const FVector A = HexWorld(Sg.a) + FVector(0, 0, Z), B = HexWorld(Sg.b) + FVector(0, 0, Z);
			const FVector Dir = (B - A); const float Len = Dir.Size();
			const float R = L.wireless ? 1.2f : 1.6f + (float)Sg.cap / 22.f;          // cable radius in cm
			FTransform Tf(FRotationMatrix::MakeFromX(Dir.GetSafeNormal()).Rotator(), (A + B) * 0.5f, FVector(Len / 100.f, R, R));
			const int32 Idx = LinkMeshes->AddInstance(Tf, true);
			FLinearColor C = L.built ? FMath::Lerp(Own, FLinearColor(1.f, 0.12f, 0.05f), FMath::Clamp((float)Sg.lastLoss / 0.30f, 0.f, 1.f)) : FLinearColor(0.3f, 0.3f, 0.35f);
			float Str = L.built ? 0.7f + 1.8f * (Sg.cap > 0 ? FMath::Clamp((float)(Sg.flow / Sg.cap), 0.f, 1.f) : 0.f) : 0.2f;
			if (Sg.sabotagedUntil > S.cycle) { C = FLinearColor(1.f, 0.3f, 0.f); Str = 2.5f; }
			if (Sg.cap <= 0) { C = FLinearColor(0.2f, 0.2f, 0.2f); Str = 0.1f; }
			LinkMeshes->SetCustomDataValue(Idx, 0, C.R, false); LinkMeshes->SetCustomDataValue(Idx, 1, C.G, false);
			LinkMeshes->SetCustomDataValue(Idx, 2, C.B, false); LinkMeshes->SetCustomDataValue(Idx, 3, Str, false);
			if (Sg.flow > 0.05 && L.built)
			{
				const int32 N = FMath::Clamp((int32)(Sg.flow / 4.0) + 1, 1, 8);
				for (int32 i = 0; i < N; ++i)
				{
					const int32 P = PacketMeshes->AddInstance(FTransform(FRotator::ZeroRotator, A, FVector(0.06f)), true);
					PacketMeshes->SetCustomDataValue(P, 0, 0.85f, false); PacketMeshes->SetCustomDataValue(P, 1, 0.97f, false);
					PacketMeshes->SetCustomDataValue(P, 2, 1.0f, false); PacketMeshes->SetCustomDataValue(P, 3, 8.f, false);
					Packets.Add({ li, si, (float)i / N });
				}
			}
		}
	}
	LinkMeshes->MarkRenderStateDirty(); PacketMeshes->MarkRenderStateDirty();
}

void AGLMapActor::RefreshDiscs()
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me();
	Discs->ClearInstances();
	auto Add = [&](int32 Hex, const FLinearColor& C, float Alpha, float Scale)
	{
		const int32 I = Discs->AddInstance(FTransform(FRotator::ZeroRotator, HexWorld(Hex) + FVector(0, 0, kPuck + 1.5f), FVector(Scale, Scale, 0.012f)), true);
		Discs->SetCustomDataValue(I, 0, C.R, false); Discs->SetCustomDataValue(I, 1, C.G, false); Discs->SetCustomDataValue(I, 2, C.B, false); Discs->SetCustomDataValue(I, 3, Alpha, false);
	};
	for (const gl::Hex& H : S.hexes)
	{
		if (H.type == gl::Sector::Barrier) continue;
		if (H.owner != Me) { auto P = H.P.find(Me); if (P != H.P.end() && P->second > 0) Add(H.id, SyndColor(Me), 0.05f + 0.25f * (float)(P->second / 100.0), 1.55f); }
		if (G.PresenceVisible(Me, H.id)) { float Sc = 1.2f; for (auto& KV : H.P) { if (KV.first == Me || KV.second <= 0) continue; Add(H.id, SyndColor(KV.first), 0.06f + 0.22f * (float)(KV.second / 100.0), Sc); Sc -= 0.25f; } }
		if (!H.roots.empty() && G.CanSee(Me, H.id)) Add(H.id, FLinearColor(1.f, 0.05f, 0.05f), 0.35f, 0.7f);
	}
	Discs->MarkRenderStateDirty();
}

void AGLMapActor::Refresh() { RefreshHexes(); RefreshBuildings(); RefreshStructures(); RefreshLinks(); RefreshDiscs(); }

void AGLMapActor::TickPackets()
{
	if (Packets.Num() == 0) return;
	const gl::GameState& S = GM->Sim().S();
	TArray<FTransform> Tfs; Tfs.Reserve(Packets.Num());
	for (FPacket& P : Packets)
	{
		const gl::Segment& Sg = S.links[P.Link].segs[P.Seg];
		const float Z = kPuck + 7.f;
		const FVector A = HexWorld(Sg.a) + FVector(0, 0, Z), B = HexWorld(Sg.b) + FVector(0, 0, Z);
		const FVector From = Sg.lastDir >= 0 ? A : B, To = Sg.lastDir >= 0 ? B : A;
		const float T = FMath::Fmod(Time * 0.55f + P.Phase, 1.f);
		const float Drop = (float)Sg.lastLoss * 160.f * T * T;
		Tfs.Add(FTransform(FRotator::ZeroRotator, FMath::Lerp(From, To, T) + FVector(0, 0, 4.f - Drop), FVector(0.06f)));
	}
	PacketMeshes->BatchUpdateInstancesTransforms(0, Tfs, true, true, true);
}

void AGLMapActor::DrawOverlays()
{
	const gl::GameState& S = GM->Sim().S(); const int32 Me = GM->Me(); UWorld* W = GetWorld();
	const FVector Y(1, 0, 0), Z(0, 1, 0);
	for (const gl::Hex& H : S.hexes)
		if (H.owner == Me && !H.dual.count(Me) && H.id != S.synds[Me].crown && H.delivered.count(Me))
			DrawDebugCircle(W, HexWorld(H.id) + FVector(0, 0, kPuck + 2.f), 90.f, 6, FColor(120, 120, 150), false, -1.f, 0, 0.8f, Y, Z, false);
	if (Selected >= 0) DrawDebugCircle(W, HexWorld(Selected) + FVector(0, 0, kPuck + 3.f), 98.f, 6, FColor::White, false, -1.f, 0, 2.5f, Y, Z, false);
	if (FromHex >= 0) DrawDebugCircle(W, HexWorld(FromHex) + FVector(0, 0, kPuck + 3.f), 98.f, 6, FColor::Yellow, false, -1.f, 0, 2.5f, Y, Z, false);
}

void AGLMapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (!GM) return;
	if (GM->ConsumeDirty()) Refresh();
	TickPackets();
	DrawOverlays();
}
