#include "GLMaterialCommandlet.h"
#include "GLMeshKit.h"
#include "GLTextureKit.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionTime.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

UGLAssetsCommandlet::UGLAssetsCommandlet()
{
	IsClient = false; IsEditor = true; IsServer = false; LogToConsole = true;
}

#if WITH_EDITOR

namespace
{
	template <class T> T* Expr(UMaterial* M, int32 X, int32 Y)
	{
		T* E = NewObject<T>(M);
		E->MaterialExpressionEditorX = X; E->MaterialExpressionEditorY = Y;
		M->GetExpressionCollection().AddExpression(E);
		return E;
	}

	bool SavePkg(UPackage* Pkg, UObject* Asset)
	{
		Pkg->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
		const bool Ok = UPackage::SavePackage(Pkg, Asset, *File, Args);
		UE_LOG(LogTemp, Display, TEXT("%s %s"), Ok ? TEXT("Saved") : TEXT("FAILED to save"), *File);
		return Ok;
	}
	UPackage* NewPkg(const TCHAR* Folder, const TCHAR* Name) { UPackage* P = CreatePackage(*FString::Printf(TEXT("/Game/%s/%s"), Folder, Name)); P->FullyLoad(); return P; }

	// ------------------------------------------------------------ textures
	UTexture2D* MakeTexture(const TCHAR* Name, const FGLTex& T)
	{
		UPackage* Pkg = NewPkg(TEXT("Textures"), Name);
		UTexture2D* Tex = NewObject<UTexture2D>(Pkg, Name, RF_Public | RF_Standalone);
		Tex->Source.Init(T.W, T.H, 1, 1, TSF_BGRA8, T.BGRA.GetData());
		Tex->SRGB = T.bSRGB;
		Tex->CompressionSettings = T.bNormal ? TC_Normalmap : (T.bSRGB ? TC_Default : TC_Masks);
		Tex->LODGroup = T.bNormal ? TEXTUREGROUP_WorldNormalMap : TEXTUREGROUP_World;
		Tex->AddressX = TA_Wrap; Tex->AddressY = TA_Wrap;
		Tex->MipGenSettings = TMGS_FromTextureGroup;
		Tex->UpdateResource();
		Tex->PostEditChange();
		return SavePkg(Pkg, Tex) ? Tex : nullptr;
	}

	// ------------------------------------------------------------ material pieces
	UMaterialExpression* BuildRim(UMaterial* M, UMaterialExpression* Width)
	{
		auto* UV = Expr<UMaterialExpressionTextureCoordinate>(M, -1400, 300);
		auto* Sub = Expr<UMaterialExpressionSubtract>(M, -1200, 300); Sub->A.Connect(0, UV); Sub->ConstB = 0.5f;
		auto* Ab = Expr<UMaterialExpressionAbs>(M, -1050, 300); Ab->Input.Connect(0, Sub);
		auto* R = Expr<UMaterialExpressionComponentMask>(M, -900, 250); R->Input.Connect(0, Ab); R->R = true; R->G = false; R->B = false; R->A = false;
		auto* G = Expr<UMaterialExpressionComponentMask>(M, -900, 350); G->Input.Connect(0, Ab); G->R = false; G->G = true; G->B = false; G->A = false;
		auto* Mx = Expr<UMaterialExpressionMax>(M, -750, 300); Mx->A.Connect(0, R); Mx->B.Connect(0, G);
		auto* Inner = Expr<UMaterialExpressionSubtract>(M, -750, 450); Inner->ConstA = 0.5f; Inner->B.Connect(0, Width);
		auto* Sub2 = Expr<UMaterialExpressionSubtract>(M, -600, 300); Sub2->A.Connect(0, Mx); Sub2->B.Connect(0, Inner);
		auto* Div = Expr<UMaterialExpressionDivide>(M, -450, 300); Div->A.Connect(0, Sub2); Div->B.Connect(0, Width);
		auto* Sat = Expr<UMaterialExpressionSaturate>(M, -300, 300); Sat->Input.Connect(0, Div);
		return Sat;
	}
	UMaterialExpression* CustomRGB(UMaterial* M, int32 Y)
	{
		auto* CD0 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, Y); CD0->DataIndex = 0; CD0->ConstDefaultValue = 0.f;
		auto* CD1 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, Y + 80); CD1->DataIndex = 1; CD1->ConstDefaultValue = 0.9f;
		auto* CD2 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, Y + 160); CD2->DataIndex = 2; CD2->ConstDefaultValue = 1.f;
		auto* RG = Expr<UMaterialExpressionAppendVector>(M, -550, Y + 40); RG->A.Connect(0, CD0); RG->B.Connect(0, CD1);
		auto* RGB = Expr<UMaterialExpressionAppendVector>(M, -400, Y + 80); RGB->A.Connect(0, RG); RGB->B.Connect(0, CD2);
		return RGB;
	}
	UMaterialExpression* CustomScalar(UMaterial* M, int32 Y, int32 Index, float Def) { auto* CD = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, Y); CD->DataIndex = Index; CD->ConstDefaultValue = Def; return CD; }
	UMaterialExpressionTextureObject* TexObj(UMaterial* M, int32 Y, UTexture2D* T) { auto* O = Expr<UMaterialExpressionTextureObject>(M, -1900, Y); O->Texture = T; return O; }

	struct FTexSet { UTexture2D* Conc; UTexture2D* Nrm; UTexture2D* Grime; UTexture2D* Win; UTexture2D* Traces; };

	// Tri-planar concrete, world-space normal, lit windows on walls, traces on unit-UV tops.
	static const TCHAR* kSurfaceHLSL = TEXT(R"HLSL(
float3 an = abs(N);
float3 w = an / max(an.x + an.y + an.z, 1e-4);
float s = 1.0 / 220.0;
float3 cX = Texture2DSample(Conc, ConcSampler, WP.yz * s).rgb;
float3 cY = Texture2DSample(Conc, ConcSampler, WP.xz * s).rgb;
float3 cZ = Texture2DSample(Conc, ConcSampler, WP.xy * s).rgb;
float3 conc = cX * w.x + cY * w.y + cZ * w.z;
float3 nX = Texture2DSample(Nrm, NrmSampler, WP.yz * s).rgb * 2.0 - 1.0;
float3 nY = Texture2DSample(Nrm, NrmSampler, WP.xz * s).rgb * 2.0 - 1.0;
float3 nZ = Texture2DSample(Nrm, NrmSampler, WP.xy * s).rgb * 2.0 - 1.0;
float3 dn = float3(0.0, nX.x, nX.y) * w.x + float3(nY.x, 0.0, nY.y) * w.y + float3(nZ.x, nZ.y, 0.0) * w.z;
float3 wn = normalize(N + dn * 0.6);
float g = Texture2DSample(Grime, GrimeSampler, float2((WP.x + WP.y) / 900.0, -WP.z / 700.0)).r;
float albedo = conc.r * lerp(0.5, 1.05, g) * conc.g;
float rough = saturate(conc.b * 0.7 + 0.2 + (1.0 - g) * 0.2);
float wall = saturate((0.55 - an.z) * 5.0);
float u = (an.x > an.y) ? WP.y : WP.x;
float2 wuv = float2(u / 34.0, WP.z / 28.0) + Seed * float2(0.37, 0.61);
float4 win = Texture2DSample(Win, WinSampler, wuv);
float glowMag = max(max(Glow.r, Glow.g), max(Glow.b, 1e-3));
float3 glowN = Glow / glowMag;
float3 warm = float3(1.0, 0.82, 0.55);
float3 winCol = lerp(warm, glowN, win.a * 0.85);
float lit = win.r * win.g * wall * saturate(glowMag * 1.6) * WindowStrength;
float tr = Texture2DSample(Traces, TracesSampler, UV).r * TraceStrength * saturate(an.z * 2.0);
Emis = winCol * lit * 3.0 + Glow * tr;
WNrm = wn;
albedo = lerp(albedo, albedo * 0.45, saturate(win.r + win.b * 0.5) * wall * WindowStrength);
return float4(albedo, albedo, albedo, rough);
)HLSL");

	UMaterialExpressionCustom* SurfaceNode(UMaterial* M, const FTexSet& Tx, UMaterialExpression* Glow, UMaterialExpression* Seed, UMaterialExpression* WindowStrength, UMaterialExpression* TraceStrength)
	{
		auto* WP = Expr<UMaterialExpressionWorldPosition>(M, -1900, -400);
		auto* N = Expr<UMaterialExpressionVertexNormalWS>(M, -1900, -300);
		auto* UV = Expr<UMaterialExpressionTextureCoordinate>(M, -1900, -200);
		auto* C = Expr<UMaterialExpressionCustom>(M, -1100, -300);
		C->Description = TEXT("Surface"); C->Code = kSurfaceHLSL; C->OutputType = CMOT_Float4;
		C->Inputs.Empty();
		auto In = [&](const TCHAR* Name, UMaterialExpression* E) { FCustomInput I; I.InputName = Name; I.Input.Connect(0, E); C->Inputs.Add(I); };
		In(TEXT("WP"), WP); In(TEXT("N"), N); In(TEXT("UV"), UV); In(TEXT("Glow"), Glow); In(TEXT("Seed"), Seed); In(TEXT("WindowStrength"), WindowStrength); In(TEXT("TraceStrength"), TraceStrength);
		In(TEXT("Conc"), TexObj(M, -500, Tx.Conc)); In(TEXT("Nrm"), TexObj(M, -400, Tx.Nrm)); In(TEXT("Grime"), TexObj(M, -300, Tx.Grime)); In(TEXT("Win"), TexObj(M, -200, Tx.Win)); In(TEXT("Traces"), TexObj(M, -100, Tx.Traces));
		FCustomOutput Emis; Emis.OutputName = TEXT("Emis"); Emis.OutputType = CMOT_Float3; C->AdditionalOutputs.Add(Emis);
		FCustomOutput WNrm; WNrm.OutputName = TEXT("WNrm"); WNrm.OutputType = CMOT_Float3; C->AdditionalOutputs.Add(WNrm);
		C->RebuildOutputs();
		return C;
	}

	// Wires a surface node into the material outputs. Tint multiplies albedo; Rough multiplies roughness.
	void WireSurface(UMaterial* M, UMaterialExpressionCustom* S, UMaterialExpression* Tint, UMaterialExpression* Metal, UMaterialExpression* Rough, UMaterialExpression* RimGlow)
	{
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		auto* Alb = Expr<UMaterialExpressionComponentMask>(M, -700, -400); Alb->Input.Connect(0, S); Alb->R = true; Alb->G = true; Alb->B = true; Alb->A = false;
		auto* Rgh = Expr<UMaterialExpressionComponentMask>(M, -700, -250); Rgh->Input.Connect(0, S); Rgh->R = false; Rgh->G = false; Rgh->B = false; Rgh->A = true;
		auto* Base = Expr<UMaterialExpressionMultiply>(M, -500, -400); Base->A.Connect(0, Alb); Base->B.Connect(0, Tint);
		auto* RoughOut = Expr<UMaterialExpressionMultiply>(M, -500, -250); RoughOut->A.Connect(0, Rgh); RoughOut->B.Connect(0, Rough);
		auto* EmisOut = Expr<UMaterialExpressionAdd>(M, 0, 150); EmisOut->A.Connect(0, RimGlow); EmisOut->B.Connect(1, S);
		ED->BaseColor.Connect(0, Base); ED->Roughness.Connect(0, RoughOut); ED->Metallic.Connect(0, Metal); ED->EmissiveColor.Connect(0, EmisOut); ED->Normal.Connect(2, S);
		M->bTangentSpaceNormal = false;
	}

	bool SaveMat(UPackage* Pkg, UMaterial* M) { M->PreEditChange(nullptr); M->PostEditChange(); return SavePkg(Pkg, M); }
	UMaterial* NewMat(const TCHAR* Name, UPackage*& Pkg) { Pkg = NewPkg(TEXT("Materials"), Name); return NewObject<UMaterial>(Pkg, Name, RF_Public | RF_Standalone); }

	bool MakeNeon(const FTexSet& Tx)
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_Neon"), Pkg);
		auto* Color = Expr<UMaterialExpressionVectorParameter>(M, -400, -600); Color->ParameterName = TEXT("Color"); Color->DefaultValue = FLinearColor(0.6f, 0.65f, 0.75f);
		auto* Emis = Expr<UMaterialExpressionVectorParameter>(M, -400, 0); Emis->ParameterName = TEXT("Emissive"); Emis->DefaultValue = FLinearColor(0.f, 0.9f, 1.f);
		auto* Str = Expr<UMaterialExpressionScalarParameter>(M, -400, 120); Str->ParameterName = TEXT("EmissiveStrength"); Str->DefaultValue = 1.f;
		auto* Met = Expr<UMaterialExpressionScalarParameter>(M, -400, 500); Met->ParameterName = TEXT("Metallic"); Met->DefaultValue = 0.15f;
		auto* Rgh = Expr<UMaterialExpressionScalarParameter>(M, -400, 600); Rgh->ParameterName = TEXT("Roughness"); Rgh->DefaultValue = 1.f;
		auto* Wid = Expr<UMaterialExpressionScalarParameter>(M, -1600, 450); Wid->ParameterName = TEXT("RimWidth"); Wid->DefaultValue = 0.06f;
		auto* Fill = Expr<UMaterialExpressionScalarParameter>(M, -400, 220); Fill->ParameterName = TEXT("Fill"); Fill->DefaultValue = 0.04f;
		auto* WinS = Expr<UMaterialExpressionScalarParameter>(M, -1600, 700); WinS->ParameterName = TEXT("WindowStrength"); WinS->DefaultValue = 0.f;
		auto* TrS = Expr<UMaterialExpressionScalarParameter>(M, -1600, 800); TrS->ParameterName = TEXT("TraceStrength"); TrS->DefaultValue = 0.f;
		auto* Seed = Expr<UMaterialExpressionConstant>(M, -1600, 900); Seed->R = 0.f;
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* Glow = Expr<UMaterialExpressionMultiply>(M, -150, 60); Glow->A.Connect(0, Emis); Glow->B.Connect(0, Str);
		auto* RimGlow = Expr<UMaterialExpressionMultiply>(M, -50, 150); RimGlow->A.Connect(0, Glow); RimGlow->B.Connect(0, RimPlus);
		UMaterialExpressionCustom* S = SurfaceNode(M, Tx, Glow, Seed, WinS, TrS);
		WireSurface(M, S, Color, Met, Rgh, RimGlow);
		return SaveMat(Pkg, M);
	}
	bool MakeNeonInst(const FTexSet& Tx)
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_NeonInst"), Pkg);
		M->bUsedWithInstancedStaticMeshes = true;
		auto* Tint = Expr<UMaterialExpressionConstant3Vector>(M, -400, -600); Tint->Constant = FLinearColor(0.55f, 0.6f, 0.7f);
		UMaterialExpression* RGB = CustomRGB(M, 0); UMaterialExpression* CD3 = CustomScalar(M, 240, 3, 1.f); UMaterialExpression* Seed = CustomScalar(M, 320, 4, 0.f);
		auto* Wid = Expr<UMaterialExpressionConstant>(M, -1600, 450); Wid->R = 0.05f;
		auto* Fill = Expr<UMaterialExpressionConstant>(M, -400, 220); Fill->R = 0.04f;
		auto* WinS = Expr<UMaterialExpressionConstant>(M, -1600, 700); WinS->R = 1.f;
		auto* TrS = Expr<UMaterialExpressionConstant>(M, -1600, 800); TrS->R = 0.f;
		auto* Met = Expr<UMaterialExpressionConstant>(M, -400, 500); Met->R = 0.12f;
		auto* Rgh = Expr<UMaterialExpressionConstant>(M, -400, 600); Rgh->R = 1.f;
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* Glow = Expr<UMaterialExpressionMultiply>(M, -150, 60); Glow->A.Connect(0, RGB); Glow->B.Connect(0, CD3);
		auto* RimGlow = Expr<UMaterialExpressionMultiply>(M, -50, 150); RimGlow->A.Connect(0, Glow); RimGlow->B.Connect(0, RimPlus);
		UMaterialExpressionCustom* S = SurfaceNode(M, Tx, Glow, Seed, WinS, TrS);
		WireSurface(M, S, Tint, Met, Rgh, RimGlow);
		return SaveMat(Pkg, M);
	}
	bool MakeGlowInst()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_GlowInst"), Pkg); UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		M->bUsedWithInstancedStaticMeshes = true;
		auto* Base = Expr<UMaterialExpressionConstant3Vector>(M, -400, -200); Base->Constant = FLinearColor(0.02f, 0.02f, 0.03f);
		UMaterialExpression* RGB = CustomRGB(M, 0); UMaterialExpression* CD3 = CustomScalar(M, 240, 3, 1.f);
		auto* Glow = Expr<UMaterialExpressionMultiply>(M, -150, 60); Glow->A.Connect(0, RGB); Glow->B.Connect(0, CD3);
		auto* UV = Expr<UMaterialExpressionTextureCoordinate>(M, -400, 300);
		auto* Tm = Expr<UMaterialExpressionTime>(M, -400, 400);
		auto* C = Expr<UMaterialExpressionCustom>(M, -100, 200); C->Description = TEXT("Pulse"); C->OutputType = CMOT_Float3;
		C->Code = TEXT("float p = 0.55 + 0.45 * smoothstep(0.3, 0.7, frac(UV.x * 5.0 - T * 1.1));\nreturn Glow * p;");
		C->Inputs.Empty();
		{ FCustomInput I; I.InputName = TEXT("UV"); I.Input.Connect(0, UV); C->Inputs.Add(I); }
		{ FCustomInput I; I.InputName = TEXT("T"); I.Input.Connect(0, Tm); C->Inputs.Add(I); }
		{ FCustomInput I; I.InputName = TEXT("Glow"); I.Input.Connect(0, Glow); C->Inputs.Add(I); }
		auto* Rgh = Expr<UMaterialExpressionConstant>(M, -400, 600); Rgh->R = 0.4f;
		ED->BaseColor.Connect(0, Base); ED->EmissiveColor.Connect(0, C); ED->Roughness.Connect(0, Rgh);
		return SaveMat(Pkg, M);
	}
	bool MakeHolo()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_Holo"), Pkg); UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		M->bUsedWithInstancedStaticMeshes = true; M->BlendMode = BLEND_Translucent; M->SetShadingModel(MSM_Unlit); M->TwoSided = true;
		UMaterialExpression* RGB = CustomRGB(M, 0); UMaterialExpression* CD3 = CustomScalar(M, 240, 3, 0.3f);
		auto* Bright = Expr<UMaterialExpressionMultiply>(M, -200, 80); Bright->A.Connect(0, RGB); Bright->ConstB = 2.5f;
		ED->EmissiveColor.Connect(0, Bright); ED->Opacity.Connect(0, CD3);
		return SaveMat(Pkg, M);
	}

	bool MakeMesh(const TCHAR* Name, FMeshDescription MD)
	{
		UPackage* Pkg = NewPkg(TEXT("Meshes"), Name);
		UStaticMesh* SM = NewObject<UStaticMesh>(Pkg, Name, RF_Public | RF_Standalone);
		SM->InitResources();
		SM->SetLightingGuid(FGuid::NewGuid());
		FStaticMeshSourceModel& Src = SM->AddSourceModel();
		Src.BuildSettings.bRecomputeNormals = false; Src.BuildSettings.bRecomputeTangents = true; Src.BuildSettings.bUseMikkTSpace = true;
		Src.BuildSettings.bGenerateLightmapUVs = false; Src.BuildSettings.bRemoveDegenerates = true; Src.BuildSettings.bBuildReversedIndexBuffer = false;
		FMeshDescription* Dst = SM->CreateMeshDescription(0);
		*Dst = MoveTemp(MD);
		SM->CommitMeshDescription(0);
		SM->GetStaticMaterials().Add(FStaticMaterial(nullptr, TEXT("Default")));
		SM->ImportVersion = EImportStaticMeshVersion::LastVersion;
		SM->Build(true);
		SM->CreateBodySetup();
		if (UBodySetup* BS = SM->GetBodySetup()) { BS->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple; BS->InvalidatePhysicsData(); BS->CreatePhysicsMeshes(); }
		SM->PostEditChange();
		return SavePkg(Pkg, SM);
	}
}

int32 UGLAssetsCommandlet::Main(const FString& Params)
{
	TMap<FString, UTexture2D*> Tex; bool Ok = true;
	for (const FGLTextureKit::FEntry& E : FGLTextureKit::Library()) { UTexture2D* T = MakeTexture(E.Name, E.Build(E.Size)); Ok = Ok && T != nullptr; Tex.Add(E.Name, T); }
	FTexSet Tx{ Tex[TEXT("T_Concrete")], Tex[TEXT("T_ConcreteN")], Tex[TEXT("T_Grime")], Tex[TEXT("T_Windows")], Tex[TEXT("T_Traces")] };
	Ok = Ok && MakeNeon(Tx) && MakeNeonInst(Tx) && MakeGlowInst() && MakeHolo();
	for (const FGLMeshKit::FEntry& E : FGLMeshKit::Library()) Ok = MakeMesh(E.Name, E.Build()) && Ok;
	UE_LOG(LogTemp, Display, TEXT("GLAssets commandlet %s"), Ok ? TEXT("succeeded") : TEXT("FAILED"));
	return Ok ? 0 : 1;
}

#else
int32 UGLAssetsCommandlet::Main(const FString&) { return 1; }
#endif
