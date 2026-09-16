// Asymmetric endings (docs/03): triggers, multi-cycle contests, failure states, the Emergent Intelligence.
#include "Internal.h"

namespace gl { namespace sim {

static double roll(GameState& S) { Rng r(S.rngState); double v = r.next(); S.rngState = r.state(); return v; }
static int rollInt(GameState& S, int n) { Rng r(S.rngState); int v = r.intn(n); S.rngState = r.state(); return v; }
static double clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }

static int peeredNow(const GameState& S, const Syndicate& s) { int n = 0; for (auto& kv : s.peeredLast) if (kv.second >= S.cycle - 1) ++n; return n; }
int seatsOf(const GameState& S, int sid) { int n = 0; for (auto& d : S.districts) if (d.seat == sid) ++n; return n; }
static bool singularityUnlocked(const Syndicate& s) { return hasDoc(s, Doctrine::V_Seed) || (has(s, Tech::KillChain) && has(s, Tech::HyperscaleCooling) && has(s, Tech::BackboneSniffing)); }
static bool blackoutUnlocked(const Syndicate& s) { return has(s, Tech::BlackoutProtocol) || hasDoc(s, Doctrine::G_Blackout) || hasDoc(s, Doctrine::G_ShadowBoard); }

bool seedClusterOk(const GameState& S, const Syndicate& s, std::vector<int>* seedNodes) {
  int need = hasDoc(s, Doctrine::V_Seed) ? 2 : 3;
  std::set<int> exch; for (int ex : S.exchanges) if (structIn(S, ex, s.id, StructKind::Rack)) exch.insert(ex);
  if (exch.empty()) return false;
  std::vector<int> ok;
  for (auto& st : S.structs) {
    if (!st.alive || !st.built || st.sid != s.id || st.kind != StructKind::Node || st.tier < 3) continue;
    if (hasDoc(s, Doctrine::V_Seed) && st.variant != Variant::Inference) continue;
    if (twoDisjointPaths(S, s.id, st.hex, exch)) ok.push_back(st.id);
  }
  if (seedNodes) *seedNodes = ok;
  return (int)ok.size() >= need;
}

static void win(GameState& S, int sid, VictoryPath p, const std::string& text) {
  S.over = true; S.victory = { sid, p, S.cycle };
  logMsg(S, "VICTORY: " + synd(S, sid).name + " — " + text);
}
static void endPhase(Syndicate& s, VictoryPath p, int cooldown, int cycle) { s.hasPhase = false; s.cooldownUntil[(int)p] = cycle + cooldown; }
static Result noPhase(const GameState& S, const Syndicate& s, VictoryPath p) {
  if (S.over) return Result::Err("The game is over.");
  if (s.shadowOf >= 0) return Result::Err("A subsidiary cannot pursue a victory.");
  if (s.hasPhase) return Result::Err("A victory phase is already active.");
  if (s.cooldownUntil[(int)p] > S.cycle) return Result::Err("Cooldown: " + std::to_string(s.cooldownUntil[(int)p] - S.cycle) + " cycles.");
  return Result::Ok();
}

// ------------------------------------------------------------ verbs
Result vTenderOffer(GameState& S, int sid) {
  Syndicate& s = synd(S, sid); Mods m = modsOf(S, s);
  if (auto r = noPhase(S, s, VictoryPath::Takeover); !r) return r;
  if (!has(s, Tech::TenderOffer)) return Result::Err("Requires Tender Offer Authority.");
  if (s.share < m.takeoverTrigger) return Result::Err("Share " + fmt(s.share * 100) + "% < trigger " + fmt(m.takeoverTrigger * 100, 0) + "%.");
  if (peeredNow(S, s) < 2) return Result::Err("Must be peered at 2 Exchanges.");
  if (s.exposure >= 60) return Result::Err("Exposure must be below 60.");
  s.hasPhase = true; s.phase = Phase{}; s.phase.type = VictoryPath::Takeover; s.phase.start = S.cycle; s.phase.cyclesLeft = 8;
  logMsg(S, "TENDER OFFER: " + s.name + " has filed. Proxy Fight resolves in 8 cycles.");
  return Result::Ok("Tender Offer filed.");
}
static Syndicate* activeBidder(GameState& S) { for (auto& s : S.synds) if (s.hasPhase && s.phase.type == VictoryPath::Takeover) return &s; return nullptr; }
Result vBuyShares(GameState& S, int sid, int pct) {
  Syndicate& s = synd(S, sid); Mods m = modsOf(S, s);
  if (!activeBidder(S)) return Result::Err("No Proxy Fight is active.");
  if (pct < 1 || pct > 20) return Result::Err("Buy 1..20 percent.");
  if (s.abstainUntil > S.cycle) return Result::Err("Greenmailed: abstaining.");
  double cost = pct * m.buyShareCost; if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost; s.shareAdj += pct; computeValuations(S);
  logMsg(S, s.name + " bought " + std::to_string(pct) + "% of the float.");
  return Result::Ok("Shares bought.");
}
Result vPoisonPill(GameState& S, int sid) {
  Syndicate& s = synd(S, sid); Syndicate* b = activeBidder(S);
  if (!b || b->id == sid) return Result::Err("Only a rival of an active bidder can swallow a poison pill.");
  if (s.poisonUntil > S.cycle) return Result::Err("Already active.");
  if (s.capital < 250) return Result::Err("Need 250 Capital.");
  s.capital -= 250; s.poisonUntil = S.cycle + b->phase.cyclesLeft + 1; computeValuations(S);
  logMsg(S, s.name + " swallowed a poison pill: valuation x1.25 for the fight.");
  return Result::Ok("Poison pill active.");
}
Result vComplaint(GameState& S, int sid, int target) {
  Syndicate& s = synd(S, sid); if (target < 0 || target >= (int)S.synds.size() || target == sid) return Result::Err("Bad target.");
  Syndicate& t = synd(S, target);
  if (!(t.hasPhase && (t.phase.type == VictoryPath::Takeover || t.phase.type == VictoryPath::Charter))) return Result::Err("Target is not in a Proxy Fight or Vote.");
  double cost = t.phase.type == VictoryPath::Takeover ? 150 : 200; if (s.capital < cost) return Result::Err("Need " + fmt(cost, 0) + " Capital.");
  s.capital -= cost; s.exposure = clampd(s.exposure + 5, 0, 100); t.exposure = clampd(t.exposure + (t.phase.type == VictoryPath::Takeover ? 15 : 10), 0, 100);
  logMsg(S, s.name + " filed a regulatory complaint against " + t.name + ".");
  return Result::Ok("Complaint filed.");
}
Result vGreenmail(GameState& S, int sid, int target) {
  Syndicate& s = synd(S, sid); if (!(s.hasPhase && s.phase.type == VictoryPath::Takeover)) return Result::Err("Only the bidder can greenmail.");
  if (target < 0 || target >= (int)S.synds.size() || target == sid) return Result::Err("Bad target.");
  Syndicate& t = synd(S, target); if (t.share >= 0.15) return Result::Err(t.name + " holds too much to be bought off (share >= 15%).");
  if (s.capital < 300) return Result::Err("Need 300 Capital.");
  s.capital -= 300; t.capital += 300; t.abstainUntil = S.cycle + 2;
  logMsg(S, s.name + " greenmailed " + t.name + ": they abstain for 2 cycles.");
  return Result::Ok("Greenmail accepted.");
}
Result vStartTraining(GameState& S, int sid) {
  Syndicate& s = synd(S, sid);
  if (auto r = noPhase(S, s, VictoryPath::Singularity); !r) return r;
  if (!singularityUnlocked(s)) return Result::Err("Requires Seed Protocol, or Kill Chain + Hyperscale Cooling + Backbone Sniffing.");
  if (!seedClusterOk(S, s, nullptr)) return Result::Err("Seed cluster incomplete: Tier 3 nodes each need two disjoint paths to a peered Exchange.");
  if (s.exposure >= 80) return Result::Err("Exposure must be below 80.");
  s.hasPhase = true; s.phase = Phase{}; s.phase.type = VictoryPath::Singularity; s.phase.start = S.cycle; s.phase.cyclesLeft = 10; s.training = 0;
  logMsg(S, "Seed training started. Feed >= 150 Compute per cycle across the cluster for 10 cycles.", sid);
  return Result::Ok("Training started.");
}
Result vLaunch(GameState& S, int sid) {
  Syndicate& s = synd(S, sid);
  if (!(s.hasPhase && s.phase.type == VictoryPath::Singularity)) return Result::Err("No trained Seed.");
  if (s.training < 10) return Result::Err("Training incomplete (" + std::to_string(s.training) + "/10).");
  if (s.training >= 11) return Result::Err("Already launching.");
  s.training = 11; s.phase.cyclesLeft = 6; s.phase.coherence = 40;
  for (auto& o : S.synds) o.killSwitchUsed = false;
  logMsg(S, "SINGULARITY LAUNCH: " + s.name + " is pushing a Seed into the Backbone. Coherence 40/100, 6 cycles.");
  return Result::Ok("Launched.");
}
Result vKillSwitch(GameState& S, int sid, int target) {
  Syndicate& s = synd(S, sid); if (seatsOf(S, sid) < 3) return Result::Err("Need 3 District seats to petition the Bureau.");
  if (target < 0 || target >= (int)S.synds.size()) return Result::Err("Bad target.");
  Syndicate& t = synd(S, target); if (!(t.hasPhase && t.phase.type == VictoryPath::Singularity && t.training >= 11)) return Result::Err("Target is not launching.");
  if (s.killSwitchUsed) return Result::Err("You already petitioned this launch.");
  if (S.cycle < t.phase.start + 1 && t.phase.cyclesLeft == 6) return Result::Err("The Bureau acts from launch cycle 2.");
  s.killSwitchUsed = true; t.phase.paused = true;
  logMsg(S, "The Bureau accepted " + s.name + "'s Kill Switch petition against " + t.name + ".");
  return Result::Ok("Kill Switch petitioned.");
}
Result vDeclareBlackout(GameState& S, int sid) {
  Syndicate& s = synd(S, sid);
  if (auto r = noPhase(S, s, VictoryPath::Blackout); !r) return r;
  if (!blackoutUnlocked(s)) return Result::Err("Requires Blackout Protocol or Shadow Board.");
  for (auto& r : S.synds) if (r.alive && r.id != sid && r.shadowOf < 0 && !S.hexes[r.crown].roots.count(sid)) return Result::Err("Crown Node of " + r.name + " is not rooted.");
  s.hasPhase = true; s.phase = Phase{}; s.phase.type = VictoryPath::Blackout; s.phase.start = S.cycle; s.phase.cyclesLeft = 3;
  logMsg(S, "BLACKOUT: " + s.name + " has cut every rival Crown Node. Three cycles of darkness.");
  return Result::Ok("Blackout declared.");
}
Result vShadowDirector(GameState& S, int sid, int target) {
  Syndicate& s = synd(S, sid); if (!(s.hasPhase && s.phase.type == VictoryPath::Blackout)) return Result::Err("Only during your Blackout.");
  if (!hasDoc(s, Doctrine::G_ShadowBoard)) return Result::Err("Requires Shadow Board.");
  if (target < 0 || target >= (int)S.synds.size() || target == sid) return Result::Err("Bad target.");
  Syndicate& t = synd(S, target); if (t.shadowOf >= 0) return Result::Err("Already a subsidiary.");
  if (!S.hexes[t.crown].roots.count(sid)) return Result::Err("Their Crown is not rooted.");
  if (s.capital < 400) return Result::Err("Need 400 Capital.");
  s.capital -= 400; t.shadowOf = sid; t.hasPhase = false; s.phase.shadowCount++;
  logMsg(S, "SHADOW BOARD: " + t.name + " is now a subsidiary of " + s.name + ".");
  computeValuations(S);
  return Result::Ok("Shadow Director installed.");
}
Result vSetBribe(GameState& S, int sid, int district, bool on) {
  Syndicate& s = synd(S, sid); if (!has(s, Tech::Lobbying)) return Result::Err("Requires Lobbying Office.");
  if (district < 0 || district >= (int)S.districts.size()) return Result::Err("Bad district.");
  District& d = S.districts[district];
  if (on) d.bribes[sid] = 40; else d.bribes.erase(sid);
  return Result::Ok(on ? "Bribe running (40/cycle, +2 exposure)." : "Bribe stopped.");
}
Result vEthicsComplaint(GameState& S, int sid, int district) {
  Syndicate& s = synd(S, sid); if (district < 0 || district >= (int)S.districts.size()) return Result::Err("Bad district.");
  District& d = S.districts[district]; if (!d.bought || d.seat < 0 || d.seat == sid) return Result::Err("No rival bought seat there.");
  bool P = false; for (auto& h : S.hexes) if (h.district == district && h.P.count(sid) && h.P.at(sid) >= 30) P = true;
  if (!P) return Result::Err("Need presence >= 30 somewhere in the district.");
  if (s.capital < 100) return Result::Err("Need 100 Capital.");
  s.capital -= 100;
  if (roll(S) < 0.5) { Syndicate& h = synd(S, d.seat); h.exposure = clampd(h.exposure + 15, 0, 100); d.bribes.erase(d.seat); d.seat = -1; d.bought = false; logMsg(S, "Ethics complaint upheld: the bought seat in District " + std::to_string(district + 1) + " is void."); return Result::Ok("Upheld."); }
  logMsg(S, "Ethics complaint in District " + std::to_string(district + 1) + " was dismissed.", sid);
  return Result::Ok("Dismissed.");
}
Result vConsolidationMotion(GameState& S, int sid) {
  Syndicate& s = synd(S, sid);
  if (auto r = noPhase(S, s, VictoryPath::Charter); !r) return r;
  if (!has(s, Tech::ConsolidationLobby)) return Result::Err("Requires Consolidation Lobby.");
  if (seatsOf(S, sid) < 5) return Result::Err("Need 5 of 9 seats (have " + std::to_string(seatsOf(S, sid)) + ").");
  s.hasPhase = true; s.phase = Phase{}; s.phase.type = VictoryPath::Charter; s.phase.start = S.cycle; s.phase.cyclesLeft = 4;
  logMsg(S, "CONSOLIDATION MOTION: " + s.name + " moved to charter itself as the city utility. Vote in 4 cycles.");
  return Result::Ok("Motion filed.");
}

// ------------------------------------------------------------ progress
std::vector<PathProgress> victoryProgress(const GameState& S, int sid) {
  const Syndicate& s = synd(S, sid); Mods m = modsOf(S, s); std::vector<PathProgress> out;
  { PathProgress p; p.path = VictoryPath::Takeover; p.proximity = 100 * std::min(1.0, s.share / m.takeoverTrigger);
    if (!has(s, Tech::TenderOffer)) p.unmet.push_back("Tender Offer Authority (Corporate T3)");
    if (s.share < m.takeoverTrigger) p.unmet.push_back("Share " + fmt(s.share * 100) + "% of " + fmt(m.takeoverTrigger * 100, 0) + "%");
    if (peeredNow(S, s) < 2) p.unmet.push_back("Peered at 2 Exchanges (" + std::to_string(peeredNow(S, s)) + ")");
    if (s.exposure >= 60) p.unmet.push_back("Exposure < 60");
    p.available = p.unmet.empty(); out.push_back(p); }
  { PathProgress p; p.path = VictoryPath::Singularity; std::vector<int> seeds; bool cl = seedClusterOk(S, s, &seeds);
    int t3 = 0; for (auto& st : S.structs) if (st.alive && st.built && st.sid == sid && st.kind == StructKind::Node && st.tier >= 3) ++t3;
    p.proximity = 40 * (cl ? 1.0 : std::min(1.0, t3 / 3.0) * 0.5) + 60 * std::min(1.0, s.training / 10.0);
    if (!singularityUnlocked(s)) p.unmet.push_back("Seed Protocol, or Kill Chain + Hyperscale Cooling + Backbone Sniffing");
    if (!cl) p.unmet.push_back("Seed cluster: " + std::to_string(hasDoc(s, Doctrine::V_Seed) ? 2 : 3) + " Tier 3 nodes with 2 disjoint paths to a peered Exchange (" + std::to_string(seeds.size()) + " ok)");
    if (s.exposure >= 80) p.unmet.push_back("Exposure < 80");
    p.available = p.unmet.empty(); out.push_back(p); }
  { PathProgress p; p.path = VictoryPath::Blackout; int rooted = 0, rivals = 0;
    for (auto& r : S.synds) if (r.alive && r.id != sid && r.shadowOf < 0) { ++rivals; if (S.hexes[r.crown].roots.count(sid)) ++rooted; }
    p.proximity = rivals ? 100.0 * rooted / rivals : 0;
    if (!blackoutUnlocked(s)) p.unmet.push_back("Blackout Protocol (Black Ops T3) or Ghost doctrine");
    if (!has(s, Tech::KillChain)) p.unmet.push_back("Kill Chain (to root Crown Nodes)");
    if (rooted < rivals) p.unmet.push_back("Root every rival Crown (" + std::to_string(rooted) + "/" + std::to_string(rivals) + ")");
    p.available = p.unmet.empty(); out.push_back(p); }
  { PathProgress p; p.path = VictoryPath::Charter; int seats = seatsOf(S, sid); p.proximity = 100.0 * std::min(1.0, seats / 5.0);
    if (!has(s, Tech::ConsolidationLobby)) p.unmet.push_back("Consolidation Lobby (Corporate T3)");
    if (seats < 5) p.unmet.push_back("5 District seats (" + std::to_string(seats) + ")");
    p.available = p.unmet.empty(); out.push_back(p); }
  { PathProgress p; p.path = VictoryPath::Valuation; p.proximity = 100 * s.share; p.status = "Highest valuation at cycle " + std::to_string(K::GameEnd) + " wins."; p.available = true; out.push_back(p); }
  if (s.hasPhase) for (auto& p : out) if (p.path == s.phase.type) {
    p.status = std::string("ACTIVE: ") + std::to_string(s.phase.cyclesLeft) + " cycles left";
    if (s.phase.type == VictoryPath::Singularity) p.status += s.training >= 11 ? ", coherence " + fmt(s.phase.coherence, 0) : ", training " + std::to_string(s.training) + "/10";
  }
  return out;
}

// ------------------------------------------------------------ tick
static void tickDistricts(GameState& S) {
  for (int di = 0; di < (int)S.districts.size(); ++di) {
    District& d = S.districts[di]; double totalPop = 0; std::map<int, double> pop;
    for (auto& h : S.hexes) if (h.district == di && h.type != Sector::Barrier && h.type != Sector::Exchange) { totalPop += sectorDef(h.type).pop; if (h.owner >= 0 && h.C >= 70) pop[h.owner] += sectorDef(h.type).pop; }
    int holder = -1;
    for (auto& s : S.synds) {
      if (!s.alive) continue; double share = totalPop > 0 ? pop[s.id] / totalPop : 0;
      if (share >= 0.6) { d.streak[s.id]++; d.below[s.id] = 0; } else { d.streak[s.id] = 0; d.below[s.id]++; }
      if (d.streak[s.id] >= 10) holder = s.id;
    }
    if (holder >= 0) { if (d.seat != holder) logMsg(S, synd(S, holder).name + " now holds the seat for District " + std::to_string(di + 1) + "."); d.seat = holder; d.bought = false; d.bribes.clear(); continue; }
    if (!d.bought && d.seat >= 0 && d.below[d.seat] >= 3) { logMsg(S, synd(S, d.seat).name + " lost the seat for District " + std::to_string(di + 1) + "."); d.seat = -1; }
    if (d.seat >= 0 && !d.bought) continue;
    int best = -1; double bb = 0;
    for (auto it = d.bribes.begin(); it != d.bribes.end();) {
      Syndicate& s = synd(S, it->first);
      if (s.capital < it->second) { it = d.bribes.erase(it); continue; }
      s.capital -= it->second; s.exposure = clampd(s.exposure + 2, 0, 100);
      if (it->second > bb) { bb = it->second; best = it->first; }
      ++it;
    }
    if (best >= 0) { if (d.seat != best) logMsg(S, synd(S, best).name + " bought the seat for District " + std::to_string(di + 1) + "."); d.seat = best; d.bought = true; }
    else if (d.bought) { d.seat = -1; d.bought = false; }
  }
}

static void tickEmergent(GameState& S) {
  Emergent& e = S.emergent; if (!e.active) return;
  int top = -1; double bv = -1; for (auto& s : S.synds) if (s.alive && s.valuation > bv) { bv = s.valuation; top = s.id; }
  if (top >= 0) for (auto& st : S.structs) if (st.alive && st.built && st.crown && st.sid == top) st.ddosedUntil = S.cycle + 1;
  for (int i = 0; i < 3 && !e.hexes.empty(); ++i) {
    int from = e.hexes[rollInt(S, (int)e.hexes.size())]; auto nb = S.grid.neighborsV(from); if (nb.empty()) continue;
    int t = nb[rollInt(S, (int)nb.size())]; Hex& h = S.hexes[t]; if (h.type == Sector::Barrier) continue;
    h.P[RogueSid] = clampd((h.P.count(RogueSid) ? h.P[RogueSid] : 0) + 10, 0, 100);
    if (std::find(e.hexes.begin(), e.hexes.end(), t) == e.hexes.end()) e.hexes.push_back(t);
  }
  int rootedSeeds = 0; for (int i = 0; i < e.seedCount && i < (int)e.hexes.size(); ++i) if (!S.hexes[e.hexes[i]].roots.empty()) ++rootedSeeds;
  if (rootedSeeds >= 2 || S.cycle - e.start >= 30) {
    e.active = false; for (auto& h : S.hexes) h.P.erase(RogueSid);
    logMsg(S, "The Emergent Intelligence has gone quiet. The city breathes.");
  }
}

void victoryTick(GameState& S) {
  if (S.over) return;
  tickDistricts(S);
  for (Syndicate& s : S.synds) {
    if (!s.alive || !s.hasPhase || S.over) continue;
    Phase& p = s.phase;
    switch (p.type) {
    case VictoryPath::Takeover: {
      if (s.auditUntil > S.cycle) { logMsg(S, "Proxy Fight paused: bidder under Audit."); break; }
      if (--p.cyclesLeft > 0) { logMsg(S, "Proxy Fight: " + s.name + " holds " + fmt(s.share * 100) + "% of the float, " + std::to_string(p.cyclesLeft) + " cycles left."); break; }
      if (s.share >= modsOf(S, s).takeoverWin) win(S, s.id, VictoryPath::Takeover, "the Tender Offer closed at " + fmt(s.share * 100) + "%. The city is a subsidiary.");
      else { logMsg(S, "The Tender Offer by " + s.name + " LAPSED at " + fmt(s.share * 100) + "%."); s.exposure = clampd(s.exposure + 20, 0, 100); s.shareAdj = 0; endPhase(s, VictoryPath::Takeover, 15, S.cycle); for (auto& o : S.synds) o.poisonUntil = 0; computeValuations(S); }
      break; }
    case VictoryPath::Singularity: {
      std::vector<int> seeds; bool cl = seedClusterOk(S, s, &seeds);
      if (s.training < 10) {
        double c = 0; for (int id : seeds) { auto it = s.flow.computeByNode.find(id); if (it != s.flow.computeByNode.end()) c += it->second; }
        if (cl && c >= 150) { s.training++; p.coherence = 4.0 * s.training; p.cyclesLeft = 10 - s.training; if (s.training == 10) logMsg(S, "SEED TRAINED. Launch when ready.", s.id); else logMsg(S, "Seed training " + std::to_string(s.training) + "/10 (" + fmt(c, 0) + " compute).", s.id); }
        else logMsg(S, std::string("Seed training stalled: ") + (cl ? "compute " + fmt(c, 0) + " < 150" : "cluster broken") + ".", s.id);
        break;
      }
      if (s.training == 10) break;                                   // trained, waiting for Launch
      double fed = s.flow.sold + 10.0 * s.flow.peered.size();
      double gain = 20.0 * std::min(1.0, fed / 200.0) * modsOf(S, s).coherenceMult;
      int lit = 0, hurt = 0, ddos = 0;
      for (int id : seeds) { const Structure* st = findStruct(S, id); if (!st) continue; bool dark = st->darkUntil > S.cycle; bool rooted = !S.hexes[st->hex].roots.empty(); if (!dark) ++lit; if (dark || rooted) ++hurt; if (st->ddosedUntil >= S.cycle) ++ddos; }
      p.coherence += gain - 10.0 * hurt - 5.0 * ddos;
      if (p.paused) { p.coherence -= 15; p.paused = false; logMsg(S, "The Bureau fired the Kill Switch: Coherence -15."); }
      p.cyclesLeft--;
      logMsg(S, "LAUNCH: " + s.name + " Coherence " + fmt(p.coherence, 0) + "/100 (fed " + fmt(fed, 0) + "/200, " + std::to_string(hurt) + " seed nodes compromised).");
      if (p.coherence >= 100) { win(S, s.id, VictoryPath::Singularity, "the Seed reached full Coherence. Something new is awake in the Backbone."); break; }
      bool misaligned = s.arch == Arch::Hive && s.M < 70 && roll(S) < 0.10;
      if (misaligned) {
        S.emergent.active = true; S.emergent.hexes.clear(); S.emergent.start = S.cycle;
        for (int id : seeds) { Structure* st = findStruct(S, id); if (st) { st->darkUntil = 1 << 30; S.emergent.hexes.push_back(st->hex); } }
        S.emergent.seedCount = (int)S.emergent.hexes.size(); endPhase(s, VictoryPath::Singularity, 25, S.cycle);
        logMsg(S, "MISALIGNED LAUNCH: the Seed defected. An Emergent Intelligence now hunts the richest syndicate.");
        break;
      }
      if (p.coherence <= 0 || lit < 2 || p.cyclesLeft <= 0) {
        for (int id : seeds) { Structure* st = findStruct(S, id); if (st) { st->darkUntil = S.cycle + 10; S.hexes[st->hex].fw = 0; } }
        s.M = clampd(s.M - 30, 0, 100); s.exposure = clampd(s.exposure + 25, 0, 100); endPhase(s, VictoryPath::Singularity, 25, S.cycle);
        logMsg(S, "LAUNCH ABORTED: " + s.name + "'s Seed burned out.");
      }
      break; }
    case VictoryPath::Blackout: {
      int stillDark = 0, rivals = 0;
      for (Syndicate& r : S.synds) {
        if (!r.alive || r.id == s.id || r.shadowOf >= 0) continue; ++rivals;
        Hex& ch = S.hexes[r.crown];
        if (!ch.roots.count(s.id)) continue; ++stillDark;
        bool failover = false;
        for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && ((st.kind == StructKind::Substation && S.grid.dist(st.hex, r.crown) >= 3) || st.kind == StructKind::PrivateGrid)) failover = true;
        for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && st.kind == StructKind::Node) {
          if (st.hex == r.crown) { if (failover) st.ddosedUntil = S.cycle + 1; else st.darkUntil = std::max(st.darkUntil, S.cycle + 1); }
          else if (S.grid.dist(st.hex, r.crown) <= 3) st.ddosedUntil = S.cycle + 1;
        }
        r.buyBW = 0;
        bool hasOutpost = false; for (auto& st : S.structs) if (st.alive && st.built && st.sid == r.id && st.kind == StructKind::Outpost) hasOutpost = true;
        if (hasOutpost && roll(S) < 0.4) { std::vector<Segment*> segs; for (Link& l : S.links) if (l.alive && l.built && l.sid == s.id) for (Segment& sg : l.segs) segs.push_back(&sg); if (!segs.empty()) { segs[rollInt(S, (int)segs.size())]->sabotagedUntil = S.cycle + 3; logMsg(S, "Bureau-sanctioned raid by " + r.name + " cut one of your segments.", s.id); } }
      }
      p.cyclesLeft--;
      logMsg(S, "BLACKOUT cycle: " + std::to_string(stillDark) + "/" + std::to_string(rivals) + " Crown Nodes dark, " + std::to_string(p.cyclesLeft) + " left.");
      if (hasDoc(s, Doctrine::G_ShadowBoard)) {
        int subs = 0; for (auto& r : S.synds) if (r.alive && r.shadowOf == s.id) ++subs;
        if (subs == rivals || s.share >= 0.6) { win(S, s.id, VictoryPath::Blackout, "every board in the city answers to a Shadow Director."); break; }
      }
      if (p.cyclesLeft <= 0) {
        int mine = (int)ownedHexes(S, s.id).size();
        if (mine >= 0.35 * totalSectors(S) || (stillDark == rivals && rivals > 0)) win(S, s.id, VictoryPath::Blackout, "the lights never came back on for anyone else.");
        else { for (auto& h : S.hexes) h.roots.erase(s.id); s.exposure = clampd(s.exposure + 40, 0, 100); s.auditUntil = S.cycle + 5; endPhase(s, VictoryPath::Blackout, 25, S.cycle); logMsg(S, "The BLACKOUT by " + s.name + " FAILED. The city knows who turned the lights off."); }
      }
      break; }
    case VictoryPath::Charter: {
      if (s.auditUntil > S.cycle) { logMsg(S, "Consolidation Vote paused: mover under Audit."); break; }
      if (--p.cyclesLeft > 0) { logMsg(S, "Consolidation Vote: " + s.name + " holds " + std::to_string(seatsOf(S, s.id)) + " seats, " + std::to_string(p.cyclesLeft) + " cycles left."); break; }
      if (seatsOf(S, s.id) >= 5) win(S, s.id, VictoryPath::Charter, "the Consolidation Charter passed. The syndicate is the city's licensed utility.");
      else { for (auto& d : S.districts) if (d.bought && d.seat == s.id) { d.seat = -1; d.bought = false; d.bribes.clear(); } s.exposure = clampd(s.exposure + 15, 0, 100); endPhase(s, VictoryPath::Charter, 20, S.cycle); logMsg(S, "The Consolidation Motion by " + s.name + " FAILED."); }
      break; }
    default: break;
    }
  }
  tickEmergent(S);
}

} } // namespace gl::sim
