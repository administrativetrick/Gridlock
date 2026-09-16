// Fail-fast test suite for the Gridlock simulation. Any failure aborts the build script.
#include "gridlock/Game.h"
#include "Internal.h"
#include <cstdio>
#include <cmath>
#include <functional>

using namespace gl;
using namespace gl::sim;

static int g_fail = 0, g_pass = 0;
#define EXPECT(cond, msg) do { if (!(cond)) { std::printf("  FAIL %s:%d  %s  (%s)\n", __FILE__, __LINE__, msg, #cond); ++g_fail; } else ++g_pass; } while (0)
#define EXPECT_NEAR(a, b, tol, msg) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) > (tol)) { std::printf("  FAIL %s:%d  %s  got %.4f expected %.4f\n", __FILE__, __LINE__, msg, _a, _b); ++g_fail; } else ++g_pass; } while (0)

static void run(const char* name, const std::function<void()>& f) { std::printf("[test] %s\n", name); f(); }

// docs/01 §11: Core Node at A, Trunk chain A-B-C-D-E, baseline archetype numbers (Hive has no output/cap/hop modifiers).
static void testWorkedExample() {
  Config cfg; cfg.player = Arch::Hive; cfg.ais = { Arch::Ghost }; cfg.seed = "worked";
  Game g(cfg); GameState& S = g.S();
  for (Hex& h : S.hexes) { h.owner = -1; h.C = 0; h.P.clear(); h.rights.clear(); h.roots.clear(); h.brownout = false; h.jamUntil.clear(); h.daemons.clear(); h.dual.clear(); h.fw = sectorDef(h.type).fw; }
  S.structs.clear(); S.links.clear(); S.cartels.clear();
  for (auto& s : S.synds) { s.ops.clear(); s.queued.clear(); s.tapIncome.clear(); s.buffer = 0; }
  // five consecutive hexes on row 3 (ids are row-major)
  int row = 3, W = S.grid.W; int A = row * W + 4;
  int ids[5] = { A, A + 1, A + 2, A + 3, A + 4 };
  Sector types[5] = { Sector::Campus, Sector::Industrial, Sector::Sprawl, Sector::Arcology, Sector::Financial };
  Syndicate& s = S.synds[0]; s.computePct = 0; s.capital = 1000;
  for (int i = 0; i < 5; ++i) { Hex& h = S.hexes[ids[i]]; h.type = types[i]; h.owner = 0; h.C = 100; h.rights.insert(0); EXPECT(S.grid.dist(ids[0], ids[i]) == i, "chain hexes must be collinear"); }
  Structure& node = addStruct(S, s, A, StructKind::Node, 2, Variant::None, true, true);
  addStruct(S, s, A, StructKind::Substation, 0, Variant::None, true);
  addLink(S, s, LinkType::Trunk, { ids[0], ids[1], ids[2], ids[3], ids[4] }, true);
  node.powerFrac = 1.0;
  Mods m = modsOf(S, s);
  EXPECT_NEAR(m.trunkCap, 20, 1e-9, "baseline trunk capacity");
  EXPECT_NEAR(nodeRaw(S, s, node), 30, 1e-9, "core node raw output");
  solveFlow(S, s);
  EXPECT_NEAR(S.hexes[ids[0]].delivered[0], 6, 1e-6, "A served locally at 0 hops");
  EXPECT_NEAR(S.hexes[ids[1]].delivered[0], 4, 1e-6, "B demand met");
  EXPECT_NEAR(S.hexes[ids[2]].delivered[0], 2, 1e-6, "C demand met");
  EXPECT_NEAR(S.hexes[ids[3]].delivered[0], 6, 1e-6, "D demand met");
  EXPECT_NEAR(S.hexes[ids[4]].delivered[0], 4.92, 0.03, "E receives 4.92 of 8 (docs/01 §11)");
  EXPECT_NEAR(s.flow.bufferAdd, 3.0, 1e-6, "10% of raw production banked");
  EXPECT_NEAR(s.flow.stranded, 1.0, 1e-6, "4 stranded at A minus 3 buffered");
  // the Backbone fix: upgrade first hop → E fully fed
  S.links.clear(); s.buffer = 0;
  s.techs[(int)Tech::BackboneFiber] = true;
  addLink(S, s, LinkType::Backbone, { ids[0], ids[1] }, true);
  addLink(S, s, LinkType::Trunk, { ids[1], ids[2], ids[3], ids[4] }, true);
  solveFlow(S, s);
  EXPECT(S.hexes[ids[4]].delivered[0] >= 8 - 1e-6, "with a Backbone first hop E is fully served (docs/01 §11 fix 2)");
}

// A node one hop from an Exchange with a rack must pay the 10 BW fee, peer, and sell its surplus.
static void testExchangePeering() {
  Config cfg; cfg.player = Arch::Hive; cfg.ais = { Arch::Ghost }; cfg.seed = "peer";
  Game g(cfg); GameState& S = g.S();
  for (Hex& h : S.hexes) { h.owner = -1; h.C = 0; h.P.clear(); h.rights.clear(); h.roots.clear(); h.brownout = false; h.dual.clear(); }
  S.structs.clear(); S.links.clear();
  for (auto& s : S.synds) { s.ops.clear(); s.queued.clear(); s.tapIncome.clear(); s.buffer = 0; }
  int ex = S.exchanges[0]; int A = -1; for (int n : S.grid.neighborsV(ex)) if (S.hexes[n].type != Sector::Barrier) { A = n; break; }
  EXPECT(A >= 0, "exchange has a buildable neighbour");
  Syndicate& s = S.synds[0]; s.computePct = 0; s.capital = 1000;
  Hex& ha = S.hexes[A]; ha.type = Sector::Campus; ha.owner = 0; ha.C = 100; ha.rights.insert(0);
  Structure& node = addStruct(S, s, A, StructKind::Node, 2, Variant::None, true, true); node.powerFrac = 1.0;
  addStruct(S, s, A, StructKind::Substation, 0, Variant::None, true);
  addStruct(S, s, ex, StructKind::Rack, 0, Variant::None, true);
  addLink(S, s, LinkType::Trunk, { A, ex }, true);
  solveFlow(S, s);
  EXPECT(s.flow.peered.size() == 1, "peered at the exchange");
  // 30 produced − 6 local − 10 fee (÷0.97 loss) − 10% buffer(3) → the rest sold
  EXPECT(s.flow.sold > 8.0, "surplus sold at the exchange");
  EXPECT_NEAR(s.flow.produced, 30, 1e-9, "production");
}

static void testDeterminism() {
  Config cfg; cfg.playerIsAI = true; cfg.seed = "det-42";
  Game a(cfg), b(cfg);
  for (int i = 0; i < 25; ++i) { a.EndCycle(); b.EndCycle(); }
  EXPECT(a.StateHash() == b.StateHash(), "same seed + same orders → identical state hash");
  EXPECT(a.S().cycle == 25, "25 cycles resolved");
}

static void testValidation() {
  Config cfg; cfg.seed = "val"; Game g(cfg); GameState& S = g.S();
  int neutral = -1; for (auto& h : S.hexes) if (h.owner == -1 && h.type != Sector::Barrier && h.type != Sector::Exchange) { neutral = h.id; break; }
  EXPECT(!g.Build(0, StructKind::Node, neutral, 1), "node on unowned hex rejected");
  EXPECT(!g.Build(0, StructKind::Node, S.synds[0].crown, 3), "tier 3 without Modular Datacenters rejected");
  EXPECT(!g.Build(0, StructKind::Node, S.synds[0].crown, 2), "second node in the same hex rejected");
  std::vector<int> nb = S.grid.neighborsV(S.synds[0].crown); int far = -1;
  for (auto& h : S.hexes) if (h.owner == -1 && h.type != Sector::Barrier && S.grid.dist(h.id, S.synds[0].crown) == 1) { far = h.id; break; }
  if (far >= 0) {
    EXPECT(!g.LayLink(0, LinkType::Trunk, { S.synds[0].crown, far }), "laying fiber without conduit rights rejected");
    EXPECT((bool)g.BuyRights(0, far), "buying rights in a neutral hex works");
    EXPECT((bool)g.LayLink(0, LinkType::Trunk, { S.synds[0].crown, far }), "laying fiber with rights works");
  }
  EXPECT(!g.LayLink(0, LinkType::Backbone, { S.synds[0].crown, nb[0] }), "Backbone without tech rejected");
  EXPECT(!g.QueueOp(0, OpKind::Root, nb[0]), "root without presence rejected");
  EXPECT(!g.QueueOp(0, OpKind::Raid, nb[0], 0), "raid without Field Teams rejected");
  EXPECT(!g.TakeDoctrine(0, Doctrine::H_Vertical), "doctrine without mandate rejected");
  EXPECT(!g.TakeDoctrine(0, Doctrine::G_LOTL), "other archetype's doctrine rejected");
  EXPECT(!g.QueueResearch(0, Tech::BackboneFiber), "tier 2 research without 2 tier-1 techs rejected");
  EXPECT((bool)g.QueueResearch(0, Tech::Trenching), "tier 1 research queues");
  int slots = g.Slots(0); int ok = 0;
  for (int i = 0; i < slots + 2; ++i) if (g.QueueOp(0, OpKind::Harden, S.synds[0].crown)) ++ok;
  EXPECT(ok <= 1, "duplicate sustained op rejected");
  int issued = 0; for (auto& h : S.hexes) if (h.owner == 0) { if (g.QueueOp(0, OpKind::Scan, h.id)) ++issued; }
  EXPECT(S.synds[0].slotsUsed <= slots, "action slots enforced");
  EXPECT(!g.TenderOffer(0), "tender offer without authority rejected");
  EXPECT(!g.DeclareBlackout(0), "blackout without unlock rejected");
}

static void testHeadlessGame() {
  Config cfg; cfg.playerIsAI = true; cfg.seed = "headless-7"; cfg.ais = { Arch::Ghost, Arch::Hive, Arch::Hegemony };
  Game g(cfg); GameState& S = g.S();
  int maxSectors = 0;
  for (int i = 0; i < 80 && !S.over; ++i) {
    g.EndCycle();
    for (auto& s : S.synds) { EXPECT(s.capital >= -1e-6, "capital never negative after resolution"); maxSectors = std::max(maxSectors, (int)ownedHexes(S, s.id).size()); }
  }
  EXPECT(maxSectors >= 8, "AI syndicates expand (someone holds >= 8 sectors by cycle 80)");
  int links = 0; for (auto& l : S.links) if (l.alive) ++links;
  EXPECT(links > 12, "fiber gets laid");
  bool anyPresence = false; for (auto& h : S.hexes) for (auto& kv : h.P) if (h.owner >= 0 && kv.first != h.owner && kv.second > 0) anyPresence = true;
  EXPECT(anyPresence, "cyber layer is in use (rival presence exists somewhere)");
  std::printf("  cycle %d, hash %s\n", S.cycle, g.StateHash().c_str());
  for (auto& s : S.synds) std::printf("  %-26s %-9s sectors %2d  capital %7.0f  share %4.1f%%  X %4.0f  M %3.0f\n", s.name.c_str(), archDef(s.arch).name, (int)ownedHexes(S, s.id).size(), s.capital, s.share * 100, s.exposure, s.M);
}

static void testControlDecay() {
  Config cfg; cfg.seed = "decay"; cfg.ais = { Arch::Ghost }; Game g(cfg); GameState& S = g.S();
  // cut every player link → the three start neighbours starve and go neutral within 4 cycles
  for (auto& l : S.links) if (l.sid == 0) l.alive = false;
  int neighbours = 0; for (auto& h : S.hexes) if (h.owner == 0 && h.id != S.synds[0].crown) ++neighbours;
  EXPECT(neighbours == 3, "three starting neighbours");
  for (int i = 0; i < 4; ++i) g.EndCycle();
  int still = 0; for (auto& h : S.hexes) if (h.owner == 0 && h.id != S.synds[0].crown) ++still;
  EXPECT(still == 0, "unfed sectors go neutral in 4 cycles (docs/01 §2.3)");
  EXPECT(S.hexes[S.synds[0].crown].owner == 0, "the crown hex (node in-hex) stays held");
}

static void testFogOfWar() {
  Config cfg; cfg.seed = "fog"; Game g(cfg); GameState& S = g.S();
  int farHex = -1; for (auto& h : S.hexes) if (h.type != Sector::Barrier && S.grid.dist(h.id, S.synds[0].crown) > 6) { farHex = h.id; break; }
  EXPECT(farHex >= 0 && !g.CanSee(0, farHex), "distant hex is fogged");
  EXPECT(g.CanSee(0, S.synds[0].crown), "own crown visible");
  int rivalCrown = S.synds[1].crown;
  EXPECT(!g.PresenceVisible(0, rivalCrown), "rival presence values hidden without Array/Scan");
}

int main() {
  run("worked example (docs/01 §11)", testWorkedExample);
  run("exchange peering and sales", testExchangePeering);
  run("determinism", testDeterminism);
  run("order validation", testValidation);
  run("control decay", testControlDecay);
  run("fog of war", testFogOfWar);
  run("80-cycle headless AI game", testHeadlessGame);
  std::printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
