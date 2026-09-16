// Full game state. Plain data; all logic lives in the sim sources.
#pragma once
#include "Types.h"
#include "Hex.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <array>

namespace gl {

struct Structure {
  int id = 0; StructKind kind = StructKind::Node; int sid = -1; int hex = -1; int tier = 0; Variant variant = Variant::None;
  bool alive = true; bool built = false; int buildLeft = 0; bool crown = false;
  bool dark = false; int darkUntil = 0; int orphanedAt = -1;
  double powerFrac = 1.0; double out = 0.0; double util = 0.0; int ddosedUntil = -1;
  bool revealed = false;                          // Ghost hidden asset discovered
};

struct Segment {
  int a = -1, b = -1; double cap = 0; double flow = 0, flow1 = 0; int sabotagedUntil = 0; int dist = 1; bool wireless = false;
  double lastLoss = 0; int lastDir = 0;           // rendering: +1 a→b, −1 b→a
};

struct Link {
  int id = 0; int sid = -1; LinkType type = LinkType::Trunk; std::vector<int> path; bool wireless = false;
  bool alive = true; bool built = false; int readyAt = 0; std::vector<Segment> segs;
};

struct HexSeen { int cycle = -1; int owner = -1; double C = 0; double fw = 0; std::vector<std::pair<StructKind, int>> structs; };

struct Hex {
  int id = 0; int q = 0, r = 0; Sector type = Sector::Sprawl; int district = 0;
  int owner = -1; double C = 0; double fw = 0;
  std::map<int, double> P;                        // sid → presence 0..100
  std::set<int> rights;                           // sids with conduit rights
  std::map<int, int> scans;                       // sid → last scan cycle
  std::set<int> revealedTo;                       // viewers for whom Ghost assets here are revealed
  std::set<int> roots;                            // sids holding root
  std::map<int, int> jamUntil;                    // sid → cycle
  std::map<int, double> delivered, demand, ratio; // per sid this cycle
  std::map<int, Priority> priority;
  std::set<int> dual;                             // sids with two disjoint paths here
  std::map<int, HexSeen> seen;                    // viewer → stale snapshot
  std::set<int> dormant; std::map<int, int> dormantStreak;
  bool rowDenial = false;                         // owner denies right of way to all rivals
  bool brownout = false; int repelled = 0;
  std::map<int, int> daemons;                     // sid → count
};

struct Op { int id = 0; OpKind kind = OpKind::Scan; int sid = -1; int target = -1; int aux = -1; int since = 0; bool sustained = false; };
struct Exec { ExecSpec spec = ExecSpec::CIO; int hired = 0; };

struct FlowResult {
  double produced = 0, delivered = 0, lost = 0, opsPool = 0, opsNeed = 0, compute = 0, sold = 0, bufferAdd = 0, stranded = 0, bought = 0;
  std::vector<int> peered;                        // exchange hex ids peered this cycle
  std::map<int, double> computeByNode;
};

struct Phase {
  VictoryPath type = VictoryPath::Takeover; int start = 0; int cyclesLeft = 0; bool paused = false;
  double coherence = 0;                           // singularity
  int shadowCount = 0;                            // ghost coup
};

struct Syndicate {
  int id = -1; std::string name; Arch arch = Arch::Hegemony; bool isAI = false; bool alive = true;
  double capital = 0, exposure = 0; int crown = -1;
  std::array<bool, (int)Tech::COUNT> techs{}; std::array<bool, (int)Doctrine::COUNT> doctrines{};
  std::vector<Tech> researchQueue; std::map<int, double> researchProgress; // tech → compute
  double computePct = 0.2;
  std::vector<Exec> execs;
  std::vector<Op> ops;                            // sustained (running) ops
  std::vector<Op> queued;                         // instant ops to resolve this cycle
  int slotsUsed = 0;
  double M = 0; int lastEmergent = -1;
  double buffer = 0; int mandate = 0; int sectorsAtReview = 0; bool revealedThisPeriod = false;
  int auditUntil = 0; double frozen = 0; bool noticed = false; int insolventStreak = 0; bool gridCut = false;
  double buyBW = 0; bool autoGrid = true;
  FlowResult flow; std::map<int, double> tapIncome; int decapUntil = -1;
  double valuation = 0, share = 0, shareAdj = 0; int poisonUntil = 0; int abstainUntil = 0;
  bool hasPhase = false; Phase phase; std::array<int, (int)VictoryPath::COUNT> cooldownUntil{};
  int shadowOf = -1;                              // subsidiary of sid
  int training = 0;                               // singularity training cycles achieved
  std::vector<Op> planned;                        // AI intent for next cycle (Oracle target)
  int wetworkUntil = 0; int zeroDay = 0;
  std::map<int, int> peeredLast;                  // exchange → last peered cycle
  int sectorsTaken = 0;
  std::set<int> everRooted;                       // hexes rooted at some point (Backdoor Firmware)
  bool killSwitchUsed = false;
  // AI memory
  std::array<std::array<double, (int)VictoryPath::COUNT>, 8> vp{};   // vp[rival][path]
  std::array<Posture, 8> posture{};
};

struct District { int seat = -1; bool bought = false; std::map<int, int> streak, below; std::map<int, double> bribes; };
struct Cartel { int target = -1; std::vector<int> members; int since = 0; };
struct LogEntry { int cycle = 0; int sid = -1; std::string text; };
struct Emergent { bool active = false; std::vector<int> hexes; int start = 0; int seedCount = 0; };
constexpr int RogueSid = -2;                       // presence key used by the Emergent Intelligence
struct VictoryResult { int winner = -1; VictoryPath path = VictoryPath::Valuation; int cycle = 0; };
struct Market { double price = K::SellPrice; double soldThisCycle = 0; };

struct GameState {
  uint32_t seed = 1; std::string seedLabel; int cycle = 0; uint32_t rngState = 1;
  Grid grid; std::vector<Hex> hexes; std::vector<int> exchanges, starts; int districtCount = 9;
  std::vector<Structure> structs; std::vector<Link> links; int nextId = 1;
  std::vector<Syndicate> synds; int playerSid = 0;
  Market market; std::vector<District> districts; std::vector<Cartel> cartels;
  bool over = false; VictoryResult victory; std::vector<LogEntry> log; Emergent emergent;
};

} // namespace gl
