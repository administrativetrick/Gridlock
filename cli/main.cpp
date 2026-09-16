// Gridlock: Silicon Syndicate — console client. Drives the sim through the public Game API only.
#include "gridlock/Game.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace gl;

static const char* COL[] = { "\x1b[96m", "\x1b[95m", "\x1b[93m", "\x1b[92m", "\x1b[94m", "\x1b[91m" };
static const char* RESET = "\x1b[0m", * DIM = "\x1b[2m", * BOLD = "\x1b[1m", * GRAY = "\x1b[90m", * INV = "\x1b[7m";

static std::string col(int sid) { return sid >= 0 && sid < 6 ? COL[sid] : GRAY; }
static std::string f1(double v) { char b[32]; std::snprintf(b, sizeof b, "%.1f", v); return b; }
static std::string f0(double v) { char b[32]; std::snprintf(b, sizeof b, "%.0f", v); return b; }
static const char* kindName(StructKind k, int tier) { return k == StructKind::Node ? nodeDef(tier).name : structDef(k).name; }
static char marker(StructKind k) { switch (k) { case StructKind::Node: return 'N'; case StructKind::Substation: return 'S'; case StructKind::PrivateGrid: return 'G'; case StructKind::Repeater: return 'R'; case StructKind::Outpost: return 'O'; case StructKind::Array: return 'A'; case StructKind::Honeypot: return 'H'; case StructKind::Lab: return 'L'; case StructKind::Rack: return 'K'; case StructKind::Tap: return 'T'; case StructKind::Cutout: return 'C'; case StructKind::Fork: return 'F'; default: return '?'; } }

struct Cli {
  Game g; int me = 0; int sel = -1;
  explicit Cli(const Config& c) : g(c) {}
  const GameState& S() const { return g.S(); }

  void drawMap() {
    const GameState& s = S();
    std::printf("\n%s  Map: 3-digit hex id + marker (N node S substation R repeater O outpost A array L lab K rack H honeypot T tap * your presence). Odd rows are offset.%s\n", DIM, RESET);
    std::printf("%s  Colours = controlling syndicate. Dim = stale (last-seen data). '...' = never seen. X = Exchange, ~ = barrier.%s\n\n", DIM, RESET);
    for (int r = 0; r < s.grid.H; ++r) {
      std::string line = (r % 2) ? "  " : "";
      for (int c = 0; c < s.grid.W; ++c) {
        int id = r * s.grid.W + c; const Hex& h = s.hexes[id];
        bool vis = g.CanSee(me, id); auto seen = h.seen.find(me); bool ever = vis || seen != h.seen.end();
        char buf[16];
        if (h.type == Sector::Barrier) { line += std::string(GRAY) + " ~~~" + RESET; continue; }
        if (!ever) { line += std::string(GRAY) + DIM + " ..." + RESET; continue; }
        int owner = vis ? h.owner : seen->second.owner;
        char mk = ' ';
        if (h.type == Sector::Exchange) mk = 'X';
        else if (vis) { for (const Structure* st : g.StructsIn(id)) if (st->built && g.AssetVisible(me, *st)) { mk = marker(st->kind); if (st->sid == me) break; } if (mk == ' ' && h.owner != me && h.P.count(me) && h.P.at(me) > 0) mk = '*'; }
        else if (!seen->second.structs.empty()) mk = marker(seen->second.structs[0].first);
        std::snprintf(buf, sizeof buf, "%3d%c", id, mk);
        std::string cell = buf;
        std::string colour = owner >= 0 ? col(owner) : (h.type == Sector::Exchange ? std::string("\x1b[97m") : std::string(GRAY));
        if (!vis) colour += DIM;
        if (id == sel) colour += INV;
        line += colour + cell + RESET;
      }
      std::printf("%s\n", line.c_str());
    }
    std::printf("\n");
  }

  void drawHeader() {
    const GameState& s = S(); const Syndicate& y = s.synds[me]; Mods m = g.ModsOf(me);
    std::printf("%s%s== GRIDLOCK: SILICON SYNDICATE ==  cycle %d/%d  %s%s (%s)%s\n", BOLD, COL[me], s.cycle, K::GameEnd, y.name.c_str(), RESET, archDef(y.arch).name, RESET);
    std::printf("Capital %s%s%s  upkeep %s/cycle  |  BW produced %s  delivered %s  lost %s  ops %s/%s  compute %s  sold %s  buffer %s\n",
      BOLD, f0(y.capital).c_str(), RESET, f0(g.UpkeepOf(me)).c_str(), f1(y.flow.produced).c_str(), f1(y.flow.delivered).c_str(), f1(y.flow.lost).c_str(), f1(y.flow.opsPool).c_str(), f1(y.flow.opsNeed).c_str(), f1(y.flow.compute).c_str(), f1(y.flow.sold).c_str(), f1(y.buffer).c_str());
    int sectors = 0; for (auto& h : s.hexes) if (h.owner == me) ++sectors;
    std::printf("Sectors %d  Exposure %s%s  Valuation share %s%%  Mandate %d  Slots %d/%d  Exchanges peered %d  Market %s/BW%s\n",
      sectors, f0(y.exposure).c_str(), y.exposure >= 60 ? " (AUDIT RISK)" : y.exposure >= 40 ? " (noticed)" : "", f1(y.share * 100).c_str(), y.mandate, y.slotsUsed, m.slots, (int)y.flow.peered.size(), f1(s.market.price).c_str(), y.arch == Arch::Hive ? ("  M " + f0(y.M)).c_str() : "");
    if (y.hasPhase) std::printf("%sACTIVE PHASE: %s, %d cycles left%s\n", BOLD, pathName(y.phase.type), y.phase.cyclesLeft, RESET);
    for (auto& o : s.synds) if (o.id != me && o.hasPhase) std::printf("%s%s is in a %s phase (%d cycles left)!%s\n", col(o.id).c_str(), o.name.c_str(), pathName(o.phase.type), o.phase.cyclesLeft, RESET);
  }

  void showLog(int n) { for (auto& l : g.RecentLog(me, n)) std::printf("  %s\n", l.c_str()); }

  void info(int id) {
    const GameState& s = S(); if (id < 0 || id >= (int)s.hexes.size()) { std::puts("Bad hex id."); return; }
    const Hex& h = s.hexes[id]; bool vis = g.CanSee(me, id); sel = id;
    std::printf("%s#%d %s%s  (q %d, r %d)  District %d  %s\n", BOLD, id, sectorDef(h.type).name, RESET, h.q, h.r, h.district + 1, vis ? "" : "[STALE — not currently visible]");
    if (h.type == Sector::Barrier) { std::puts("  Barrier: conduit only (+15 crossing premium into the next hex)."); return; }
    const SectorDef& d = sectorDef(h.type);
    std::printf("  Base yield %s  base demand %s  conduit %s  pop %d  |  rights: %s\n", f0(d.cap).c_str(), f0(d.demand).c_str(), f0(d.conduit).c_str(), d.pop, (h.rights.count(me) || h.owner == me) ? "yes" : "no");
    int owner = h.owner; double C = h.C;
    if (!vis) { auto it = h.seen.find(me); if (it == h.seen.end()) { std::puts("  Never seen. Scan it or move adjacent."); return; } owner = it->second.owner; C = it->second.C; std::printf("  Last seen cycle %d.\n", it->second.cycle); }
    std::printf("  %sPHYSICAL%s  owner: %s%s%s  integrity %s%s\n", BOLD, RESET, col(owner).c_str(), owner >= 0 ? s.synds[owner].name.c_str() : "neutral", RESET, f0(C).c_str(), h.brownout && vis ? "  BROWNOUT" : "");
    if (vis && h.delivered.count(me)) std::printf("  your delivery %s / demand %s  (r %s)%s%s\n", f1(h.delivered.at(me)).c_str(), f1(h.demand.count(me) ? h.demand.at(me) : 0).c_str(), f1(h.ratio.count(me) ? h.ratio.at(me) : 0).c_str(), h.dual.count(me) ? "  dual-path" : "  SINGLE PATH", h.priority.count(me) && h.priority.at(me) == Priority::Critical ? "  CRITICAL" : h.priority.count(me) && h.priority.at(me) == Priority::Low ? "  low" : "");
    if (vis) for (const Structure* st : g.StructsIn(id)) {
      if (!g.AssetVisible(me, *st)) continue;
      std::printf("    %s[%d] %s%s (%s)%s", col(st->sid).c_str(), st->id, kindName(st->kind, st->tier), st->crown ? " CROWN" : "", s.synds[st->sid].name.c_str(), RESET);
      if (!st->built) std::printf("  under construction (%d)", st->buildLeft);
      if (st->darkUntil > s.cycle) std::printf("  DARK (%d)", st->darkUntil - s.cycle);
      if (st->kind == StructKind::Node && st->built) std::printf("  power %s%%  out %s  util %s%%", f0(st->powerFrac * 100).c_str(), f1(st->out).c_str(), f0(st->util * 100).c_str());
      if (st->orphanedAt >= 0) std::printf("  ORPHANED since %d%s", st->orphanedAt, (h.owner == me && s.cycle - st->orphanedAt >= 3) ? " — seizable" : "");
      std::printf("\n");
    }
    if (vis) for (const Link* l : g.LinksThrough(id)) {
      if (!g.LinkVisible(me, *l)) continue;
      std::printf("    %slink %d %s (%s) %d→%d%s%s", col(l->sid).c_str(), l->id, linkDef(l->type).name, s.synds[l->sid].name.c_str(), l->path.front(), l->path.back(), l->built ? "" : " (laying)", RESET);
      for (size_t i = 0; i < l->segs.size(); ++i) { const Segment& sg = l->segs[i]; if (sg.a == id || sg.b == id) std::printf("  seg%zu %d-%d flow %s/%s loss %s%%%s", i, sg.a, sg.b, f1(sg.flow).c_str(), f0(sg.cap).c_str(), f1(sg.lastLoss * 100).c_str(), sg.sabotagedUntil > s.cycle ? " SABOTAGED" : ""); }
      std::printf("\n");
    }
    std::printf("  %sCYBER%s  firewall %s  your presence %s", BOLD, RESET, vis ? f0(h.fw).c_str() : "?", f0(h.P.count(me) ? h.P.at(me) : 0).c_str());
    if (vis && g.PresenceVisible(me, id)) { for (auto& kv : h.P) if (kv.first != me) std::printf("  %s%s P %s%s", col(kv.first).c_str(), kv.first == RogueSid ? "ROGUE AI" : s.synds[kv.first].name.c_str(), f0(kv.second).c_str(), RESET); }
    else if (vis && h.owner == me) { bool contested = false; for (auto& kv : h.P) if (kv.first != me && kv.second >= 40) contested = true; std::printf("  rival presence: %s (build an Array or Scan to read values)", contested ? "CONTESTED (+2 demand)" : "unknown"); }
    if (vis && !h.roots.empty()) { std::printf("  ROOTED by:"); for (int r : h.roots) std::printf(" %s", s.synds[r].name.c_str()); }
    if (vis && h.daemons.count(me)) std::printf("  your daemons %d", h.daemons.at(me));
    std::printf("\n");
  }

  void net() {
    const GameState& s = S(); const Syndicate& y = s.synds[me];
    std::printf("%sNETWORK%s\n", BOLD, RESET);
    for (auto& st : s.structs) if (st.alive && st.sid == me) {
      std::printf("  [%d] %-20s #%-3d %s", st.id, kindName(st.kind, st.tier), st.hex, st.built ? "" : ("building " + std::to_string(st.buildLeft) + "  ").c_str());
      if (st.kind == StructKind::Node && st.built) std::printf("power %s%%  out %s  util %s%%", f0(st.powerFrac * 100).c_str(), f1(st.out).c_str(), f0(st.util * 100).c_str());
      if (st.darkUntil > s.cycle) std::printf("  DARK");
      std::printf("\n");
    }
    for (auto& l : s.links) if (l.alive && l.sid == me) {
      double fl = 0; bool sab = false; for (auto& sg : l.segs) { fl = std::max(fl, sg.flow); if (sg.sabotagedUntil > s.cycle) sab = true; }
      std::printf("  link %d %-16s %d→%d  %zu segs  cap %s  peak flow %s%s%s\n", l.id, linkDef(l.type).name, l.path.front(), l.path.back(), l.segs.size(), f0(l.segs[0].cap).c_str(), f1(fl).c_str(), l.built ? "" : "  (laying)", sab ? "  SABOTAGED" : "");
    }
    std::printf("  Sectors:");
    for (auto& h : s.hexes) if (h.owner == me) std::printf(" #%d(C%s r%s)", h.id, f0(h.C).c_str(), f1(h.ratio.count(me) ? h.ratio.at(me) : 0).c_str());
    std::printf("\n  Peered exchanges: %zu   Bought BW pending: %s   Compute share %s%%\n", y.flow.peered.size(), f0(y.buyBW).c_str(), f0(y.computePct * 100).c_str());
  }

  void ops() {
    const Syndicate& y = S().synds[me];
    std::printf("%sRUNNING OPS%s\n", BOLD, RESET); for (auto& o : y.ops) std::printf("  [%d] %-10s #%d since %d\n", o.id, opDef(o.kind).name, o.target, o.since);
    std::printf("%sQUEUED THIS CYCLE%s\n", BOLD, RESET); for (auto& o : y.queued) std::printf("  [%d] %-10s #%d\n", o.id, opDef(o.kind).name, o.target);
    std::printf("%sOP KINDS%s (op <name> <hex> [aux])\n", BOLD, RESET);
    for (int i = 0; i < (int)OpKind::COUNT; ++i) { const OpDef& d = opDef((OpKind)i); if (d.archOnly && d.arch != y.arch) continue; std::printf("  %-14s bw %-3s cap %-3s X %-3s %s%s\n", d.name, f0(d.bw).c_str(), f0(d.capital).c_str(), f0(d.x).c_str(), d.sustained ? "[sustained] " : "", d.desc); }
  }

  void research(const std::string& arg) {
    const Syndicate& y = S().synds[me];
    if (!arg.empty()) { int i = std::atoi(arg.c_str()); if (i < 0 || i >= TechCount) { std::puts("Bad tech index."); return; } Result r = g.QueueResearch(me, (Tech)i); std::puts(r.msg.c_str()); return; }
    std::printf("%sRESEARCH%s  (research <index>; compute share: compute <pct>)  queue:", BOLD, RESET);
    for (Tech t : y.researchQueue) std::printf(" %s", techDef(t).name); std::printf("\n");
    for (int b = 0; b < 4; ++b) {
      std::printf("  %s%s%s\n", BOLD, branchName((Branch)b), RESET);
      for (int i = 0; i < TechCount; ++i) { const TechDef& d = techDef((Tech)i); if ((int)d.branch != b) continue; bool done = y.techs[i]; auto pr = y.researchProgress.find(i); Result can = g.CanResearch(me, (Tech)i);
        std::printf("   [%2d] T%d %-24s %5s %s%s  %s\n", i, d.tier, d.name, f0(d.cost * g.ModsOf(me).research[b]).c_str(), done ? "DONE " : can ? "avail" : "lock ", pr != y.researchProgress.end() ? (" (" + f0(pr->second) + ")").c_str() : "", d.desc); }
    }
  }

  void doctrine(const std::string& arg) {
    const Syndicate& y = S().synds[me];
    if (!arg.empty()) { int i = std::atoi(arg.c_str()); if (i < 0 || i >= DoctrineCount) { std::puts("Bad doctrine index."); return; } Result r = g.TakeDoctrine(me, (Doctrine)i); std::puts(r.msg.c_str()); return; }
    std::printf("%sDOCTRINE%s  Mandate available: %d  (doctrine <index>)  KPI: %s\n", BOLD, RESET, y.mandate, archDef(y.arch).kpi);
    for (int i = 0; i < DoctrineCount; ++i) { const DoctrineDef& d = doctrineDef((Doctrine)i); if (d.arch != y.arch) continue; std::printf("   [%2d] T%d %-26s %s  %s\n", i, d.tier, d.name, y.doctrines[i] ? "ADOPTED" : g.CanTakeDoctrine(me, (Doctrine)i) ? "avail  " : "locked ", d.desc); }
  }

  void board() {
    const GameState& s = S();
    std::printf("%sBOARD ROOM%s\n  Valuations (public):\n", BOLD, RESET);
    for (auto& o : s.synds) std::printf("    %s%-26s%s %-18s val %7s  share %5s%%  seats %d  exposure %3s%s%s\n", col(o.id).c_str(), o.name.c_str(), RESET, archDef(o.arch).name, f0(o.valuation).c_str(), f1(o.share * 100).c_str(), g.SeatsOf(o.id), f0(o.exposure).c_str(), o.hasPhase ? (std::string("  PHASE: ") + pathName(o.phase.type)).c_str() : "", o.shadowOf >= 0 ? "  (subsidiary)" : "");
    std::printf("  Your paths:\n");
    for (auto& p : g.Progress(me)) { std::printf("    %-17s %3s%%  %s%s\n", pathName(p.path), f0(p.proximity).c_str(), p.available ? "READY " : "", p.status.c_str()); for (auto& u : p.unmet) std::printf("        needs: %s\n", u.c_str()); }
    std::printf("  Districts:"); for (int i = 0; i < (int)s.districts.size(); ++i) { const District& d = s.districts[i]; std::printf(" D%d:%s%s%s", i + 1, col(d.seat).c_str(), d.seat >= 0 ? (d.bought ? "bought" : "held") : "open", RESET); } std::printf("\n");
    for (auto& c : s.cartels) { std::printf("  CARTEL against %s:", s.synds[c.target].name.c_str()); for (int m : c.members) std::printf(" %s", s.synds[m].name.c_str()); std::printf("\n"); }
    if (s.emergent.active) std::printf("  %sAN EMERGENT INTELLIGENCE IS LOOSE IN THE BACKBONE.%s\n", BOLD, RESET);
    std::puts("  Verbs: tender | shares <pct> | pill | complaint <sid> | greenmail <sid> | train | launch | killswitch <sid> | blackout | shadow <sid> | bribe <district> on|off | ethics <district> | motion");
  }

  void help() {
    std::puts("COMMANDS");
    std::puts("  end                      end the cycle (resolve)           auto <n>       run n cycles");
    std::puts("  map | log | help | quit  redraw / recent log");
    std::puts("  info <hex>               inspect a hex (both layers)       net            your network, sectors, power");
    std::puts("  plan <hex>               preview a fiber route + cost + projected survival from your nearest network hex");
    std::puts("  lay <hex> [trunk|backbone|dark|armored]   buy rights along the planned route and lay it");
    std::puts("  layfrom <from> <to> [type]                same, from a chosen network hex");
    std::puts("  uplink <a> <b>           microwave link (range 3)          rights <hex>   buy conduit rights only");
    std::puts("  build <kind> <hex> [tier] kinds: node repeater substation privategrid outpost array honeypot lab rack tap cutout fork");
    std::puts("       node variants: build node <hex> 2 phantom | build node <hex> 3 inference | build node <hex> 4 citadel");
    std::puts("  op <kind> <hex> [aux]    ops: scan intrusion harden purge siphon jam root ddos raid repair eminent sever covert daemon oracle retrain");
    std::puts("       raid aux: 0 sever 1 darken 2 seize-rights 3 crown.  repair: op repair <linkId> <segIndex>.  oracle: op oracle <sid>");
    std::puts("  ops | cancel <opId>      list / cancel operations          pri <hex> critical|normal|low");
    std::puts("  compute <pct>            share of node output into research (0..50, Hive 0..100)");
    std::puts("  research [idx] | doctrine [idx] | hire <cio|cso|hoi|cfo|fixer|counsel> | buybw <n> | deny <hex> on|off");
    std::puts("  seize <structId> | scuttle <structId>     orphaned structures");
    std::puts("  board                    valuations, victory paths, districts, cartels, victory verbs");
  }

  bool layTo(int from, int to, LinkType lt, bool preview) {
    PathPlan p = g.PlanPath(me, from, to, lt);
    if (!p.ok) { std::puts(p.msg.c_str()); return false; }
    std::printf("Route:"); for (int h : p.path) std::printf(" %d", h);
    std::printf("\n  %zu segments, link %s + rights %s (%zu hexes) = %s Capital. Projected survival at the far end: %s%%\n", p.path.size() - 1, f0(p.linkCost).c_str(), f0(p.rightsCost).c_str(), p.needRights.size(), f0(p.total).c_str(), f0(p.projectedSurvival * 100).c_str());
    if (preview) return true;
    for (int r : p.needRights) { Result rr = g.BuyRights(me, r); if (!rr) { std::printf("  rights #%d: %s\n", r, rr.msg.c_str()); return false; } }
    Result r = g.LayLink(me, lt, p.path); std::puts(r.msg.c_str()); return (bool)r;
  }
  int nearestNet(int to) {
    const GameState& s = S(); int best = -1, bd = 1 << 30;
    for (auto& st : s.structs) if (st.alive && st.sid == me) { int d = s.grid.dist(st.hex, to); if (d < bd) { bd = d; best = st.hex; } }
    for (auto& l : s.links) if (l.alive && l.sid == me) for (int h : l.path) if (s.hexes[h].type != Sector::Barrier) { int d = s.grid.dist(h, to); if (d < bd) { bd = d; best = h; } }
    return best;
  }
  static LinkType parseLink(const std::string& t) { if (t == "backbone") return LinkType::Backbone; if (t == "dark") return LinkType::Dark; if (t == "armored") return LinkType::Armored; return LinkType::Trunk; }
  static StructKind parseKind(const std::string& k) {
    static const char* names[] = { "node", "repeater", "substation", "privategrid", "outpost", "array", "honeypot", "lab", "rack", "tap", "cutout", "fork" };
    for (int i = 0; i < 12; ++i) if (k == names[i]) return (StructKind)i; return StructKind::COUNT;
  }
  static OpKind parseOp(const std::string& k) {
    static const char* names[] = { "scan", "intrusion", "harden", "purge", "siphon", "jam", "root", "ddos", "raid", "repair", "eminent", "sever", "covert", "daemon", "oracle", "retrain" };
    for (int i = 0; i < 16; ++i) if (k == names[i]) return (OpKind)i; return OpKind::COUNT;
  }

  void loop() {
    drawHeader(); drawMap(); help(); showLog(8);
    std::string line;
    while (true) {
      if (S().over) { const auto& v = S().victory; std::printf("\n%s%s=== GAME OVER (cycle %d): %s wins by %s ===%s\n", BOLD, col(v.winner).c_str(), v.cycle, S().synds[v.winner].name.c_str(), pathName(v.path), RESET); showLog(12); std::puts("Type quit to exit."); }
      std::printf("\n%s> %s", COL[me], RESET); std::fflush(stdout);
      if (!std::getline(std::cin, line)) break;
      std::istringstream is(line); std::vector<std::string> t; std::string w; while (is >> w) t.push_back(w);
      if (t.empty()) continue;
      auto arg = [&](size_t i, int def = -1) { return i < t.size() ? std::atoi(t[i].c_str()) : def; };
      auto sarg = [&](size_t i) { return i < t.size() ? t[i] : std::string(); };
      const std::string& c = t[0];
      Result r;
      if (c == "quit" || c == "exit") break;
      else if (c == "help") help();
      else if (c == "map") { drawHeader(); drawMap(); }
      else if (c == "log") showLog(arg(1, 25));
      else if (c == "end") { if (S().over) { std::puts("Game over."); continue; } g.EndCycle(); drawHeader(); drawMap(); showLog(14); }
      else if (c == "auto") { int n = std::max(1, arg(1, 1)); for (int i = 0; i < n && !S().over; ++i) g.EndCycle(); drawHeader(); drawMap(); showLog(14); }
      else if (c == "info") info(arg(1, sel));
      else if (c == "sel") { sel = arg(1); drawMap(); info(sel); }
      else if (c == "net") net();
      else if (c == "ops") ops();
      else if (c == "board") board();
      else if (c == "plan") { int to = arg(1); int from = nearestNet(to); if (from < 0) std::puts("No network."); else layTo(from, to, parseLink(sarg(2)), true); }
      else if (c == "lay") { int to = arg(1); int from = nearestNet(to); if (from < 0) std::puts("No network."); else layTo(from, to, parseLink(sarg(2)), false); }
      else if (c == "layfrom") layTo(arg(1), arg(2), parseLink(sarg(3)), false);
      else if (c == "uplink") { r = g.LayUplink(me, arg(1), arg(2)); std::puts(r.msg.c_str()); }
      else if (c == "rights") { r = g.BuyRights(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "build") { StructKind k = parseKind(sarg(1)); if (k == StructKind::COUNT) { std::puts("Unknown structure kind."); continue; } Variant v = Variant::None; std::string vs = sarg(3); if (vs == "phantom") v = Variant::Phantom; else if (vs == "inference") v = Variant::Inference; else if (vs == "citadel") v = Variant::Citadel; r = g.Build(me, k, arg(2), arg(3, 1), v); std::puts(r.msg.c_str()); }
      else if (c == "op") { OpKind k = parseOp(sarg(1)); if (k == OpKind::COUNT) { std::puts("Unknown op."); continue; } r = g.QueueOp(me, k, arg(2), arg(3, -1)); std::puts(r.msg.c_str()); }
      else if (c == "cancel") { r = g.CancelOp(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "pri") { std::string p = sarg(2); r = g.SetPriority(me, arg(1), p == "critical" ? Priority::Critical : p == "low" ? Priority::Low : Priority::Normal); std::puts(r.msg.c_str()); }
      else if (c == "compute") { r = g.SetCompute(me, arg(1, 20) / 100.0); std::puts(r.msg.c_str()); }
      else if (c == "research") research(sarg(1));
      else if (c == "doctrine") doctrine(sarg(1));
      else if (c == "hire") { std::string sp = sarg(1); ExecSpec e = sp == "cso" ? ExecSpec::CSO : sp == "hoi" ? ExecSpec::HOI : sp == "cfo" ? ExecSpec::CFO : sp == "fixer" ? ExecSpec::Fixer : sp == "counsel" ? ExecSpec::Counsel : ExecSpec::CIO; r = g.HireExec(me, e); std::puts(r.msg.c_str()); }
      else if (c == "buybw") { r = g.BuyBandwidth(me, arg(1, 10)); std::puts(r.msg.c_str()); }
      else if (c == "deny") { r = g.SetRowDenial(me, arg(1), sarg(2) != "off"); std::puts(r.msg.c_str()); }
      else if (c == "seize") { r = g.Seize(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "scuttle") { r = g.Scuttle(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "tender") { r = g.TenderOffer(me); std::puts(r.msg.c_str()); }
      else if (c == "shares") { r = g.BuyShares(me, arg(1, 1)); std::puts(r.msg.c_str()); }
      else if (c == "pill") { r = g.PoisonPill(me); std::puts(r.msg.c_str()); }
      else if (c == "complaint") { r = g.Complaint(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "greenmail") { r = g.Greenmail(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "train") { r = g.StartTraining(me); std::puts(r.msg.c_str()); }
      else if (c == "launch") { r = g.Launch(me); std::puts(r.msg.c_str()); }
      else if (c == "killswitch") { r = g.PetitionKillSwitch(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "blackout") { r = g.DeclareBlackout(me); std::puts(r.msg.c_str()); }
      else if (c == "shadow") { r = g.ShadowDirector(me, arg(1)); std::puts(r.msg.c_str()); }
      else if (c == "bribe") { r = g.SetBribe(me, arg(1) - 1, sarg(2) != "off"); std::puts(r.msg.c_str()); }
      else if (c == "ethics") { r = g.EthicsComplaint(me, arg(1) - 1); std::puts(r.msg.c_str()); }
      else if (c == "motion") { r = g.ConsolidationMotion(me); std::puts(r.msg.c_str()); }
      else std::puts("Unknown command. Type help.");
    }
  }
};

int main(int argc, char** argv) {
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); DWORD mode = 0; if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
  Config cfg;
  std::printf("%sGRIDLOCK: SILICON SYNDICATE%s\n\n", BOLD, RESET);
  if (argc >= 2 && std::string(argv[1]) == "--headless") {
    cfg.playerIsAI = true; cfg.seed = argc >= 3 ? argv[2] : "bench"; int n = argc >= 4 ? std::atoi(argv[3]) : 200;
    Game g(cfg); for (int i = 0; i < n && !g.S().over; ++i) g.EndCycle();
    const GameState& s = g.S();
    for (auto& y : s.synds) { int sec = 0; for (auto& hx : s.hexes) if (hx.owner == y.id) ++sec; std::printf("%-26s %-18s sectors %2d capital %7.0f share %5.1f%%\n", y.name.c_str(), archDef(y.arch).name, sec, y.capital, y.share * 100); }
    if (s.over) std::printf("Winner: %s by %s at cycle %d\n", s.synds[s.victory.winner].name.c_str(), pathName(s.victory.path), s.victory.cycle);
    for (auto& l : g.RecentLog(-1, 15)) std::printf("  %s\n", l.c_str());
    if (argc >= 5) { int sid = std::atoi(argv[4]); std::printf("--- log of %s ---\n", s.synds[sid].name.c_str()); for (auto& l : g.RecentLog(sid, 80)) std::printf("  %s\n", l.c_str()); }
    return 0;
  }
  std::puts("Choose your syndicate:");
  for (int i = 0; i < 3; ++i) std::printf("  %d) %-18s \"%s\"\n", i + 1, archDef((Arch)i).name, archDef((Arch)i).tag);
  std::printf("> "); std::string in; std::getline(std::cin, in); int pick = std::atoi(in.c_str()); if (pick < 1 || pick > 3) pick = 1; cfg.player = (Arch)(pick - 1);
  std::printf("Seed (blank = gridlock): "); std::getline(std::cin, in); if (!in.empty()) cfg.seed = in;
  Cli cli(cfg);
  cli.loop();
  return 0;
}
