#include "GLHUD.h"
#include "GLGameMode.h"
#include "GLPlayerController.h"
#include "GLMapActor.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

static FString S2F(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }
static const FLinearColor kInk(0.80f, 0.90f, 1.00f);
static const FLinearColor kDim(0.45f, 0.52f, 0.62f);
static const FLinearColor kWarn(1.00f, 0.55f, 0.20f);
static const FLinearColor kBad(1.00f, 0.25f, 0.25f);

void AGLHUD::Panel(float X, float Y, float W, float H)
{
	DrawRect(FLinearColor(0.02f, 0.03f, 0.06f, 0.82f), X, Y, W, H);
	DrawRect(FLinearColor(0.20f, 0.60f, 0.80f, 0.6f), X, Y, W, 1.f);
	DrawRect(FLinearColor(0.20f, 0.60f, 0.80f, 0.6f), X, Y + H - 1.f, W, 1.f);
}

float AGLHUD::Line(float X, float& Y, const FString& S, const FLinearColor& C, UFont* F, float Scale)
{
	DrawText(S, C, X, Y, F, Scale);
	Y += 14.f * Scale;
	return Y;
}

void AGLHUD::DrawHUD()
{
	Super::DrawHUD();
	AGLGameMode* GM = Cast<AGLGameMode>(UGameplayStatics::GetGameMode(this));
	AGLPlayerController* PC = Cast<AGLPlayerController>(GetOwningPlayerController());
	if (!GM || !PC || !Canvas) return;
	UFont* F = GEngine->GetSmallFont();
	DrawTopBar(GM, PC, F);
	DrawInspector(GM, PC, F);
	DrawLog(GM, F);
	if (PC->bHelp) DrawHelp(F);
	if (PC->bBoard) DrawBoard(GM, F);
	const gl::GameState& S = GM->Sim().S();
	if (S.over)
	{
		const float W = Canvas->SizeX, H = Canvas->SizeY;
		Panel(W * 0.25f, H * 0.4f, W * 0.5f, 70.f);
		float Y = H * 0.4f + 12.f;
		Line(W * 0.25f + 16.f, Y, FString::Printf(TEXT("GAME OVER - cycle %d"), S.victory.cycle), kInk, F, 1.6f);
		Line(W * 0.25f + 16.f, Y, FString::Printf(TEXT("%s wins by %s"), *S2F(S.synds[S.victory.winner].name), UTF8_TO_TCHAR(gl::pathName(S.victory.path))), AGLMapActor::SyndColor(S.victory.winner), F, 1.4f);
	}
}

void AGLHUD::DrawTopBar(AGLGameMode* GM, AGLPlayerController* PC, UFont* F)
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const gl::Syndicate& Me = S.synds[GM->Me()]; const gl::Mods M = G.ModsOf(GM->Me());
	const float W = Canvas->SizeX;
	Panel(0, 0, W, 62.f);
	float Y = 6.f;
	int Sectors = 0; for (auto& H : S.hexes) if (H.owner == Me.id) ++Sectors;
	Line(10.f, Y, FString::Printf(TEXT("GRIDLOCK: SILICON SYNDICATE     cycle %d / %d     %s  (%s)     sectors %d"), S.cycle, gl::K::GameEnd, *S2F(Me.name), UTF8_TO_TCHAR(gl::archDef(Me.arch).name), Sectors), AGLMapActor::SyndColor(Me.id), F, 1.25f);
	Y += 4.f;
	Line(10.f, Y, FString::Printf(TEXT("Capital %.0f  (upkeep %.0f/cycle)   BW produced %.1f  delivered %.1f  lost %.1f  ops %.1f/%.1f  compute %.1f  sold %.1f  buffer %.1f   Exposure %.0f%s   Share %.1f%%   Mandate %d   Slots %d/%d%s"),
		Me.capital, G.UpkeepOf(Me.id), Me.flow.produced, Me.flow.delivered, Me.flow.lost, Me.flow.opsPool, Me.flow.opsNeed, Me.flow.compute, Me.flow.sold, Me.buffer,
		Me.exposure, Me.exposure >= 60 ? TEXT(" AUDIT RISK") : Me.exposure >= 40 ? TEXT(" noticed") : TEXT(""), Me.share * 100.0, Me.mandate, Me.slotsUsed, M.slots,
		Me.arch == gl::Arch::Hive ? *FString::Printf(TEXT("   M %.0f"), Me.M) : TEXT("")), kInk, F);
	FString Status = PC->ModeText();
	if (!PC->LastMsg.IsEmpty()) Status += TEXT("      > ") + PC->LastMsg;
	Line(10.f, Y, Status, PC->LastMsg.IsEmpty() ? kDim : kWarn, F, 1.1f);
	if (Me.hasPhase) Line(10.f, Y, FString::Printf(TEXT("ACTIVE PHASE: %s, %d cycles left"), UTF8_TO_TCHAR(gl::pathName(Me.phase.type)), Me.phase.cyclesLeft), kWarn, F);
	for (auto& O : S.synds) if (O.id != Me.id && O.alive && O.hasPhase) Line(10.f, Y, FString::Printf(TEXT("%s is in a %s phase (%d cycles left)!"), *S2F(O.name), UTF8_TO_TCHAR(gl::pathName(O.phase.type)), O.phase.cyclesLeft), AGLMapActor::SyndColor(O.id), F);
}

void AGLHUD::DrawInspector(AGLGameMode* GM, AGLPlayerController* PC, UFont* F)
{
	if (PC->Sel < 0) return;
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S(); const int32 Me = GM->Me();
	const gl::Hex& H = S.hexes[PC->Sel];
	const float W = Canvas->SizeX, PW = 420.f, X = W - PW - 8.f; float Y = 72.f;
	Panel(X, 66.f, PW, 330.f);
	const bool Vis = G.CanSee(Me, H.id);
	Line(X + 10.f, Y, FString::Printf(TEXT("#%d %s   (q %d, r %d)   District %d%s"), H.id, UTF8_TO_TCHAR(gl::sectorDef(H.type).name), H.q, H.r, H.district + 1, Vis ? TEXT("") : TEXT("   [STALE]")), kInk, F, 1.2f);
	if (H.type == gl::Sector::Barrier) { Line(X + 10.f, Y, TEXT("Barrier: conduit only, +15 crossing premium into the next hex."), kDim, F); return; }
	const gl::SectorDef& D = gl::sectorDef(H.type);
	Line(X + 10.f, Y, FString::Printf(TEXT("yield %.0f   demand %.0f   conduit %.0f   pop %d   rights %s"), D.cap, D.demand, D.conduit, D.pop, (H.rights.count(Me) || H.owner == Me) ? TEXT("yes") : TEXT("no")), kDim, F);
	int OwnerSid = H.owner; double C = H.C;
	auto Seen = H.seen.find(Me);
	if (!Vis) { if (Seen == H.seen.end()) { Line(X + 10.f, Y, TEXT("Never seen. Scan it or move adjacent."), kDim, F); return; } OwnerSid = Seen->second.owner; C = Seen->second.C; Line(X + 10.f, Y, FString::Printf(TEXT("last seen cycle %d"), Seen->second.cycle), kDim, F); }
	Y += 4.f;
	Line(X + 10.f, Y, TEXT("PHYSICAL"), kInk, F, 1.1f);
	Line(X + 10.f, Y, FString::Printf(TEXT("owner %s   integrity %.0f%s"), OwnerSid >= 0 ? *S2F(S.synds[OwnerSid].name) : TEXT("neutral"), C, (Vis && H.brownout) ? TEXT("   BROWNOUT") : TEXT("")), OwnerSid >= 0 ? AGLMapActor::SyndColor(OwnerSid) : kDim, F);
	if (Vis && H.delivered.count(Me))
	{
		auto Pri = H.priority.find(Me);
		Line(X + 10.f, Y, FString::Printf(TEXT("your delivery %.1f / demand %.1f  (r %.2f)   %s%s"), H.delivered.at(Me), H.demand.count(Me) ? H.demand.at(Me) : 0.0, H.ratio.count(Me) ? H.ratio.at(Me) : 0.0,
			H.dual.count(Me) ? TEXT("dual-path") : TEXT("SINGLE PATH"), Pri != H.priority.end() && Pri->second == gl::Priority::Critical ? TEXT("   CRITICAL") : Pri != H.priority.end() && Pri->second == gl::Priority::Low ? TEXT("   low") : TEXT("")), kInk, F);
	}
	if (Vis) for (const gl::Structure* St : G.StructsIn(H.id))
	{
		if (!G.AssetVisible(Me, *St)) continue;
		FString L = FString::Printf(TEXT("  [%d] %s%s  (%s)"), St->id, St->kind == gl::StructKind::Node ? UTF8_TO_TCHAR(gl::nodeDef(St->tier).name) : UTF8_TO_TCHAR(gl::structDef(St->kind).name), St->crown ? TEXT(" CROWN") : TEXT(""), *S2F(S.synds[St->sid].name));
		if (!St->built) L += FString::Printf(TEXT("  building %d"), St->buildLeft);
		if (St->darkUntil > S.cycle) L += TEXT("  DARK");
		if (St->kind == gl::StructKind::Node && St->built) L += FString::Printf(TEXT("  power %.0f%%  out %.1f  util %.0f%%"), St->powerFrac * 100, St->out, St->util * 100);
		if (St->orphanedAt >= 0) L += TEXT("  ORPHANED");
		Line(X + 10.f, Y, L, AGLMapActor::SyndColor(St->sid), F);
	}
	if (Vis) for (const gl::Link* L : G.LinksThrough(H.id))
	{
		if (!G.LinkVisible(Me, *L)) continue;
		FString T = FString::Printf(TEXT("  link %d %s (%s) %d->%d%s"), L->id, UTF8_TO_TCHAR(gl::linkDef(L->type).name), *S2F(S.synds[L->sid].name), L->path.front(), L->path.back(), L->built ? TEXT("") : TEXT(" laying"));
		for (const gl::Segment& Sg : L->segs) if (Sg.a == H.id || Sg.b == H.id) T += FString::Printf(TEXT("  [%d-%d %.1f/%.0f loss %.0f%%%s]"), Sg.a, Sg.b, Sg.flow, Sg.cap, Sg.lastLoss * 100, Sg.sabotagedUntil > S.cycle ? TEXT(" SABOTAGED") : TEXT(""));
		Line(X + 10.f, Y, T, AGLMapActor::SyndColor(L->sid), F);
	}
	Y += 4.f;
	Line(X + 10.f, Y, TEXT("CYBER"), kInk, F, 1.1f);
	FString Cy = FString::Printf(TEXT("firewall %s   your presence %.0f"), Vis ? *FString::Printf(TEXT("%.0f"), H.fw) : TEXT("?"), H.P.count(Me) ? H.P.at(Me) : 0.0);
	if (Vis && G.PresenceVisible(Me, H.id)) { for (auto& KV : H.P) if (KV.first != Me) Cy += FString::Printf(TEXT("   %s P %.0f"), KV.first == gl::RogueSid ? TEXT("ROGUE AI") : *S2F(S.synds[KV.first].name), KV.second); }
	else if (Vis && H.owner == Me) { bool Contested = false; for (auto& KV : H.P) if (KV.first != Me && KV.second >= 40) Contested = true; Cy += Contested ? TEXT("   CONTESTED (+2 demand) - build an Array or Scan to read values") : TEXT("   rival presence unknown (Array/Scan reveals it)"); }
	Line(X + 10.f, Y, Cy, kInk, F);
	if (Vis && !H.roots.empty()) { FString R = TEXT("ROOTED by:"); for (int Rt : H.roots) R += TEXT(" ") + S2F(S.synds[Rt].name); Line(X + 10.f, Y, R, kBad, F); }
	if (Vis && H.daemons.count(Me)) Line(X + 10.f, Y, FString::Printf(TEXT("your daemons %d"), H.daemons.at(Me)), kInk, F);
}

void AGLHUD::DrawLog(AGLGameMode* GM, UFont* F)
{
	const float H = Canvas->SizeY; float Y = H - 150.f;
	Panel(0, H - 156.f, 640.f, 156.f);
	for (const std::string& L : GM->Sim().RecentLog(GM->Me(), 10)) Line(8.f, Y, S2F(L), kDim, F);
}

void AGLHUD::DrawHelp(UFont* F)
{
	float Y = 76.f; const float X = 8.f;
	Panel(0, 66.f, 470.f, 300.f);
	Line(X, Y, TEXT("F1 help   V board room   Space/Enter end cycle   Esc/right-click cancel   Home recentre"), kInk, F);
	Line(X, Y, TEXT("arrows or W/A/S/E pan   mouse wheel zoom   left-click select / confirm"), kDim, F);
	Line(X, Y, TEXT("L lay fiber (click start in your network, then destination; rights bought on the way)"), kInk, F);
	Line(X, Y, TEXT("1 Edge node  2 Core node  3 Hyperscale  4 Substation  5 Repeater  6 Peering Rack"), kInk, F);
	Line(X, Y, TEXT("7 Outpost  8 Surveillance Array  9 Lab  0 Honeypot        then click one of your hexes"), kInk, F);
	Line(X, Y, TEXT("I Intrusion  H Harden  P Purge  K Siphon  J Jam  R Root  D DDoS  C Scan  N Daemon"), kInk, F);
	Line(X, Y, TEXT("T queue research   M adopt doctrine   X cycle sector priority   B buy 10 BW   G deny right-of-way"), kInk, F);
	Line(X, Y, TEXT("F5 Tender Offer  F6 buy 1% shares  F7 poison pill  F8 train Seed  F9 launch  F10 Blackout  F11 Charter motion  F12 kill switch"), kInk, F);
	Y += 6.f;
	Line(X, Y, TEXT("Read the map: hex height = sector type, colour = who holds it, fading = integrity, dim = stale data."), kDim, F);
	Line(X, Y, TEXT("Links: thickness = capacity, colour shifts red with loss, white packets fall off the line as they are lost."), kDim, F);
	Line(X, Y, TEXT("Rings: cyan = your presence, other colours = rivals you can see, red hexagon = rooted, grey hexagon = single path."), kDim, F);
}

void AGLHUD::DrawBoard(AGLGameMode* GM, UFont* F)
{
	const gl::Game& G = GM->Sim(); const gl::GameState& S = G.S();
	const float W = Canvas->SizeX; const float PW = 700.f, X = (W - PW) * 0.5f; float Y = 90.f;
	Panel(X, 80.f, PW, 380.f);
	Line(X + 10.f, Y, TEXT("BOARD ROOM"), kInk, F, 1.3f);
	for (auto& O : S.synds)
		Line(X + 10.f, Y, FString::Printf(TEXT("%-26s %-18s val %7.0f  share %5.1f%%  seats %d  X %3.0f%s%s%s"), *S2F(O.name), UTF8_TO_TCHAR(gl::archDef(O.arch).name), O.valuation, O.share * 100.0, G.SeatsOf(O.id), O.exposure,
			O.hasPhase ? *FString::Printf(TEXT("  PHASE %s"), UTF8_TO_TCHAR(gl::pathName(O.phase.type))) : TEXT(""), O.shadowOf >= 0 ? TEXT("  subsidiary") : TEXT(""), O.alive ? TEXT("") : TEXT("  DELISTED")), AGLMapActor::SyndColor(O.id), F);
	Y += 6.f;
	Line(X + 10.f, Y, TEXT("Your paths"), kInk, F, 1.1f);
	for (const gl::PathProgress& P : G.Progress(GM->Me()))
	{
		Line(X + 10.f, Y, FString::Printf(TEXT("%-18s %3.0f%%  %s%s"), UTF8_TO_TCHAR(gl::pathName(P.path)), P.proximity, P.available ? TEXT("READY  ") : TEXT(""), *S2F(P.status)), P.available ? kWarn : kInk, F);
		for (const std::string& U : P.unmet) Line(X + 30.f, Y, TEXT("needs: ") + S2F(U), kDim, F);
	}
	FString Dist = TEXT("Districts:");
	for (int i = 0; i < (int)S.districts.size(); ++i) { const gl::District& D = S.districts[i]; Dist += FString::Printf(TEXT("  D%d:%s"), i + 1, D.seat >= 0 ? (D.bought ? TEXT("bought") : TEXT("held")) : TEXT("open")); }
	Line(X + 10.f, Y, Dist, kDim, F);
	for (auto& C : S.cartels) { FString T = TEXT("CARTEL against ") + S2F(S.synds[C.target].name) + TEXT(":"); for (int Mb : C.members) T += TEXT(" ") + S2F(S.synds[Mb].name); Line(X + 10.f, Y, T, kBad, F); }
	if (S.emergent.active) Line(X + 10.f, Y, TEXT("AN EMERGENT INTELLIGENCE IS LOOSE IN THE BACKBONE."), kBad, F, 1.1f);
}
