// Internal helpers shared by the sim sources. Not part of the public API.
#pragma once
#include "gridlock/Game.h"
#include "gridlock/Rng.h"
#include <set>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace gl { namespace sim {

// RNG scope: pulls the stream out of the state and writes it back on exit so every roll is recorded.
struct RngScope {
  GameState& S; Rng r;
  explicit RngScope(GameState& s) : S(s), r(s.rngState) {}
  ~RngScope() { S.rngState = r.state(); }
};

// ---- state access
inline Syndicate& synd(GameState& S, int sid) { GL_CHECK(sid >= 0 && sid < (int)S.synds.size(), "sid range"); return S.synds[sid]; }
inline const Syndicate& synd(const GameState& S, int sid) { GL_CHECK(sid >= 0 && sid < (int)S.synds.size(), "sid range"); return S.synds[sid]; }
inline Hex& hex(GameState& S, int id) { GL_CHECK(id >= 0 && id < (int)S.hexes.size(), "hex range"); return S.hexes[id]; }
inline const Hex& hex(const GameState& S, int id) { GL_CHECK(id >= 0 && id < (int)S.hexes.size(), "hex range"); return S.hexes[id]; }
inline bool has(const Syndicate& s, Tech t) { return s.techs[(int)t]; }
inline bool hasDoc(const Syndicate& s, Doctrine d) { return s.doctrines[(int)d]; }

Structure* findStruct(GameState& S, int id);
const Structure* findStruct(const GameState& S, int id);
const Structure* structIn(const GameState& S, int hexId, int sid, StructKind kind, bool builtOnly = true);
Structure* structIn(GameState& S, int hexId, int sid, StructKind kind, bool builtOnly = true);
std::vector<Structure*> structsOf(GameState& S, int sid, StructKind kind = StructKind::COUNT);
std::vector<const Structure*> structsOf(const GameState& S, int sid, StructKind kind = StructKind::COUNT);
std::vector<const Structure*> structsIn(const GameState& S, int hexId);
Link* findLink(GameState& S, int id);
bool anyStructIn(const GameState& S, int hexId, int sid);
std::vector<int> ownedHexes(const GameState& S, int sid);
int totalSectors(const GameState& S);
std::set<int> networkHexes(const GameState& S, int sid);
void logMsg(GameState& S, const std::string& text, int sid = -1);

Structure& addStruct(GameState& S, Syndicate& s, int hexId, StructKind kind, int tier, Variant v, bool built, bool crown = false);
Link& addLink(GameState& S, Syndicate& s, LinkType type, const std::vector<int>& path, bool built);
void createGame(GameState& S, const Config& cfg);

// ---- derived numbers
Mods modsOf(const GameState& S, const Syndicate& s);
double nodeRaw(const GameState& S, const Syndicate& s, const Structure& st);
double nodeMW(const GameState& S, const Syndicate& s, const Structure& st);
double nodeFwBase(const Structure& st);
double sectorDemand(const GameState& S, const Hex& h, int sid);
double fwBaseFor(const GameState& S, const Hex& h);
double maxRivalP(const Hex& h, int sid);
bool canReach(const GameState& S, const Syndicate& s, const Hex& h);
bool backboneReach(const GameState& S, const Syndicate& s, const Hex& h);
double buildCost(const GameState& S, const Syndicate& s, StructKind kind, int tier, Variant v);
double linkCostPerHex(const GameState& S, const Syndicate& s, LinkType t);
double conduitCost(const GameState& S, const Hex& to, const Hex* from);
double totalUpkeep(const GameState& S, const Syndicate& s);
std::vector<int> aliveRivals(const GameState& S, int sid);
bool inCartelWith(const GameState& S, int a, int b);
const Cartel* cartelAgainst(const GameState& S, int target);
bool isSubsidiary(const GameState& S, int sid);

// ---- flow (Flow.cpp)
void solveFlow(GameState& S, Syndicate& s);
bool twoDisjointPaths(const GameState& S, int sid, int from, const std::set<int>& targets);

// ---- vision (Vision.cpp)
bool canSee(const GameState& S, int viewer, int hexId);
bool assetVisible(const GameState& S, int viewer, const Structure& st);
bool linkVisible(const GameState& S, int viewer, const Link& l);
bool presenceVisible(const GameState& S, int viewer, int hexId);
void snapshotVision(GameState& S, int viewer);

// ---- resolution (Resolve.cpp)
void resolveCycle(GameState& S);
void validateInvariants(const GameState& S, const char* stage);
void addExposure(GameState& S, Syndicate& s, double amount, bool attack, int victim = -1);
void computeValuations(GameState& S);

// ---- victory (Victory.cpp)
void victoryTick(GameState& S);
std::vector<PathProgress> victoryProgress(const GameState& S, int sid);
Result vTenderOffer(GameState& S, int sid);
Result vBuyShares(GameState& S, int sid, int pct);
Result vPoisonPill(GameState& S, int sid);
Result vComplaint(GameState& S, int sid, int target);
Result vGreenmail(GameState& S, int sid, int target);
Result vStartTraining(GameState& S, int sid);
Result vLaunch(GameState& S, int sid);
Result vKillSwitch(GameState& S, int sid, int target);
Result vDeclareBlackout(GameState& S, int sid);
Result vShadowDirector(GameState& S, int sid, int target);
Result vSetBribe(GameState& S, int sid, int district, bool on);
Result vEthicsComplaint(GameState& S, int sid, int district);
Result vConsolidationMotion(GameState& S, int sid);
bool seedClusterOk(const GameState& S, const Syndicate& s, std::vector<int>* seedNodes);
int seatsOf(const GameState& S, int sid);

// ---- AI (AI.cpp)
void aiPlan(Game& g, int sid);

inline std::string fmt(double v, int dec = 1) { char b[64]; std::snprintf(b, sizeof b, "%.*f", dec, v); return b; }

} } // namespace gl::sim
