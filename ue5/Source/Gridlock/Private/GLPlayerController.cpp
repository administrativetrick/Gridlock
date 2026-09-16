#include "GLPlayerController.h"
#include "GLGameMode.h"
#include "GLMapActor.h"
#include "GLCameraPawn.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

static FString S2F(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }

AGLPlayerController::AGLPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

AGLGameMode* AGLPlayerController::GM() const { return Cast<AGLGameMode>(UGameplayStatics::GetGameMode(this)); }

void AGLPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	Recenter();
}

void AGLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGLPlayerController::OnClick);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGLPlayerController::OnCancel);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AGLPlayerController::OnEndCycle);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AGLPlayerController::OnEndCycle);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AGLPlayerController::OnCancel);
	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AGLPlayerController::ToggleHelp);
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AGLPlayerController::ToggleBoard);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AGLPlayerController::ToggleResearch);
	InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AGLPlayerController::ToggleDoctrine);
	InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AGLPlayerController::SetLay);
	InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AGLPlayerController::TogglePriorityKey);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AGLPlayerController::BuyBW);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AGLPlayerController::ToggleDeny);
	InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &AGLPlayerController::Recenter);
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AGLPlayerController::ZoomIn);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AGLPlayerController::ZoomOut);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Node, 1>);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Node, 2>);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Node, 3>);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Substation, 0>);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Repeater, 0>);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Rack, 0>);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Outpost, 0>);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Array, 0>);
	InputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Lab, 0>);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AGLPlayerController::BuildKey<(int32)gl::StructKind::Honeypot, 0>);
	InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Intrusion>);
	InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Harden>);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Purge>);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Siphon>);
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Jam>);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Root>);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::DDoS>);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Scan>);
	InputComponent->BindKey(EKeys::N, IE_Pressed, this, &AGLPlayerController::OpKey<(int32)gl::OpKind::Daemon>);
	InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AGLPlayerController::VerbKey<0>);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AGLPlayerController::VerbKey<1>);
	InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AGLPlayerController::VerbKey<2>);
	InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &AGLPlayerController::VerbKey<3>);
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AGLPlayerController::VerbKey<4>);
	InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AGLPlayerController::VerbKey<5>);
	InputComponent->BindKey(EKeys::F11, IE_Pressed, this, &AGLPlayerController::VerbKey<6>);
	InputComponent->BindKey(EKeys::F12, IE_Pressed, this, &AGLPlayerController::VerbKey<7>);
	InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AGLPlayerController::Pan<0, true>);   InputComponent->BindKey(EKeys::Up, IE_Released, this, &AGLPlayerController::Pan<0, false>);
	InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AGLPlayerController::Pan<1, true>); InputComponent->BindKey(EKeys::Down, IE_Released, this, &AGLPlayerController::Pan<1, false>);
	InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AGLPlayerController::Pan<2, true>); InputComponent->BindKey(EKeys::Left, IE_Released, this, &AGLPlayerController::Pan<2, false>);
	InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AGLPlayerController::Pan<3, true>);InputComponent->BindKey(EKeys::Right, IE_Released, this, &AGLPlayerController::Pan<3, false>);
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AGLPlayerController::Pan<0, true>);    InputComponent->BindKey(EKeys::W, IE_Released, this, &AGLPlayerController::Pan<0, false>);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AGLPlayerController::Pan<1, true>);    InputComponent->BindKey(EKeys::S, IE_Released, this, &AGLPlayerController::Pan<1, false>);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AGLPlayerController::Pan<2, true>);    InputComponent->BindKey(EKeys::A, IE_Released, this, &AGLPlayerController::Pan<2, false>);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AGLPlayerController::Pan<3, true>);    InputComponent->BindKey(EKeys::E, IE_Released, this, &AGLPlayerController::Pan<3, false>);
}

void AGLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (AGLCameraPawn* P = Cast<AGLCameraPawn>(GetPawn()))
	{
		const float Speed = 0.9f * P->GetZoom();
		FVector D(0);
		if (PanState[0]) D.Y += 1; if (PanState[1]) D.Y -= 1; if (PanState[2]) D.X -= 1; if (PanState[3]) D.X += 1;
		if (!D.IsNearlyZero()) P->AddActorWorldOffset(D.GetSafeNormal() * Speed * DeltaTime);
	}
}

void AGLPlayerController::Report(const gl::Result& R) { LastMsg = S2F(R.msg); Sync(); }
void AGLPlayerController::Sync() { if (AGLGameMode* G = GM()) { if (G->Map) { G->Map->Selected = Sel; G->Map->FromHex = FromHex; } G->MarkDirty(); } }

FString AGLPlayerController::ModeText() const
{
	switch (Mode)
	{
	case EGLMode::Lay: return FromHex < 0 ? TEXT("LAY FIBER - click a hex in your network to start") : TEXT("LAY FIBER - click the destination hex");
	case EGLMode::Build: return FString::Printf(TEXT("BUILD %s - click one of your hexes"), BuildKind == (int32)gl::StructKind::Node ? UTF8_TO_TCHAR(gl::nodeDef(BuildTier).name) : UTF8_TO_TCHAR(gl::structDef((gl::StructKind)BuildKind).name));
	case EGLMode::Op: return FString::Printf(TEXT("OP %s - click the target hex"), UTF8_TO_TCHAR(gl::opDef((gl::OpKind)OpKind).name));
	default: return TEXT("SELECT - click a hex to inspect it");
	}
}

void AGLPlayerController::OnClick()
{
	AGLGameMode* G = GM(); if (!G || !G->Map) return;
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return;
	const int32 Hex = G->Map->HexFromComponent(Hit.GetComponent());
	if (Hex < 0) return;
	HandleHexClick(Hex);
}

void AGLPlayerController::HandleHexClick(int32 Hex)
{
	AGLGameMode* G = GM(); gl::Game& Sim = G->Sim(); const int32 Me = G->Me();
	switch (Mode)
	{
	case EGLMode::Select: Sel = Hex; LastMsg.Empty(); Sync(); break;
	case EGLMode::Lay:
		if (FromHex < 0)
		{
			const gl::Hex& H = Sim.S().hexes[Hex]; bool InNet = H.owner == Me;
			for (const gl::Link& L : Sim.S().links) if (L.alive && L.sid == Me) for (int P : L.path) if (P == Hex) InNet = true;
			for (const gl::Structure& St : Sim.S().structs) if (St.alive && St.sid == Me && St.hex == Hex) InNet = true;
			if (!InNet) { LastMsg = TEXT("Start from a hex in your network."); Sync(); return; }
			FromHex = Hex; Sel = Hex; Sync();
		}
		else
		{
			const gl::Syndicate& S0 = Sim.S().synds[Me];
			gl::LinkType LT = S0.techs[(int)gl::Tech::BackboneFiber] ? gl::LinkType::Backbone : gl::LinkType::Trunk;
			gl::PathPlan P = Sim.PlanPath(Me, FromHex, Hex, LT);
			if (!P.ok) LastMsg = S2F(P.msg);
			else
			{
				bool Ok = true; for (int R : P.needRights) { gl::Result RR = Sim.BuyRights(Me, R); if (!RR) { LastMsg = S2F(RR.msg); Ok = false; break; } }
				if (Ok) { gl::Result R = Sim.LayLink(Me, LT, P.path); LastMsg = S2F(R.msg) + FString::Printf(TEXT("  (projected survival %.0f%%)"), P.projectedSurvival * 100.0); }
			}
			FromHex = -1; Sel = Hex; Mode = EGLMode::Select; Sync();
		}
		break;
	case EGLMode::Build:
	{
		gl::Variant V = gl::Variant::None; const gl::Syndicate& S0 = Sim.S().synds[Me];
		if (BuildKind == (int32)gl::StructKind::Node && BuildTier == 3 && S0.arch == gl::Arch::Hive) V = gl::Variant::Inference;
		if (BuildKind == (int32)gl::StructKind::Node && BuildTier == 2 && S0.arch == gl::Arch::Ghost) V = gl::Variant::Phantom;
		Report(Sim.Build(Me, (gl::StructKind)BuildKind, Hex, BuildTier, V));
		Sel = Hex; Mode = EGLMode::Select; Sync();
		break;
	}
	case EGLMode::Op:
		Report(Sim.QueueOp(Me, (gl::OpKind)OpKind, Hex));
		Sel = Hex; Mode = EGLMode::Select; Sync();
		break;
	}
}

void AGLPlayerController::OnEndCycle() { if (AGLGameMode* G = GM()) { G->EndCycle(); LastMsg = FString::Printf(TEXT("Cycle %d resolved."), G->Sim().S().cycle); Sync(); } }
void AGLPlayerController::OnCancel() { Mode = EGLMode::Select; FromHex = -1; LastMsg.Empty(); bBoard = bResearch = bDoctrine = false; Sync(); }
void AGLPlayerController::SetLay() { Mode = EGLMode::Lay; FromHex = -1; LastMsg.Empty(); Sync(); }
void AGLPlayerController::SetBuild(int32 Kind, int32 Tier) { Mode = EGLMode::Build; BuildKind = Kind; BuildTier = Tier; LastMsg.Empty(); Sync(); }
void AGLPlayerController::SetOp(int32 Kind) { Mode = EGLMode::Op; OpKind = Kind; LastMsg.Empty(); Sync(); }

void AGLPlayerController::AutoQueueTech()
{
	AGLGameMode* G = GM(); gl::Game& Sim = G->Sim();
	static const gl::Tech Order[] = { gl::Tech::GridContracts, gl::Tech::Trenching, gl::Tech::HardenedKernels, gl::Tech::RedundantPeering, gl::Tech::DPI, gl::Tech::ShellCompanies, gl::Tech::BackboneFiber, gl::Tech::ModularDC, gl::Tech::FuturesDesk, gl::Tech::Persistence, gl::Tech::FieldTeams, gl::Tech::Honeypots, gl::Tech::ZeroDay, gl::Tech::VerticalIntegration, gl::Tech::Lobbying, gl::Tech::HyperscaleCooling, gl::Tech::Buyback, gl::Tech::TenderOffer, gl::Tech::KillChain, gl::Tech::ConsolidationLobby, gl::Tech::BackboneSniffing, gl::Tech::SabotageDoctrine, gl::Tech::BlackoutProtocol };
	for (gl::Tech T : Order) if (Sim.CanResearch(G->Me(), T)) { Report(Sim.QueueResearch(G->Me(), T)); return; }
	LastMsg = TEXT("Nothing researchable right now."); Sync();
}
void AGLPlayerController::QueueTechAt(int32 I) { if (AGLGameMode* G = GM()) Report(G->Sim().QueueResearch(G->Me(), (gl::Tech)I)); }
void AGLPlayerController::ClearResearch() { if (AGLGameMode* G = GM()) Report(G->Sim().ClearResearch(G->Me())); }
void AGLPlayerController::TakeDoctrineAt(int32 I) { if (AGLGameMode* G = GM()) Report(G->Sim().TakeDoctrine(G->Me(), (gl::Doctrine)I)); }
void AGLPlayerController::AutoDoctrine()
{
	AGLGameMode* G = GM(); gl::Game& Sim = G->Sim();
	for (int i = 0; i < gl::DoctrineCount; ++i) if (Sim.CanTakeDoctrine(G->Me(), (gl::Doctrine)i)) { Report(Sim.TakeDoctrine(G->Me(), (gl::Doctrine)i)); return; }
	LastMsg = FString::Printf(TEXT("No doctrine available (Mandate %d; Board Review every 12 cycles)."), Sim.S().synds[G->Me()].mandate); Sync();
}
void AGLPlayerController::SetSelPriority(gl::Priority P) { AGLGameMode* G = GM(); if (Sel < 0) { LastMsg = TEXT("Select one of your hexes first."); Sync(); return; } Report(G->Sim().SetPriority(G->Me(), Sel, P)); }
void AGLPlayerController::TogglePriorityKey()
{
	AGLGameMode* G = GM(); if (Sel < 0) { LastMsg = TEXT("Select one of your hexes first."); Sync(); return; }
	const gl::Hex& H = G->Sim().S().hexes[Sel]; auto It = H.priority.find(G->Me());
	gl::Priority Next = (It == H.priority.end() || It->second == gl::Priority::Normal) ? gl::Priority::Critical : It->second == gl::Priority::Critical ? gl::Priority::Low : gl::Priority::Normal;
	SetSelPriority(Next);
}
void AGLPlayerController::BuyBW() { if (AGLGameMode* G = GM()) Report(G->Sim().BuyBandwidth(G->Me(), 10)); }
void AGLPlayerController::ToggleDeny() { AGLGameMode* G = GM(); if (Sel < 0) return; const gl::Hex& H = G->Sim().S().hexes[Sel]; Report(G->Sim().SetRowDenial(G->Me(), Sel, !H.rowDenial)); }
void AGLPlayerController::Recenter()
{
	AGLGameMode* G = GM(); if (!G || !G->Map) return;
	if (APawn* P = GetPawn()) { const gl::GameState& S = G->Sim().S(); P->SetActorLocation(G->Map->HexWorld(S.synds[G->Me()].crown)); }
}
void AGLPlayerController::ZoomIn() { if (AGLCameraPawn* P = Cast<AGLCameraPawn>(GetPawn())) P->SetZoom(P->GetZoom() * 0.85f); }
void AGLPlayerController::ZoomOut() { if (AGLCameraPawn* P = Cast<AGLCameraPawn>(GetPawn())) P->SetZoom(P->GetZoom() / 0.85f); }

void AGLPlayerController::Verb(int32 Which)
{
	AGLGameMode* G = GM(); gl::Game& Sim = G->Sim(); const int32 Me = G->Me();
	int32 Rival = -1;
	for (const gl::Syndicate& S0 : Sim.S().synds) if (S0.alive && S0.id != Me && S0.hasPhase) { Rival = S0.id; break; }
	switch (Which)
	{
	case 0: Report(Sim.TenderOffer(Me)); break;
	case 1: Report(Sim.BuyShares(Me, 1)); break;
	case 2: Report(Sim.PoisonPill(Me)); break;
	case 3: Report(Sim.StartTraining(Me)); break;
	case 4: Report(Sim.Launch(Me)); break;
	case 5: Report(Sim.DeclareBlackout(Me)); break;
	case 6: Report(Sim.ConsolidationMotion(Me)); break;
	case 7: Report(Rival >= 0 ? Sim.PetitionKillSwitch(Me, Rival) : gl::Result::Err("No rival is launching.")); break;
	case 8: Report(Rival >= 0 ? Sim.Complaint(Me, Rival) : gl::Result::Err("No rival is in a fight or vote.")); break;
	default: break;
	}
}
