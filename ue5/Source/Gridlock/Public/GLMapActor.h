#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GLMapActor.generated.h"

class AGLGameMode;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Presentation of the city: hex prisms, structure markers, fiber links with visible packet loss. Reads the sim through the viewer's fog.
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
	UPROPERTY() TObjectPtr<AGLGameMode> GM = nullptr;
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> HexMeshes;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> HexMats;
	UPROPERTY() TMap<int32, TObjectPtr<UStaticMeshComponent>> StructComps;
	UPROPERTY() TMap<int32, TObjectPtr<UMaterialInstanceDynamic>> StructMats;
	UPROPERTY() TObjectPtr<UMaterialInterface> BaseMat = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylinder = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone = nullptr;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere = nullptr;
	TMap<const UPrimitiveComponent*, int32> CompToHex;
	float Time = 0.f;

	void BuildHexMesh(UProceduralMeshComponent* Mesh, float Radius, float Height) const;
	void RefreshStructures();
	void DrawLinks();
	void DrawOverlays();
};
