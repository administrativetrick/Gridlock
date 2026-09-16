#include "Internal.h"

namespace gl { namespace sim {

void generateMap(GameState& S, Rng& rng, int W, int H, int nSynd, int nEx);

// ------------------------------------------------------------------ lookups
Structure* findStruct(GameState& S, int id) { for (auto& st : S.structs) if (st.id == id && st.alive) return &st; return nullptr; }
const Structure* findStruct(const GameState& S, int id) { for (auto& st : S.structs) if (st.id == id && st.alive) return &st; return nullptr; }
const Structure* structIn(const GameState& S, int hexId, int sid, StructKind kind, bool builtOnly) {
  for (auto& st : S.structs) if (st.alive && st.hex == hexId && st.sid == sid && st.kind == kind && (!builtOnly || st.built)) return &st;
  return nullptr;
}
Structure* structIn(GameState& S, int hexId, int sid, StructKind kind, bool builtOnly) {
  for (auto& st : S.structs) if (st.alive && st.hex == hexId && st.sid == sid && st.kind == kind && (!builtOnly || st.built)) return &st;
  return nullptr;
}
std::vector<Structure*> structsOf(GameState& S, int sid, StructKind kind) {
  std::vector<Structure*> out; for (auto& st : S.structs) if (st.alive && st.sid == sid && (kind == StructKind::COUNT || st.kind == kind)) out.push_back(&st); return out;
}
std::vector<const Structure*> structsOf(const GameState& S, int sid, StructKind kind) {
  std::vector<const Structure*> out; for (auto& st : S.structs) if (st.alive && st.sid == sid && (kind == StructKind::COUNT || st.kind == kind)) out.push_back(&st); return out;
}
std::vector<const Structure*> structsIn(const GameState& S, int hexId) {
  std::vector<const Structure*> out; for (auto& st : S.structs) if (st.alive && st.hex == hexId) out.push_back(&st); return out;
}
Link* findLink(GameState& S, int id) { for (auto& l : S.links) if (l.id == id && l.alive) return &l; return nullptr; }
bool anyStructIn(const GameState& S, int hexId, int sid) { for (auto& st : S.structs) if (st.alive && st.hex == hexId && st.sid == sid) return true; return false; }
std::vector<int> ownedHexes(const GameState& S, int sid) { std::vector<int> o; for (auto& h : S.hexes) if (h.owner == sid) o.push_back(h.id); return o; }
int totalSectors(const GameState& S) { int n = 0; for (auto& h : S.hexes) if (h.type != Sector::Barrier && h.type != Sector::Exchange) ++n; return n; }
std::set<int> networkHexes(const GameState& S, int sid) {
  std::set<int> out;
  for (auto& st : S.structs) if (st.alive && st.sid == sid) out.insert(st.hex);
  for (auto& l : S.links) if (l.alive && l.sid == sid) for (int id : l.path) out.insert(id);
  return out;
}
void logMsg(GameState& S, const std::string& text, int sid) {
  S.log.push_back({ S.cycle, sid, text });
  if (S.log.size() > 600) S.log.erase(S.log.begin(), S.log.begin() + 200);
}
std::vector<int> aliveRivals(const GameState& S, int sid) { std::vector<int> o; for (auto& s : S.synds) if (s.alive && s.id != sid) o.push_back(s.id); return o; }
const Cartel* cartelAgainst(const GameState& S, int target) { for (auto& c : S.cartels) if (c.target == target) return &c; return nullptr; }
bool inCartelWith(const GameState& S, int a, int b) {
  for (auto& c : S.cartels) { bool fa = false, fb = false; for (int m : c.members) { if (m == a) fa = true; if (m == b) fb = true; } if (fa && fb) return true; }
  return false;
}
bool isSubsidiary(const GameState& S, int sid) { return synd(S, sid).shadowOf >= 0; }

// ------------------------------------------------------------------ construction
Structure& addStruct(GameState& S, Syndicate& s, int hexId, StructKind kind, int tier, Variant v, bool built, bool crown) {
  Structure st; st.id = S.nextId++; st.kind = kind; st.sid = s.id; st.hex = hexId; st.tier = tier; st.variant = v; st.crown = crown;
  int build = kind == StructKind::Node ? nodeDef(tier).build : structDef(kind).build;
  if (v == Variant::Inference) build = 5;
  if (!built) build = std::max(1, build - modsOf(S, s).buildSpeed);
  st.built = built; st.buildLeft = built ? 0 : build;
  S.structs.push_back(st);
  return S.structs.back();
}
Link& addLink(GameState& S, Syndicate& s, LinkType type, const std::vector<int>& path, bool built) {
  GL_CHECK(path.size() >= 2, "link path needs two hexes");
  const LinkDef& d = linkDef(type); Mods m = modsOf(S, s);
  double cap = type == LinkType::Trunk ? m.trunkCap : type == LinkType::Uplink ? m.uplinkCap : d.cap;
  Link l; l.id = S.nextId++; l.sid = s.id; l.type = type; l.path = path; l.wireless = d.wireless; l.built = built; l.readyAt = built ? S.cycle : S.cycle + 1;
  if (l.wireless) { Segment sg; sg.a = path[0]; sg.b = path[1]; sg.cap = cap; sg.wireless = true; sg.dist = S.grid.dist(path[0], path[1]); l.segs.push_back(sg); }
  else for (size_t i = 0; i + 1 < path.size(); ++i) { Segment sg; sg.a = path[i]; sg.b = path[i + 1]; sg.cap = cap; l.segs.push_back(sg); }
  S.links.push_back(l);
  return S.links.back();
}

void createGame(GameState& S, const Config& cfg) {
  S.seedLabel = cfg.seed; S.seed = hashStr(cfg.seed); Rng rng(S.seed);
  int n = 1 + (int)cfg.ais.size();
  GL_CHECK(n >= 2 && n <= 6, "2..6 syndicates");
  generateMap(S, rng, cfg.W, cfg.H, n, 3);
  S.districts.assign(S.districtCount, District{});
  int used[3] = { 0, 0, 0 };
  for (int i = 0; i < n; ++i) {
    Syndicate s; s.id = i; s.arch = i == 0 ? cfg.player : cfg.ais[i - 1]; s.isAI = i != 0 || cfg.playerIsAI;
    s.name = (i == 0 && !cfg.playerIsAI) ? "Your Syndicate" : aiName(s.arch, used[(int)s.arch]++);
    s.capital = K::StartCapital; s.M = s.arch == Arch::Hive ? 20 : 0;
    s.execs = { Exec{ ExecSpec::CIO, 0 }, Exec{ ExecSpec::CSO, 0 } };
    S.synds.push_back(s);
  }
  for (int i = 0; i < n; ++i) {
    Syndicate& s = S.synds[i]; int start = S.starts[i]; Hex& h = S.hexes[start];
    s.crown = start; h.owner = i; h.C = 100; h.fw = 50; h.rights.insert(i);
    addStruct(S, s, start, StructKind::Node, 2, Variant::None, true, true);
    addStruct(S, s, start, StructKind::Substation, 0, Variant::None, true);
    int placed = 0;
    for (int nb : S.grid.neighborsV(start)) {
      Hex& nh = S.hexes[nb];
      if (nh.type == Sector::Barrier || nh.type == Sector::Exchange || nh.owner != -1 || placed >= 3) continue;
      nh.owner = i; nh.C = 100; nh.fw = sectorDef(nh.type).fw; nh.rights.insert(i);
      addLink(S, s, LinkType::Trunk, { start, nb }, true);
      ++placed;
    }
    s.sectorsAtReview = 1 + placed;
  }
  S.rngState = rng.state();
  logMsg(S, "The city wakes. " + std::to_string(n) + " syndicates, one grid.");
}

// ------------------------------------------------------------------ derived numbers
Mods modsOf(const GameState& S, const Syndicate& s) {
  Mods m; m.slots = K::BaseSlots + (int)s.execs.size();
  if (s.wetworkUntil > S.cycle) m.slots -= 1;
  switch (s.arch) {
  case Arch::Hegemony:
    m.nodeOut = 1.25; m.buildCost = 0.8; m.linkCost = 0.8; m.trunkCap = 30; m.subRange = 3;
    m.research[(int)Branch::Infrastructure] *= 0.7; m.research[(int)Branch::Netwar] *= 1.3; m.fwAdd -= 10; m.xRaid = 1.25; m.xBuild = 2; m.intrusionCost = 1.25; m.hopReset = 3;
    break;
  case Arch::Ghost:
    m.hidden = true; m.xMult = 0.5; m.intrusionCost = 0.7; m.intrusionX = 0; m.pDecay = 1; m.yieldMult = 0.8; m.nodeOut = 0.75;
    m.maxTier = has(s, Tech::ModularDC) ? 3 : 2; m.research[(int)Branch::Netwar] *= 0.75; m.research[(int)Branch::BlackOps] *= 0.75; m.research[(int)Branch::Infrastructure] *= 1.4;
    m.attribution = 0.4; m.scanEvasion = 0.5;
    break;
  case Arch::Hive:
    m.computeCap = 1.0; m.researchSlots = 2; for (double& r : m.research) r *= 0.8; m.research[(int)Branch::Netwar] *= 0.8; m.research[(int)Branch::BlackOps] *= 1.5;
    m.powerMult = 1.3; m.opsCost = 1.0 - s.M / 200.0; m.fwAdd += 5; m.adaptive = 1; m.raidAgainst = 0.10; m.daemonCap = (int)(s.M / 10);
    break;
  default: break;
  }
  if (has(s, Tech::Trenching)) m.linkCost *= 0.75;
  if (has(s, Tech::GridContracts)) m.gridCap = K::GridCapTech;
  if (has(s, Tech::MicrowaveMesh)) { m.uplinkLoss = 0.06; m.uplinkCap = 12; }
  if (has(s, Tech::FortifiedConduit)) m.severImmune = true;
  if (has(s, Tech::HardenedKernels)) m.fwAdd += 10;
  if (has(s, Tech::Persistence)) m.pDecay = std::min(m.pDecay, 2.0);
  if (has(s, Tech::TrafficShaping)) m.jam = 0.20;
  if (has(s, Tech::KillChain)) m.rootCost = 8;
  if (has(s, Tech::AdaptiveFW)) m.adaptive = std::max(m.adaptive, 2.0);
  if (has(s, Tech::DPI)) m.dpi = true;
  if (has(s, Tech::ShellCompanies)) m.xCool = K::ExposureCoolTech;
  if (has(s, Tech::FuturesDesk)) m.sellMult *= 1.25;
  if (has(s, Tech::VerticalIntegration)) m.nodeUpkeep *= 0.8;
  if (has(s, Tech::DataBrokerage)) m.skim = 0.3;
  if (has(s, Tech::Buyback)) m.buyShareCost = 70;
  if (has(s, Tech::DeadDrops)) m.bufferCap = K::BufferCapTech;
  if (has(s, Tech::SabotageDoctrine)) m.raidBonus += 0.2;
  if (has(s, Tech::FalseFlags)) m.attribution = std::max(m.attribution, 0.3);
  if (hasDoc(s, Doctrine::H_Vertical)) m.nodeUpkeep *= 0.75;
  if (hasDoc(s, Doctrine::H_Kinetic)) { m.raidRange = 3; m.raidBase = 0.7; m.xRaid *= 1.5; }
  if (hasDoc(s, Doctrine::H_PeeringCartel)) { m.sellFloor = 1.5; m.bureauAdd -= 10; }
  if (hasDoc(s, Doctrine::H_Monopoly)) { m.takeoverTrigger = 0.35; m.takeoverWin = 0.45; }
  if (hasDoc(s, Doctrine::G_FalseFlag)) m.attribution = 0.6;
  if (hasDoc(s, Doctrine::G_Deniability)) { m.xMult = 0.25; m.yieldMult = 0.7; }
  if (hasDoc(s, Doctrine::V_DataLake)) m.mGain = 1.5;
  if (hasDoc(s, Doctrine::V_Adversarial)) { m.adaptive = 4; m.purgeCost = 3; }
  if (hasDoc(s, Doctrine::V_Generative)) { m.daemonCap = (int)(s.M / 5); m.copyChance = 0.45; }
  if (hasDoc(s, Doctrine::V_AlignmentWaiver)) m.coherenceMult = 0.75;
  for (const Exec& e : s.execs) switch (e.spec) {
    case ExecSpec::CIO: m.buildSpeed = 1; break;
    case ExecSpec::CSO: m.fwAdd += 5; break;
    case ExecSpec::HOI: m.intrusionGain += 3; break;
    case ExecSpec::CFO: m.upkeepMult *= 0.95; break;
    case ExecSpec::Fixer: m.raidBonus += 0.1; m.xRaid *= 0.75; break;
    case ExecSpec::Counsel: m.bureauAdd += 10; break;
    default: break;
  }
  int labs = 0; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Lab) ++labs;
  m.researchSlots = std::min(m.researchSlots + labs, s.arch == Arch::Hive ? 4 : 3);
  return m;
}

double nodeRaw(const GameState& S, const Syndicate& s, const Structure& st) {
  GL_CHECK(st.kind == StructKind::Node, "nodeRaw on non-node");
  Mods m = modsOf(S, s);
  double raw = nodeDef(st.tier).raw;
  if (st.variant == Variant::Phantom) raw *= 0.75;
  if (st.variant == Variant::Inference) raw = 60;
  raw *= m.nodeOut;
  if (st.tier == 3 && has(s, Tech::HyperscaleCooling)) raw *= 1.25;
  if (st.ddosedUntil >= S.cycle) raw *= 0.5;
  if (st.crown && s.decapUntil >= S.cycle) raw *= 0.5;
  return raw;
}
double nodeMW(const GameState& S, const Syndicate& s, const Structure& st) {
  double mw = nodeDef(st.tier).mw; if (st.variant == Variant::Inference) mw = 20;
  return mw * modsOf(S, s).powerMult;
}
double nodeFwBase(const Structure& st) { return st.variant == Variant::Phantom ? 45 : nodeDef(st.tier).fw; }

double sectorDemand(const GameState& S, const Hex& h, int sid) {
  if (h.type == Sector::Barrier) return 0;
  const SectorDef& def = sectorDef(h.type);
  if (h.type == Sector::Exchange) return def.demand;
  double d = def.demand; const Syndicate& s = synd(S, sid);
  bool contested = false; for (auto& kv : h.P) if (kv.first != sid && kv.second >= 40) contested = true;
  if (contested) d += 2;
  if (structIn(S, h.id, sid, StructKind::Array)) d += 1;
  if (s.auditUntil > S.cycle) d += 1;
  for (int rid : h.roots) if (rid != sid) d += 4;
  if (hasDoc(s, Doctrine::H_CompanyTown) && (h.type == Sector::Sprawl || h.type == Sector::Arcology)) {
    bool near = false;
    for (int n : S.grid.neighborsV(h.id)) if (structIn(S, n, sid, StructKind::Substation) || structIn(S, n, sid, StructKind::PrivateGrid)) near = true;
    if (near) d = std::max(1.0, d - 2);
  }
  return d;
}

double fwBaseFor(const GameState& S, const Hex& h) {
  if (h.type == Sector::Exchange) return 60;
  double f = sectorDef(h.type).fw;
  if (h.owner >= 0) {
    const Syndicate& s = synd(S, h.owner); Mods m = modsOf(S, s);
    f += m.fwAdd;
    const Structure* node = structIn(S, h.id, h.owner, StructKind::Node);
    if (node) f = std::max(f, nodeFwBase(*node) + m.fwAdd);
    if (hasDoc(s, Doctrine::H_Fortress) && (structIn(S, h.id, h.owner, StructKind::Substation) || structIn(S, h.id, h.owner, StructKind::Outpost) || structIn(S, h.id, h.owner, StructKind::PrivateGrid))) f += 30;
  }
  return std::max(0.0, std::min(100.0, f));
}
double maxRivalP(const Hex& h, int sid) { double m = 0; for (auto& kv : h.P) if (kv.first != sid && kv.second > m) m = kv.second; return m; }

bool backboneReach(const GameState& S, const Syndicate& s, const Hex& h) {
  for (int ex : s.flow.peered) if (S.grid.dist(ex, h.id) <= 4) return true;
  return false;
}
bool canReach(const GameState& S, const Syndicate& s, const Hex& h) {
  if (h.owner == s.id) return true;
  auto it = h.P.find(s.id); if (it != h.P.end() && it->second >= 1) return true;
  for (int n : S.grid.neighborsV(h.id)) {
    const Hex& nh = S.hexes[n];
    if ((nh.owner == s.id && nh.C >= 25) || anyStructIn(S, n, s.id)) return true;
    if (hasDoc(s, Doctrine::G_LOTL)) { auto p = nh.P.find(s.id); if (p != nh.P.end() && p->second >= 60) return true; }
  }
  return backboneReach(S, s, h);
}

double buildCost(const GameState& S, const Syndicate& s, StructKind kind, int tier, Variant v) {
  Mods m = modsOf(S, s);
  if (kind == StructKind::Node) { double c = nodeDef(tier).cost; if (v == Variant::Inference) c = 850; return std::round(c * m.buildCost); }
  return std::round(structDef(kind).cost * m.buildCost);
}
double linkCostPerHex(const GameState& S, const Syndicate& s, LinkType t) { return std::round(linkDef(t).cost * modsOf(S, s).linkCost); }
double conduitCost(const GameState& S, const Hex& to, const Hex* from) {
  (void)S; double c = sectorDef(to.type).conduit;
  if (from && from->type == Sector::Barrier && to.type != Sector::Barrier) c += 15;
  return c;
}

double totalUpkeep(const GameState& S, const Syndicate& s) {
  Mods m = modsOf(S, s); double u = 0;
  for (auto& st : S.structs) {
    if (!st.alive || st.sid != s.id || !st.built) continue;
    if (st.kind == StructKind::Node) { double nu = nodeDef(st.tier).upkeep * m.nodeUpkeep; if (hasDoc(s, Doctrine::V_Autoscaling)) nu *= std::max(0.2, st.util); u += nu; }
    else u += structDef(st.kind).upkeep;
  }
  for (auto& l : S.links) if (l.alive && l.sid == s.id && l.built) u += l.wireless ? linkDef(l.type).upkeep : linkDef(l.type).upkeep * (double)l.segs.size();
  u += K::ExecUpkeep * (double)std::max<size_t>(0, s.execs.size() > 2 ? s.execs.size() - 2 : 0);   // the two founders draw no salary
  return u * m.upkeepMult;
}

} } // namespace gl::sim
