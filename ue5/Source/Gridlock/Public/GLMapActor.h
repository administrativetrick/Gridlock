#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GLMapActor.generated.h"

class AGLGameMode;
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Presentation of the city: neon-rimmed hex pucks, brutalist building clusters (instanced, per-instance syndicate glow),
// glowing fiber meshes with packet spheres that drop away with loss, holographic presence discs, a wet reflective ground.
// Reads the sim through the viewer's fog only.
UCLASS()
class AGLMapActor : public AActor
{
	GENERATED_BODY()
public:
	AGLMapActor();
	void Init(AGLGameMode* InGM);
	void Refresh();
	virtual void Tick(float DeltaSeconds) override;

	int32 HexFromComponent(const UPrimitiveComponent* Comp) const;
	FVector HexWorld(int32 Hex) const;
	float HexHeight(int32 Hex) const;
	FVector MapCenter() const;

	static FLinearColor SyndColor(int32 Sid);

	int32 Selected = -1;
	int32 FromHex = -1;

private:
	struct FBuilding { int32 Hex; };
	struct FSegRef { int32 Link; int32 Seg; };
	struct FPacket { int32 Link; int32 Seg; float Phase; };

	UPROPERTY() TObjectPtr<AGLGameMode> GM = nullptr;
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> HexMeshes;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> HexMats;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Buildings = nullptr;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> LinkMeshes = nullptr;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PacketMeshes = nullptr;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Discs = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ground = nullptr;
	UPROPERTY() TMap<int32, TObjectPtr<UStaticMeshComponent>> StructComps;
	UPROPERTY() TMap<int32, TObjectPtr<UMaterialInstanceDynamic>> StructMats;
	UPROPERTY() TObjectPtr<UMaterialInterface> MatNeon = nullptr;
	UPROPERTY() TObjectPtr<UMaterialInterface> MatInst = nullptr;
	UPROPERTY() TObjectPtr<UMaterialInterface> MatHolo = nullptr;
	UPROPERTY() TObjectPtr<UMaterialInterface> MatGlow = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylinder = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Plane = nullptr;

	TMap<const UPrimitiveComponent*, int32> CompToHex;
	TArray<FBuilding> BuildingRefs;
	TArray<FSegRef> SegRefs;
	TArray<FPacket> Packets;
	float Time = 0.f;

	void BuildHexMesh(UProceduralMeshComponent* Mesh, float Radius, float Height) const;
	void BuildCity();
	void RefreshHexes();
	void RefreshBuildings();
	void RefreshStructures();
	void RefreshLinks();
	void RefreshDiscs();
	void TickPackets();
	void DrawOverlays();
	void GlowOf(int32 Hex, FLinearColor& OutColor, float& OutStrength, bool& OutVisible) const;
};
