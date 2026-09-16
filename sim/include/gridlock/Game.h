// Public API of the Gridlock simulation. The console client and the UE5 module both drive the game through this class only.
#pragma once
#include "State.h"
#include "Data.h"
#include <string>
#include <vector>

namespace gl {

struct Result {
  bool ok = true; std::string msg;
  static Result Ok(std::string m = "") { return { true, std::move(m) }; }
  static Result Err(std::string m) { return { false, std::move(m) }; }
  explicit operator bool() const { return ok; }
};

struct Config {
  std::string seed = "gridlock";
  Arch player = Arch::Hegemony;
  std::vector<Arch> ais = { Arch::Ghost, Arch::Hive, Arch::Hegemony };
  int W = 24, H = 18;
  bool playerIsAI = false;          // for headless balance runs
};

struct PathPlan {
  bool ok = false; std::string msg;
  std::vector<int> path; std::vector<int> needRights;
  double rightsCost = 0, linkCost = 0, total = 0;
  double projectedSurvival = 1.0;   // product of (1 − base loss) along the path from `from`
};

struct PathProgress { VictoryPath path; double proximity = 0; bool available = false; std::vector<std::string> unmet; std::string status; };

struct Mods {
  double nodeOut = 1, buildCost = 1, linkCost = 1, trunkCap = 20, subRange = 2, fwAdd = 0, xMult = 1, xRaid = 1, xBuild = 0;
  double intrusionCost = 1, intrusionX = 1, intrusionGain = 0, pDecay = 5, yieldMult = 1; int maxTier = 3; bool hidden = false;
  double opsCost = 1, powerMult = 1, computeCap = 0.5; int researchSlots = 1; double research[4] = { 1, 1, 1, 1 };
  double raidBase = 0.6; int raidRange = 2; double raidBonus = 0, upkeepMult = 1, nodeUpkeep = 1, sellMult = 1, sellFloor = K::SellFloor;
  double gridCap = K::GridCap, bufferCap = K::BufferCap, xCool = K::ExposureCool, skim = 0.2; int hopReset = 0;
  double uplinkLoss = 0.10, uplinkCap = 8, jam = 0.10, rootCost = 12, purgeCost = 6, bureauAdd = 0, attribution = 0;
  int daemonCap = 0; double copyChance = 0.3, adaptive = 0; int buildSpeed = 0; int slots = K::BaseSlots;
  bool severImmune = false; double takeoverTrigger = 0.40, takeoverWin = 0.51, buyShareCost = 100, mGain = 1, coherenceMult = 1;
  double raidAgainst = 0, scanEvasion = 0; bool dpi = false;
};

class Game {
public:
  explicit Game(const Config& cfg);
  GameState& S() { return st_; }
  const GameState& S() const { return st_; }
  const Config& Cfg() const { return cfg_; }

  // ---- planning-phase orders (validated fully before any mutation)
  Result BuyRights(int sid, int hex);
  PathPlan PlanPath(int sid, int from, int to, LinkType type) const;
  Result LayLink(int sid, LinkType type, const std::vector<int>& path);
  Result LayUplink(int sid, int a, int b);
  Result Build(int sid, StructKind kind, int hex, int tier = 1, Variant variant = Variant::None);
  Result QueueOp(int sid, OpKind kind, int target, int aux = -1);
  Result CancelOp(int sid, int opId);
  Result SetPriority(int sid, int hex, Priority p);
  Result SetCompute(int sid, double pct);
  Result QueueResearch(int sid, Tech t);
  Result ClearResearch(int sid);
  Result TakeDoctrine(int sid, Doctrine d);
  Result HireExec(int sid, ExecSpec spec);
  Result BuyBandwidth(int sid, double amount);
  Result SetRowDenial(int sid, int hex, bool on);
  Result Seize(int sid, int structId);
  Result Scuttle(int sid, int structId);
  // ---- victory verbs (docs/03)
  Result TenderOffer(int sid);
  Result BuyShares(int sid, int pct);
  Result PoisonPill(int sid);
  Result Complaint(int sid, int target);
  Result Greenmail(int sid, int target);
  Result StartTraining(int sid);
  Result Launch(int sid);
  Result PetitionKillSwitch(int sid, int target);
  Result DeclareBlackout(int sid);
  Result ShadowDirector(int sid, int target);
  Result SetBribe(int sid, int district, bool on);
  Result EthicsComplaint(int sid, int district);
  Result ConsolidationMotion(int sid);

  // ---- turn
  void EndCycle();                 // AI plans, then the 10-step resolution (docs/00 §5)
  void RunAIForAll();              // used by headless mode

  // ---- queries (all respect fog when a viewer is given)
  bool CanSee(int viewer, int hex) const;
  bool AssetVisible(int viewer, const Structure& s) const;
  bool LinkVisible(int viewer, const Link& l) const;
  bool PresenceVisible(int viewer, int hex) const;
  Mods ModsOf(int sid) const;
  int Slots(int sid) const;
  double UpkeepOf(int sid) const;
  double SectorDemand(int hex, int sid) const;
  double RaidChance(int sid, int hex) const;
  Result CanResearch(int sid, Tech t) const;
  Result CanTakeDoctrine(int sid, Doctrine d) const;
  std::vector<PathProgress> Progress(int sid) const;
  std::vector<std::string> RecentLog(int viewer, int max = 30) const;
  const Structure* StructAt(int hex, int sid, StructKind kind) const;
  std::vector<const Structure*> StructsIn(int hex) const;
  std::vector<const Link*> LinksThrough(int hex) const;
  double ValuationShare(int sid) const { return st_.synds[sid].share; }
  int SeatsOf(int sid) const;
  std::string StateHash() const;   // deterministic digest for desync/regression tests

private:
  GameState st_; Config cfg_;
};

} // namespace gl
