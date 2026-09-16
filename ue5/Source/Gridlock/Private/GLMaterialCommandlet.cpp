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

	struct FTexSet { UTexture2D* ConcA; UTexture2D* ConcM; UTexture2D* ConcN; UTexture2D* MetM; UTexture2D* MetN; UTexture2D* Grime; UTexture2D* Win; UTexture2D* Sign; UTexture2D* Traces; };

	// PBR surface: tri-planar concrete or metal (world-space normal), grime and puddles, lit flickering windows
	// with dark frames and spandrels, neon signage bands, circuit traces on unit-UV tops.
	static const TCHAR* kSurfaceHLSL = TEXT(R"HLSL(
float3 an = abs(N);
float3 w = an / max(an.x + an.y + an.z, 1e-4);
float sc = 1.0 / 300.0;
float sm = 1.0 / 160.0;
float3 cA = Texture2DSample(ConcA, ConcASampler, WP.yz * sc).rgb * w.x + Texture2DSample(ConcA, ConcASampler, WP.xz * sc).rgb * w.y + Texture2DSample(ConcA, ConcASampler, WP.xy * sc).rgb * w.z;
float3 cM = Texture2DSample(ConcM, ConcMSampler, WP.yz * sc).rgb * w.x + Texture2DSample(ConcM, ConcMSampler, WP.xz * sc).rgb * w.y + Texture2DSample(ConcM, ConcMSampler, WP.xy * sc).rgb * w.z;
float3 cnX = Texture2DSample(ConcN, ConcNSampler, WP.yz * sc).rgb * 2.0 - 1.0;
float3 cnY = Texture2DSample(ConcN, ConcNSampler, WP.xz * sc).rgb * 2.0 - 1.0;
float3 cnZ = Texture2DSample(ConcN, ConcNSampler, WP.xy * sc).rgb * 2.0 - 1.0;
float3 dnC = float3(0.0, cnX.x, cnX.y) * w.x + float3(cnY.x, 0.0, cnY.y) * w.y + float3(cnZ.x, cnZ.y, 0.0) * w.z;
float3 mM = Texture2DSample(MetM, MetMSampler, WP.yz * sm).rgb * w.x + Texture2DSample(MetM, MetMSampler, WP.xz * sm).rgb * w.y + Texture2DSample(MetM, MetMSampler, WP.xy * sm).rgb * w.z;
float3 mnX = Texture2DSample(MetN, MetNSampler, WP.yz * sm).rgb * 2.0 - 1.0;
float3 mnY = Texture2DSample(MetN, MetNSampler, WP.xz * sm).rgb * 2.0 - 1.0;
float3 mnZ = Texture2DSample(MetN, MetNSampler, WP.xy * sm).rgb * 2.0 - 1.0;
float3 dnM = float3(0.0, mnX.x, mnX.y) * w.x + float3(mnY.x, 0.0, mnY.y) * w.y + float3(mnZ.x, mnZ.y, 0.0) * w.z;
float3 mA = lerp(float3(0.38, 0.42, 0.5), float3(0.9, 0.75, 0.15), 1.0 - mM.b);
float3 alb = lerp(cA, mA * (0.6 + 0.4 * mM.r), MetalMix);
float ao = lerp(cM.r, mM.r, MetalMix);
float rough = lerp(cM.g, mM.g, MetalMix);
float metal = mM.b * MetalMix;
float3 dn = lerp(dnC, dnM, MetalMix);
float3 g = Texture2DSample(Grime, GrimeSampler, float2((WP.x + WP.y) / 1100.0, -WP.z / 900.0)).rgb;
alb *= lerp(0.55, 1.05, g.r);
rough = saturate(rough + (1.0 - g.r) * 0.18);
float puddle = saturate((g.g - 0.5) * 3.5) * Wet * saturate(N.z * 2.0 - 0.6);
rough = lerp(rough, 0.04, puddle);
alb = lerp(alb, alb * 0.45, puddle);
metal = max(metal, puddle * 0.3);
float wall = saturate((0.55 - an.z) * 5.0);
float u = (an.x > an.y) ? WP.y : WP.x;
float2 wuv = float2(u / 34.0, WP.z / 28.0) + Seed * float2(0.37, 0.61);
float4 win = Texture2DSample(Win, WinSampler, wuv);
float2 cid = floor(wuv);
float ch = frac(sin(dot(cid, float2(12.9898, 78.233)) + Seed * 3.1) * 43758.5453);
float flick = 0.82 + 0.18 * sin(T * (0.6 + ch * 2.5) + ch * 40.0);
float glowMag = max(max(Glow.r, Glow.g), max(Glow.b, 1e-3));
float3 glowN = Glow / glowMag;
float3 winCol = win.a < 0.33 ? float3(1.0, 0.82, 0.55) : (win.a < 0.66 ? glowN : float3(1.0, 0.35, 0.85));
float lit = win.r * win.g * wall * saturate(glowMag * 1.6) * WindowStrength * flick;
float frames = saturate(win.b) * wall * WindowStrength;
alb = lerp(alb, alb * 0.3, frames);
alb = lerp(alb, float3(0.02, 0.03, 0.05), win.r * wall * WindowStrength * 0.9);
rough = lerp(rough, 0.12, win.r * wall * WindowStrength);
float2 suv = float2(u / 120.0, WP.z / 60.0) + Seed * float2(0.71, 0.13);
float4 sg = Texture2DSample(Sign, SignSampler, suv);
float band = step(0.74, frac(WP.z / 150.0 + Seed * 0.53));
float3 sigCol = sg.g < 0.25 ? float3(0.0, 0.9, 1.0) : (sg.g < 0.5 ? float3(1.0, 0.2, 0.8) : (sg.g < 0.75 ? float3(1.0, 0.55, 0.1) : float3(0.5, 1.0, 0.2)));
float sig = sg.r * band * wall * SignStrength * saturate(glowMag * 1.4) * (0.75 + 0.25 * sin(T * 4.0 + Seed * 7.0 + sg.g * 20.0));
float2 ruv = UV - 0.5;
float cr = cos(TraceRot), sr = sin(TraceRot);
ruv = float2(ruv.x * cr - ruv.y * sr, ruv.x * sr + ruv.y * cr) + 0.5;
float2 trs = Texture2DSample(Traces, TracesSampler, ruv).rg;
float built = saturate((Develop - trs.g) * 12.0 + 0.5);          // elements appear in build order as the sector develops
float tr = trs.r * built * TraceStrength * saturate(an.z * 2.0);
Emis = winCol * lit * 3.0 + sigCol * sig * 4.0 + Glow * tr;
WNrm = normalize(N + dn * 0.65);
Metal = metal;
AO = ao;
return float4(alb, rough);
)HLSL");

	struct FSurfaceInputs { UMaterialExpression* Glow; UMaterialExpression* Seed; UMaterialExpression* WindowStrength; UMaterialExpression* TraceStrength; UMaterialExpression* SignStrength; UMaterialExpression* MetalMix; UMaterialExpression* Wet; UMaterialExpression* TraceRot; UMaterialExpression* Develop; };

	UMaterialExpressionCustom* SurfaceNode(UMaterial* M, const FTexSet& Tx, const FSurfaceInputs& In)
	{
		auto* WP = Expr<UMaterialExpressionWorldPosition>(M, -1900, -400);
		auto* N = Expr<UMaterialExpressionVertexNormalWS>(M, -1900, -300);
		auto* UV = Expr<UMaterialExpressionTextureCoordinate>(M, -1900, -200);
		auto* Tm = Expr<UMaterialExpressionTime>(M, -1900, -100);
		auto* C = Expr<UMaterialExpressionCustom>(M, -1100, -300);
		C->Description = TEXT("Surface"); C->Code = kSurfaceHLSL; C->OutputType = CMOT_Float4;
		C->Inputs.Empty();
		auto Add = [&](const TCHAR* Name, UMaterialExpression* E) { FCustomInput I; I.InputName = Name; I.Input.Connect(0, E); C->Inputs.Add(I); };
		Add(TEXT("WP"), WP); Add(TEXT("N"), N); Add(TEXT("UV"), UV); Add(TEXT("T"), Tm);
		Add(TEXT("Glow"), In.Glow); Add(TEXT("Seed"), In.Seed); Add(TEXT("WindowStrength"), In.WindowStrength); Add(TEXT("TraceStrength"), In.TraceStrength);
		Add(TEXT("SignStrength"), In.SignStrength); Add(TEXT("MetalMix"), In.MetalMix); Add(TEXT("Wet"), In.Wet); Add(TEXT("TraceRot"), In.TraceRot); Add(TEXT("Develop"), In.Develop);
		Add(TEXT("ConcA"), TexObj(M, -900, Tx.ConcA)); Add(TEXT("ConcM"), TexObj(M, -800, Tx.ConcM)); Add(TEXT("ConcN"), TexObj(M, -700, Tx.ConcN));
		Add(TEXT("MetM"), TexObj(M, -600, Tx.MetM)); Add(TEXT("MetN"), TexObj(M, -500, Tx.MetN));
		Add(TEXT("Grime"), TexObj(M, -400, Tx.Grime)); Add(TEXT("Win"), TexObj(M, -300, Tx.Win)); Add(TEXT("Sign"), TexObj(M, -200, Tx.Sign)); Add(TEXT("Traces"), TexObj(M, -100, Tx.Traces));
		auto Out = [&](const TCHAR* Name, ECustomMaterialOutputType Type) { FCustomOutput O; O.OutputName = Name; O.OutputType = Type; C->AdditionalOutputs.Add(O); };
		Out(TEXT("Emis"), CMOT_Float3); Out(TEXT("WNrm"), CMOT_Float3); Out(TEXT("Metal"), CMOT_Float1); Out(TEXT("AO"), CMOT_Float1);
		C->RebuildOutputs();
		return C;
	}

	// Wires the surface node into the material outputs: outputs 0 main (albedo, rough), 1 Emis, 2 WNrm, 3 Metal, 4 AO.
	void WireSurface(UMaterial* M, UMaterialExpressionCustom* S, UMaterialExpression* Tint, UMaterialExpression* MetalAdd, UMaterialExpression* Rough, UMaterialExpression* RimGlow)
	{
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		auto* Alb = Expr<UMaterialExpressionComponentMask>(M, -700, -400); Alb->Input.Connect(0, S); Alb->R = true; Alb->G = true; Alb->B = true; Alb->A = false;
		auto* Rgh = Expr<UMaterialExpressionComponentMask>(M, -700, -250); Rgh->Input.Connect(0, S); Rgh->R = false; Rgh->G = false; Rgh->B = false; Rgh->A = true;
		auto* Base = Expr<UMaterialExpressionMultiply>(M, -500, -400); Base->A.Connect(0, Alb); Base->B.Connect(0, Tint);
		auto* RoughOut = Expr<UMaterialExpressionMultiply>(M, -500, -250); RoughOut->A.Connect(0, Rgh); RoughOut->B.Connect(0, Rough);
		auto* EmisOut = Expr<UMaterialExpressionAdd>(M, 0, 150); EmisOut->A.Connect(0, RimGlow); EmisOut->B.Connect(1, S);
		auto* MetalOut = Expr<UMaterialExpressionAdd>(M, -500, -100); MetalOut->A.Connect(3, S); MetalOut->B.Connect(0, MetalAdd);
		auto* MetalSat = Expr<UMaterialExpressionSaturate>(M, -350, -100); MetalSat->Input.Connect(0, MetalOut);
		ED->BaseColor.Connect(0, Base); ED->Roughness.Connect(0, RoughOut); ED->Metallic.Connect(0, MetalSat); ED->EmissiveColor.Connect(0, EmisOut); ED->Normal.Connect(2, S); ED->AmbientOcclusion.Connect(4, S);
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
		auto* SgS = Expr<UMaterialExpressionScalarParameter>(M, -1600, 1000); SgS->ParameterName = TEXT("SignStrength"); SgS->DefaultValue = 0.f;
		auto* MMix = Expr<UMaterialExpressionScalarParameter>(M, -1600, 1100); MMix->ParameterName = TEXT("MetalMix"); MMix->DefaultValue = 0.f;
		auto* Wet = Expr<UMaterialExpressionScalarParameter>(M, -1600, 1200); Wet->ParameterName = TEXT("Wet"); Wet->DefaultValue = 0.f;
		auto* TRot = Expr<UMaterialExpressionScalarParameter>(M, -1600, 1300); TRot->ParameterName = TEXT("TraceRot"); TRot->DefaultValue = 0.f;
		auto* Dev = Expr<UMaterialExpressionScalarParameter>(M, -1600, 1400); Dev->ParameterName = TEXT("Develop"); Dev->DefaultValue = 1.f;
		auto* Seed = Expr<UMaterialExpressionConstant>(M, -1600, 900); Seed->R = 0.f;
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* Glow = Expr<UMaterialExpressionMultiply>(M, -150, 60); Glow->A.Connect(0, Emis); Glow->B.Connect(0, Str);
		auto* RimGlow = Expr<UMaterialExpressionMultiply>(M, -50, 150); RimGlow->A.Connect(0, Glow); RimGlow->B.Connect(0, RimPlus);
		UMaterialExpressionCustom* S = SurfaceNode(M, Tx, FSurfaceInputs{ Glow, Seed, WinS, TrS, SgS, MMix, Wet, TRot, Dev });
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
		auto* SgS = Expr<UMaterialExpressionConstant>(M, -1600, 1000); SgS->R = 1.f;
		auto* MMix = Expr<UMaterialExpressionConstant>(M, -1600, 1100); MMix->R = 0.1f;
		auto* Wet = Expr<UMaterialExpressionConstant>(M, -1600, 1200); Wet->R = 0.f;
		auto* TRot = Expr<UMaterialExpressionConstant>(M, -1600, 1300); TRot->R = 0.f;
		auto* Dev = Expr<UMaterialExpressionConstant>(M, -1600, 1400); Dev->R = 1.f;
		auto* Met = Expr<UMaterialExpressionConstant>(M, -400, 500); Met->R = 0.02f;
		auto* Rgh = Expr<UMaterialExpressionConstant>(M, -400, 600); Rgh->R = 1.f;
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* Glow = Expr<UMaterialExpressionMultiply>(M, -150, 60); Glow->A.Connect(0, RGB); Glow->B.Connect(0, CD3);
		auto* RimGlow = Expr<UMaterialExpressionMultiply>(M, -50, 150); RimGlow->A.Connect(0, Glow); RimGlow->B.Connect(0, RimPlus);
		UMaterialExpressionCustom* S = SurfaceNode(M, Tx, FSurfaceInputs{ Glow, Seed, WinS, TrS, SgS, MMix, Wet, TRot, Dev });
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
	FTexSet Tx{ Tex[TEXT("T_ConcreteA")], Tex[TEXT("T_ConcreteM")], Tex[TEXT("T_ConcreteN")], Tex[TEXT("T_MetalM")], Tex[TEXT("T_MetalN")], Tex[TEXT("T_Grime")], Tex[TEXT("T_Windows")], Tex[TEXT("T_Signage")], Tex[TEXT("T_Traces")] };
	Ok = Ok && MakeNeon(Tx) && MakeNeonInst(Tx) && MakeGlowInst() && MakeHolo();
	for (const FGLMeshKit::FEntry& E : FGLMeshKit::Library()) Ok = MakeMesh(E.Name, E.Build()) && Ok;
	UE_LOG(LogTemp, Display, TEXT("GLAssets commandlet %s"), Ok ? TEXT("succeeded") : TEXT("FAILED"));
	return Ok ? 0 : 1;
}

#else
int32 UGLAssetsCommandlet::Main(const FString&) { return 1; }
#endif
