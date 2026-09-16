// Public API: order validation (nothing mutates unless every check passes), queries, the turn driver.
#include "Internal.h"
#include <sstream>

namespace gl {
using namespace sim;

Game::Game(const Config& cfg) : cfg_(cfg) {
  createGame(st_, cfg);
  validateInvariants(st_, "create");
  for (auto& s : st_.synds) if (s.isAI) aiPlan(*this, s.id);
}

// ------------------------------------------------------------ helpers
static Result checkSid(const GameState& S, int sid) {
  if (sid < 0 || sid >= (int)S.synds.size()) return Result::Err("Bad syndicate id.");
  if (S.over) return Result::Err("The game is over.");
  if (!S.synds[sid].alive) return Result::Err("Syndicate is out of the game.");
  return Result::Ok();
}
static Result checkHex(const GameState& S, int h) { if (h < 0 || h >= (int)S.hexes.size()) return Result::Err("Bad hex id."); return Result::Ok(); }
static bool rightsOk(const GameState& S, const Hex& h, int sid) { return h.type == Sector::Barrier || h.type == Sector::Exchange || h.owner == sid || h.rights.count(sid); }
static bool canBuyRights(const GameState& S, const Hex& h, int sid, std::string* why) {
  if (h.type == Sector::Barrier) { if (why) *why = "Barriers carry conduit for free; buy rights in the hex beyond."; return false; }
  if (h.type == Sector::Exchange) { if (why) *why = "The Bureau owns the Exchange ducts; rights are free."; return false; }
  if (h.owner == sid || h.rights.count(sid)) { if (why) *why = "You already hold rights there."; return false; }
  if (h.owner >= 0 && h.C >= 50 && !(h.brownout && h.C < 50)) { if (why) *why = "Rival-controlled sector: wait for a brownout, Raid, Eminent Domain or Covert Conduit."; return false; }
  (void)S; return true;
}

// ------------------------------------------------------------ orders
Result Game::BuyRights(int sid, int hexId) {
  if (auto r = checkSid(st_, sid); !r) return r; if (auto r = checkHex(st_, hexId); !r) return r;
  Syndicate& s = synd(st_, sid); Hex& h = hex(st_, hexId); std::string why;
  if (!canBuyRights(st_, h, sid, &why)) return Result::Err(why);
  double cost = sectorDef(h.type).conduit; if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost; h.rights.insert(sid);
  return Result::Ok("Conduit rights bought for " + fmt(cost, 0) + ".");
}

PathPlan Game::PlanPath(int sid, int from, int to, LinkType type) const {
  PathPlan p; const GameState& S = st_;
  if (auto r = checkSid(S, sid); !r) { p.msg = r.msg; return p; }
  if (from < 0 || to < 0 || from >= (int)S.hexes.size() || to >= (int)S.hexes.size()) { p.msg = "Bad hex."; return p; }
  const Syndicate& s = synd(S, sid);
  std::set<int> net = networkHexes(S, sid);
  if (!net.count(from)) { p.msg = "Start hex must be in your network (a structure or an existing link)."; return p; }
  if (S.hexes[to].type == Sector::Barrier) { p.msg = "Cannot end a link on a barrier."; return p; }
  const LinkDef& ld = linkDef(type);
  if (ld.hasTech && !has(s, ld.tech)) { p.msg = std::string("Requires ") + techDef(ld.tech).name + "."; return p; }
  if (ld.archOnly && s.arch != ld.arch) { p.msg = "Archetype-exclusive link."; return p; }
  double perHex = linkCostPerHex(S, s, type);
  auto cost = [&](int a, int b) -> double {
    const Hex& hb = S.hexes[b]; const Hex& ha = S.hexes[a];
    if (hb.type == Sector::Barrier) return perHex + 0.5;
    if (!rightsOk(S, hb, sid) && !canBuyRights(S, hb, sid, nullptr)) return -1;
    double c = perHex + (rightsOk(S, hb, sid) ? 0 : sectorDef(hb.type).conduit) + (ha.type == Sector::Barrier ? 15 : 0);
    return c + 0.01;
  };
  p.path = astar(S.grid, from, to, cost);
  if (p.path.empty()) { p.msg = "No route: rival-held sectors block the way."; return p; }
  // hop count at the start hex: BFS over own links from resets
  std::set<int> resets; for (auto& st : S.structs) if (st.alive && st.built && st.sid == sid && (st.kind == StructKind::Node || st.kind == StructKind::Repeater)) resets.insert(st.hex);
  std::map<int, std::vector<int>> adj; for (auto& l : S.links) if (l.alive && l.sid == sid) for (size_t i = 0; i + 1 < l.path.size(); ++i) { adj[l.path[i]].push_back(l.path[i + 1]); adj[l.path[i + 1]].push_back(l.path[i]); }
  std::map<int, int> hops; std::vector<int> q(resets.begin(), resets.end()); for (int r : q) hops[r] = 0;
  for (size_t i = 0; i < q.size(); ++i) for (int n : adj[q[i]]) if (!hops.count(n)) { hops[n] = hops[q[i]] + 1; q.push_back(n); }
  int h0 = hops.count(from) ? hops[from] : 0; Mods m = modsOf(S, s);
  double surv = 1.0;
  for (size_t i = 1; i < p.path.size(); ++i) {
    const Hex& hb = S.hexes[p.path[i]]; const Hex& ha = S.hexes[p.path[i - 1]];
    int hn = h0 + (int)i; if (m.hopReset > 0) hn = ((h0 + (int)i - 1) % m.hopReset) + 1;
    surv *= 1.0 - std::min(K::MaxHopLoss, 0.02 + 0.01 * hn);
    p.linkCost += perHex + (ha.type == Sector::Barrier ? 15 : 0);
    if (hb.type != Sector::Barrier && !rightsOk(S, hb, sid)) { p.needRights.push_back(hb.id); p.rightsCost += sectorDef(hb.type).conduit; }
  }
  p.projectedSurvival = surv; p.total = p.linkCost + p.rightsCost; p.ok = true;
  return p;
}

Result Game::LayLink(int sid, LinkType type, const std::vector<int>& path) {
  if (auto r = checkSid(st_, sid); !r) return r;
  if (path.size() < 2) return Result::Err("A link needs at least two hexes.");
  Syndicate& s = synd(st_, sid); const LinkDef& ld = linkDef(type);
  if (ld.wireless) return Result::Err("Use LayUplink for wireless.");
  if (ld.hasTech && !has(s, ld.tech)) return Result::Err(std::string("Requires ") + techDef(ld.tech).name + ".");
  if (ld.archOnly && s.arch != ld.arch) return Result::Err("Archetype-exclusive link.");
  double cost = 0; std::set<int> net = networkHexes(st_, sid);
  if (!net.count(path[0]) && !net.count(path.back())) return Result::Err("One end must touch your network.");
  for (size_t i = 0; i < path.size(); ++i) {
    if (auto r = checkHex(st_, path[i]); !r) return r;
    const Hex& h = hex(st_, path[i]);
    if (i > 0) { if (st_.grid.dist(path[i - 1], path[i]) != 1) return Result::Err("Path hexes must be adjacent."); cost += linkCostPerHex(st_, s, type) + (hex(st_, path[i - 1]).type == Sector::Barrier ? 15 : 0); }
    if (!rightsOk(st_, h, sid)) return Result::Err("No conduit rights in #" + std::to_string(h.id) + ".");
  }
  if (hex(st_, path.back()).type == Sector::Barrier || hex(st_, path[0]).type == Sector::Barrier) return Result::Err("Links cannot end on a barrier.");
  if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost;
  addLink(st_, s, type, path, false);
  Mods m = modsOf(st_, s);
  if (m.xBuild > 0) { bool seen = false; for (int h : path) for (int n : st_.grid.neighborsV(h)) for (auto& st : st_.structs) if (st.alive && st.built && st.kind == StructKind::Array && st.sid != sid && (st.hex == n || st.hex == h)) seen = true; if (seen) s.exposure = std::min(100.0, s.exposure + m.xBuild); }
  return Result::Ok(std::string(ld.name) + " laid (" + std::to_string(path.size() - 1) + " segments, " + fmt(cost, 0) + "). Live next cycle.");
}

Result Game::LayUplink(int sid, int a, int b) {
  if (auto r = checkSid(st_, sid); !r) return r; if (auto r = checkHex(st_, a); !r) return r; if (auto r = checkHex(st_, b); !r) return r;
  Syndicate& s = synd(st_, sid); if (a == b) return Result::Err("Same hex.");
  if (st_.grid.dist(a, b) > 3) return Result::Err("Uplink range is 3 hexes.");
  if (hex(st_, a).type == Sector::Barrier || hex(st_, b).type == Sector::Barrier) return Result::Err("Towers cannot stand on barriers.");
  std::set<int> net = networkHexes(st_, sid); if (!net.count(a) && !net.count(b)) return Result::Err("One tower must be in your network.");
  double cost = linkDef(LinkType::Uplink).cost; if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost; addLink(st_, s, LinkType::Uplink, { a, b }, false);
  return Result::Ok("Uplink towers ordered. Live next cycle.");
}

Result Game::Build(int sid, StructKind kind, int hexId, int tier, Variant variant) {
  if (auto r = checkSid(st_, sid); !r) return r; if (auto r = checkHex(st_, hexId); !r) return r;
  Syndicate& s = synd(st_, sid); Hex& h = hex(st_, hexId); Mods m = modsOf(st_, s);
  if (kind == StructKind::COUNT) return Result::Err("Bad structure.");
  if (h.type == Sector::Barrier) return Result::Err("Nothing can be built on a barrier.");
  const StructDef& sd = structDef(kind);
  if (kind != StructKind::Node) {
    if (sd.archOnly && s.arch != sd.arch) return Result::Err("Archetype-exclusive structure.");
    if (sd.hasTech && !has(s, sd.tech)) return Result::Err(std::string("Requires ") + techDef(sd.tech).name + ".");
  }
  for (auto& st : st_.structs) if (st.alive && st.hex == hexId && st.sid == sid && st.kind == kind) return Result::Err("You already have one of those there.");
  bool own = h.owner == sid;
  switch (kind) {
  case StructKind::Node: {
    if (!own) return Result::Err("Nodes need a controlled sector.");
    if (variant == Variant::Citadel) { if (s.arch != Arch::Hegemony) return Result::Err("Citadel is Hegemony-only."); if (!has(s, Tech::HyperscaleCooling)) return Result::Err("Requires Hyperscale Cooling."); tier = 4; }
    else if (variant == Variant::Inference) { if (s.arch != Arch::Hive) return Result::Err("Inference Farm is Hive-only."); tier = 3; }
    else if (variant == Variant::Phantom) { if (s.arch != Arch::Ghost) return Result::Err("Phantom Node is Ghost-only."); tier = 2; }
    if (tier < 1 || tier > 4) return Result::Err("Tier 1..4.");
    if (tier > m.maxTier && variant != Variant::Citadel) return Result::Err("Tier " + std::to_string(tier) + " exceeds your maximum (" + std::to_string(m.maxTier) + ").");
    if (tier >= 3 && !has(s, Tech::ModularDC) && variant != Variant::Citadel) return Result::Err("Requires Modular Datacenters.");
    if (tier == 3 && !(h.type == Sector::Industrial || h.type == Sector::Campus)) return Result::Err("Tier 3 needs an Industrial or Campus hex.");
    break; }
  case StructKind::Repeater: if (!(own || (h.owner == -1 && h.rights.count(sid)))) return Result::Err("Repeaters need your sector or a neutral hex with your rights."); break;
  case StructKind::Rack: if (h.type != Sector::Exchange) return Result::Err("Peering Racks go in an Exchange."); if (!networkHexes(st_, sid).count(hexId)) return Result::Err("Lay fiber into the Exchange first."); break;
  case StructKind::Tap: { if (h.owner == sid || h.owner < 0) return Result::Err("Taps go in rival sectors."); double P = h.P.count(sid) ? h.P[sid] : 0; if (P < 40) return Result::Err("Need presence >= 40 there."); break; }
  case StructKind::PrivateGrid: if (!own) return Result::Err("Needs your sector."); if (h.type != Sector::Industrial) return Result::Err("Private Grid needs an Industrial hex."); break;
  case StructKind::Lab: case StructKind::Fork: if (!own || !structIn(st_, hexId, sid, StructKind::Node)) return Result::Err("Needs one of your built nodes in the hex."); break;
  default: if (!own) return Result::Err("Needs your controlled sector."); break;
  }
  double cost = buildCost(st_, s, kind, tier, variant);
  if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost;
  Structure& st = addStruct(st_, s, hexId, kind, kind == StructKind::Node ? tier : 0, variant, false);
  if (m.xBuild > 0) { for (int n : st_.grid.neighborsV(hexId)) for (auto& a : st_.structs) if (a.alive && a.built && a.kind == StructKind::Array && a.sid != sid && (a.hex == n || a.hex == hexId)) { s.exposure = std::min(100.0, s.exposure + m.xBuild); break; } }
  return Result::Ok(std::string(kind == StructKind::Node ? nodeDef(tier).name : sd.name) + " ordered (" + fmt(cost, 0) + "), ready in " + std::to_string(st.buildLeft) + " cycle(s).");
}

Result Game::QueueOp(int sid, OpKind kind, int target, int aux) {
  if (auto r = checkSid(st_, sid); !r) return r;
  Syndicate& s = synd(st_, sid); Mods m = modsOf(st_, s); const OpDef& d = opDef(kind);
  if (d.archOnly && s.arch != d.arch) return Result::Err("Archetype-exclusive operation.");
  if (kind == OpKind::Raid && !has(s, Tech::FieldTeams)) return Result::Err("Requires Field Teams.");
  if (kind == OpKind::Eminent && !hasDoc(s, Doctrine::H_Eminent)) return Result::Err("Requires the Eminent Domain doctrine.");
  if (kind == OpKind::Sever && !hasDoc(s, Doctrine::H_RightOfWay)) return Result::Err("Requires the Right of Way doctrine.");
  if (s.slotsUsed >= m.slots) return Result::Err("No action slots left this cycle (" + std::to_string(m.slots) + ").");
  auto queueIt = [&](bool sustained) {
    Op o; o.id = st_.nextId++; o.kind = kind; o.sid = sid; o.target = target; o.aux = aux; o.since = st_.cycle; o.sustained = sustained;
    if (sustained) s.ops.push_back(o); else s.queued.push_back(o);
    s.slotsUsed++;
    return Result::Ok(std::string(d.name) + " queued" + (sustained ? " (sustained)." : "."));
  };
  if (kind == OpKind::Oracle) { if (target < 0 || target >= (int)st_.synds.size() || target == sid) return Result::Err("Target a rival syndicate id."); if (s.M < 50) return Result::Err("Oracle needs M >= 50."); return queueIt(false); }
  if (kind == OpKind::Retrain) { return queueIt(false); }
  if (kind == OpKind::Repair) {
    Link* L = findLink(st_, target); if (!L || L->sid != sid) return Result::Err("Target must be one of your link ids.");
    if (aux < 0 || aux >= (int)L->segs.size()) return Result::Err("Bad segment index.");
    if (s.capital < 30) return Result::Err("Need 30 Capital."); s.capital -= 30; return queueIt(false);
  }
  if (auto r = checkHex(st_, target); !r) return r;
  Hex& h = hex(st_, target); double P = h.P.count(sid) ? h.P[sid] : 0;
  if (d.sustained) for (const Op& o : s.ops) if (o.kind == kind && o.target == target) return Result::Err("That operation is already running there.");
  if (d.own) { if (h.owner != sid) return Result::Err("Only in your own sector."); return queueIt(d.sustained); }
  if (h.type == Sector::Barrier) return Result::Err("Nothing to attack on a barrier.");
  switch (kind) {
  case OpKind::Scan: if (!canReach(st_, s, h)) return Result::Err("Out of reach."); return queueIt(false);
  case OpKind::Intrusion: if (h.owner == sid) return Result::Err("That is your subnet."); if (h.type == Sector::Exchange) return Result::Err("The Bureau's Exchange cannot be intruded."); if (!canReach(st_, s, h)) return Result::Err("Out of reach: need adjacency, presence, or Backbone reach."); return queueIt(true);
  case OpKind::Siphon: case OpKind::Jam: if (P < 40) return Result::Err("Need presence >= 40."); if (h.owner < 0 || h.owner == sid) return Result::Err("Target a rival sector."); return queueIt(true);
  case OpKind::Root: {
    if (P < 70) return Result::Err("Need presence >= 70.");
    bool crown = false; for (auto& st : st_.structs) if (st.alive && st.hex == target && st.crown) crown = true;
    if (crown && !has(s, Tech::KillChain)) return Result::Err("Crown Nodes need Kill Chain.");
    return queueIt(false); }
  case OpKind::DDoS: {
    if (!backboneReach(st_, s, h)) return Result::Err("DDoS needs Backbone reach (within 4 hexes of an Exchange you peer at).");
    bool node = false; for (auto& st : st_.structs) if (st.alive && st.built && st.hex == target && st.kind == StructKind::Node && st.sid != sid) node = true;
    if (!node) return Result::Err("No rival node there."); return queueIt(false); }
  case OpKind::Raid: {
    bool outpost = false; for (auto& st : st_.structs) if (st.alive && st.built && st.sid == sid && st.kind == StructKind::Outpost && st_.grid.dist(st.hex, target) <= m.raidRange) outpost = true;
    if (!outpost) return Result::Err("Need a built Outpost within " + std::to_string(m.raidRange) + " hexes.");
    if (aux < 0 || aux > 3) return Result::Err("Effect: 0 sever, 1 darken, 2 seize rights, 3 crown.");
    if (aux == 3 && !has(s, Tech::Decapitation)) return Result::Err("Crown raids need Decapitation.");
    if (h.owner == sid) return Result::Err("That is your sector.");
    if (s.capital < 60) return Result::Err("Need 60 Capital."); s.capital -= 60; return queueIt(false); }
  case OpKind::Eminent: { if (h.owner < 0 || h.owner == sid) return Result::Err("Target a rival sector."); if (h.C >= 70) return Result::Err("Their integrity is too high (>= 70)."); double c = 2 * sectorDef(h.type).conduit; if (s.capital < c) return Result::Err("Need " + fmt(c, 0) + " Capital."); s.capital -= c; return queueIt(false); }
  case OpKind::Sever: { if (h.owner != sid || h.C < 75) return Result::Err("You must hold the hex at integrity >= 75."); if (!structIn(st_, target, sid, StructKind::Outpost)) return Result::Err("Needs your Outpost in the hex."); return queueIt(false); }
  case OpKind::Covert: { if (h.owner < 0 || h.owner == sid) return Result::Err("Target a rival sector."); if (P < 60) return Result::Err("Need presence >= 60."); double c = 1.5 * sectorDef(h.type).conduit; if (s.capital < c) return Result::Err("Need " + fmt(c, 0) + " Capital."); s.capital -= c; return queueIt(false); }
  case OpKind::Daemon: { bool ok = h.owner == sid || P >= 10 || canReach(st_, s, h); if (!ok) return Result::Err("Daemons need presence >= 10 or reach."); int total = 0; for (auto& hx : st_.hexes) { auto it = hx.daemons.find(sid); if (it != hx.daemons.end()) total += it->second; } if (total >= m.daemonCap) return Result::Err("Daemon cap reached (" + std::to_string(m.daemonCap) + " = M/10)."); return queueIt(false); }
  default: return Result::Err("Unknown operation.");
  }
}

Result Game::CancelOp(int sid, int opId) {
  if (auto r = checkSid(st_, sid); !r) return r; Syndicate& s = synd(st_, sid);
  for (auto it = s.ops.begin(); it != s.ops.end(); ++it) if (it->id == opId) { s.ops.erase(it); return Result::Ok("Operation cancelled."); }
  for (auto it = s.queued.begin(); it != s.queued.end(); ++it) if (it->id == opId) { s.queued.erase(it); s.slotsUsed = std::max(0, s.slotsUsed - 1); return Result::Ok("Order withdrawn."); }
  return Result::Err("No such operation.");
}
Result Game::SetPriority(int sid, int hexId, Priority p) { if (auto r = checkSid(st_, sid); !r) return r; if (auto r = checkHex(st_, hexId); !r) return r; Hex& h = hex(st_, hexId); if (h.owner != sid) return Result::Err("Not your sector."); h.priority[sid] = p; return Result::Ok("Priority set."); }
Result Game::SetCompute(int sid, double pct) { if (auto r = checkSid(st_, sid); !r) return r; Syndicate& s = synd(st_, sid); double cap = modsOf(st_, s).computeCap; if (pct < 0 || pct > cap + 1e-9) return Result::Err("Compute share must be 0.." + fmt(cap * 100, 0) + "%."); s.computePct = pct; return Result::Ok("Compute allocation set."); }
Result Game::CanResearch(int sid, Tech t) const {
  const Syndicate& s = synd(st_, sid); if ((int)t < 0 || (int)t >= TechCount) return Result::Err("Bad tech.");
  if (has(s, t)) return Result::Err("Already researched.");
  for (Tech q : s.researchQueue) if (q == t) return Result::Err("Already queued.");
  const TechDef& d = techDef(t);
  if (d.tier > 1) { int lower = 0; for (int i = 0; i < TechCount; ++i) { const TechDef& o = techDef((Tech)i); if (o.branch == d.branch && o.tier == d.tier - 1 && has(s, (Tech)i)) ++lower; } if (lower < 2) return Result::Err(std::string("Needs 2 Tier ") + std::to_string(d.tier - 1) + " techs in " + branchName(d.branch) + "."); }
  return Result::Ok();
}
Result Game::QueueResearch(int sid, Tech t) { if (auto r = checkSid(st_, sid); !r) return r; if (auto r = CanResearch(sid, t); !r) return r; synd(st_, sid).researchQueue.push_back(t); return Result::Ok(std::string(techDef(t).name) + " queued."); }
Result Game::ClearResearch(int sid) { if (auto r = checkSid(st_, sid); !r) return r; synd(st_, sid).researchQueue.clear(); return Result::Ok("Research queue cleared (progress kept)."); }
Result Game::CanTakeDoctrine(int sid, Doctrine d) const {
  const Syndicate& s = synd(st_, sid); if ((int)d < 0 || (int)d >= DoctrineCount) return Result::Err("Bad doctrine.");
  const DoctrineDef& dd = doctrineDef(d); if (dd.arch != s.arch) return Result::Err("Not your archetype's doctrine.");
  if (hasDoc(s, d)) return Result::Err("Already adopted.");
  if (s.mandate < 1) return Result::Err("No Mandate available (Board Review every 12 cycles).");
  if (dd.tier > 1) { int lower = 0; for (int i = 0; i < DoctrineCount; ++i) if (doctrineDef((Doctrine)i).arch == s.arch && doctrineDef((Doctrine)i).tier == dd.tier - 1 && hasDoc(s, (Doctrine)i)) ++lower; if (lower < 2) return Result::Err("Needs 2 Tier " + std::to_string(dd.tier - 1) + " doctrines."); }
  return Result::Ok();
}
Result Game::TakeDoctrine(int sid, Doctrine d) { if (auto r = checkSid(st_, sid); !r) return r; if (auto r = CanTakeDoctrine(sid, d); !r) return r; Syndicate& s = synd(st_, sid); s.mandate--; s.doctrines[(int)d] = true; logMsg(st_, "PRESS RELEASE: " + s.name + " adopts the doctrine \"" + doctrineDef(d).name + "\"."); return Result::Ok(std::string(doctrineDef(d).name) + " adopted."); }
Result Game::HireExec(int sid, ExecSpec spec) { if (auto r = checkSid(st_, sid); !r) return r; Syndicate& s = synd(st_, sid); if ((int)s.execs.size() >= K::MaxExecs) return Result::Err("Board is full (5)."); if (s.capital < K::ExecCost) return Result::Err("Need 200 Capital."); s.capital -= K::ExecCost; s.execs.push_back({ spec, st_.cycle }); return Result::Ok(std::string(execDef(spec).name) + " hired. +1 action slot."); }
Result Game::BuyBandwidth(int sid, double amount) { if (auto r = checkSid(st_, sid); !r) return r; Syndicate& s = synd(st_, sid); bool peered = false; for (auto& kv : s.peeredLast) if (kv.second >= st_.cycle) peered = true; if (!peered) return Result::Err("You are not peered at an Exchange."); if (amount <= 0 || amount > K::BuyCap) return Result::Err("Buy 1..20 BW."); s.buyBW = amount; return Result::Ok("Order placed: " + fmt(amount, 0) + " BW at 4/BW, delivered at the Exchange next cycle."); }
Result Game::SetRowDenial(int sid, int hexId, bool on) { if (auto r = checkSid(st_, sid); !r) return r; if (auto r = checkHex(st_, hexId); !r) return r; Hex& h = hex(st_, hexId); if (h.owner != sid) return Result::Err("Not your sector."); h.rowDenial = on; return Result::Ok(on ? "Right-of-way denied to rivals (+10% loss on their links here)." : "Right-of-way restored."); }
Result Game::Seize(int sid, int structId) {
  if (auto r = checkSid(st_, sid); !r) return r; Structure* st = findStruct(st_, structId); if (!st) return Result::Err("No such structure.");
  if (st->sid == sid) return Result::Err("Already yours."); Hex& h = hex(st_, st->hex); if (h.owner != sid) return Result::Err("You do not control that sector.");
  if (st->orphanedAt < 0 || st_.cycle - st->orphanedAt < K::OrphanCycles) return Result::Err("Not yet seizable (orphans become seizable after 3 cycles).");
  Syndicate& s = synd(st_, sid); double cost = 0.5 * (st->kind == StructKind::Node ? nodeDef(st->tier).cost : structDef(st->kind).cost);
  if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost; int old = st->sid; st->sid = sid; st->orphanedAt = -1; st->crown = false; s.exposure = std::min(100.0, s.exposure + 2);
  logMsg(st_, "Your orphaned " + std::string(st->kind == StructKind::Node ? "node" : structDef(st->kind).name) + " in #" + std::to_string(st->hex) + " was seized by " + s.name + ".", old);
  return Result::Ok("Structure seized.");
}
Result Game::Scuttle(int sid, int structId) { if (auto r = checkSid(st_, sid); !r) return r; Structure* st = findStruct(st_, structId); if (!st || st->sid != sid) return Result::Err("Not your structure."); if (st->crown) return Result::Err("You cannot scuttle your Crown Node."); st->alive = false; return Result::Ok("Scuttled."); }

Result Game::TenderOffer(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vTenderOffer(st_, sid); }
Result Game::BuyShares(int sid, int pct) { if (auto r = checkSid(st_, sid); !r) return r; return vBuyShares(st_, sid, pct); }
Result Game::PoisonPill(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vPoisonPill(st_, sid); }
Result Game::Complaint(int sid, int t) { if (auto r = checkSid(st_, sid); !r) return r; return vComplaint(st_, sid, t); }
Result Game::Greenmail(int sid, int t) { if (auto r = checkSid(st_, sid); !r) return r; return vGreenmail(st_, sid, t); }
Result Game::StartTraining(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vStartTraining(st_, sid); }
Result Game::Launch(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vLaunch(st_, sid); }
Result Game::PetitionKillSwitch(int sid, int t) { if (auto r = checkSid(st_, sid); !r) return r; return vKillSwitch(st_, sid, t); }
Result Game::DeclareBlackout(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vDeclareBlackout(st_, sid); }
Result Game::ShadowDirector(int sid, int t) { if (auto r = checkSid(st_, sid); !r) return r; return vShadowDirector(st_, sid, t); }
Result Game::SetBribe(int sid, int d, bool on) { if (auto r = checkSid(st_, sid); !r) return r; return vSetBribe(st_, sid, d, on); }
Result Game::EthicsComplaint(int sid, int d) { if (auto r = checkSid(st_, sid); !r) return r; return vEthicsComplaint(st_, sid, d); }
Result Game::ConsolidationMotion(int sid) { if (auto r = checkSid(st_, sid); !r) return r; return vConsolidationMotion(st_, sid); }

// ------------------------------------------------------------ turn
void Game::EndCycle() {
  if (st_.over) return;
  resolveCycle(st_);
  if (!st_.over) for (auto& s : st_.synds) if (s.isAI && s.alive) aiPlan(*this, s.id);
}
void Game::RunAIForAll() { for (auto& s : st_.synds) if (s.alive) aiPlan(*this, s.id); }

// ------------------------------------------------------------ queries
bool Game::CanSee(int v, int h) const { return canSee(st_, v, h); }
bool Game::AssetVisible(int v, const Structure& s) const { return assetVisible(st_, v, s); }
bool Game::LinkVisible(int v, const Link& l) const { return linkVisible(st_, v, l); }
bool Game::PresenceVisible(int v, int h) const { return presenceVisible(st_, v, h); }
Mods Game::ModsOf(int sid) const { return modsOf(st_, synd(st_, sid)); }
int Game::Slots(int sid) const { return modsOf(st_, synd(st_, sid)).slots; }
double Game::UpkeepOf(int sid) const { return totalUpkeep(st_, synd(st_, sid)); }
double Game::SectorDemand(int h, int sid) const { return sectorDemand(st_, hex(st_, h), sid); }
double Game::RaidChance(int sid, int hexId) const {
  const Syndicate& s = synd(st_, sid); Mods m = modsOf(st_, s); const Hex& h = hex(st_, hexId);
  double c = m.raidBase + m.raidBonus; double P = h.P.count(sid) ? h.P.at(sid) : 0; c += 0.10 * std::floor(P / 25);
  for (auto& st : st_.structs) if (st.alive && st.built && st.sid != sid && st.kind == StructKind::Outpost && st_.grid.dist(st.hex, hexId) <= 1) c -= 0.30;
  if (h.owner >= 0) c += modsOf(st_, synd(st_, h.owner)).raidAgainst;
  return std::max(0.05, std::min(0.95, c));
}
std::vector<PathProgress> Game::Progress(int sid) const { return victoryProgress(st_, sid); }
int Game::SeatsOf(int sid) const { return seatsOf(st_, sid); }
std::vector<std::string> Game::RecentLog(int viewer, int max) const {
  std::vector<std::string> out;
  for (int i = (int)st_.log.size() - 1; i >= 0 && (int)out.size() < max; --i) { const LogEntry& e = st_.log[i]; if (e.sid == -1 || e.sid == viewer) out.push_back("[" + std::to_string(e.cycle) + "] " + e.text); }
  std::reverse(out.begin(), out.end()); return out;
}
const Structure* Game::StructAt(int h, int sid, StructKind k) const { return structIn(st_, h, sid, k, false); }
std::vector<const Structure*> Game::StructsIn(int h) const { return structsIn(st_, h); }
std::vector<const Link*> Game::LinksThrough(int h) const { std::vector<const Link*> out; for (auto& l : st_.links) if (l.alive && std::find(l.path.begin(), l.path.end(), h) != l.path.end()) out.push_back(&l); return out; }

std::string Game::StateHash() const {
  uint64_t h = 1469598103934665603ull;
  auto mix = [&](long long v) { h ^= (uint64_t)v; h *= 1099511628211ull; };
  mix(st_.cycle); mix(st_.rngState);
  for (auto& s : st_.synds) { mix((long long)std::llround(s.capital * 1000)); mix((long long)std::llround(s.exposure * 1000)); mix((long long)std::llround(s.M * 1000)); mix((long long)s.ops.size()); mix((long long)std::llround(s.valuation)); }
  for (auto& x : st_.hexes) { mix(x.owner); mix((long long)std::llround(x.C * 1000)); mix((long long)std::llround(x.fw * 1000)); for (auto& kv : x.P) { mix(kv.first); mix((long long)std::llround(kv.second * 1000)); } }
  for (auto& st : st_.structs) if (st.alive) { mix(st.id); mix(st.hex); mix(st.built); mix((long long)std::llround(st.powerFrac * 1000)); }
  char buf[32]; std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)h); return buf;
}

} // namespace gl
