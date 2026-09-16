#pragma once
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <functional>
#include <unordered_map>

namespace gl {

struct Axial { int q, r; };
inline int64_t axialKey(int q, int r) { return (int64_t(q) << 32) ^ (uint32_t)r; }
inline int hexDist(Axial a, Axial b) { int dq = a.q - b.q, dr = a.r - b.r; return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2; }
constexpr int HexDirs[6][2] = { {1,0},{1,-1},{0,-1},{-1,0},{-1,1},{0,1} };

// Minimal grid topology used by pathfinding; the full Hex record lives in State.h.
struct Grid {
  int W = 0, H = 0;
  std::vector<Axial> pos;                       // by hex id
  std::unordered_map<int64_t, int> idx;         // axialKey → id
  int at(int q, int r) const { auto it = idx.find(axialKey(q, r)); return it == idx.end() ? -1 : it->second; }
  void neighbors(int id, int out[6], int& n) const {
    n = 0; const Axial& p = pos[id];
    for (auto& d : HexDirs) { int k = at(p.q + d[0], p.r + d[1]); if (k >= 0) out[n++] = k; }
  }
  std::vector<int> neighborsV(int id) const { int b[6], n; neighbors(id, b, n); return std::vector<int>(b, b + n); }
  std::vector<int> within(int id, int radius) const {
    std::vector<int> out; const Axial& c = pos[id];
    for (int dq = -radius; dq <= radius; ++dq) for (int dr = -radius; dr <= radius; ++dr) {
      if (std::abs(dq + dr) > radius) continue; int k = at(c.q + dq, c.r + dr); if (k >= 0 && k != id) out.push_back(k);
    }
    return out;
  }
  int dist(int a, int b) const { return hexDist(pos[a], pos[b]); }
};

// A* over hex ids. cost(from,to) returns < 0 for impassable.
std::vector<int> astar(const Grid& g, int start, int goal, const std::function<double(int, int)>& cost);

} // namespace gl
