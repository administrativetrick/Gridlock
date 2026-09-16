# Appendix A — Technical Notes (Unreal Engine 5)

How the systems in 01–03 map onto an engine architecture. This is a direction document
for a vertical slice, not an implementation spec.

## 1. Simulation architecture

**Principle: the simulation is a pure, deterministic, engine-agnostic C++ module.**
Rendering, UI and AI read from it; nothing writes to it except the order queue.

```
GridlockSim (plain C++ static lib, no UObject)
   ├─ Map            hex grid, sector table, district regions, barrier flags
   ├─ Assets         nodes, links (per-hex segments), structures, per-syndicate
   ├─ Cyber          presence[syndicate][sector], firewall[sector], roots, daemons
   ├─ Orders         per-cycle order set per syndicate (validated, serializable)
   ├─ Resolve()      the 10-step pipeline from 00 §5, fixed order, fixed RNG stream
   ├─ Flow           per-syndicate max-flow with multiplicative loss
   ├─ Victory        proximity metrics + phase state machines from 03
   └─ Snapshot       full state serialize / hash (save, replay, desync detection)

GridlockRuntime (UE5 GameModule)
   ├─ USimSubsystem      owns GridlockSim, ticks Resolve() at end of Planning
   ├─ UFogView           per-viewer "what does syndicate S see" projection of the state
   ├─ Presentation       hex actors / ISM, spline links, Niagara flow, HUD bindings
   └─ AI                 Threat Board + planners; consumes UFogView(S), never raw state
```

Why: a headless sim lets us (a) run thousands of AI-vs-AI games for balance overnight,
(b) do lockstep multiplayer by exchanging order sets and comparing state hashes, and
(c) unit-test the worked example in 01 §11 to the decimal.

## 2. Determinism

- **Fixed-point** for all economy math (int64 milli-units: 1 BW = 1000). No floats in
  the sim. Loss multipliers are integer permille.
- **One seeded RNG stream per subsystem per cycle** (`raid`, `attribution`,
  `emergent`, `daemon-copy`), advanced in a fixed order. Order sets are sorted by
  (syndicate id, order id) before resolution.
- **State hash every cycle**; multiplayer clients compare hashes and rewind-and-replay
  from the last agreed snapshot on mismatch.

## 3. The flow solver

Per syndicate, per cycle. Graph size: ≤ 600 hex nodes, ≤ 2000 segment edges, typically
5–15 sources and 30–70 sinks.

- Loss is multiplicative per hop, which max-flow does not model natively. We use a
  **successive-shortest-path** approach: repeatedly find the path from any lit source to
  the highest-priority unmet sink that maximizes *survival* (product of (1 − L_hop)),
  push flow along it up to the min residual capacity, recompute utilization-dependent
  congestion on touched edges, repeat. Sinks are visited in priority order
  (Critical → ops → Normal → Compute → peering fee → sales → Buffer → Low).
- Survival maximization is a shortest path on `−log(1 − L_hop)` weights; with integer
  permille losses we precompute a 1000-entry table so the whole thing stays integer.
- Complexity is trivially fine at this scale (< 1 ms per syndicate in Release), and the
  greedy result is what players *expect* to see ("my rich sectors got fed first"), which
  matters more than global optimality.
- **Redundancy (01 §4.3):** the solver marks a sink dual-path if a second
  edge-disjoint path exists after the first is found (one extra BFS on the residual
  graph). Fallback on sabotage happens naturally on re-solve.
- **Hop counter reset** for Repeaters and Iron Backbone is a per-edge property computed
  when a chain is laid, not at solve time.

## 4. Fog and information

`UFogView(S)` produces, for syndicate S, a **filtered copy** of the state with:

- sectors S has no vision on replaced by their **last-seen snapshot + cycle stamp**,
- Ghost hidden assets removed unless revealed,
- Dark Fiber removed unless lit,
- other syndicates' Presence values present only where S has an Array or a fresh Scan,
- Valuations and seats always present (public data).

The **AI consumes only this view.** The debug overlay renders the AI's VP matrix next to
what it can see so designers can confirm the "no cheating" rule (03 §7.6) at a glance.

## 5. Presentation

### 5.1 World

- **Isometric orthographic** camera, fixed pitch, 4 zoom stops (city / district /
  sector / building). Free rotation in 60° steps to match the hex grid.
- **Hexes as Instanced Static Meshes**, one ISM per sector type, with per-instance
  custom data (C, contest flag, owner colour, selection) read in the material.
- **Brutalist kitbash set** in **Nanite**: 40–60 modular concrete blocks assembled
  procedurally per sector type from a seed, so 600 hexes never repeat. Nodes and
  Substations are hero meshes with emissive detail.
- **Lumen** GI with emissive neon as the dominant light source; a single dim sky. The
  city reads as concrete lit from within by the network — which is the theme.
- **Links as splines** along hex centres. Mesh: a thin cable; thickness from capacity,
  emissive intensity from utilization. **Niagara ribbon particles** flow along the
  spline: rate = flow, colour = loss (cool cyan at 3 %, hot magenta at 20 %+), with
  particles visibly *dropping off* the cable at each hop in proportion to L_hop. Packet
  loss is thereby literally visible as particles falling into the dark.
- **Presence overlay**: a holographic wireframe volume above each sector, opacity =
  your P, tinted per rival when you have Array vision. Toggle layer.
- **Weather / time of day** is cosmetic, but the **Blackout** and **Launch** phases get
  bespoke lighting states (city goes dark district by district; Exchange hexes pulse).

### 5.2 UI

- **CommonUI + UMG**, wireframe visual language: hairline borders, monospace numerics,
  scanline overlay at low opacity, no filled panels except the Board Room.
- **Top bar**: Capital, net BW (produced / delivered / lost), MW, Exposure gauge,
  Valuation ticker, cycle counter and end-turn.
- **Sector inspector**: dual-layer card — physical (structures, links, power) on the left,
  cyber (F, every visible P, roots, daemons) on the right, joined by the C ring.
- **Projected-loss ghost** while laying fiber: the path preview shows survival % at each
  hop and the resulting r at the destination, before you pay.
- **Board Room**: full-screen holographic city with the five paths (03 §8).

### 5.3 Performance budget (target 60 fps at 1440p, mid-range GPU)

| Item | Budget |
|---|---|
| Sim Resolve() | < 50 ms (all syndicates, on end-turn, async task) |
| Hex ISM + Nanite city | ~4 ms |
| Lumen | ~5 ms (Software RT fallback acceptable) |
| Niagara link flows | ~1.5 ms, LOD by zoom stop (ribbons off at city zoom → emissive only) |
| UI | ~1 ms |

## 6. Data-driven content

- **DataAssets** for sector types, link types, structures, techs, doctrines, ops, AI
  personas — every table in 01–03 is a DataTable row set, hot-reloadable.
- **Gameplay Tags** for sector type, archetype, op class, victory path; used by
  doctrine effects ("−25 % cost on ops tagged `Op.Cyber.Intrusion`").
- **Map generator** parameters: hex count, syndicate count, Exchange count, barrier
  density, district count. Guarantees from 00 §4 (Exchanges behind a barrier, not
  adjacent to starts) are enforced by rejection sampling on the seed.

## 7. AI implementation sketch

- **Threat Board** (03 §7.1) recomputed each cycle from `UFogView`.
- **Planner**: utility AI over a fixed order vocabulary. Each candidate order gets
  scores from the persona's weights (expand / defend / intrude / contain) and the
  posture; top-N by score subject to action slots, Capital and BW reservations.
- **Expansion target** selection reuses the player's own projected-loss preview
  (same solver call), so AI networks obey the same topology lessons.
- **Balance harness**: headless AI-vs-AI runs across archetype permutations, logging
  Valuation curves, victory type/cycle, and Cartel formation cycle. Targets: every
  archetype wins 25–40 % of mirror-free 4-player games; median victory cycle 140–170;
  no ending > 45 % of all wins.

## 8. Vertical-slice scope (suggested)

1. Sim core + flow solver + worked example as a unit test.
2. 150-hex map, 2 syndicates (player Hegemony vs AI Ghost), Trunk/Backbone/Repeater/
   Substation/Node T1–T2, Scan/Intrusion/Harden/Purge/Siphon/Jam/Root.
3. Fog view + stale-data rendering.
4. Niagara link flows with visible packet loss.
5. One ending: Hostile Takeover with the Proxy Fight and the Ghost AI's Contain
   behaviour.

That slice proves all three pillars: you can see loss, you can be blinded, and the
economy decides the ending.
