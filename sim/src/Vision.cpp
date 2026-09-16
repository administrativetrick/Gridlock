// Fog of war: what a syndicate can see, and stale snapshots of what it saw (docs/00 pillar 1, A §4).
#include "Internal.h"

namespace gl { namespace sim {

static bool basicSee(const GameState& S, int viewer, int hexId) {
  const Hex& h = S.hexes[hexId];
  if (h.owner == viewer) return true;
  if (anyStructIn(S, hexId, viewer)) return true;
  auto p = h.P.find(viewer); if (p != h.P.end() && p->second >= 1) return true;
  auto sc = h.scans.find(viewer); if (sc != h.scans.end() && sc->second >= S.cycle - K::ScanFreshness) return true;
  auto dm = h.daemons.find(viewer); if (dm != h.daemons.end() && dm->second > 0) return true;
  for (int n : S.grid.neighborsV(hexId)) {
    const Hex& nh = S.hexes[n];
    if (nh.owner == viewer || anyStructIn(S, n, viewer)) return true;
  }
  return false;
}

bool canSee(const GameState& S, int viewer, int hexId) {
  if (viewer < 0) return true;
  if (basicSee(S, viewer, hexId)) return true;
  for (const Cartel& c : S.cartels) {
    bool member = std::find(c.members.begin(), c.members.end(), viewer) != c.members.end();
    if (!member) continue;
    for (int m : c.members) if (m != viewer && basicSee(S, m, hexId)) return true;
  }
  for (const Syndicate& s : S.synds) if (s.shadowOf == viewer && basicSee(S, s.id, hexId)) return true;
  return false;
}

bool assetVisible(const GameState& S, int viewer, const Structure& st) {
  if (viewer < 0 || st.sid == viewer) return true;
  const Syndicate& o = synd(S, st.sid);
  if (modsOf(S, o).hidden) {
    if (st.variant == Variant::Phantom && !st.revealed) return false;
    if (!st.revealed && !S.hexes[st.hex].revealedTo.count(viewer)) return false;
  }
  if (st.kind == StructKind::Node && o.noticed && st.variant != Variant::Phantom) return true;
  return canSee(S, viewer, st.hex);
}

bool linkVisible(const GameState& S, int viewer, const Link& l) {
  if (viewer < 0 || l.sid == viewer) return true;
  if (l.type == LinkType::Dark) { bool lit = false; for (auto& sg : l.segs) if (sg.flow > 1e-6) lit = true; if (!lit) return false; }
  const Syndicate& o = synd(S, l.sid);
  if (modsOf(S, o).hidden) { bool rev = false; for (int h : l.path) if (S.hexes[h].revealedTo.count(viewer)) rev = true; if (!rev) return false; }
  for (int h : l.path) if (canSee(S, viewer, h)) return true;
  return false;
}

bool presenceVisible(const GameState& S, int viewer, int hexId) {
  if (viewer < 0) return true;
  const Hex& h = S.hexes[hexId];
  if (structIn(S, hexId, viewer, StructKind::Array) || structIn(S, hexId, viewer, StructKind::Honeypot)) return true;
  for (int n : S.grid.neighborsV(hexId)) if (structIn(S, n, viewer, StructKind::Array)) return true;
  auto sc = h.scans.find(viewer); if (sc != h.scans.end() && sc->second >= S.cycle - K::ScanFreshness) return true;
  if (h.roots.count(viewer)) return true;
  return false;
}

void snapshotVision(GameState& S, int viewer) {
  for (Hex& h : S.hexes) {
    if (!canSee(S, viewer, h.id)) continue;
    HexSeen hs; hs.cycle = S.cycle; hs.owner = h.owner; hs.C = h.C; hs.fw = h.fw;
    for (const Structure* st : structsIn(S, h.id)) if (st->built && assetVisible(S, viewer, *st)) hs.structs.push_back({ st->kind, st->sid });
    h.seen[viewer] = hs;
  }
}

} } // namespace gl::sim
