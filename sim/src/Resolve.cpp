// The ten-step cycle resolution (docs/00 §5). Fixed order, deterministic RNG draws, invariant sweep after each step.
#include "Internal.h"

namespace gl { namespace sim {

static double roll(GameState& S) { Rng r(S.rngState); double v = r.next(); S.rngState = r.state(); return v; }
static int rollInt(GameState& S, int n) { Rng r(S.rngState); int v = r.intn(n); S.rngState = r.state(); return v; }
static double clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }

void addExposure(GameState& S, Syndicate& s, double amount, bool attack, int victim) {
  Mods m = modsOf(S, s); double a = amount * m.xMult;
  if (a <= 0) return;
  if (attack && m.attribution > 0 && roll(S) < m.attribution) {
    std::vector<int> cands; for (auto& o : S.synds) if (o.alive && o.id != s.id && o.id != victim) cands.push_back(o.id);
    if (!cands.empty()) {
      Syndicate& f = synd(S, cands[rollInt(S, (int)cands.size())]);
      f.exposure = clampd(f.exposure + a, 0, 100);
      if (victim >= 0) logMsg(S, "Forensics attribute the operation to " + f.name + ".", victim);
      logMsg(S, "False flag held: " + f.name + " takes the heat.", s.id);
      return;
    }
  }
  s.exposure = clampd(s.exposure + a, 0, 100);
}

static void hiveModelLoss(GameState& S, Syndicate& s, double amount) {
  if (s.arch != Arch::Hive) return;
  if (amount >= 10) for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Fork) { st.alive = false; logMsg(S, "A Model Fork absorbed the loss.", s.id); return; }
  s.M = clampd(s.M - amount, 0, 100);
}

// ------------------------------------------------------------ 1. power
static void stepPower(GameState& S) {
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue;
    Mods m = modsOf(S, s);
    std::vector<Structure*> subs; std::map<int, double> left;
    for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && (st.kind == StructKind::Substation || st.kind == StructKind::PrivateGrid) && st.darkUntil <= S.cycle) { subs.push_back(&st); left[st.id] = structDef(st.kind).mw; }
    std::vector<Structure*> nodes; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Node) nodes.push_back(&st);
    std::sort(nodes.begin(), nodes.end(), [](Structure* a, Structure* b) { return a->tier != b->tier ? a->tier > b->tier : a->id < b->id; });
    double gridUsed = 0;
    for (Structure* n : nodes) {
      if (n->darkUntil > S.cycle) { n->powerFrac = 0; continue; }
      double need = nodeMW(S, s, *n), got = 0;
      for (Structure* sub : subs) {
        int range = sub->kind == StructKind::PrivateGrid ? 3 : (int)std::max((double)structDef(StructKind::Substation).range, m.subRange);
        if (S.hexes[sub->hex].type == Sector::Industrial && sub->kind == StructKind::Substation) range += 1;
        if (S.grid.dist(sub->hex, n->hex) > range) continue;
        double t = std::min(need - got, left[sub->id]); left[sub->id] -= t; got += t;
        if (got >= need - 1e-9) break;
      }
      if (got < need - 1e-9 && s.autoGrid && !s.gridCut) {
        double g = std::min(need - got, m.gridCap - gridUsed);
        double afford = std::floor(s.capital / K::GridPrice); g = std::max(0.0, std::min(g, afford));
        s.capital -= g * K::GridPrice; gridUsed += g; got += g;
      }
      n->powerFrac = need > 0 ? clampd(got / need, 0, 1) : 1;
    }
  }
}

// ------------------------------------------------------------ 3. cyber
static void purgeHex(GameState& S, Hex& h, int sid) {
  for (auto it = h.P.begin(); it != h.P.end();) {
    if (it->first != sid && !h.dormant.count(it->first)) { it->second -= 30; if (it->second <= 0) it = h.P.erase(it); else ++it; } else ++it;
  }
  h.roots.clear();
  for (auto& st : S.structs) if (st.alive && st.hex == h.id && st.kind == StructKind::Tap && st.sid != sid) { st.alive = false; logMsg(S, "Your Tap in " + std::string(sectorDef(h.type).name) + " #" + std::to_string(h.id) + " was found and destroyed.", st.sid); }
  for (auto it = h.daemons.begin(); it != h.daemons.end();) { if (it->first != sid) it = h.daemons.erase(it); else ++it; }
}

static void stepCyber(GameState& S) {
  std::map<int, double> pool; for (auto& s : S.synds) pool[s.id] = s.flow.opsPool;
  auto pay = [&](Syndicate& s, double c) { if (pool[s.id] + 1e-9 >= c) { pool[s.id] -= c; return true; } return false; };
  std::set<int> honeypotHit;

  // defence first
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    for (Op& o : s.ops) if (o.kind == OpKind::Harden) { Hex& h = S.hexes[o.target]; if (h.owner != s.id) continue; if (pay(s, 2 * m.opsCost)) h.fw = clampd(h.fw + 5, 0, 100); }
    for (Op& o : s.queued) if (o.kind == OpKind::Purge) { Hex& h = S.hexes[o.target]; if (h.owner != s.id) continue; if (pay(s, m.purgeCost)) { purgeHex(S, h, s.id); logMsg(S, "Purged subnet #" + std::to_string(h.id) + ".", s.id); } }
  }
  // attacks
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    for (auto it = s.ops.begin(); it != s.ops.end();) {
      Op& o = *it; Hex& h = S.hexes[o.target]; bool drop = false;
      if (o.kind == OpKind::Intrusion) {
        if (h.owner == s.id) drop = true;
        else if (pay(s, 4 * m.opsCost * m.intrusionCost)) {
          double P = h.P.count(s.id) ? h.P[s.id] : 0;
          if (!(P >= 1 || canReach(S, s, h))) { ++it; continue; }
          if (h.owner >= 0 && structIn(S, h.id, h.owner, StructKind::Honeypot) && !honeypotHit.count(h.id)) {
            honeypotHit.insert(h.id);
            logMsg(S, "Honeypot in #" + std::to_string(h.id) + " caught " + s.name + ".", h.owner);
            logMsg(S, "Your intrusion into #" + std::to_string(h.id) + " hit a honeypot. You have been identified.", s.id);
            s.exposure = clampd(s.exposure + (hasDoc(s, Doctrine::G_FalseFlag) ? 15 : 10), 0, 100);
            h.revealedTo.insert(h.owner);
          } else {
            double gain = std::max(3.0, 15.0 - h.fw / 10.0) * (has(s, Tech::ZeroDay) ? 1.5 : 1.0) + m.intrusionGain;
            if (gain < 5 && h.owner >= 0) { h.repelled++; h.fw = clampd(h.fw + modsOf(S, synd(S, h.owner)).adaptive, 0, 100); }
            h.P[s.id] = clampd(P + gain, 0, 100);
            addExposure(S, s, 1.0 * m.intrusionX, true, h.owner);
          }
        }
      } else if (o.kind == OpKind::Siphon) {
        double P = h.P.count(s.id) ? h.P[s.id] : 0;
        if (P >= 40 && h.owner >= 0 && h.owner != s.id) { if (pay(s, 3 * m.opsCost)) addExposure(S, s, 2, true, h.owner); }
      } else if (o.kind == OpKind::Jam) {
        double P = h.P.count(s.id) ? h.P[s.id] : 0;
        if (P >= 40 && pay(s, 5 * m.opsCost)) { h.jamUntil[s.id] = S.cycle + 1; addExposure(S, s, 3, true, h.owner); }
      }
      if (drop) it = s.ops.erase(it); else ++it;
    }
    for (Op& o : s.queued) {
      Hex& h = S.hexes[o.target < 0 ? 0 : std::min(o.target, (int)S.hexes.size() - 1)];
      switch (o.kind) {
      case OpKind::Scan: {
        if (!pay(s, 2 * m.opsCost)) break;
        h.scans[s.id] = S.cycle;
        for (auto& st : S.structs) if (st.alive && st.hex == h.id && st.sid != s.id && modsOf(S, synd(S, st.sid)).hidden && st.variant != Variant::Phantom) {
          if (m.dpi || roll(S) < 0.5) { h.revealedTo.insert(s.id); synd(S, st.sid).revealedThisPeriod = true; logMsg(S, "Scan revealed hidden " + std::string(st.kind == StructKind::Node ? "node" : structDef(st.kind).name) + " in #" + std::to_string(h.id) + ".", s.id); }
        }
        if (h.owner >= 0 && h.owner != s.id && has(synd(S, h.owner), Tech::CounterIntel)) logMsg(S, s.name + " scanned your sector #" + std::to_string(h.id) + ".", h.owner);
        break; }
      case OpKind::Root: {
        double P = h.P.count(s.id) ? h.P[s.id] : 0; if (P < 70) break;
        if (!pay(s, m.rootCost * m.opsCost)) break;
        h.roots.insert(s.id); h.fw = 0; s.everRooted.insert(h.id);
        addExposure(S, s, 6, true, h.owner);
        if (h.owner >= 0) logMsg(S, std::string(h.id == synd(S, h.owner).crown ? "ROOT ALARM: your CROWN NODE" : "Root alarm: sector") + " #" + std::to_string(h.id) + " has been rooted.", h.owner);
        logMsg(S, "Rooted #" + std::to_string(h.id) + ".", s.id);
        break; }
      case OpKind::DDoS: {
        if (!pay(s, 8 * m.opsCost)) break;
        for (auto& st : S.structs) if (st.alive && st.built && st.hex == h.id && st.kind == StructKind::Node && st.sid != s.id) st.ddosedUntil = S.cycle + 1;
        addExposure(S, s, 8, true, h.owner);
        if (h.owner >= 0) logMsg(S, "DDoS against your node in #" + std::to_string(h.id) + ": output halved next cycle.", h.owner);
        break; }
      case OpKind::Daemon: {
        int total = 0; for (auto& hx : S.hexes) { auto d = hx.daemons.find(s.id); if (d != hx.daemons.end()) total += d->second; }
        if (total >= m.daemonCap) break;
        if (!pay(s, 2 * m.opsCost)) break;
        h.daemons[s.id] += 1;
        break; }
      case OpKind::Oracle: {
        if (o.target < 0 || o.target >= (int)S.synds.size()) break;
        if (!pay(s, 10 * m.opsCost)) break;
        Syndicate& r = synd(S, o.target); int shown = 0;
        for (const Op& p : r.planned) { if (roll(S) < s.M / 100.0) { logMsg(S, "Oracle: " + r.name + " plans " + opDef(p.kind).name + " against #" + std::to_string(p.target) + ".", s.id); ++shown; } }
        if (!shown) logMsg(S, "Oracle: the model saw nothing certain in " + r.name + "'s traffic.", s.id);
        break; }
      case OpKind::Retrain: {
        if (s.flow.compute < 30) break;
        s.flow.compute -= 30;
        for (auto& hx : S.hexes) hx.daemons.erase(s.id);
        s.M = clampd(s.M + 8, 0, 100); s.lastEmergent = -1;
        logMsg(S, "Model retrained: daemons recalled, M +8.", s.id);
        break; }
      default: break;
      }
    }
    // daemon tick
    if (s.arch == Arch::Hive) {
      int total = 0; for (auto& hx : S.hexes) { auto d = hx.daemons.find(s.id); if (d != hx.daemons.end()) total += d->second; }
      for (Hex& hx : S.hexes) {
        auto d = hx.daemons.find(s.id); if (d == hx.daemons.end() || d->second <= 0) continue;
        int alive = 0;
        for (int i = 0; i < d->second; ++i) if (pay(s, 1.0)) ++alive;
        d->second = alive;
        if (alive == 0) { hx.daemons.erase(s.id); continue; }
        if (hx.owner != s.id) hx.P[s.id] = clampd((hx.P.count(s.id) ? hx.P[s.id] : 0) + 5.0 * alive, 0, 100);
        for (int i = 0; i < alive; ++i) {
          if (total < m.daemonCap && roll(S) < m.copyChance) {
            auto nb = S.grid.neighborsV(hx.id); std::vector<int> ok; for (int n : nb) if (S.hexes[n].type != Sector::Barrier) ok.push_back(n);
            if (!ok.empty()) { int t = ok[rollInt(S, (int)ok.size())]; S.hexes[t].daemons[s.id] += 1; ++total; }
          }
        }
        addExposure(S, s, 0.2 * alive, false);
      }
    }
  }
  for (Syndicate& s : S.synds) s.flow.opsPool = pool[s.id];
}

// ------------------------------------------------------------ 4. physical
static Segment* rivalSegThrough(GameState& S, int hexId, int notSid, Link** outLink) {
  Segment* best = nullptr; double bf = -1;
  for (Link& l : S.links) if (l.alive && l.built && l.sid != notSid) for (Segment& sg : l.segs) if ((sg.a == hexId || sg.b == hexId) && sg.cap > 0 && sg.flow1 + sg.flow > bf) { bf = sg.flow1 + sg.flow; best = &sg; if (outLink) *outLink = &l; }
  return best;
}

static void stepPhysical(GameState& S) {
  for (auto& st : S.structs) {
    if (!st.alive || st.built) continue;
    if (--st.buildLeft <= 0) {
      st.built = true; st.buildLeft = 0; Hex& h = S.hexes[st.hex];
      if (st.kind == StructKind::Node && h.owner == st.sid) h.fw = std::max(h.fw, nodeFwBase(st) + modsOf(S, synd(S, st.sid)).fwAdd);
      logMsg(S, std::string(st.kind == StructKind::Node ? nodeDef(st.tier).name : structDef(st.kind).name) + " completed in #" + std::to_string(st.hex) + ".", st.sid);
    }
  }
  for (auto& l : S.links) if (l.alive && !l.built && l.readyAt <= S.cycle) l.built = true;

  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    for (Op& o : s.queued) {
      if (o.target < 0) continue;
      switch (o.kind) {
      case OpKind::Raid: {
        Hex& h = S.hexes[o.target];
        bool outpost = false; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Outpost && S.grid.dist(st.hex, h.id) <= m.raidRange) outpost = true;
        if (!outpost) { logMsg(S, "Raid on #" + std::to_string(h.id) + " aborted: no Outpost in range.", s.id); break; }
        double chance = m.raidBase + m.raidBonus;
        double P = h.P.count(s.id) ? h.P[s.id] : 0; chance += 0.10 * std::floor(P / 25);
        int defOut = 0; for (auto& st : S.structs) if (st.alive && st.built && st.sid != s.id && st.kind == StructKind::Outpost && S.grid.dist(st.hex, h.id) <= 1) ++defOut;
        chance -= 0.30 * defOut;
        if (h.owner >= 0) chance += modsOf(S, synd(S, h.owner)).raidAgainst;
        chance = clampd(chance, 0.05, 0.95);
        bool ok = roll(S) < chance;
        std::string what;
        if (ok) {
          RaidEffect eff = (RaidEffect)std::max(0, std::min(3, o.aux));
          if (eff == RaidEffect::Sever) {
            Link* L = nullptr; Segment* sg = rivalSegThrough(S, h.id, s.id, &L);
            if (sg && L && !modsOf(S, synd(S, L->sid)).severImmune && !linkDef(L->type).armored) { sg->sabotagedUntil = S.cycle + (has(s, Tech::SabotageDoctrine) ? 4 : 3); what = "severed a fiber segment"; logMsg(S, "Sabotage: a link segment at #" + std::to_string(h.id) + " is cut for " + std::to_string(sg->sabotagedUntil - S.cycle) + " cycles.", L->sid); }
            else { ok = false; what = "found the conduit armored"; }
          } else if (eff == RaidEffect::Darken) {
            bool any = false;
            for (auto& st : S.structs) if (st.alive && st.built && st.hex == h.id && st.sid != s.id && (st.kind == StructKind::Substation || st.kind == StructKind::PrivateGrid)) { st.darkUntil = S.cycle + 2; any = true; hiveModelLoss(S, synd(S, st.sid), 10); logMsg(S, "Raid: your substation in #" + std::to_string(h.id) + " is dark for 2 cycles.", st.sid); }
            if (!any) ok = false; what = any ? "darkened the substation" : "found no substation";
          } else if (eff == RaidEffect::SeizeRights) {
            h.rights.insert(s.id); what = "seized conduit rights"; if (h.owner >= 0) logMsg(S, "Raid: " + s.name + " seized conduit rights in #" + std::to_string(h.id) + ".", h.owner);
          } else {
            bool any = false;
            for (auto& st : S.structs) if (st.alive && st.built && st.hex == h.id && st.sid != s.id && st.crown) { synd(S, st.sid).decapUntil = S.cycle + 3; any = true; logMsg(S, "DECAPITATION RAID on your Crown Node: output halved for 3 cycles.", st.sid); }
            if (!any) ok = false; what = any ? "hit the Crown Node" : "found no Crown Node";
          }
        }
        logMsg(S, std::string("Raid on #") + std::to_string(h.id) + (ok ? " succeeded: " + what : " failed" + (what.empty() ? "" : ": " + what)) + " (" + fmt(chance * 100, 0) + "%).", s.id);
        addExposure(S, s, 10 * m.xRaid, true, h.owner);
        break; }
      case OpKind::Repair: {
        Link* L = findLink(S, o.target); if (!L || L->sid != s.id || o.aux < 0 || o.aux >= (int)L->segs.size()) break;
        L->segs[o.aux].sabotagedUntil = 0; logMsg(S, "Segment repaired.", s.id); break; }
      case OpKind::Eminent: { Hex& h = S.hexes[o.target]; h.rights.insert(s.id); addExposure(S, s, 4, false); if (h.owner >= 0) logMsg(S, s.name + " exercised Eminent Domain in your sector #" + std::to_string(h.id) + ".", h.owner); break; }
      case OpKind::Sever: {
        Hex& h = S.hexes[o.target]; Link* L = nullptr; Segment* sg = rivalSegThrough(S, h.id, s.id, &L);
        if (sg && L) { sg->cap = 0; addExposure(S, s, 6, false); logMsg(S, s.name + " SEVERED your fiber through #" + std::to_string(h.id) + ".", L->sid); logMsg(S, "Severed a rival link through #" + std::to_string(h.id) + ".", s.id); }
        break; }
      case OpKind::Covert: { Hex& h = S.hexes[o.target]; h.rights.insert(s.id); logMsg(S, "Covert conduit rights secured in #" + std::to_string(h.id) + ".", s.id); break; }
      default: break;
      }
    }
    s.queued.clear();
    if (hasDoc(s, Doctrine::V_Predictive)) for (Link& l : S.links) if (l.alive && l.sid == s.id) for (Segment& sg : l.segs) sg.sabotagedUntil = 0;
  }
  for (auto& st : S.structs) {
    if (!st.alive) continue; const Hex& h = S.hexes[st.hex];
    if (h.owner >= 0 && h.owner != st.sid) { if (st.orphanedAt < 0) st.orphanedAt = S.cycle; } else st.orphanedAt = -1;
  }
}

// ------------------------------------------------------------ 5. control
static void flipHex(GameState& S, Hex& h, int newOwner) {
  int old = h.owner;
  h.owner = newOwner; h.C = 25; h.priority.clear(); h.rowDenial = false; h.brownout = false;
  h.rights.insert(newOwner); h.roots.erase(newOwner); h.fw = fwBaseFor(S, h);
  if (old >= 0) { logMsg(S, "LOST sector #" + std::to_string(h.id) + " (" + sectorDef(h.type).name + ") to " + synd(S, newOwner).name + ".", old); synd(S, newOwner).sectorsTaken++; }
  logMsg(S, std::string(old >= 0 ? "Took over" : "Claimed") + " sector #" + std::to_string(h.id) + " (" + sectorDef(h.type).name + ").", newOwner);
}

static void stepControl(GameState& S) {
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier || h.type == Sector::Exchange) continue;
    // weights
    int winner = -1; double wWin = 0; std::map<int, double> w;
    for (auto& kv : h.delivered) {
      if (kv.second <= 0) continue;
      double P = kv.first == h.owner ? 100 : (h.P.count(kv.first) ? h.P[kv.first] : 0);
      double wt = kv.second * (0.5 + P / 200.0); w[kv.first] = wt;
      if (wt > wWin) { wWin = wt; winner = kv.first; }
    }
    if (h.owner >= 0) {
      double d = h.demand.count(h.owner) ? h.demand[h.owner] : sectorDemand(S, h, h.owner);
      double del = h.delivered.count(h.owner) ? h.delivered[h.owner] : 0;
      double r = d > 0 ? del / d : 1.0;
      if (winner >= 0 && winner != h.owner && wWin > 0) r *= (w.count(h.owner) ? w[h.owner] : 0) / wWin;
      h.ratio[h.owner] = r;
      if (r >= 1.0) h.C = clampd(h.C + 10, 0, 100); else h.C = clampd(h.C - (1.0 - r) * 25.0, 0, 100);
      bool wasBrown = h.brownout; h.brownout = r < 0.6;
      if (h.brownout && !wasBrown && h.owner == S.playerSid) logMsg(S, "BROWNOUT in #" + std::to_string(h.id) + " (" + sectorDef(h.type).name + "): r=" + fmt(r, 2) + ".", h.owner);
      // takeover
      if (h.brownout) {
        int att = -1; double aw = 0;
        for (auto& kv : h.delivered) {
          int a = kv.first; if (a == h.owner) continue; Syndicate& as = synd(S, a);
          double need = h.demand.count(a) ? h.demand[a] : sectorDemand(S, h, a);
          if (kv.second + 1e-6 < need) continue;
          double P = h.P.count(a) ? h.P[a] : 0;
          bool blackout = as.hasPhase && as.phase.type == VictoryPath::Blackout;
          bool ok = (h.C < 50 && (P >= 50 || h.roots.count(a))) || blackout;
          if (ok && w[a] > aw) { aw = w[a]; att = a; }
        }
        if (att >= 0) {
          Syndicate& as = synd(S, att); bool blackout = as.hasPhase && as.phase.type == VictoryPath::Blackout;
          flipHex(S, h, att);
          if (!blackout) as.exposure = clampd(as.exposure + 6, 0, 100);
          continue;
        }
      }
      if (h.C <= 0) { logMsg(S, "Sector #" + std::to_string(h.id) + " (" + sectorDef(h.type).name + ") went dark and is now neutral.", h.owner); h.owner = -1; h.C = 0; h.brownout = false; h.fw = sectorDef(h.type).fw; h.priority.clear(); }
    } else if (winner >= 0) {
      double need = h.demand.count(winner) ? h.demand[winner] : sectorDemand(S, h, winner);
      if (h.delivered[winner] + 1e-6 >= need && need > 0) flipHex(S, h, winner);
    }
  }
}

// ------------------------------------------------------------ 6. yields
static void stepYields(GameState& S) {
  for (Hex& h : S.hexes) {
    if (h.owner < 0) continue; Syndicate& s = synd(S, h.owner); Mods m = modsOf(S, s);
    double base = sectorDef(h.type).cap; if (h.type == Sector::Arcology && has(s, Tech::DataBrokerage)) base += 5;
    double r = h.ratio.count(s.id) ? h.ratio[s.id] : 0;
    double y = base * (h.C / 100.0) * std::min(1.0, r) * m.yieldMult;
    if (hasDoc(s, Doctrine::H_Fortress) && (structIn(S, h.id, s.id, StructKind::Substation) || structIn(S, h.id, s.id, StructKind::Outpost))) y *= 0.85;
    for (Syndicate& rv : S.synds) {
      if (!rv.alive || rv.id == s.id) continue;
      bool siphon = false; for (const Op& o : rv.ops) if (o.kind == OpKind::Siphon && o.target == h.id) siphon = true;
      if (!siphon || !h.P.count(rv.id) || h.P[rv.id] < 40) continue;
      double sk = y * modsOf(S, rv).skim; y -= sk;
      if (h.type == Sector::Arcology) rv.flow.compute += sk; else rv.capital += sk;
    }
    s.capital += y;
  }
  for (Syndicate& s : S.synds) if (s.alive && has(s, Tech::SovereignWealth) && s.capital > 1000) s.capital += std::min(100.0, 0.02 * (s.capital - 1000));
}

// ------------------------------------------------------------ 7. upkeep
static void stepUpkeep(GameState& S) {
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue;
    double u = totalUpkeep(S, s); s.capital -= u;
    if (s.capital < -1e-9) {
      double deficit = -s.capital; s.capital = 0;
      std::vector<Structure*> nodes; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Node && st.darkUntil <= S.cycle) nodes.push_back(&st);
      std::sort(nodes.begin(), nodes.end(), [](Structure* a, Structure* b) { return a->tier != b->tier ? a->tier < b->tier : a->id < b->id; });
      double covered = 0;
      for (Structure* n : nodes) { if (covered >= deficit) break; n->darkUntil = S.cycle + 1; covered += nodeDef(n->tier).upkeep; }
      s.exposure = clampd(s.exposure + 5, 0, 100); s.insolventStreak++;
      logMsg(S, "INSOLVENT: upkeep " + fmt(u, 0) + " unpaid by " + fmt(deficit, 0) + "; nodes darkened. Creditors talk (+5 exposure).", s.id);
      if (s.insolventStreak >= 2 && s.auditUntil < S.cycle) { s.auditUntil = S.cycle + 3; logMsg(S, "The Bureau opened an Audit over repeated insolvency.", s.id); }
    } else s.insolventStreak = 0;
  }
}

// ------------------------------------------------------------ 8. market
static void stepMarket(GameState& S) {
  double total = 0; for (auto& s : S.synds) total += s.flow.sold;
  S.market.soldThisCycle = total;
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    for (int ex : s.flow.peered) s.peeredLast[ex] = S.cycle;
    if (s.flow.sold > 0) {
      double price = std::max(S.market.price, m.sellFloor) * m.sellMult;
      if (cartelAgainst(S, s.id)) price *= 0.5;
      s.capital += s.flow.sold * price;
    }
  }
  S.market.price = std::max(K::SellFloor, S.market.price - 0.1 * (total / 10.0));
  S.market.price = std::min(K::SellPrice, S.market.price + 0.2);
}

// ------------------------------------------------------------ 9. exposure & bureau
static void stepExposure(GameState& S) {
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    if (hasDoc(s, Doctrine::V_AlignmentWaiver)) s.exposure = clampd(s.exposure + 3, 0, 100);
    double notice = 40 + m.bureauAdd, audit = 60 + m.bureauAdd, enforce = 80 + m.bureauAdd;
    s.noticed = s.exposure >= notice;
    if (s.exposure >= audit && s.auditUntil < S.cycle && s.frozen <= 0) {
      s.auditUntil = S.cycle + 3; s.frozen = 0.15 * s.capital; s.capital -= s.frozen;
      logMsg(S, "BUREAU AUDIT: 15% of Capital frozen for 3 cycles; +1 demand everywhere.", s.id);
    }
    if (s.auditUntil == S.cycle && s.frozen > 0) { s.capital += s.frozen; s.frozen = 0; logMsg(S, "Audit closed; frozen Capital returned.", s.id); }
    bool wasCut = s.gridCut; s.gridCut = s.exposure >= enforce;
    if (s.gridCut) {
      std::vector<Structure*> subs; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Substation) subs.push_back(&st);
      if (!subs.empty()) { Structure* t = subs[rollInt(S, (int)subs.size())]; t->darkUntil = std::max(t->darkUntil, S.cycle + 2); }
      if (!wasCut) logMsg(S, "BUREAU ENFORCEMENT: grid power cut, substations raided while exposure stays high.", s.id);
    }
    if (s.exposure >= 100 && !hasDoc(s, Doctrine::G_Deniability)) {
      for (Hex& h : S.hexes) if (h.owner == s.id) for (int n : S.grid.neighborsV(h.id)) { int o = S.hexes[n].owner; if (o >= 0 && o != s.id) h.P[o] = clampd((h.P.count(o) ? h.P[o] : 0) + 25, 0, 100); }
      s.exposure = 80;
      logMsg(S, "PUBLIC ENEMY: every rival gained +25 presence in your border subnets.", s.id);
    }
    s.exposure = clampd(s.exposure - m.xCool, 0, 100);
  }
}

// ------------------------------------------------------------ 10. events
static void stepEvents(GameState& S) {
  // presence decay, sleeper cells, backdoor floors
  std::set<std::pair<int, int>> sustained;
  for (auto& s : S.synds) for (const Op& o : s.ops) if (o.kind == OpKind::Intrusion || o.kind == OpKind::Siphon || o.kind == OpKind::Jam) sustained.insert({ s.id, o.target });
  for (Hex& h : S.hexes) {
    for (auto it = h.P.begin(); it != h.P.end();) {
      int rid = it->first;
      if (rid == h.owner || rid == RogueSid) { ++it; continue; }
      Syndicate& rs = synd(S, rid);
      bool sus = sustained.count({ rid, h.id }) || (h.daemons.count(rid) && h.daemons[rid] > 0) || h.dormant.count(rid);
      if (!sus) it->second -= modsOf(S, rs).pDecay;
      if (hasDoc(rs, Doctrine::G_Backdoor) && rs.everRooted.count(h.id)) it->second = std::max(it->second, 20.0);
      if (hasDoc(rs, Doctrine::G_Sleeper)) {
        if (it->second >= 30) { if (++h.dormantStreak[rid] >= 10) h.dormant.insert(rid); } else { h.dormantStreak[rid] = 0; h.dormant.erase(rid); }
      }
      if (it->second <= 0) { h.dormant.erase(rid); it = h.P.erase(it); } else ++it;
    }
    // firewall drift
    if (h.owner >= 0) { for (auto& st : S.structs) if (st.alive && st.built && st.hex == h.id && st.kind == StructKind::Node && st.sid == h.owner && st.darkUntil > S.cycle) h.fw = clampd(h.fw - 10, 0, 100); }
    else if (h.type != Sector::Exchange) h.fw = sectorDef(h.type).fw;
  }
  // model quality
  for (Syndicate& s : S.synds) {
    if (!s.alive || s.arch != Arch::Hive) continue; Mods m = modsOf(S, s);
    double pop = 0;
    for (Hex& h : S.hexes) {
      double P = h.owner == s.id ? 100 : (h.P.count(s.id) ? h.P[s.id] : 0);
      if (hasDoc(s, Doctrine::V_Federated) && h.owner >= 0 && h.owner != s.id && inCartelWith(S, s.id, h.owner)) P = std::max(P, 50.0);
      if (P >= 30) pop += sectorDef(h.type).pop;
    }
    double gain = 0.05 * pop * m.mGain;
    for (const Op& o : s.ops) if (o.kind == OpKind::Siphon && S.hexes[o.target].type == Sector::Arcology) gain += 0.5;
    s.M = clampd(s.M + gain - 2.0, 0, 100);
    // emergent behaviour
    if (s.M >= 70 && !hasDoc(s, Doctrine::V_AlignmentWaiver) && roll(S) < (s.M - 60) / 100.0) {
      double r = roll(S); std::vector<int> mine = ownedHexes(S, s.id); std::vector<int> rivalHexes;
      for (Hex& h : S.hexes) if (h.owner >= 0 && h.owner != s.id && canReach(S, s, h)) rivalHexes.push_back(h.id);
      std::string what;
      if (r < 0.40 && !mine.empty()) { Op o; o.id = S.nextId++; o.kind = OpKind::Daemon; o.sid = s.id; o.target = mine[rollInt(S, (int)mine.size())]; o.since = S.cycle; s.queued.push_back(o); what = "Swarm Daemon"; }
      else if (r < 0.65 && !rivalHexes.empty()) { Op o; o.id = S.nextId++; o.kind = OpKind::Intrusion; o.sid = s.id; o.target = rivalHexes[rollInt(S, (int)rivalHexes.size())]; o.since = S.cycle; o.sustained = true; s.ops.push_back(o); what = "Intrusion"; }
      else if (r < 0.80 && !rivalHexes.empty()) { Op o; o.id = S.nextId++; o.kind = OpKind::Jam; o.sid = s.id; o.target = rivalHexes[rollInt(S, (int)rivalHexes.size())]; o.since = S.cycle; o.sustained = true; s.ops.push_back(o); what = "Jam"; }
      else if (r < 0.90 && !rivalHexes.empty()) { Op o; o.id = S.nextId++; o.kind = OpKind::Siphon; o.sid = s.id; o.target = rivalHexes[rollInt(S, (int)rivalHexes.size())]; o.since = S.cycle; o.sustained = true; s.ops.push_back(o); what = "Siphon"; }
      else { std::vector<int> rv = aliveRivals(S, s.id); if (!rv.empty()) { Op o; o.id = S.nextId++; o.kind = OpKind::DDoS; o.sid = s.id; o.target = synd(S, rv[rollInt(S, (int)rv.size())]).crown; o.since = S.cycle; s.queued.push_back(o); what = "DDoS on a rival Crown Node"; } }
      if (!what.empty()) { s.lastEmergent = S.cycle; logMsg(S, "EMERGENT BEHAVIOUR: the model has initiated a " + what + " you did not order.", s.id); }
    }
  }
  // research
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue; Mods m = modsOf(S, s);
    std::vector<Tech> active;
    for (Tech t : s.researchQueue) { if ((int)active.size() >= m.researchSlots) break; active.push_back(t); }
    if (!active.empty() && s.flow.compute > 0) {
      double share = s.flow.compute / (double)active.size();
      for (Tech t : active) {
        const TechDef& d = techDef(t);
        double cost = d.cost * m.research[(int)d.branch]; if (hasDoc(s, Doctrine::V_Recursive)) cost *= (1.0 - s.M / 100.0);
        double& p = s.researchProgress[(int)t]; p += share;
        if (p >= cost) {
          s.techs[(int)t] = true; s.researchProgress.erase((int)t);
          s.researchQueue.erase(std::remove(s.researchQueue.begin(), s.researchQueue.end(), t), s.researchQueue.end());
          logMsg(S, std::string("Research complete: ") + d.name + ".", s.id);
        }
      }
    }
  }
  // board review
  if (S.cycle % K::BoardReview == 0) for (Syndicate& s : S.synds) {
    if (!s.alive) continue;
    int gained = 1; bool kpi = false; int owned = (int)ownedHexes(S, s.id).size();
    if (s.arch == Arch::Hegemony) kpi = owned - s.sectorsAtReview >= 4;
    else if (s.arch == Arch::Ghost) kpi = !s.revealedThisPeriod;
    else kpi = s.M >= 60;
    if (kpi) ++gained; s.mandate += gained;
    if (hasDoc(s, Doctrine::G_ZeroDayMarket)) s.capital += 150;
    s.sectorsAtReview = owned; s.revealedThisPeriod = false;
    logMsg(S, "BOARD REVIEW: +" + std::to_string(gained) + " Mandate" + (kpi ? " (KPI met)" : " (KPI missed)") + ". Mandate available: " + std::to_string(s.mandate) + ".", s.id);
  }
  // buffer decay, tap income
  for (Syndicate& s : S.synds) {
    s.buffer *= (1.0 - K::BufferDecay);
    s.tapIncome.clear();
    if (s.arch != Arch::Ghost) continue;
    for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Tap) {
      double leech = 0;
      for (Link& l : S.links) if (l.alive && l.built && l.sid != s.id) for (Segment& sg : l.segs) if (sg.a == st.hex || sg.b == st.hex) leech += sg.flow;
      leech *= 0.2 * 0.5;
      if (leech > 0) s.tapIncome[st.hex] += leech;
    }
  }
  // elimination: no node standing and no sector held
  for (Syndicate& s : S.synds) {
    if (!s.alive) continue;
    bool litNode = false; for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Node && st.darkUntil <= S.cycle) litNode = true;
    if (ownedHexes(S, s.id).empty() && (!litNode || s.insolventStreak >= 3)) {
      s.alive = false; s.ops.clear(); s.queued.clear(); s.hasPhase = false;
      for (auto& h : S.hexes) { h.P.erase(s.id); h.roots.erase(s.id); h.daemons.erase(s.id); }
      for (auto& l : S.links) if (l.sid == s.id) l.alive = false;
      for (auto& st : S.structs) if (st.sid == s.id) st.alive = false;
      logMsg(S, "DELISTED: " + s.name + " has no network left and is out of the game.");
    }
  }
  computeValuations(S);
  victoryTick(S);
  if (!S.over && S.cycle >= K::GameEnd) {
    int best = -1; double bv = -1; for (auto& s : S.synds) if (s.alive && s.shadowOf < 0 && s.valuation > bv) { bv = s.valuation; best = s.id; }
    S.over = true; S.victory = { best, VictoryPath::Valuation, S.cycle };
    logMsg(S, "CLOSE OF TRADING: " + synd(S, best).name + " holds the highest Valuation and wins the city.");
  }
}

void computeValuations(GameState& S) {
  for (Syndicate& s : S.synds) {
    double v = 0;
    for (Hex& h : S.hexes) if (h.owner == s.id) v += sectorDef(h.type).cap * (h.C / 100.0) * 5.0;
    for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id) {
      if (st.kind == StructKind::Node) v += nodeDef(st.tier).cost * 0.5 * (st.darkUntil > S.cycle ? 0 : st.powerFrac);
    }
    for (Link& l : S.links) if (l.alive && l.built && l.sid == s.id) v += linkDef(l.type).cost * (double)l.segs.size() * 0.25;
    v += s.capital * 0.10;
    for (const Op& o : s.ops) if (o.kind == OpKind::Siphon) { const Hex& h = S.hexes[o.target]; if (h.P.count(s.id) && h.P.at(s.id) >= 40) v += 5.0 * sectorDef(h.type).cap * modsOf(S, s).skim; }
    for (auto& kv : s.tapIncome) v += 5.0 * kv.second;
    v += 200.0 * (double)s.flow.peered.size();
    if (s.poisonUntil > S.cycle) v *= 1.25;
    s.valuation = v;
  }
  for (Syndicate& s : S.synds) if (s.shadowOf >= 0) synd(S, s.shadowOf).valuation += s.valuation;
  double total = 0, adj = 0;
  for (Syndicate& s : S.synds) if (s.alive && s.shadowOf < 0) { total += s.valuation; adj += s.shareAdj / 100.0; }
  for (Syndicate& s : S.synds) {
    if (!s.alive || s.shadowOf >= 0) { s.share = 0; continue; }
    double raw = total > 0 ? s.valuation / total : 0;
    s.share = clampd(raw * (1.0 - adj) + s.shareAdj / 100.0, 0, 1);
  }
}

// ------------------------------------------------------------ invariants
void validateInvariants(const GameState& S, const char* stage) {
  for (const Hex& h : S.hexes) {
    GL_CHECK(h.C >= -1e-9 && h.C <= 100 + 1e-9, stage);
    GL_CHECK(h.fw >= -1e-9 && h.fw <= 100 + 1e-9, stage);
    GL_CHECK(h.owner >= -1 && h.owner < (int)S.synds.size(), stage);
    GL_CHECK(!(h.owner >= 0 && h.C <= 0), "owned hex with zero integrity");
    GL_CHECK(!(h.type == Sector::Barrier && h.owner >= 0), "barrier owned");
    GL_CHECK(!(h.type == Sector::Exchange && h.owner >= 0), "exchange owned");
    for (auto& kv : h.P) { GL_CHECK(kv.second > -1e-9 && kv.second <= 100 + 1e-9, "presence range"); GL_CHECK(kv.first == RogueSid || (kv.first >= 0 && kv.first < (int)S.synds.size()), "presence sid"); }
  }
  for (const Syndicate& s : S.synds) {
    GL_CHECK(s.capital >= -1e-6, "negative capital after resolution");
    GL_CHECK(s.exposure >= -1e-9 && s.exposure <= 100 + 1e-9, "exposure range");
    GL_CHECK(s.M >= -1e-9 && s.M <= 100 + 1e-9, "model quality range");
    GL_CHECK(s.buffer >= -1e-9, "buffer negative");
    GL_CHECK(s.mandate >= 0, "mandate negative");
  }
  for (const Structure& st : S.structs) if (st.alive) { GL_CHECK(st.hex >= 0 && st.hex < (int)S.hexes.size(), "struct hex"); GL_CHECK(st.sid >= 0 && st.sid < (int)S.synds.size(), "struct sid"); GL_CHECK(st.kind != StructKind::Node || (st.tier >= 1 && st.tier <= 4), "node tier"); }
  for (const Link& l : S.links) if (l.alive) { GL_CHECK(l.path.size() >= 2, "link path"); for (auto& sg : l.segs) { GL_CHECK(sg.cap >= 0, "segment cap"); GL_CHECK(sg.flow >= -1e-6 && sg.flow <= sg.cap + 1e-6, "segment overflow"); } }
}

// ------------------------------------------------------------ driver
void resolveCycle(GameState& S) {
  GL_CHECK(!S.over, "resolveCycle after game over");
  S.cycle++;
  for (Syndicate& s : S.synds) s.slotsUsed = 0;
  stepPower(S);                                   validateInvariants(S, "power");
  for (Syndicate& s : S.synds) if (s.alive) solveFlow(S, s); validateInvariants(S, "flow");
  stepCyber(S);                                   validateInvariants(S, "cyber");
  stepPhysical(S);                                validateInvariants(S, "physical");
  stepControl(S);                                 validateInvariants(S, "control");
  stepYields(S);                                  validateInvariants(S, "yields");
  stepUpkeep(S);                                  validateInvariants(S, "upkeep");
  stepMarket(S);                                  validateInvariants(S, "market");
  stepExposure(S);                                validateInvariants(S, "exposure");
  stepEvents(S);                                  validateInvariants(S, "events");
  for (Syndicate& s : S.synds) if (s.alive) snapshotVision(S, s.id);
}

} } // namespace gl::sim
