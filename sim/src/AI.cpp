// Rival syndicates: Threat Board (fog-limited), postures, cartels, economy, defence, archetype offence, victory pursuit, containment.
// The AI acts only through the public Game API, so it obeys the same validation as the player.
#include "Internal.h"

namespace gl { namespace sim {

namespace {

struct Ctx { Game& g; GameState& S; Syndicate& s; Mods m; double reserve; };

double roll(GameState& S) { Rng r(S.rngState); double v = r.next(); S.rngState = r.state(); return v; }

std::vector<Tech> researchOrder(Arch a) {
  switch (a) {
  case Arch::Hegemony: return { Tech::GridContracts, Tech::Trenching, Tech::RedundantPeering, Tech::BackboneFiber, Tech::ModularDC, Tech::HardenedKernels, Tech::FieldTeams, Tech::FuturesDesk, Tech::ShellCompanies, Tech::VerticalIntegration, Tech::Buyback, Tech::HyperscaleCooling, Tech::TenderOffer, Tech::SabotageDoctrine, Tech::Lobbying, Tech::DPI, Tech::ConsolidationLobby, Tech::FortifiedConduit };
  case Arch::Ghost: return { Tech::Persistence, Tech::DPI, Tech::ShellCompanies, Tech::HardenedKernels, Tech::ZeroDay, Tech::Honeypots, Tech::FuturesDesk, Tech::DataBrokerage, Tech::TrafficShaping, Tech::FieldTeams, Tech::CounterIntel, Tech::FalseFlags, Tech::KillChain, Tech::AdaptiveFW, Tech::DeadDrops, Tech::Lobbying, Tech::BlackoutProtocol };
  default: return { Tech::HardenedKernels, Tech::DPI, Tech::GridContracts, Tech::Persistence, Tech::ZeroDay, Tech::Trenching, Tech::ModularDC, Tech::AdaptiveFW, Tech::BackboneFiber, Tech::HyperscaleCooling, Tech::BackboneSniffing, Tech::KillChain, Tech::ShellCompanies, Tech::RedundantPeering, Tech::VerticalIntegration, Tech::FuturesDesk };
  }
}
std::vector<Doctrine> doctrineOrder(Arch a) {
  switch (a) {
  case Arch::Hegemony: return { Doctrine::H_Vertical, Doctrine::H_Eminent, Doctrine::H_Rings, Doctrine::H_PeeringCartel, Doctrine::H_Monopoly, Doctrine::H_Fortress };
  case Arch::Ghost: return { Doctrine::G_LOTL, Doctrine::G_Sleeper, Doctrine::G_FalseFlag, Doctrine::G_Backdoor, Doctrine::G_Blackout, Doctrine::G_ShadowBoard };
  default: return { Doctrine::V_DataLake, Doctrine::V_Autoscaling, Doctrine::V_Adversarial, Doctrine::V_Generative, Doctrine::V_Seed, Doctrine::V_Recursive };
  }
}

// ---------------------------------------------------------------- threat board (uses only what this AI can see)
void threatBoard(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s;
  for (Syndicate& r : S.synds) {
    if (r.id == s.id || r.id >= 8) continue;
    auto& vp = s.vp[r.id]; for (double& v : vp) v = 0;
    if (!r.alive || r.shadowOf >= 0) { s.posture[r.id] = Posture::Expand; continue; }
    bool knowTech = false; for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && st.kind == StructKind::Lab && canSee(S, s.id, st.hex) && c.m.dpi) knowTech = true;
    // Takeover: share is public
    { double x = std::max(0.0, std::min(1.0, (r.share - 0.25) / 0.15)); double conf = knowTech ? (has(r, Tech::TenderOffer) ? 1.0 : 0.6) : 0.85; vp[(int)VictoryPath::Takeover] = 100 * x * conf; if (r.hasPhase && r.phase.type == VictoryPath::Takeover) vp[(int)VictoryPath::Takeover] = 100; }
    // Singularity: T3 nodes seen, compute rings seen, Seed Protocol public, launch public
    { int t3 = 0; double compute = 0; for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && st.kind == StructKind::Node && st.tier >= 3 && assetVisible(S, s.id, st)) { ++t3; auto it = r.flow.computeByNode.find(st.id); if (it != r.flow.computeByNode.end()) compute += it->second; }
      bool sniff = has(s, Tech::BackboneSniffing); if (sniff) for (auto& kv : s.peeredLast) if (kv.second >= S.cycle - 1 && r.peeredLast.count(kv.first) && r.peeredLast.at(kv.first) >= S.cycle - 1) compute = std::max(compute, r.flow.compute);
      double v = 30 * std::min(1.0, t3 / 3.0) + (hasDoc(r, Doctrine::V_Seed) ? 30 : 0) + 40 * std::min(1.0, compute / 150.0);
      if (r.hasPhase && r.phase.type == VictoryPath::Singularity) v = std::max(v, r.training >= 11 ? 100.0 : 70.0);
      vp[(int)VictoryPath::Singularity] = v; }
    // Blackout: presence on our own crown (only if we can see presence there), rooted crown is an alarm we always get
    { const Hex& ch = S.hexes[s.crown]; double P = presenceVisible(S, s.id, s.crown) && ch.P.count(r.id) ? ch.P.at(r.id) : 0; double v = 60 * std::min(1.0, P / 70.0);
      if (ch.roots.count(r.id)) v = 100; if (knowTech && has(r, Tech::KillChain)) v += 20; if (hasDoc(r, Doctrine::G_Blackout) || hasDoc(r, Doctrine::G_ShadowBoard)) v += 15;
      vp[(int)VictoryPath::Blackout] = std::min(100.0, v); }
    // Charter: seats are public
    { int seats = seatsOf(S, r.id); double v = 100 * std::min(1.0, seats / 5.0); if (r.hasPhase && r.phase.type == VictoryPath::Charter) v = 100; vp[(int)VictoryPath::Charter] = v; }
    double mx = 0; for (int i = 0; i < 4; ++i) mx = std::max(mx, vp[i]);
    bool phase = r.hasPhase;
    s.posture[r.id] = (mx > 85 || phase) ? Posture::Desperation : mx > 65 ? Posture::Contain : mx >= 40 ? Posture::Hedge : Posture::Expand;
    if (r.shadowOf == s.id) s.posture[r.id] = Posture::Expand;
  }
}

void cartels(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s;
  for (Syndicate& t : S.synds) {
    if (t.id == s.id || !t.alive || t.id >= 8 || s.posture[t.id] < Posture::Contain) continue;
    for (Syndicate& u : S.synds) {
      if (u.id == s.id || u.id == t.id || !u.alive || !u.isAI || t.id >= 8 || u.posture[t.id] < Posture::Contain) continue;
      Cartel* ex = nullptr; for (auto& ca : S.cartels) if (ca.target == t.id) ex = &ca;
      if (!ex) { Cartel ca; ca.target = t.id; ca.members = { s.id, u.id }; ca.since = S.cycle; S.cartels.push_back(ca); logMsg(S, "ANTI-TRUST CONSORTIUM: " + s.name + " and " + u.name + " form a cartel against " + t.name + "."); }
      else { if (std::find(ex->members.begin(), ex->members.end(), s.id) == ex->members.end()) { ex->members.push_back(s.id); logMsg(S, s.name + " joined the cartel against " + t.name + "."); } }
    }
  }
  for (auto it = S.cartels.begin(); it != S.cartels.end();) {
    bool anyContain = false; for (int m : it->members) if (synd(S, m).alive && it->target < 8 && synd(S, m).posture[it->target] >= Posture::Contain) anyContain = true;
    bool infight = false; for (int m : it->members) for (int n : it->members) if (m != n && m < 8 && n < 8 && synd(S, m).posture[n] >= Posture::Contain) infight = true;
    if ((!anyContain && S.cycle - it->since >= 5) || infight || !synd(S, it->target).alive) { logMsg(S, "The cartel against " + synd(S, it->target).name + " dissolved."); it = S.cartels.erase(it); } else ++it;
  }
}

// ---------------------------------------------------------------- economy
int nearestNetworkHex(Ctx& c, const std::set<int>& net, int target) { int best = -1, bd = 1 << 30; for (int h : net) { if (c.S.hexes[h].type == Sector::Barrier) continue; int d = c.S.grid.dist(h, target); if (d < bd) { bd = d; best = h; } } return best; }

bool expandOnce(Ctx& c, bool wantExchange) {
  GameState& S = c.S; Syndicate& s = c.s; std::set<int> net = networkHexes(S, s.id);
  double budget = s.capital - c.reserve; if (budget < 60) return false;
  struct Cand { int hex; double score; PathPlan plan; };
  std::vector<Cand> cands;
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier || net.count(h.id)) continue;
    bool exch = h.type == Sector::Exchange;
    if (exch) { if (!wantExchange || structIn(S, h.id, s.id, StructKind::Rack, false)) continue; }
    else if (h.owner != -1) continue;
    int bd = 99; for (int n : net) bd = std::min(bd, S.grid.dist(n, h.id)); if (bd > 4) continue;
    int from = nearestNetworkHex(c, net, h.id); if (from < 0) continue;
    LinkType lt = LinkType::Trunk; if (has(s, Tech::BackboneFiber) && structIn(S, from, s.id, StructKind::Node)) lt = LinkType::Backbone;
    if (s.arch == Arch::Ghost) lt = LinkType::Dark;
    PathPlan p = c.g.PlanPath(s.id, from, h.id, lt); if (!p.ok || p.total > budget) continue;
    double value = exch ? 90 : sectorDef(h.type).cap + sectorDef(h.type).pop * (s.arch == Arch::Hive ? 3 : 0.5);
    if (exch && s.arch == Arch::Hegemony) value += 40;
    double sc = value * p.projectedSurvival / (p.total + 25);
    cands.push_back({ h.id, sc, p });
  }
  if (cands.empty()) return false;
  std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
  Cand& b = cands[0];
  for (int r : b.plan.needRights) if (!c.g.BuyRights(s.id, r)) return false;
  LinkType lt = LinkType::Trunk; if (has(s, Tech::BackboneFiber) && structIn(S, b.plan.path[0], s.id, StructKind::Node)) lt = LinkType::Backbone; if (s.arch == Arch::Ghost) lt = LinkType::Dark;
  return (bool)c.g.LayLink(s.id, lt, b.plan.path);
}

void economy(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; Game& g = c.g;
  // compute allocation
  s.computePct = s.arch == Arch::Hive ? (s.M < 50 ? 0.6 : 0.5) : s.arch == Arch::Ghost ? 0.3 : 0.2;
  if (s.hasPhase && s.phase.type == VictoryPath::Singularity && s.training < 10) s.computePct = 1.0;
  if (s.hasPhase && s.phase.type == VictoryPath::Singularity && s.training >= 11) s.computePct = 0.0;
  double surplus = s.flow.stranded + s.flow.sold + s.flow.bufferAdd;
  bool saturated = s.flow.produced > 0 && surplus < 3.0;
  int owned = (int)ownedHexes(S, s.id).size();
  // power first: every node gets a substation within range (grid power costs 5/MW/cycle)
  for (auto& st : S.structs) {
    if (!st.alive || st.sid != s.id || st.kind != StructKind::Node) continue;
    bool powered = false; int range = (int)c.m.subRange;
    for (auto& sub : S.structs) if (sub.alive && sub.sid == s.id && (sub.kind == StructKind::Substation || sub.kind == StructKind::PrivateGrid) && S.grid.dist(sub.hex, st.hex) <= range) powered = true;
    if (powered) continue;
    if (s.capital < buildCost(S, s, StructKind::Substation, 0, Variant::None) + c.reserve) break;
    int best = -1; double bs = -1;
    for (int h : S.grid.within(st.hex, range)) { const Hex& hx = S.hexes[h]; if (hx.owner != s.id || structIn(S, h, s.id, StructKind::Substation, false)) continue; double sc = (hx.type == Sector::Industrial ? 2 : 1) - S.grid.dist(h, st.hex) * 0.1; if (sc > bs) { bs = sc; best = h; } }
    if (!structIn(S, st.hex, s.id, StructKind::Substation, false) && (best < 0 || bs < 1.5)) best = st.hex;
    if (best >= 0) { g.Build(s.id, StructKind::Substation, best); break; }
  }
  // bandwidth saturated → a new node beats more cable
  if (saturated) {
    int tier = (c.m.maxTier >= 2 && s.capital >= buildCost(S, s, StructKind::Node, 2, Variant::None) + c.reserve + 50) ? 2 : 1;
    if (s.capital >= buildCost(S, s, StructKind::Node, tier, Variant::None) + c.reserve) {
      int best = -1; double bs = -1e9;
      for (Hex& h : S.hexes) {
        if (h.owner != s.id) continue; bool hasNode = false; for (auto& st : S.structs) if (st.alive && st.hex == h.id && st.kind == StructKind::Node) hasNode = true; if (hasNode) continue;
        double far = 99; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node) far = std::min(far, (double)S.grid.dist(st.hex, h.id));
        bool powered = false; for (auto& sub : S.structs) if (sub.alive && sub.sid == s.id && (sub.kind == StructKind::Substation || sub.kind == StructKind::PrivateGrid) && S.grid.dist(sub.hex, h.id) <= (int)c.m.subRange) powered = true;
        double sc = std::min(far, 4.0) + (powered ? 1.5 : 0) + (h.type == Sector::Campus || h.type == Sector::Industrial ? 0.5 : 0);
        if (sc > bs) { bs = sc; best = h.id; }
      }
      if (best >= 0) g.Build(s.id, StructKind::Node, best, tier, (s.arch == Arch::Ghost && tier == 2) ? Variant::Phantom : Variant::None);
    }
  }
  // rack at any reachable exchange
  std::set<int> net = networkHexes(S, s.id);
  for (int ex : S.exchanges) if (net.count(ex) && !structIn(S, ex, s.id, StructKind::Rack, false) && s.capital >= buildCost(S, s, StructKind::Rack, 0, Variant::None) + c.reserve) g.Build(s.id, StructKind::Rack, ex);
  // surveillance at crown early
  if (S.cycle >= 3 && !structIn(S, s.crown, s.id, StructKind::Array, false) && s.capital > 200) g.Build(s.id, StructKind::Array, s.crown);
  // anti-loss: starving sectors far from a source
  for (Hex& h : S.hexes) {
    if (h.owner != s.id || !h.ratio.count(s.id) || h.ratio[s.id] >= 0.85) continue;
    if (structIn(S, h.id, s.id, StructKind::Node, false) || structIn(S, h.id, s.id, StructKind::Repeater, false)) continue;
    bool near = false; for (auto& st : S.structs) if (st.alive && st.sid == s.id && (st.kind == StructKind::Node || st.kind == StructKind::Repeater) && S.grid.dist(st.hex, h.id) <= 2) near = true;
    if (near) continue;
    double nodeC = buildCost(S, s, StructKind::Node, 1, Variant::None);
    if (s.capital > nodeC + 200 + c.reserve) { if (g.Build(s.id, StructKind::Node, h.id, 1)) break; }
    else if (s.capital > buildCost(S, s, StructKind::Repeater, 0, Variant::None) + c.reserve) { if (g.Build(s.id, StructKind::Repeater, h.id)) break; }
  }
  // core node when rich and network large
  int nodes = 0; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node) ++nodes;
  if (owned > nodes * 6 && s.capital > buildCost(S, s, StructKind::Node, 2, Variant::None) + 150 + c.reserve) {
    int best = -1; double bs = -1;
    for (Hex& h : S.hexes) { if (h.owner != s.id || structIn(S, h.id, s.id, StructKind::Node, false)) continue; double far = 99; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node) far = std::min(far, (double)S.grid.dist(st.hex, h.id)); if (far < 3) continue; double sc = far + (h.type == Sector::Campus || h.type == Sector::Industrial ? 1 : 0); if (sc > bs) { bs = sc; best = h.id; } }
    if (best >= 0) g.Build(s.id, StructKind::Node, best, c.m.maxTier >= 2 ? 2 : 1, s.arch == Arch::Ghost ? Variant::Phantom : Variant::None);
  }
  // tier 3 for hive / hegemony when able
  if (has(s, Tech::ModularDC) && s.capital > 1000 + c.reserve) {
    int t3 = 0; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node && st.tier >= 3) ++t3;
    if (t3 < 3) for (Hex& h : S.hexes) { if (h.owner != s.id || !(h.type == Sector::Industrial || h.type == Sector::Campus) || structIn(S, h.id, s.id, StructKind::Node, false)) continue; bool nearNode = false; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node && S.grid.dist(st.hex, h.id) <= 2) nearNode = true; if (!nearNode) continue; if (g.Build(s.id, StructKind::Node, h.id, 3, s.arch == Arch::Hive ? Variant::Inference : Variant::None)) break; }
  }
  // labs and execs
  int labs = 0; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Lab) ++labs;
  if (labs < 1 && s.capital > 550 + c.reserve) g.Build(s.id, StructKind::Lab, s.crown);
  if ((int)s.execs.size() < 4 && s.capital > 900 + c.reserve) g.HireExec(s.id, s.arch == Arch::Ghost ? ExecSpec::HOI : s.arch == Arch::Hegemony ? ExecSpec::Fixer : ExecSpec::CFO);
  // outposts on chokepoints (hegemony)
  if (s.arch == Arch::Hegemony && s.capital > 600 + c.reserve) {
    int outposts = 0; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Outpost) ++outposts;
    if (outposts < 3) for (Hex& h : S.hexes) { if (h.owner != s.id || structIn(S, h.id, s.id, StructKind::Outpost, false)) continue; bool border = false; for (int n : S.grid.neighborsV(h.id)) if (S.hexes[n].owner >= 0 && S.hexes[n].owner != s.id) border = true; if (border) { g.Build(s.id, StructKind::Outpost, h.id); break; } }
  }
  // honeypot at crown
  if (has(s, Tech::Honeypots) && !structIn(S, s.crown, s.id, StructKind::Honeypot, false) && s.capital > 150 + c.reserve) g.Build(s.id, StructKind::Honeypot, s.crown);
  // expansion
  bool wantEx = true; for (auto& kv : s.peeredLast) if (kv.second >= S.cycle - 1) wantEx = false;
  int pushes = s.capital > 800 ? 2 : 1;
  if (!saturated || owned < 5 || wantEx) for (int i = 0; i < pushes; ++i) if (!expandOnce(c, wantEx)) break;
  // buy bandwidth when starving
  bool starving = false; for (Hex& h : S.hexes) if (h.owner == s.id && h.ratio.count(s.id) && h.ratio[s.id] < 0.8) starving = true;
  if (starving && s.capital > 300 + c.reserve) g.BuyBandwidth(s.id, 10);
  // research & doctrine
  std::vector<Tech> order = researchOrder(s.arch);
  while ((int)s.researchQueue.size() < 2) { bool added = false; for (Tech t : order) if (g.CanResearch(s.id, t)) { g.QueueResearch(s.id, t); added = true; break; } if (!added) break; }
  for (Doctrine d : doctrineOrder(s.arch)) if (g.CanTakeDoctrine(s.id, d)) { g.TakeDoctrine(s.id, d); break; }
}

// ---------------------------------------------------------------- defence
bool running(const Syndicate& s, OpKind k, int target) { for (const Op& o : s.ops) if (o.kind == k && o.target == target) return true; return false; }
void defence(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; Game& g = c.g;
  if (!running(s, OpKind::Harden, s.crown)) g.QueueOp(s.id, OpKind::Harden, s.crown);
  int purges = 0;
  for (Hex& h : S.hexes) {
    if (h.owner != s.id || purges >= 2) continue;
    bool rooted = false; for (int r : h.roots) if (r != s.id) rooted = true;
    double P = presenceVisible(S, s.id, h.id) ? maxRivalP(h, s.id) : 0;
    if (rooted || P >= (h.id == s.crown ? 30 : 50)) { if (g.QueueOp(s.id, OpKind::Purge, h.id)) ++purges; }
  }
  // failover substation for the crown when a blackout is suspected
  bool suspect = false; for (int i = 0; i < 8 && i < (int)S.synds.size(); ++i) if (i != s.id && s.vp[i][(int)VictoryPath::Blackout] > 50) suspect = true;
  if (suspect && s.capital > 400 + c.reserve) {
    bool has2 = false; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Substation && S.grid.dist(st.hex, s.crown) >= 3) has2 = true;
    if (!has2) for (Hex& h : S.hexes) if (h.owner == s.id && S.grid.dist(h.id, s.crown) >= 3 && S.grid.dist(h.id, s.crown) <= 4 && !structIn(S, h.id, s.id, StructKind::Substation, false)) { if (g.Build(s.id, StructKind::Substation, h.id)) break; }
  }
}

// ---------------------------------------------------------------- offence
double hostility(Ctx& c, int rid) { if (rid < 0 || rid >= 8) return 1; double h = 1; if (c.s.posture[rid] >= Posture::Contain) h *= 2; const Cartel* ca = cartelAgainst(c.S, rid); if (ca && std::find(ca->members.begin(), ca->members.end(), c.s.id) != ca->members.end()) h *= 1.5; if (synd(c.S, rid).shadowOf == c.s.id) h = 0; return h; }

std::vector<int> rivalTargets(Ctx& c) {
  std::vector<std::pair<double, int>> t;
  for (Hex& h : c.S.hexes) {
    if (h.owner < 0 || h.owner == c.s.id || h.type == Sector::Exchange) continue;
    if (!canSee(c.S, c.s.id, h.id) || !canReach(c.S, c.s, h)) continue;
    double v = sectorDef(h.type).cap * hostility(c, h.owner); if (h.id == synd(c.S, h.owner).crown) v += 10;
    if (v > 0) t.push_back({ v, h.id });
  }
  std::sort(t.begin(), t.end(), [](auto& a, auto& b) { return a.first > b.first; });
  std::vector<int> out; for (auto& p : t) out.push_back(p.second); return out;
}

void takeoverAttempts(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; std::set<int> net = networkHexes(S, s.id);
  for (Hex& h : S.hexes) {
    if (h.owner < 0 || h.owner == s.id || !h.brownout || h.C >= 50 || net.count(h.id)) continue;
    double P = h.P.count(s.id) ? h.P[s.id] : 0; if (P < 50 && !h.roots.count(s.id)) continue;
    int from = nearestNetworkHex(c, net, h.id); if (from < 0 || S.grid.dist(from, h.id) != 1) continue;
    if (!c.g.BuyRights(s.id, h.id)) continue;
    c.g.LayLink(s.id, LinkType::Trunk, { from, h.id }); return;
  }
}

void offence(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; Game& g = c.g;
  std::vector<int> targets = rivalTargets(c);
  int intrusions = 0; for (const Op& o : s.ops) if (o.kind == OpKind::Intrusion) ++intrusions;
  int maxIntr = s.arch == Arch::Ghost ? 3 : s.arch == Arch::Hive ? 1 : 2;
  for (int t : targets) { if (intrusions >= maxIntr) break; if (running(s, OpKind::Intrusion, t)) continue; if (g.QueueOp(s.id, OpKind::Intrusion, t)) ++intrusions; }
  // siphons / jams where presence allows
  int siphons = 0; for (const Op& o : s.ops) if (o.kind == OpKind::Siphon) ++siphons;
  for (Hex& h : S.hexes) {
    if (h.owner < 0 || h.owner == s.id) continue; double P = h.P.count(s.id) ? h.P[s.id] : 0; if (P < 40) continue;
    if (siphons < 3 && !running(s, OpKind::Siphon, h.id) && sectorDef(h.type).cap >= 15) { if (g.QueueOp(s.id, OpKind::Siphon, h.id)) ++siphons; }
    if (s.arch == Arch::Ghost && !structIn(S, h.id, s.id, StructKind::Tap, false) && s.capital > 200 + c.reserve) { bool rivalLink = false; for (auto& l : S.links) if (l.alive && l.built && l.sid != s.id && std::find(l.path.begin(), l.path.end(), h.id) != l.path.end()) rivalLink = true; if (rivalLink) g.Build(s.id, StructKind::Tap, h.id); }
    if (P >= 70 && !h.roots.count(s.id) && (s.posture[std::min(h.owner, 7)] >= Posture::Contain || s.arch == Arch::Ghost)) g.QueueOp(s.id, OpKind::Root, h.id);
  }
  takeoverAttempts(c);
  // scans for ghost/hive to keep vision fresh
  if ((s.arch != Arch::Hegemony) && S.cycle % 3 == 0) { for (Hex& h : S.hexes) { if (h.owner >= 0 && h.owner != s.id && canReach(S, s, h) && (!h.scans.count(s.id) || h.scans[s.id] < S.cycle - 3)) { g.QueueOp(s.id, OpKind::Scan, h.id); break; } } }
  // raids (hegemony)
  if (s.arch == Arch::Hegemony && has(s, Tech::FieldTeams) && s.capital > 200 + c.reserve) {
    for (Hex& h : S.hexes) {
      if (h.owner < 0 || h.owner == s.id || hostility(c, h.owner) < 1.5 || !canSee(S, s.id, h.id)) continue;
      bool sub = false; for (auto& st : S.structs) if (st.alive && st.built && st.hex == h.id && st.sid == h.owner && st.kind == StructKind::Substation && assetVisible(S, s.id, st)) sub = true;
      if (sub && g.QueueOp(s.id, OpKind::Raid, h.id, (int)RaidEffect::Darken)) break;
    }
  }
  // daemons (hive)
  if (s.arch == Arch::Hive) {
    int total = 0; for (auto& h : S.hexes) { auto it = h.daemons.find(s.id); if (it != h.daemons.end()) total += it->second; }
    if (total < c.m.daemonCap) { int best = -1; double bp = -1; for (Hex& h : S.hexes) { if (h.type == Sector::Barrier) continue; double P = h.owner == s.id ? 100 : (h.P.count(s.id) ? h.P[s.id] : 0); if (P < 10 && !canReach(S, s, h)) continue; double sc = sectorDef(h.type).pop + (h.owner != s.id ? 2 : 0) - (h.daemons.count(s.id) ? h.daemons[s.id] * 3 : 0); if (sc > bp) { bp = sc; best = h.id; } } if (best >= 0) g.QueueOp(s.id, OpKind::Daemon, best); }
  }
}

// ---------------------------------------------------------------- victory pursuit & containment
void pursue(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; Game& g = c.g;
  if (s.arch == Arch::Hegemony) {
    if (!s.hasPhase) g.TenderOffer(s.id);
    if (s.hasPhase && s.phase.type == VictoryPath::Takeover) { int pct = (int)std::floor((s.capital - c.reserve) / c.m.buyShareCost); if (pct >= 2) g.BuyShares(s.id, std::min(10, pct)); }
  }
  if (s.arch == Arch::Hive) {
    if (!s.hasPhase) g.StartTraining(s.id);
    if (s.hasPhase && s.phase.type == VictoryPath::Singularity && s.training == 10) g.Launch(s.id);
    // second path between T3 nodes for the ring
    if (hasDoc(s, Doctrine::V_Seed) && s.capital > 300 + c.reserve) {
      std::vector<int> t3; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Node && st.tier >= 3) t3.push_back(st.hex);
      std::set<int> exch; for (int ex : S.exchanges) if (structIn(S, ex, s.id, StructKind::Rack)) exch.insert(ex);
      for (int h : t3) if (!exch.empty() && !twoDisjointPaths(S, s.id, h, exch)) { for (int ex : exch) { PathPlan p = g.PlanPath(s.id, h, ex, LinkType::Trunk); if (p.ok && p.total < s.capital - c.reserve) { for (int r : p.needRights) g.BuyRights(s.id, r); g.LayLink(s.id, LinkType::Trunk, p.path); break; } } break; }
    }
  }
  if (s.arch == Arch::Ghost) { if (!s.hasPhase) g.DeclareBlackout(s.id); if (s.hasPhase && s.phase.type == VictoryPath::Blackout && hasDoc(s, Doctrine::G_ShadowBoard)) for (auto& r : S.synds) if (r.alive && r.id != s.id && r.shadowOf < 0) g.ShadowDirector(s.id, r.id); }
  // charter for anyone with lobbying
  if (has(s, Tech::Lobbying)) {
    int seats = seatsOf(S, s.id);
    if (seats >= 3 && s.capital > 400 + c.reserve) { int bribes = 0; for (auto& d : S.districts) if (d.bribes.count(s.id)) ++bribes; if (bribes < 2) for (int i = 0; i < (int)S.districts.size(); ++i) if (S.districts[i].seat != s.id && !S.districts[i].bribes.count(s.id)) { g.SetBribe(s.id, i, true); break; } }
    if (!s.hasPhase) g.ConsolidationMotion(s.id);
  }
}

void contain(Ctx& c) {
  GameState& S = c.S; Syndicate& s = c.s; Game& g = c.g;
  for (Syndicate& r : S.synds) {
    if (r.id == s.id || !r.alive || r.id >= 8 || s.posture[r.id] < Posture::Contain) continue;
    int path = 0; for (int i = 1; i < 4; ++i) if (s.vp[r.id][i] > s.vp[r.id][path]) path = i;
    switch ((VictoryPath)path) {
    case VictoryPath::Takeover:
      if (r.hasPhase && r.phase.type == VictoryPath::Takeover) { if (s.capital > 250 + c.reserve) g.BuyShares(s.id, 2); if (s.poisonUntil <= S.cycle && s.capital > 400) g.PoisonPill(s.id); if (r.exposure > 40 && s.capital > 300) g.Complaint(s.id, r.id); }
      break;
    case VictoryPath::Singularity:
      for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && st.kind == StructKind::Node && st.tier >= 3 && assetVisible(S, s.id, st)) { if (r.training >= 11) g.QueueOp(s.id, OpKind::DDoS, st.hex); if (!running(s, OpKind::Intrusion, st.hex)) g.QueueOp(s.id, OpKind::Intrusion, st.hex); }
      if (r.training >= 11) g.PetitionKillSwitch(s.id, r.id);
      break;
    case VictoryPath::Blackout: break;                 // defence() already hardens, purges and builds failover
    case VictoryPath::Charter:
      for (int i = 0; i < (int)S.districts.size(); ++i) { District& d = S.districts[i]; if (d.seat == r.id && d.bought) { if (has(s, Tech::Lobbying) && s.capital > 300) g.SetBribe(s.id, i, true); g.EthicsComplaint(s.id, i); break; } }
      if (r.hasPhase && r.phase.type == VictoryPath::Charter && s.capital > 300) g.Complaint(s.id, r.id);
      break;
    default: break;
    }
  }
}

} // namespace

void aiPlan(Game& g, int sid) {
  GameState& S = g.S(); Syndicate& s = synd(S, sid);
  if (!s.alive || S.over) return;
  if (s.shadowOf >= 0) { if (!running(s, OpKind::Harden, s.crown)) g.QueueOp(sid, OpKind::Harden, s.crown); s.planned = s.queued; return; }
  Ctx c{ g, S, s, modsOf(S, s), 40 + totalUpkeep(S, s) };
  threatBoard(c);
  cartels(c);
  economy(c);
  defence(c);
  offence(c);
  pursue(c);
  contain(c);
  s.planned = s.queued;
  for (const Op& o : s.ops) if (o.since == S.cycle) s.planned.push_back(o);
}

} } // namespace gl::sim
