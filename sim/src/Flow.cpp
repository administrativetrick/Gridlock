// Per-syndicate bandwidth routing: successive best-survival paths with multiplicative per-hop loss (docs/01 §4, A §3).
#include "Internal.h"
#include <queue>
#include <unordered_map>

namespace gl { namespace sim {

namespace {

struct Edge { int to; Segment* seg; Link* link; };
struct Graph {
  std::unordered_map<int, std::vector<Edge>> adj;
  std::set<int> hexes, resets;
};

constexpr int MaxH = 12;
inline int enc(int hex, int h) { return hex * (MaxH + 1) + h; }

struct Solver {
  GameState& S; Syndicate& s; Mods m; Graph g;
  std::map<int, double> src;                 // hex → residual
  std::set<int> prevDual;
  bool pass2 = false;
  std::vector<double> dist; std::vector<int> cameState; std::vector<Segment*> cameSeg; std::vector<double> cameLoss; std::vector<int> cameDir;

  Solver(GameState& S_, Syndicate& s_) : S(S_), s(s_), m(modsOf(S_, s_)) {}

  void buildGraph() {
    for (Link& l : S.links) {
      if (!l.alive || l.sid != s.id || !l.built) continue;
      for (Segment& sg : l.segs) {
        sg.flow = 0; sg.lastLoss = 0; sg.lastDir = 0;
        g.adj[sg.a].push_back({ sg.b, &sg, &l }); g.adj[sg.b].push_back({ sg.a, &sg, &l });
        g.hexes.insert(sg.a); g.hexes.insert(sg.b);
      }
    }
    for (auto& st : S.structs) if (st.alive && st.sid == s.id) {
      g.hexes.insert(st.hex);
      if (st.built && (st.kind == StructKind::Repeater || st.kind == StructKind::Node)) g.resets.insert(st.hex);
    }
  }

  void initSources() {
    src.clear(); s.flow.produced = 0;
    for (auto& st : S.structs) {
      if (!st.alive || st.sid != s.id || st.kind != StructKind::Node || !st.built) continue;
      bool dark = st.darkUntil > S.cycle;
      double out = dark ? 0 : nodeRaw(S, s, st) * st.powerFrac;
      st.out = out; st.util = 0;
      if (out > 0) { src[st.hex] += out; s.flow.produced += out; }
    }
    for (auto& kv : s.tapIncome) if (kv.second > 0) { src[kv.first] += kv.second; s.flow.produced += kv.second; }
  }

  double hopLoss(const Segment& sg, const Link& l, int toHex, int hn, int target) const {
    const Hex& h = S.hexes[toHex];
    double L;
    if (sg.wireless) L = m.uplinkLoss * sg.dist;
    else {
      double base = (l.type == LinkType::Backbone && has(s, Tech::Superconducting)) ? 0.01 + 0.005 * hn : 0.02 + 0.01 * hn;
      double cong = 0;
      if (pass2 && sg.cap > 0) { double util = sg.flow1 / sg.cap; cong = std::max(0.0, util - 0.8) * 0.25; }
      if (has(s, Tech::RedundantPeering) && prevDual.count(target)) cong = 0;
      double cont = maxRivalP(h, s.id) / 100.0 * 0.10;
      double dmg = sg.sabotagedUntil > S.cycle ? 0.15 : 0;
      double row = 0;
      if (h.owner >= 0 && h.owner != s.id && h.C >= 50) {
        const Cartel* c = cartelAgainst(S, s.id);
        bool denied = h.rowDenial || (c && std::find(c->members.begin(), c->members.end(), h.owner) != c->members.end());
        if (denied) row = 0.10;
      }
      double jam = 0;
      for (auto& kv : h.jamUntil) if (kv.first != s.id && kv.second >= S.cycle) jam += modsOf(S, synd(S, kv.first)).jam;
      double tap = 0;
      for (auto& st : S.structs) if (st.alive && st.built && st.kind == StructKind::Tap && st.hex == toHex && st.sid != s.id) tap = 0.20;
      L = base + cong + cont + dmg + row + jam + tap;
    }
    if (hasDoc(s, Doctrine::H_Rings) && prevDual.count(target)) L *= 0.5;
    return std::min(K::MaxHopLoss, L);
  }

  // Route up to `need` BW to `target`. Returns delivered. Records used segments if `used` given.
  double route(int target, double need, std::set<Segment*>* used) {
    double delivered = 0;
    for (int iter = 0; iter < 30 && need > 1e-6; ++iter) {
      // direct draw when a source sits in the target hex
      auto sit = src.find(target);
      if (sit != src.end() && sit->second > 1e-9) { double take = std::min(need, sit->second); sit->second -= take; need -= take; delivered += take; continue; }
      if (g.adj.find(target) == g.adj.end()) break;
      // multi-source Dijkstra over (hex, h)
      size_t N = S.hexes.size() * (MaxH + 1);
      dist.assign(N, 1e18); cameState.assign(N, -1); cameSeg.assign(N, nullptr); cameLoss.assign(N, 0); cameDir.assign(N, 0);
      using QN = std::pair<double, int>;
      std::priority_queue<QN, std::vector<QN>, std::greater<QN>> pq;
      for (auto& kv : src) if (kv.second > 1e-9 && g.adj.count(kv.first)) { dist[enc(kv.first, 0)] = 0; pq.push({ 0, enc(kv.first, 0) }); }
      int found = -1;
      while (!pq.empty()) {
        auto [d, st] = pq.top(); pq.pop();
        if (d > dist[st]) continue;
        int hx = st / (MaxH + 1), h = st % (MaxH + 1);
        if (hx == target) { found = st; break; }
        auto ai = g.adj.find(hx); if (ai == g.adj.end()) continue;
        for (const Edge& e : ai->second) {
          double resid = e.seg->cap - e.seg->flow; if (resid <= 1e-9) continue;
          int hn = h + 1;
          double L = hopLoss(*e.seg, *e.link, e.to, hn, target);
          double w = -std::log(1.0 - L);
          int hs = (g.resets.count(e.to) || (m.hopReset > 0 && hn >= m.hopReset)) ? 0 : std::min(hn, MaxH);
          int ns = enc(e.to, hs);
          if (d + w < dist[ns]) { dist[ns] = d + w; cameState[ns] = st; cameSeg[ns] = e.seg; cameLoss[ns] = L; cameDir[ns] = (e.seg->a == hx) ? 1 : -1; pq.push({ d + w, ns }); }
        }
      }
      if (found < 0) break;
      // reconstruct
      std::vector<Segment*> segs; std::vector<double> losses; std::vector<int> dirs; int cur = found;
      while (cameState[cur] >= 0) { segs.push_back(cameSeg[cur]); losses.push_back(cameLoss[cur]); dirs.push_back(cameDir[cur]); cur = cameState[cur]; }
      std::reverse(segs.begin(), segs.end()); std::reverse(losses.begin(), losses.end()); std::reverse(dirs.begin(), dirs.end());
      int srcHex = cur / (MaxH + 1);
      GL_CHECK(src.count(srcHex), "path must start at a source");
      double F = std::min(src[srcHex], 1e18); double surv = 1.0;
      std::vector<double> survBefore;
      for (size_t k = 0; k < segs.size(); ++k) { survBefore.push_back(surv); F = std::min(F, (segs[k]->cap - segs[k]->flow) / surv); surv *= (1.0 - losses[k]); }
      F = std::min(F, need / surv);
      if (F <= 1e-9) break;
      for (size_t k = 0; k < segs.size(); ++k) { segs[k]->flow += F * survBefore[k]; segs[k]->lastLoss = losses[k]; segs[k]->lastDir = dirs[k]; if (used) used->insert(segs[k]); }
      src[srcHex] -= F; delivered += F * surv; need -= F * surv;
    }
    return delivered;
  }

  double drawAny(double need) {
    double got = 0;
    for (auto& kv : src) { if (need <= 1e-9) break; double t = std::min(need, kv.second); kv.second -= t; need -= t; got += t; }
    return got;
  }
  double residual() const { double r = 0; for (auto& kv : src) r += kv.second; return r; }

  bool dualPath(int target, const std::set<Segment*>& used) {
    std::set<int> seen; std::vector<int> q;
    for (auto& kv : src) if (g.adj.count(kv.first) || kv.first == target) { seen.insert(kv.first); q.push_back(kv.first); }
    for (auto& st : S.structs) if (st.alive && st.built && st.sid == s.id && st.kind == StructKind::Node && st.darkUntil <= S.cycle) if (!seen.count(st.hex)) { seen.insert(st.hex); q.push_back(st.hex); }
    while (!q.empty()) {
      int cur = q.back(); q.pop_back();
      if (cur == target) return true;
      auto ai = g.adj.find(cur); if (ai == g.adj.end()) continue;
      for (const Edge& e : ai->second) { if (used.count(e.seg)) continue; if (e.seg->cap <= 0) continue; if (!seen.count(e.to)) { seen.insert(e.to); q.push_back(e.to); } }
    }
    return false;
  }

  struct Sink { int hex; double need; Priority pri; int hops; double cap; };

  void runPass(std::vector<Sink>& sinks, double opsNeed) {
    initSources();
    for (Sink& k : sinks) if (k.pri == Priority::Critical) serveSector(k);
    double pool = drawAny(opsNeed); if (pass2) s.flow.opsPool = pool;
    for (Sink& k : sinks) if (k.pri == Priority::Normal) serveSector(k);
    for (Sink& k : sinks) if (k.pri == Priority::Low) serveSector(k);
    // compute
    for (auto& st : S.structs) {
      if (!st.alive || st.sid != s.id || st.kind != StructKind::Node || !st.built || st.out <= 0) continue;
      auto it = src.find(st.hex); if (it == src.end()) continue;
      double cap = std::min(m.computeCap, s.computePct) * st.out;
      double c = std::min(cap, it->second); it->second -= c;
      double eff = st.variant == Variant::Inference ? c * 1.5 : c;
      if (pass2) { s.flow.compute += eff; s.flow.computeByNode[st.id] = eff; }
    }
    // exchange fee + sales
    for (int ex : S.exchanges) {
      if (!g.hexes.count(ex) || !structIn(S, ex, s.id, StructKind::Rack)) continue;
      double fee = route(ex, sectorDef(Sector::Exchange).demand, nullptr);
      if (fee >= sectorDef(Sector::Exchange).demand - 0.01) {
        if (pass2) s.flow.peered.push_back(ex);
        double sold = route(ex, 1e9, nullptr);
        if (pass2) s.flow.sold += sold;
      }
    }
    // buffer
    double add = std::min({ 0.10 * s.flow.produced, residual(), std::max(0.0, m.bufferCap - s.buffer) });
    drawAny(add);
    if (pass2) { s.flow.bufferAdd = add; s.buffer += add; s.flow.stranded = residual(); }
    // utilization for autoscaling / rendering
    if (pass2) for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node && st.built && st.out > 0) {
      double left = src.count(st.hex) ? src[st.hex] : 0; st.util = std::max(0.0, std::min(1.0, 1.0 - left / st.out));
    }
  }

  void serveSector(Sink& k) {
    std::set<Segment*> used;
    double got = route(k.hex, k.need, pass2 ? &used : nullptr);
    if (pass2) {
      Hex& h = S.hexes[k.hex];
      h.delivered[s.id] = got; h.demand[s.id] = k.need;
      s.flow.delivered += got;
      if (got >= k.need - 1e-6 && k.need > 0 && dualPath(k.hex, used)) h.dual.insert(s.id);
    }
  }
};

} // namespace

void solveFlow(GameState& S, Syndicate& s) {
  Solver sv(S, s);
  for (Hex& h : S.hexes) { if (h.dual.count(s.id)) sv.prevDual.insert(h.id); h.dual.erase(s.id); h.delivered.erase(s.id); h.demand.erase(s.id); h.ratio.erase(s.id); }
  FlowResult keep; keep.bought = s.flow.bought; s.flow = FlowResult{}; s.flow.bought = keep.bought;
  sv.buildGraph();

  // sinks
  std::set<int> srcHexes; for (auto& st : S.structs) if (st.alive && st.sid == s.id && st.kind == StructKind::Node && st.built && st.darkUntil <= S.cycle) srcHexes.insert(st.hex);
  std::map<int, int> hops; { std::vector<int> q(srcHexes.begin(), srcHexes.end()); for (int h : q) hops[h] = 0; size_t i = 0; while (i < q.size()) { int cur = q[i++]; auto ai = sv.g.adj.find(cur); if (ai == sv.g.adj.end()) continue; for (auto& e : ai->second) if (!hops.count(e.to)) { hops[e.to] = hops[cur] + 1; q.push_back(e.to); } } }
  std::vector<Solver::Sink> sinks;
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier || h.type == Sector::Exchange) continue;
    bool own = h.owner == s.id;
    bool inGraph = sv.g.hexes.count(h.id) > 0;
    bool claimable = inGraph && h.owner == -1;
    bool attack = inGraph && h.owner >= 0 && !own && ((h.brownout && h.C < 50 && h.P.count(s.id) && h.P.at(s.id) >= 50) || h.roots.count(s.id));
    if (s.hasPhase && s.phase.type == VictoryPath::Blackout) attack = attack || (inGraph && h.owner >= 0 && !own && h.brownout);
    if (!(own || claimable || attack)) continue;
    Solver::Sink k; k.hex = h.id; k.need = sectorDemand(S, h, s.id);
    auto pi = h.priority.find(s.id); k.pri = pi == h.priority.end() ? Priority::Normal : pi->second;
    if (!own) k.pri = Priority::Low;                       // claims/takeovers are served after everything you already hold
    k.hops = hops.count(h.id) ? hops[h.id] : 99; k.cap = sectorDef(h.type).cap;
    sinks.push_back(k);
  }
  std::sort(sinks.begin(), sinks.end(), [](const Solver::Sink& a, const Solver::Sink& b) {
    if (a.hops != b.hops) return a.hops < b.hops; if (a.cap != b.cap) return a.cap > b.cap; return a.hex < b.hex; });

  // ops need
  double opsNeed = 0;
  for (const Op& o : s.ops) { const OpDef& d = opDef(o.kind); double c = d.bw * sv.m.opsCost; if (o.kind == OpKind::Intrusion) c *= sv.m.intrusionCost; opsNeed += c; }
  for (const Op& o : s.queued) { const OpDef& d = opDef(o.kind); double c = d.bw * sv.m.opsCost; if (o.kind == OpKind::Root) c = sv.m.rootCost * sv.m.opsCost; if (o.kind == OpKind::Purge) c = sv.m.purgeCost; if (o.kind == OpKind::Intrusion) c *= sv.m.intrusionCost; opsNeed += c; }
  for (Hex& h : S.hexes) { auto it = h.daemons.find(s.id); if (it != h.daemons.end()) opsNeed += it->second * 1.0; }
  s.flow.opsNeed = opsNeed;

  // bought bandwidth appears as a source at the first exchange peered last cycle
  int buyEx = -1; for (auto& kv : s.peeredLast) if (kv.second == S.cycle - 1) { buyEx = kv.first; break; }
  if (s.buyBW > 0 && buyEx >= 0) {
    double price = K::BuyPrice; for (auto& r : S.synds) if (r.id != s.id && hasDoc(r, Doctrine::H_PeeringCartel) && r.peeredLast.count(buyEx) && r.peeredLast.at(buyEx) == S.cycle - 1) price += 1.0;
    const Cartel* c = cartelAgainst(S, s.id); if (c) price *= 2.0;
    double amt = std::min(s.buyBW, K::BuyCap); double cost = amt * price;
    if (s.capital >= cost) { s.capital -= cost; s.flow.bought = amt; s.tapIncome[buyEx] += amt; logMsg(S, "Bought " + fmt(amt) + " BW at the Exchange for " + fmt(cost, 0), s.id); }
  }
  s.buyBW = 0;

  // pass 1 (no congestion) → pass 2 (congestion from pass-1 utilization)
  sv.pass2 = false; sv.runPass(sinks, opsNeed);
  for (Link& l : S.links) if (l.alive && l.sid == s.id) for (Segment& sg : l.segs) { sg.flow1 = sg.flow; sg.flow = 0; }
  sv.pass2 = true; sv.runPass(sinks, opsNeed);
  // bought BW was injected via tapIncome for this cycle only; drop it
  if (s.flow.bought > 0 && buyEx >= 0) { s.tapIncome[buyEx] -= s.flow.bought; if (s.tapIncome[buyEx] <= 1e-9) s.tapIncome.erase(buyEx); }
  s.flow.lost = std::max(0.0, s.flow.produced - (s.flow.delivered + s.flow.opsPool + s.flow.compute + s.flow.sold + s.flow.bufferAdd + s.flow.stranded + 10.0 * s.flow.peered.size()));
}

bool twoDisjointPaths(const GameState& S, int sid, int from, const std::set<int>& targets) {
  std::unordered_map<int, std::vector<std::pair<int, const Segment*>>> adj;
  for (const Link& l : S.links) if (l.alive && l.sid == sid && l.built) for (const Segment& sg : l.segs) { if (sg.cap <= 0) continue; adj[sg.a].push_back({ sg.b, &sg }); adj[sg.b].push_back({ sg.a, &sg }); }
  std::set<const Segment*> banned;
  for (int round = 0; round < 2; ++round) {
    std::map<int, std::pair<int, const Segment*>> came; std::vector<int> q{ from }; came[from] = { -1, nullptr }; int hit = -1;
    for (size_t i = 0; i < q.size() && hit < 0; ++i) {
      int cur = q[i]; if (targets.count(cur) && cur != from) { hit = cur; break; }
      for (auto& e : adj[cur]) { if (banned.count(e.second) || came.count(e.first)) continue; came[e.first] = { cur, e.second }; q.push_back(e.first); }
    }
    if (hit < 0) return false;
    for (int c = hit; came[c].first >= 0; c = came[c].first) banned.insert(came[c].second);
  }
  return true;
}

} } // namespace gl::sim
