#include "gridlock/Hex.h"
#include <queue>
#include <unordered_map>
#include <algorithm>

namespace gl {

std::vector<int> astar(const Grid& g, int start, int goal, const std::function<double(int, int)>& cost) {
  struct QN { double f; int id; bool operator<(const QN& o) const { return f > o.f; } };
  std::priority_queue<QN> open;
  std::unordered_map<int, double> gScore; std::unordered_map<int, int> came;
  gScore[start] = 0; open.push({ (double)g.dist(start, goal), start });
  while (!open.empty()) {
    QN cur = open.top(); open.pop();
    if (cur.id == goal) {
      std::vector<int> path{ goal };
      while (came.count(path.back())) path.push_back(came[path.back()]);
      std::reverse(path.begin(), path.end());
      return path;
    }
    auto gi = gScore.find(cur.id);
    if (gi == gScore.end()) continue;
    double gc = gi->second;
    int nb[6], n; g.neighbors(cur.id, nb, n);
    for (int i = 0; i < n; ++i) {
      double c = cost(cur.id, nb[i]); if (c < 0) continue;
      double ng = gc + c;
      auto it = gScore.find(nb[i]);
      if (it == gScore.end() || ng < it->second) { gScore[nb[i]] = ng; came[nb[i]] = cur.id; open.push({ ng + g.dist(nb[i], goal), nb[i] }); }
    }
  }
  return {};
}

} // namespace gl
