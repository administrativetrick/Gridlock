#include "GLMaterialCommandlet.h"
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
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

UGLMaterialCommandlet::UGLMaterialCommandlet()
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

	// rim(uv) = saturate((max(|u-0.5|,|v-0.5|) - (0.5 - w)) / w): 1 on the edge of every face, 0 inside.
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

	bool Save(UPackage* Pkg, UMaterial* M)
	{
		M->PreEditChange(nullptr);
		M->PostEditChange();
		Pkg->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
		const bool Ok = UPackage::SavePackage(Pkg, M, *File, Args);
		UE_LOG(LogTemp, Display, TEXT("%s %s"), Ok ? TEXT("Saved") : TEXT("FAILED to save"), *File);
		return Ok;
	}

	UMaterial* NewMat(const TCHAR* Name, UPackage*& Pkg)
	{
		const FString Path = FString::Printf(TEXT("/Game/Materials/%s"), Name);
		Pkg = CreatePackage(*Path);
		Pkg->FullyLoad();
		UMaterial* M = NewObject<UMaterial>(Pkg, Name, RF_Public | RF_Standalone);
		return M;
	}

	bool MakeNeon()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_Neon"), Pkg);
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		auto* Color = Expr<UMaterialExpressionVectorParameter>(M, -400, -200); Color->ParameterName = TEXT("Color"); Color->DefaultValue = FLinearColor(0.05f, 0.06f, 0.08f);
		auto* Emis = Expr<UMaterialExpressionVectorParameter>(M, -400, 0); Emis->ParameterName = TEXT("Emissive"); Emis->DefaultValue = FLinearColor(0.f, 0.9f, 1.f);
		auto* Str = Expr<UMaterialExpressionScalarParameter>(M, -400, 120); Str->ParameterName = TEXT("EmissiveStrength"); Str->DefaultValue = 1.f;
		auto* Met = Expr<UMaterialExpressionScalarParameter>(M, -400, 500); Met->ParameterName = TEXT("Metallic"); Met->DefaultValue = 0.2f;
		auto* Rgh = Expr<UMaterialExpressionScalarParameter>(M, -400, 600); Rgh->ParameterName = TEXT("Roughness"); Rgh->DefaultValue = 0.7f;
		auto* Wid = Expr<UMaterialExpressionScalarParameter>(M, -1600, 450); Wid->ParameterName = TEXT("RimWidth"); Wid->DefaultValue = 0.06f;
		auto* Fill = Expr<UMaterialExpressionScalarParameter>(M, -400, 220); Fill->ParameterName = TEXT("Fill"); Fill->DefaultValue = 0.04f;   // faint glow inside the face
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* ES = Expr<UMaterialExpressionMultiply>(M, -150, 60); ES->A.Connect(0, Emis); ES->B.Connect(0, Str);
		auto* Out = Expr<UMaterialExpressionMultiply>(M, 0, 150); Out->A.Connect(0, ES); Out->B.Connect(0, RimPlus);
		ED->BaseColor.Connect(0, Color);
		ED->EmissiveColor.Connect(0, Out);
		ED->Metallic.Connect(0, Met);
		ED->Roughness.Connect(0, Rgh);
		return Save(Pkg, M);
	}

	bool MakeNeonInst()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_NeonInst"), Pkg);
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		M->bUsedWithInstancedStaticMeshes = true;
		auto* Base = Expr<UMaterialExpressionConstant3Vector>(M, -400, -200); Base->Constant = FLinearColor(0.035f, 0.04f, 0.055f);
		auto* CD0 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 0); CD0->DataIndex = 0; CD0->ConstDefaultValue = 0.f;
		auto* CD1 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 80); CD1->DataIndex = 1; CD1->ConstDefaultValue = 0.9f;
		auto* CD2 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 160); CD2->DataIndex = 2; CD2->ConstDefaultValue = 1.f;
		auto* CD3 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 240); CD3->DataIndex = 3; CD3->ConstDefaultValue = 1.f;
		auto* RG = Expr<UMaterialExpressionAppendVector>(M, -550, 40); RG->A.Connect(0, CD0); RG->B.Connect(0, CD1);
		auto* RGB = Expr<UMaterialExpressionAppendVector>(M, -400, 80); RGB->A.Connect(0, RG); RGB->B.Connect(0, CD2);
		auto* Wid = Expr<UMaterialExpressionConstant>(M, -1600, 450); Wid->R = 0.07f;
		auto* Fill = Expr<UMaterialExpressionConstant>(M, -400, 220); Fill->R = 0.05f;
		UMaterialExpression* Rim = BuildRim(M, Wid);
		auto* RimPlus = Expr<UMaterialExpressionAdd>(M, -150, 300); RimPlus->A.Connect(0, Rim); RimPlus->B.Connect(0, Fill);
		auto* ES = Expr<UMaterialExpressionMultiply>(M, -150, 60); ES->A.Connect(0, RGB); ES->B.Connect(0, CD3);
		auto* Out = Expr<UMaterialExpressionMultiply>(M, 0, 150); Out->A.Connect(0, ES); Out->B.Connect(0, RimPlus);
		auto* Met = Expr<UMaterialExpressionConstant>(M, -400, 500); Met->R = 0.15f;
		auto* Rgh = Expr<UMaterialExpressionConstant>(M, -400, 600); Rgh->R = 0.75f;
		ED->BaseColor.Connect(0, Base);
		ED->EmissiveColor.Connect(0, Out);
		ED->Metallic.Connect(0, Met);
		ED->Roughness.Connect(0, Rgh);
		return Save(Pkg, M);
	}

	// Uniform glow (no rim) from custom data: fiber, packets.
	bool MakeGlowInst()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_GlowInst"), Pkg);
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		M->bUsedWithInstancedStaticMeshes = true;
		auto* Base = Expr<UMaterialExpressionConstant3Vector>(M, -400, -200); Base->Constant = FLinearColor(0.02f, 0.02f, 0.03f);
		auto* CD0 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 0); CD0->DataIndex = 0; CD0->ConstDefaultValue = 0.f;
		auto* CD1 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 80); CD1->DataIndex = 1; CD1->ConstDefaultValue = 0.9f;
		auto* CD2 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 160); CD2->DataIndex = 2; CD2->ConstDefaultValue = 1.f;
		auto* CD3 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 240); CD3->DataIndex = 3; CD3->ConstDefaultValue = 1.f;
		auto* RG = Expr<UMaterialExpressionAppendVector>(M, -550, 40); RG->A.Connect(0, CD0); RG->B.Connect(0, CD1);
		auto* RGB = Expr<UMaterialExpressionAppendVector>(M, -400, 80); RGB->A.Connect(0, RG); RGB->B.Connect(0, CD2);
		auto* Out = Expr<UMaterialExpressionMultiply>(M, -150, 60); Out->A.Connect(0, RGB); Out->B.Connect(0, CD3);
		auto* Rgh = Expr<UMaterialExpressionConstant>(M, -400, 600); Rgh->R = 0.4f;
		ED->BaseColor.Connect(0, Base);
		ED->EmissiveColor.Connect(0, Out);
		ED->Roughness.Connect(0, Rgh);
		return Save(Pkg, M);
	}

	bool MakeHolo()
	{
		UPackage* Pkg; UMaterial* M = NewMat(TEXT("M_Holo"), Pkg);
		UMaterialEditorOnlyData* ED = M->GetEditorOnlyData();
		M->bUsedWithInstancedStaticMeshes = true;
		M->BlendMode = BLEND_Translucent;
		M->SetShadingModel(MSM_Unlit);
		M->TwoSided = true;
		auto* CD0 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 0); CD0->DataIndex = 0; CD0->ConstDefaultValue = 0.f;
		auto* CD1 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 80); CD1->DataIndex = 1; CD1->ConstDefaultValue = 0.9f;
		auto* CD2 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 160); CD2->DataIndex = 2; CD2->ConstDefaultValue = 1.f;
		auto* CD3 = Expr<UMaterialExpressionPerInstanceCustomData>(M, -700, 240); CD3->DataIndex = 3; CD3->ConstDefaultValue = 0.3f;
		auto* RG = Expr<UMaterialExpressionAppendVector>(M, -550, 40); RG->A.Connect(0, CD0); RG->B.Connect(0, CD1);
		auto* RGB = Expr<UMaterialExpressionAppendVector>(M, -400, 80); RGB->A.Connect(0, RG); RGB->B.Connect(0, CD2);
		auto* Bright = Expr<UMaterialExpressionMultiply>(M, -200, 80); Bright->A.Connect(0, RGB); Bright->ConstB = 2.5f;
		ED->EmissiveColor.Connect(0, Bright);
		ED->Opacity.Connect(0, CD3);
		return Save(Pkg, M);
	}
}

int32 UGLMaterialCommandlet::Main(const FString& Params)
{
	const bool Ok = MakeNeon() && MakeNeonInst() && MakeGlowInst() && MakeHolo();
	UE_LOG(LogTemp, Display, TEXT("GLMaterial commandlet %s"), Ok ? TEXT("succeeded") : TEXT("FAILED"));
	return Ok ? 0 : 1;
}

#else
int32 UGLMaterialCommandlet::Main(const FString&) { return 1; }
#endif
