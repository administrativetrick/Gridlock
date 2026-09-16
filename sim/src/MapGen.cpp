#include "Internal.h"

namespace gl { namespace sim {

void generateMap(GameState& S, Rng& rng, int W, int H, int nSynd, int nEx);

void generateMap(GameState& S, Rng& rng, int W, int H, int nSynd, int nEx) {
  GL_CHECK(W >= 12 && H >= 10, "map too small");
  Grid& g = S.grid; g.W = W; g.H = H; g.pos.clear(); g.idx.clear(); S.hexes.clear();
  for (int r = 0; r < H; ++r) {
    int off = r / 2;
    for (int c = 0; c < W; ++c) {
      int q = c - off; int id = (int)S.hexes.size();
      Hex h; h.id = id; h.q = q; h.r = r; h.type = Sector::Sprawl;
      S.hexes.push_back(h); g.pos.push_back({ q, r }); g.idx[axialKey(q, r)] = id;
    }
  }
  // centre = mean position
  double sq = 0, sr = 0; for (auto& p : g.pos) { sq += p.q; sr += p.r; }
  Axial center{ (int)std::lround(sq / g.pos.size()), (int)std::lround(sr / g.pos.size()) };
  int maxD = 1; for (auto& p : g.pos) maxD = std::max(maxD, hexDist(p, center));

  // barriers: river west→east, rail north→south
  std::set<int> barrier;
  auto walk = [&](int start, const std::vector<std::pair<int, int>>& dirs, int steps) {
    int cur = start; if (cur < 0) return;
    for (int i = 0; i < steps; ++i) {
      barrier.insert(cur);
      const Axial& p = g.pos[cur];
      std::vector<int> opts; for (auto& d : dirs) { int k = g.at(p.q + d.first, p.r + d.second); if (k >= 0) opts.push_back(k); }
      if (opts.empty()) break;
      cur = rng.pick(opts);
    }
  };
  int riverRow = (int)std::floor(H * rng.range(0.35, 0.65));
  walk(g.at(-(riverRow / 2), riverRow), { {1,0},{1,0},{1,-1},{0,1} }, W + 6);
  int railCol = (int)std::floor(W * rng.range(0.3, 0.7));
  walk(g.at(railCol, 0), { {0,1},{0,1},{-1,1},{1,0} }, H + 4);
  for (int id : barrier) S.hexes[id].type = Sector::Barrier;

  // sector bands
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier) continue;
    double d = hexDist(g.pos[h.id], center) / (double)maxD; double n = rng.next();
    if (d < 0.22)      h.type = n < 0.55 ? Sector::Financial : n < 0.9 ? Sector::Campus : Sector::Arcology;
    else if (d < 0.45) h.type = n < 0.3 ? Sector::Campus : n < 0.55 ? Sector::Arcology : n < 0.8 ? Sector::Industrial : Sector::Sprawl;
    else if (d < 0.7)  h.type = n < 0.35 ? Sector::Industrial : n < 0.5 ? Sector::Arcology : n < 0.9 ? Sector::Sprawl : Sector::Undercity;
    else               h.type = n < 0.6 ? Sector::Sprawl : n < 0.9 ? Sector::Undercity : Sector::Industrial;
  }
  for (int id : barrier) for (int n : g.neighborsV(id)) if (S.hexes[n].type != Sector::Barrier && rng.chance(0.18)) S.hexes[n].type = Sector::Port;

  // starts
  std::vector<int> cands;
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier) continue;
    double d = hexDist(g.pos[h.id], center) / (double)maxD;
    if (d > 0.35 && d < 0.75 && g.neighborsV(h.id).size() == 6) cands.push_back(h.id);
  }
  rng.shuffle(cands);
  S.starts.clear();
  for (int sp = 9; sp >= 3 && (int)S.starts.size() < nSynd; --sp) {
    for (int c : cands) {
      if (std::find(S.starts.begin(), S.starts.end(), c) != S.starts.end()) continue;
      bool ok = true; for (int s : S.starts) if (g.dist(s, c) < sp) ok = false;
      if (ok) S.starts.push_back(c);
      if ((int)S.starts.size() >= nSynd) break;
    }
  }
  GL_CHECK((int)S.starts.size() == nSynd, "could not place all syndicate starts");
  for (int s : S.starts) { S.hexes[s].type = Sector::Campus; for (int n : g.neighborsV(s)) if (S.hexes[n].type == Sector::Barrier) S.hexes[n].type = Sector::Sprawl; }

  // exchanges
  std::vector<int> exc;
  for (Hex& h : S.hexes) {
    if (h.type == Sector::Barrier) continue;
    bool far = true; for (int s : S.starts) if (g.dist(s, h.id) < 5) far = false;
    if (!far) continue;
    double d = hexDist(g.pos[h.id], center) / (double)maxD;
    bool nearBarrier = false; for (int n : g.neighborsV(h.id)) if (S.hexes[n].type == Sector::Barrier) nearBarrier = true;
    if (d < 0.3 || nearBarrier) exc.push_back(h.id);
  }
  rng.shuffle(exc);
  S.exchanges.clear();
  for (int c : exc) { bool ok = true; for (int e : S.exchanges) if (g.dist(e, c) < 6) ok = false; if (ok) S.exchanges.push_back(c); if ((int)S.exchanges.size() >= nEx) break; }
  GL_CHECK(!S.exchanges.empty(), "no exchange could be placed");
  for (int e : S.exchanges) S.hexes[e].type = Sector::Exchange;

  // districts
  std::vector<int> nb; for (Hex& h : S.hexes) if (h.type != Sector::Barrier) nb.push_back(h.id);
  rng.shuffle(nb);
  std::vector<int> seeds;
  for (int c : nb) { bool ok = true; for (int s : seeds) if (g.dist(s, c) < 5) ok = false; if (ok) seeds.push_back(c); if ((int)seeds.size() >= 9) break; }
  while ((int)seeds.size() < 9) seeds.push_back(rng.pick(nb));
  S.districtCount = 9;
  for (Hex& h : S.hexes) { int best = 0, bd = 1 << 30; for (int i = 0; i < 9; ++i) { int d = g.dist(seeds[i], h.id); if (d < bd) { bd = d; best = i; } } h.district = best; }
  for (Hex& h : S.hexes) h.fw = sectorDef(h.type).fw;
}

} } // namespace gl::sim
