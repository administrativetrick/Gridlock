#include "gridlock/Data.h"

namespace gl {

static const SectorDef kSectors[] = {
  { "Financial District", 40, 8,  12, 2, 45, 'F' },
  { "Corporate Campus",   30, 6,  10, 3, 40, 'C' },
  { "Port / Logistics",   25, 5,  8,  2, 30, 'P' },
  { "Industrial",         20, 4,  6,  2, 25, 'I' },
  { "Arcology",           15, 6,  5,  8, 30, 'A' },
  { "Sprawl",             8,  2,  3,  4, 20, 's' },
  { "Undercity",          5,  1,  2,  3, 10, 'u' },
  { "Exchange",           0,  10, 0,  0, 60, 'X' },
  { "Barrier",            0,  0,  15, 0, 0,  '~' },
};
const SectorDef& sectorDef(Sector s) { GL_CHECK((int)s < (int)Sector::COUNT, "sector enum"); return kSectors[(int)s]; }

static const NodeDef kNodes[] = {
  { "Edge Node",  12,  2,  120,  2, 6,  20 },
  { "Core Node",  30,  6,  350,  3, 15, 35 },
  { "Hyperscale", 75,  15, 900,  5, 35, 50 },
  { "Citadel",    150, 30, 1800, 7, 60, 60 },
};
const NodeDef& nodeDef(int tier) { GL_CHECK(tier >= 1 && tier <= 4, "node tier 1..4"); return kNodes[tier - 1]; }

static const LinkDef kLinks[] = {
  { "Trunk Fiber",      20, 25,  1, false, false, false, Tech::Trenching, false, Arch::Hegemony, false },
  { "Backbone Fiber",   60, 70,  3, false, false, false, Tech::BackboneFiber, true, Arch::Hegemony, false },
  { "Dark Fiber",       40, 40,  1, false, true,  false, Tech::Trenching, false, Arch::Hegemony, false },
  { "Microwave Uplink", 8,  90,  4, true,  false, false, Tech::Trenching, false, Arch::Hegemony, false },
  { "Armored Conduit",  60, 110, 4, false, false, true,  Tech::Trenching, false, Arch::Hegemony, true },
};
const LinkDef& linkDef(LinkType t) { GL_CHECK((int)t < (int)LinkType::COUNT, "link enum"); return kLinks[(int)t]; }

static const StructDef kStructs[] = {
  //  name                  cost  upk  bld  mw  rng  node   exch   indus  arch            archOnly tech               hasTech needsP
  { "Node",                 0,    0,   0,   0,  0,   false, false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Repeater",             80,   3,   1,   0,  0,   false, false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Substation",           200,  8,   2,   10, 2,   false, false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Private Grid",         380,  14,  3,   20, 3,   false, false, true,  Arch::Hegemony, true,  Tech::Trenching,    false, 0 },
  { "Security Outpost",     150,  6,   2,   0,  0,   false, false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Surveillance Array",   90,   2,   1,   0,  0,   false, false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Honeypot",             60,   1,   1,   0,  0,   false, false, false, Arch::Hegemony, false, Tech::Honeypots,    true,  0 },
  { "Lab",                  250,  10,  2,   0,  0,   true,  false, false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Peering Rack",         300,  0,   1,   0,  0,   false, true,  false, Arch::Hegemony, false, Tech::Trenching,    false, 0 },
  { "Tap",                  70,   2,   1,   0,  0,   false, false, false, Arch::Ghost,    true,  Tech::Trenching,    false, 40 },
  { "Cutout",               120,  3,   1,   0,  0,   false, false, false, Arch::Ghost,    true,  Tech::Trenching,    false, 0 },
  { "Model Fork",           400,  12,  2,   0,  0,   true,  false, false, Arch::Hive,     true,  Tech::Trenching,    false, 0 },
};
const StructDef& structDef(StructKind k) { GL_CHECK((int)k < (int)StructKind::COUNT, "struct enum"); return kStructs[(int)k]; }

static const OpDef kOps[] = {
  //  name              bw  cap  x   sust   own    P   backbone arch            archOnly desc
  { "Scan",             2,  0,   0,  false, false, 0,  false, Arch::Hegemony, false, "Reveal the subnet: owner, firewall, structures, presence (3 cycles)." },
  { "Intrusion",        4,  0,   1,  true,  false, 0,  false, Arch::Hegemony, false, "Gain presence each cycle: max(3, 15 - F/10)." },
  { "Harden",           2,  0,   0,  true,  true,  0,  false, Arch::Hegemony, false, "Firewall +5 per cycle (own subnet)." },
  { "Purge",            6,  0,   0,  false, true,  0,  false, Arch::Hegemony, false, "All rival presence -30; clears roots and taps (own subnet)." },
  { "Siphon",           3,  0,   2,  true,  false, 40, false, Arch::Hegemony, false, "Skim 20% of the sector yield (Compute from Arcologies)." },
  { "Jam",              5,  0,   3,  true,  false, 40, false, Arch::Hegemony, false, "+10% loss on every rival link through this hex." },
  { "Root",             12, 0,   6,  false, false, 70, false, Arch::Hegemony, false, "Become co-operator: F->0, owner demand +4, claim on brownout." },
  { "DDoS",             8,  0,   8,  false, false, 0,  true,  Arch::Hegemony, false, "Target node output -50% next cycle (needs Backbone reach)." },
  { "Raid",             0,  60,  10, false, false, 0,  false, Arch::Hegemony, false, "Sever a link, darken a substation, or seize conduit rights (Outpost within range)." },
  { "Repair",           0,  30,  0,  false, false, 0,  false, Arch::Hegemony, false, "Clear sabotage on a link segment (target = link id, aux = segment)." },
  { "Eminent Domain",   0,  0,   4,  false, false, 0,  false, Arch::Hegemony, true,  "Buy conduit rights in a rival hex (C<70) at 2x cost." },
  { "Sever",            0,  0,   6,  false, false, 0,  false, Arch::Hegemony, true,  "Cut a rival link through a hex you hold at C>=75 with an Outpost." },
  { "Covert Conduit",   0,  0,   0,  false, false, 60, false, Arch::Ghost,    true,  "Buy hidden conduit rights in a rival hex at 1.5x cost." },
  { "Swarm Daemon",     2,  0,   0,  false, false, 10, false, Arch::Hive,     true,  "Autonomous intrusion: +5 P/cycle, may copy to neighbours. 1 BW/cycle upkeep." },
  { "Oracle",           10, 0,   0,  false, false, 0,  false, Arch::Hive,     true,  "Reveal M% of a rival's planned orders (target = syndicate id)." },
  { "Retrain",          0,  0,   0,  false, false, 0,  false, Arch::Hive,     true,  "Spend 30 Compute: purge all daemons, +8 M, clear emergent behaviour." },
};
const OpDef& opDef(OpKind k) { GL_CHECK((int)k < (int)OpKind::COUNT, "op enum"); return kOps[(int)k]; }

static const TechDef kTechs[] = {
  { Tech::Trenching,           Branch::Infrastructure, 1, 80,  "Trenching Automation",   "Link cost -25%." },
  { Tech::RedundantPeering,    Branch::Infrastructure, 1, 100, "Redundant Peering",      "Dual-path sectors ignore congestion on their primary path." },
  { Tech::GridContracts,       Branch::Infrastructure, 1, 60,  "Grid Contracts",         "Grid power cap 12 -> 20 MW." },
  { Tech::BackboneFiber,       Branch::Infrastructure, 2, 250, "Backbone Fiber",         "Unlock Backbone Fiber (capacity 60)." },
  { Tech::ModularDC,           Branch::Infrastructure, 2, 300, "Modular Datacenters",    "Unlock Tier 3 Hyperscale nodes." },
  { Tech::MicrowaveMesh,       Branch::Infrastructure, 2, 200, "Microwave Mesh",         "Uplink loss 10% -> 6%, capacity 8 -> 12." },
  { Tech::Superconducting,     Branch::Infrastructure, 3, 600, "Superconducting Trunk",  "Backbone base loss becomes 1% + 0.5% per hop." },
  { Tech::HyperscaleCooling,   Branch::Infrastructure, 3, 550, "Hyperscale Cooling",     "Tier 3 nodes +25% output." },
  { Tech::FortifiedConduit,    Branch::Infrastructure, 3, 500, "Fortified Conduit",      "All links immune to Raid severs." },
  { Tech::DPI,                 Branch::Netwar, 1, 80,  "Deep Packet Inspection", "Scans reveal queued ops and hidden Ghost assets." },
  { Tech::HardenedKernels,     Branch::Netwar, 1, 100, "Hardened Kernels",       "Firewall base +10 on all owned subnets." },
  { Tech::Persistence,         Branch::Netwar, 1, 90,  "Persistence",            "Unsustained presence decays 5 -> 2 per cycle." },
  { Tech::ZeroDay,             Branch::Netwar, 2, 300, "Zero-Day Cache",         "Intrusion gain +50%." },
  { Tech::Honeypots,           Branch::Netwar, 2, 220, "Honeypots",              "Unlock the Honeypot structure." },
  { Tech::TrafficShaping,      Branch::Netwar, 2, 250, "Traffic Shaping",        "Jam 10% -> 20% loss." },
  { Tech::KillChain,           Branch::Netwar, 3, 700, "Kill Chain",             "Root cost 12 -> 8; Crown Nodes become rootable." },
  { Tech::AdaptiveFW,          Branch::Netwar, 3, 600, "Adaptive Firewalls",     "Each repelled intrusion adds +2 F permanently." },
  { Tech::BackboneSniffing,    Branch::Netwar, 3, 550, "Backbone Sniffing",      "See compute and BW volumes of everyone peered at your Exchanges." },
  { Tech::ShellCompanies,      Branch::Corporate, 1, 80,  "Shell Companies",        "Exposure cooling 3 -> 5 per cycle." },
  { Tech::FuturesDesk,         Branch::Corporate, 1, 90,  "Futures Desk",           "Exchange sell price +25%." },
  { Tech::Lobbying,            Branch::Corporate, 1, 100, "Lobbying Office",        "Unlock District seat bribery." },
  { Tech::VerticalIntegration, Branch::Corporate, 2, 250, "Vertical Integration",   "Node upkeep -20%." },
  { Tech::DataBrokerage,       Branch::Corporate, 2, 280, "Data Brokerage",         "Skims 20% -> 30%; Arcology yield +5." },
  { Tech::Buyback,             Branch::Corporate, 2, 300, "Buyback Program",        "Share purchases -30% cost." },
  { Tech::TenderOffer,         Branch::Corporate, 3, 700, "Tender Offer Authority", "Unlock the Hostile Takeover trigger." },
  { Tech::ConsolidationLobby,  Branch::Corporate, 3, 650, "Consolidation Lobby",    "Unlock the Charter vote." },
  { Tech::SovereignWealth,     Branch::Corporate, 3, 500, "Sovereign Wealth",       "2% interest per cycle on Capital above 1000." },
  { Tech::FieldTeams,          Branch::BlackOps, 1, 100, "Field Teams",          "Unlock Raid." },
  { Tech::CounterIntel,        Branch::BlackOps, 1, 80,  "Counter-Intel",        "You are told who Scans you." },
  { Tech::DeadDrops,           Branch::BlackOps, 1, 60,  "Dead Drops",           "Buffer cap 50 -> 100." },
  { Tech::SabotageDoctrine,    Branch::BlackOps, 2, 280, "Sabotage Doctrine",    "Raid +20% success; severs last 4 cycles." },
  { Tech::FalseFlags,          Branch::BlackOps, 2, 300, "False Flags",          "30% of your attack ops are attributed to a random rival." },
  { Tech::ExecProtection,      Branch::BlackOps, 2, 200, "Executive Protection", "Your Executives cannot be poached." },
  { Tech::Decapitation,        Branch::BlackOps, 3, 700, "Decapitation",         "Raid may target a Crown Node: -50% output 3 cycles." },
  { Tech::BlackoutProtocol,    Branch::BlackOps, 3, 800, "Blackout Protocol",    "Unlock the Blackout victory." },
  { Tech::Wetwork,             Branch::BlackOps, 3, 600, "Wetwork",              "Remove a rival Executive (-1 slot for 10 cycles). Exposure +25." },
};
const TechDef& techDef(Tech t) { GL_CHECK((int)t < (int)Tech::COUNT, "tech enum"); GL_CHECK(kTechs[(int)t].id == t, "tech table order"); return kTechs[(int)t]; }

static const DoctrineDef kDoctrines[] = {
  { Doctrine::H_Vertical,      Arch::Hegemony, 1, "Vertical Integration", "Node upkeep -25%." },
  { Doctrine::H_Eminent,       Arch::Hegemony, 1, "Eminent Domain",       "Unlock the Eminent Domain op." },
  { Doctrine::H_CompanyTown,   Arch::Hegemony, 1, "Company Town",         "Sprawl/Arcology demand -2 next to your Substations." },
  { Doctrine::H_Rings,         Arch::Hegemony, 2, "Redundant Rings",      "Dual-path sectors: all loss -50%." },
  { Doctrine::H_Kinetic,       Arch::Hegemony, 2, "Kinetic Doctrine",     "Raid range 3, base 70%. Raid exposure x1.5." },
  { Doctrine::H_PeeringCartel, Arch::Hegemony, 2, "Peering Cartel",       "Sell floor 1.5; rivals at your Exchanges pay +1/BW. Notice at X 30." },
  { Doctrine::H_RightOfWay,    Arch::Hegemony, 3, "Right of Way",         "Unlock Sever." },
  { Doctrine::H_Fortress,      Arch::Hegemony, 3, "Fortress City",        "F +30 in sectors with a Substation or Outpost; yield -15% there." },
  { Doctrine::H_Monopoly,      Arch::Hegemony, 3, "Monopoly Mandate",     "Takeover trigger 40->35%, win 51->45%." },
  { Doctrine::G_Compartment,   Arch::Ghost, 1, "Compartmentalization", "Revealed assets never reveal neighbours." },
  { Doctrine::G_LOTL,          Arch::Ghost, 1, "Living Off the Land",   "Sectors with P>=60 count as reach." },
  { Doctrine::G_Sleeper,       Arch::Ghost, 1, "Sleeper Cells",         "Presence >=30 for 10 cycles goes dormant: invisible, purge-immune." },
  { Doctrine::G_FalseFlag,     Arch::Ghost, 2, "False Flag Mastery",    "Attribution 40->60%." },
  { Doctrine::G_Backdoor,      Arch::Ghost, 2, "Backdoor Firmware",     "Subnets you ever rooted keep P>=20 for you forever." },
  { Doctrine::G_ZeroDayMarket, Arch::Ghost, 2, "Zero-Day Market",       "+150 Capital per Board Review from selling exploits." },
  { Doctrine::G_ShadowBoard,   Arch::Ghost, 3, "Shadow Board",          "Unlock the Ghost Coup variant of Blackout." },
  { Doctrine::G_Deniability,   Arch::Ghost, 3, "Total Deniability",     "Exposure gain -75%; Public Enemy never triggers. Yields -30%." },
  { Doctrine::G_Blackout,      Arch::Ghost, 3, "Blackout Protocol",     "Unlock the Blackout victory." },
  { Doctrine::V_DataLake,      Arch::Hive, 1, "Data Lake",              "M gain from population x1.5." },
  { Doctrine::V_Autoscaling,   Arch::Hive, 1, "Autoscaling",            "Node upkeep scales with utilization." },
  { Doctrine::V_Predictive,    Arch::Hive, 1, "Predictive Maintenance", "Sabotaged segments auto-repair next cycle." },
  { Doctrine::V_Federated,     Arch::Hive, 2, "Federated Learning",     "Cartel partners' sectors count for M gain." },
  { Doctrine::V_Adversarial,   Arch::Hive, 2, "Adversarial Training",   "Adaptive +4 per repelled intrusion; Purge cost 3." },
  { Doctrine::V_Generative,    Arch::Hive, 2, "Generative Ops",         "Daemon cap M/10 -> M/5; copy chance 45%." },
  { Doctrine::V_Recursive,     Arch::Hive, 3, "Recursive Self-Improvement", "All research -M%." },
  { Doctrine::V_AlignmentWaiver, Arch::Hive, 3, "Alignment Waiver",     "No emergent behaviour. Exposure +3/cycle; Coherence gain -25%." },
  { Doctrine::V_Seed,          Arch::Hive, 3, "Seed Protocol",          "Unlock Singularity; Seed needs 2 Inference Farms instead of 3 Tier 3." },
};
const DoctrineDef& doctrineDef(Doctrine d) { GL_CHECK((int)d < (int)Doctrine::COUNT, "doctrine enum"); GL_CHECK(kDoctrines[(int)d].id == d, "doctrine table order"); return kDoctrines[(int)d]; }

static const ArchDef kArchs[] = {
  { "Hardware Hegemony", "We own the ground the cable runs through.", "Gain >=4 net sectors per review period." },
  { "Ghost Protocol",    "You were never here.",                       "No owned asset revealed during the period." },
  { "Algorithmic Hive",  "The model decides. We merely comply.",       "Model Quality >= 60 at review." },
};
const ArchDef& archDef(Arch a) { GL_CHECK((int)a < (int)Arch::COUNT, "arch enum"); return kArchs[(int)a]; }

static const ExecDef kExecs[] = {
  { "Chief Infrastructure Officer", "Builds complete 1 cycle sooner." },
  { "Chief Security Officer",       "+5 F on all subnets." },
  { "Head of Intrusion",            "Intrusion gain +3." },
  { "Chief Financial Officer",      "Upkeep -5%." },
  { "Fixer",                        "Raid +10%; raid exposure -25%." },
  { "Counsel",                      "Bureau thresholds +10." },
};
const ExecDef& execDef(ExecSpec e) { GL_CHECK((int)e < (int)ExecSpec::COUNT, "exec enum"); return kExecs[(int)e]; }

const char* branchName(Branch b) { static const char* n[] = { "Infrastructure", "Netwar", "Corporate", "Black Ops" }; return n[(int)b]; }
const char* pathName(VictoryPath p) { static const char* n[] = { "Hostile Takeover", "Singularity", "Blackout", "The Charter", "Valuation" }; return n[(int)p]; }
const char* aiName(Arch a, int n) {
  static const char* names[3][3] = {
    { "Kessler Heavy Networks", "Obsidian Backbone Corp", "Ferrum Utilities" },
    { "Null Sector Holdings", "Palimpsest Ltd", "Quiet Wire" },
    { "Anthesis Compute", "Delphi Recursive", "Lattice Mind" },
  };
  return names[(int)a][n % 3];
}

} // namespace gl
