#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace gl {

// Deterministic mulberry32. State is one uint32 so it serializes trivially.
class Rng {
public:
  explicit Rng(uint32_t seed = 1) : a_(seed ? seed : 1u) {}
  double next() {
    a_ += 0x6D2B79F5u;
    uint32_t t = a_;
    t = (t ^ (t >> 15)) * (1u | t);
    t ^= t + ((t ^ (t >> 7)) * (61u | t));
    return double(t ^ (t >> 14)) / 4294967296.0;
  }
  int    intn(int n) { return n <= 0 ? 0 : int(next() * n); }
  bool   chance(double p) { return next() < p; }
  double range(double lo, double hi) { return lo + next() * (hi - lo); }
  template <class T> const T& pick(const std::vector<T>& v) { return v[intn((int)v.size())]; }
  template <class T> void shuffle(std::vector<T>& v) { for (int i = (int)v.size() - 1; i > 0; --i) { int j = intn(i + 1); std::swap(v[i], v[j]); } }
  uint32_t state() const { return a_; }
  void setState(uint32_t s) { a_ = s ? s : 1u; }
private:
  uint32_t a_;
};

inline uint32_t hashStr(const std::string& s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s) { h ^= c; h *= 16777619u; }
  return h;
}

} // namespace gl
