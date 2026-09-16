// Gridlock: Silicon Syndicate — core enums and constants for the engine-agnostic simulation.
#pragma once
#include <cstdint>

namespace gl {

enum class Sector : uint8_t { Financial, Campus, Port, Industrial, Arcology, Sprawl, Undercity, Exchange, Barrier, COUNT };
enum class Arch : uint8_t { Hegemony, Ghost, Hive, COUNT };
enum class StructKind : uint8_t { Node, Repeater, Substation, PrivateGrid, Outpost, Array, Honeypot, Lab, Rack, Tap, Cutout, Fork, COUNT };
enum class Variant : uint8_t { None, Phantom, Inference, Citadel };
enum class LinkType : uint8_t { Trunk, Backbone, Dark, Uplink, Armored, COUNT };
enum class OpKind : uint8_t { Scan, Intrusion, Harden, Purge, Siphon, Jam, Root, DDoS, Raid, Repair, Eminent, Sever, Covert, Daemon, Oracle, Retrain, COUNT };
enum class RaidEffect : uint8_t { Sever, Darken, SeizeRights, Crown };
enum class Priority : uint8_t { Normal, Critical, Low };
enum class Branch : uint8_t { Infrastructure, Netwar, Corporate, BlackOps, COUNT };

enum class Tech : uint8_t {
  Trenching, RedundantPeering, GridContracts, BackboneFiber, ModularDC, MicrowaveMesh, Superconducting, HyperscaleCooling, FortifiedConduit,
  DPI, HardenedKernels, Persistence, ZeroDay, Honeypots, TrafficShaping, KillChain, AdaptiveFW, BackboneSniffing,
  ShellCompanies, FuturesDesk, Lobbying, VerticalIntegration, DataBrokerage, Buyback, TenderOffer, ConsolidationLobby, SovereignWealth,
  FieldTeams, CounterIntel, DeadDrops, SabotageDoctrine, FalseFlags, ExecProtection, Decapitation, BlackoutProtocol, Wetwork,
  COUNT
};

enum class Doctrine : uint8_t {
  H_Vertical, H_Eminent, H_CompanyTown, H_Rings, H_Kinetic, H_PeeringCartel, H_RightOfWay, H_Fortress, H_Monopoly,
  G_Compartment, G_LOTL, G_Sleeper, G_FalseFlag, G_Backdoor, G_ZeroDayMarket, G_ShadowBoard, G_Deniability, G_Blackout,
  V_DataLake, V_Autoscaling, V_Predictive, V_Federated, V_Adversarial, V_Generative, V_Recursive, V_AlignmentWaiver, V_Seed,
  COUNT
};

enum class ExecSpec : uint8_t { CIO, CSO, HOI, CFO, Fixer, Counsel, COUNT };
enum class VictoryPath : uint8_t { Takeover, Singularity, Blackout, Charter, Valuation, COUNT };
enum class Posture : uint8_t { Expand, Hedge, Contain, Desperation };

namespace K {
  constexpr double GridPrice = 5.0;
  constexpr double GridCap = 12.0, GridCapTech = 20.0;
  constexpr double SellPrice = 2.0, SellFloor = 1.0, BuyPrice = 4.0, BuyCap = 20.0;
  constexpr double BufferCap = 50.0, BufferCapTech = 100.0, BufferDecay = 0.2;
  constexpr double ExposureCool = 3.0, ExposureCoolTech = 5.0;
  constexpr int BoardReview = 12, GameEnd = 200;
  constexpr double StartCapital = 600.0, ExecCost = 200.0, ExecUpkeep = 10.0;
  constexpr int BaseSlots = 3, MaxExecs = 5;
  constexpr int ScanFreshness = 3;
  constexpr int OrphanCycles = 3;
  constexpr double MaxHopLoss = 0.60;
}

} // namespace gl
