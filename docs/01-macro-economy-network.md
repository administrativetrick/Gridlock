# 01 — Macro-Economy & Network Expansion

**Module 1 of 3.** How Bandwidth and Capital are produced each cycle, how laying fiber
*is* territory expansion, and how packet loss punishes the overextended syndicate.

Design goal for this module: **the player should be able to look at their network on
the map and know, without opening a spreadsheet, where it is strong and where it is
thin.** Every number below has a visual: link thickness = capacity, link brightness =
utilization, link colour = loss, sector fill = Control Integrity.

---

## 1. The resource model

| Resource | Stockpiled? | Produced by | Consumed by | Visual |
|---|---|---|---|---|
| **Capital ¤** | Yes | Sector yields, Exchange sales, skims | Construction, upkeep, bribes, shares | Ticker, top bar |
| **Bandwidth BW** | **No** (small Buffer only) | Nodes (lit) | Sector control, operations, research, Exchange fee | Flowing particles on fiber |
| **Power MW** | No | Substations, city grid | Nodes | Lit / dark node glyph |
| **Compute** | No | BW allocated at a node | Research | Progress ring on Lab |
| **Exposure X** | Yes (0–100, decays) | Operations, construction in view | Bureau reactions | Heat gauge |

**Why Bandwidth does not stockpile.** A stockpile lets a player bank for ten cycles
and then do everything at once, which flattens the dual-layer tension. Use-it-or-lose-it
throughput means the *shape* of the network at this exact cycle is what matters, and a
sabotaged link this cycle hurts this cycle. The Buffer (§7) is a deliberately small
shock absorber, not a bank.

## 2. Sectors

### 2.1 Sector type table

Yields and demands are per cycle at Control Integrity 100 and fully met demand.

| Type | Capital base ¤ | BW demand | Conduit cost ¤ (one-time) | Population | Firewall base | Notes |
|---|---|---|---|---|---|---|
| Financial District | 40 | 8 | 12 | 2 | 45 | Highest value; high firewall; skim target |
| Corporate Campus | 30 | 6 | 10 | 3 | 40 | Typical start-adjacent |
| Port / Logistics | 25 | 5 | 8 | 2 | 30 | Almost always a barrier crossing → choke point |
| Industrial | 20 | 4 | 6 | 2 | 25 | Only type that may host a Substation at full output |
| Arcology (dense residential) | 15 | 6 | 5 | 8 | 30 | Data harvest: skims here yield **Compute**, not ¤ |
| Sprawl (low residential) | 8 | 2 | 3 | 4 | 20 | Cheap filler; loss-neutral corridors |
| Undercity | 5 | 1 | 2 | 3 | 10 | Grey market access; −50% Exposure for ops staged here |
| **Exchange** | 0 | **10** (peering fee) | — | 0 | 60 (Bureau-owned) | Cannot be controlled; connect to it |
| Barrier (river / rail / freeway) | 0 | 0 | **15 crossing premium**, added to the destination hex | 0 | — | Conduit only, no structures |

Population feeds the Charter victory (see 03) and the Hive's data ingestion (see 02).

### 2.2 Demand modifiers

Demand is what the sector draws from your network every cycle to stay yours.

| Condition | Demand change | Rationale |
|---|---|---|
| Any rival has Presence ≥ 40 in the subnet | **+2** | Contested subnets cost more to hold. This is the main cyber → physical coupling. |
| Your Surveillance Array active in sector | +1 | Vision has upkeep (Pillar 1) |
| Sector contains one of your nodes | Demand served first, at 0 hops, 0 loss | Nodes defend their own hex |
| Bureau Audit active on you | +1 all sectors | Compliance overhead |

### 2.3 Control Integrity (C)

Each sector holds a value C ∈ [0,100] per controlling syndicate.

```
r = delivered_BW / demand                       (ratio of supply to need)

if r ≥ 1.0 :  C += 10                            (cap 100)
if r < 1.0 :  C −= (1 − r) × 25                  (floor 0)
if r < 0.6 :  sector is in BROWNOUT this cycle    (yield 0; rivals may claim; see §5.4)
if C == 0  :  sector goes NEUTRAL                 (your structures there are ORPHANED, §5.5)
```

Time-to-lose at typical ratios, from C = 100:

| r | C loss / cycle | Cycles to 0 |
|---|---|---|
| 0.9 | 2.5 | 40 |
| 0.75 | 6.25 | 16 |
| 0.5 | 12.5 | 8 |
| 0.0 | 25 | 4 |

The 4-cycle floor at r = 0 is deliberate: a severed link is a *crisis*, not an instant
loss. The player has four cycles to reroute, repair or reinforce.

### 2.4 Capital yield

```
yield_i = base_i × (C_i / 100) × min(1, r_i) × (1 + modifiers) − skims_i
```

A fully held sector yields its base. A sector at C = 60, r = 0.8 yields 48 % of base.
Skims are paid to intruders who have a Siphon running (§8).

## 3. Infrastructure catalogue

### 3.1 Nodes (Bandwidth sources)

| Tier | Name | Raw BW | Power | Build ¤ | Build cycles | Upkeep ¤ | Firewall base | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | Edge Node | 12 | 2 MW | 120 | 2 | 6 | 20 | Any non-barrier hex |
| 2 | Core Node | 30 | 6 MW | 350 | 3 | 15 | 35 | Start HQ is a T2 **Crown Node** (F 50) |
| 3 | Hyperscale | 75 | 15 MW | 900 | 5 | 35 | 50 | Requires *Modular Datacenters* tech; Industrial or Campus hex only |

Rules that matter:

- **A node with less than full power outputs proportionally.** 4 MW into a Core Node
  (6 MW) gives 20 BW. Zero power = dark node = zero output *and* its firewall drops by
  10/cycle (no one is watching the logs).
- **Compute cap:** at most 50 % of a node's raw output may be allocated to Compute.
  You cannot run a pure research farm on the base ruleset (the Hive can, see 02).
- **A node is a route source.** Loss is counted in hops *since the last source or
  repeater* (§4.2). Placing nodes is the primary anti-loss tool.

### 3.2 Links (Bandwidth carriers)

Links are laid hex-by-hex along a path you own conduit rights for (§5.1).

| Link | Capacity BW | Cost ¤ / hex | Upkeep ¤ / hex | Base loss | Special |
|---|---|---|---|---|---|
| Trunk Fiber | 20 | 25 | 1 | standard (§4.2) | Default |
| Backbone Fiber | 60 | 70 | 3 | standard | Requires *Backbone Fiber* tech |
| Dark Fiber | 40 | 40 | 1 | standard | **Invisible to rivals until lit** (carrying flow). Lay now, light later. |
| Microwave Uplink | 8 | 90 per tower pair | 4 | **10 %/hex, flat** | No conduit rights needed; crosses barriers; **spoofable** (a rival with P ≥ 50 in either endpoint redirects 50 % of the flow to themselves) |
| Armored Conduit | 60 | 110 | 4 | standard | Immune to physical sever. Hegemony-only (02). |

A hex may carry links from multiple syndicates (shared ducts). Capacity is per link.

### 3.3 Supporting structures

| Structure | Cost ¤ | Upkeep ¤ | Effect |
|---|---|---|---|
| **Repeater** | 80 | 3 | Resets the hop counter (§4.2). Passthrough, no capacity of its own. |
| **Substation** | 200 | 8 | 10 MW to nodes within 2 hexes (radius 3 on Industrial). Prime raid target. |
| **Security Outpost** | 150 | 6 | Raids against structures within 1 hex: attacker success −30 %. Enables *your* raids within 2 hexes. |
| **Surveillance Array** | 90 | 2 + 1 BW demand | Reveals all rival Presence values in the sector and 6 neighbours each cycle. |
| **Honeypot** | 60 | 1 | Decoy subnet: first rival Intrusion each cycle "succeeds" against a fake, revealing the attacker's identity and adding +5 Exposure to them. Requires *Honeypots* tech. |
| **Lab** | 250 | 10 | +1 concurrent research slot (base 1, max 3). Must sit in a node hex. |
| **Exchange Peering Rack** | 300 | 0 | Built *in an Exchange hex you deliver ≥ 10 BW to*. Unlocks the market (§7) and Backbone reach (§8.3). |

### 3.4 Power

```
MW available = Σ substations in range + grid purchase
grid purchase: 5 ¤ / MW / cycle, cap 12 MW per syndicate (20 with Grid Contracts tech)
Bureau cuts grid power to any syndicate with Exposure ≥ 80.
```

Power is intentionally the *cheapest* system to understand and the *most brittle* to
lose. A Core Node on grid power alone costs 30 ¤/cycle, twice its own upkeep, so the
economic answer is always a Substation, and Substations are the softest physical target.

## 4. The flow model

### 4.1 What gets solved each cycle

Step 2 of Resolution runs a **max-flow with per-hop multiplicative loss** from every
lit node, over every link you own, to every sink:

1. **Sector demand** in your controlled or claimed sectors (priority order below),
2. **Reserved operations** (queued ops declare their BW cost and a staging node),
3. **Compute allocation** (per node, up to cap),
4. **Exchange peering fee** (10 BW at the Exchange hex),
5. **Market sales** (only surplus that reaches an Exchange),
6. **Buffer top-up** (§7).

Sink priority is player-settable per sector: **Critical / Normal / Low**. Default is
Normal, with the solver breaking ties by Capital base (rich sectors first). Ops are
served *after* Critical sectors and *before* Normal ones, so a player who marks
everything Critical starves their own operations — a legible, deliberate trade-off.

### 4.2 Loss per hop — the distance tax

```
h        = hops since the last source (node or repeater), counting from 1
L_base   = 2 % + 1 % × h                      (3 %, 4 %, 5 %, 6 %, 7 % …)
L_cong   = max(0, util − 0.80) × 25 %         (0 % at ≤ 80 % utilization, 5 % at 100 %)
L_cont   = (max rival Presence in this hex / 100) × 10 %    (up to 10 %)
L_dmg    = 15 % if the link segment is sabotaged (until repaired)
L_row    = 10 % if a rival with C ≥ 50 in this hex has declared Right-of-Way Denial

L_hop    = min(60 %, L_base + L_cong + L_cont + L_dmg + L_row)
delivered = sent × Π (1 − L_hop) over the path
```

Cumulative *base* survival along an unbroken chain from a single source:

| Hops | Survival | Comment |
|---|---|---|
| 1 | 97 % | |
| 2 | 93 % | |
| 3 | 89 % | Comfortable reach of one node |
| 4 | 83 % | |
| 5 | 78 % | Practical limit before a repeater |
| 6 | 71 % | |
| 8 | 58 % | |
| 10 | 44 % | You are paying two BW to deliver one |

Every extra hop costs more than the last. This is the whole overextension mechanic in
one curve; everything else (congestion, contest, sabotage) is a situational multiplier.

### 4.3 Redundancy

A sector reachable by **two edge-disjoint paths** from lit sources:

- uses the lower-loss path for its full demand,
- and, if a segment on that path is sabotaged or congested, the solver falls back to
  the second path **in the same cycle** with no brownout.

With the *Redundant Peering* tech, a dual-path sector additionally ignores L_cong on
its primary. Rings beat chains. The UI draws single-path sectors with a thin dashed
outline ("single point of failure") so the player sees fragility before a rival does.

## 5. Territory = reach

### 5.1 Conduit rights

You may lay a link through a hex only if you hold **Conduit Rights** there.

| Hex status | How to obtain rights |
|---|---|
| Neutral | Pay the hex's conduit cost (one-time, instant). Barrier crossings add the 15 ¤ premium to the far hex. Multiple syndicates may hold rights in the same neutral hex. |
| Controlled by a rival (C ≥ 50) | **Cannot buy.** Options: (a) wait for a brownout (§5.4); (b) **Raid** to seize rights for 3 cycles (needs Outpost within 2, *Field Teams* tech); (c) **Covert Conduit** at Presence ≥ 60 (Ghost Protocol only). |
| Controlled by you | Free. |
| Exchange | Free for everyone. The Bureau owns the ducts. |

Rights persist while the hex stays neutral or yours. If a rival gains control of a hex
you have rights in, your rights stay valid **but** the rival may declare
**Right-of-Way Denial** (+10 % loss, §4.2) — or, with Hegemony's *Right of Way* perk,
sever the link outright.

### 5.2 Claiming a sector

A neutral sector becomes yours at the end of any cycle in which:

1. your delivered BW there ≥ its demand, **and**
2. no other syndicate holds C > 0 there.

C starts at 25 on the claim cycle and climbs by +10 per satisfied cycle. First yield at
C = 25 is a quarter of base, so a new sector pays back its conduit cost in about 2–3
cycles for a Sprawl hex and ~1 cycle for a Financial District.

### 5.3 Contested sectors

If two syndicates both deliver to a neutral sector, or a rival delivers to your
sector, **effective delivery** is weighted by cyber footing:

```
weight_s = delivered_s × (0.5 + Presence_s / 200)      // Presence 0 → ×0.5, Presence 100 → ×1.0
the highest-weight syndicate is "winning": its r uses its full delivered
every other syndicate's r is scaled by (their weight / winner weight)
```

The owner of a sector is credited with **Presence = 100** in their own subnet by
default (they run it), so a defender with a full pipe is very hard to out-deliver
unless the attacker has already pushed Presence inside. **You cannot take a sector by
cable alone** — you need the cyber layer too. That is the pillar, expressed as math.

### 5.4 Brownouts and takeovers

A sector in **brownout** (r < 0.6) with C < 50 is claimable by any rival who delivers
≥ demand *and* has Presence ≥ 50 in its subnet. On claim:

- the defender's C drops to 0 (sector flips),
- the attacker starts at C = 25,
- every defender structure in the hex is **orphaned** (§5.5),
- the attacker gains +6 Exposure (takeovers are noticed).

### 5.5 Orphaned structures

Your node/substation/outpost sitting in a hex you no longer control:

- keeps functioning for **3 cycles** (nodes still output; the new owner sees them),
- then becomes **seizable** by the hex owner for 50 % of build cost (they inherit it)
  or **scuttled** by you for free (you deny it).
- You can rescue it by re-taking the hex within the 3 cycles.

This gives a sector flip a satisfying second act rather than a binary swap.

### 5.6 Wireless expansion

Microwave Uplinks let you leap a barrier or reach an island of value without conduit,
at 10 % flat loss per hex and capacity 8. Two towers (one each end) are needed. The
spoof rule (§3.2) makes an uplink a **liability** on a contested border: the intended
use is early-game reach and temporary bridging, not a permanent artery.

## 6. Capital: sources, sinks, insolvency

### 6.1 Income

| Source | Formula |
|---|---|
| Sector yields | §2.4, summed |
| Exchange BW sales | surplus BW reaching an Exchange × price (§7) |
| Skims | 20 % of a rival sector's yield per running Siphon (§8) |
| Grey market | ≤ 10 BW / cycle from any Undercity you control at 0.8 ¤ / BW (no Exchange needed) |
| Interest | 2 % / cycle on Capital above 1000 ¤ (*Sovereign Wealth* tech) |

### 6.2 Upkeep

```
upkeep = Σ node upkeep + Σ link upkeep (per hex) + Σ structure upkeep + grid MW × 5
```

Upkeep is **linear** in infrastructure on purpose. The nonlinear brakes on expansion are
loss (§4.2), demand contest (§2.2), attention (§9) and Exposure (§10) — all of which the
player can *see* and *engineer around*. A hidden administrative-drag multiplier would
violate the "read the map, not the spreadsheet" goal.

### 6.3 Insolvency

If Capital would go negative after upkeep:

1. Structures are **darkened** in order of lowest (yield protected ÷ upkeep) until
   upkeep is affordable. Darkened nodes output 0 and lose firewall (§3.1).
2. Exposure +5 ("creditors talk").
3. The Bureau opens an **Audit** if this happens two cycles running.

You cannot go below 0 Capital; you go dark instead. This keeps the death spiral visible
on the map (sectors flicker out from the edges) rather than as red text.

## 7. The Exchange and the market

Connect to an Exchange by delivering 10 BW to its hex every cycle (the peering fee) and
building a **Peering Rack** (300 ¤). Then:

| Action | Terms |
|---|---|
| **Sell surplus** | Base 2.0 ¤ / BW. City-wide price drops 0.1 ¤ per 10 BW sold that cycle (all syndicates), floor 1.0 ¤. Recovers 0.2 ¤ / cycle toward 2.0. |
| **Buy BW** | 4.0 ¤ / BW, max 20 BW / cycle. Bought BW appears as a *source* at the Exchange hex and routes outward from there (subject to loss). |
| **Backbone reach** | You may stage operations against **any sector within 4 hops of any Exchange** you are connected to, at +50 % BW cost (§8.3). |
| **Peering intelligence** | With *Backbone Sniffing* (tech), see the volume every other connected syndicate pushes through this Exchange. Compute-heavy flows are a Singularity tell (03). |

**Buffer.** Up to 10 % of raw production per cycle may be banked, cap 50 BW (100 with
*Dead Drops*). The Buffer decays 20 % per cycle and may be spent only on **sector
demand and defensive ops** (Harden, Purge, Repair), never on attacks or research. It
exists so a single sabotaged link becomes a scramble, not an instant cascade.

## 8. Operations (the Bandwidth sinks that fight)

Operations are the game's units. Each costs BW from a **staging node** and, for
attacks, adds Exposure. Full tables live with the archetypes in 02; this is the base set.

### 8.1 Cyber layer

| Op | BW | Duration | Requires | Effect | Exposure |
|---|---|---|---|---|---|
| Scan | 2 | instant | adjacency or P ≥ 1 | Reveal subnet: owner, F, structures, C, all rival P (3-cycle freshness) | 0 |
| Intrusion | 4 / cycle | sustained | adjacency or Backbone reach | P += max(3, 15 − F/10) per cycle; P decays 5/cycle when not sustained (2 with *Persistence*) | +1 / cycle |
| Harden | 2 / cycle | sustained | own subnet | F += 5 / cycle (cap 100) | 0 |
| Purge | 6 | instant | own subnet | All rival P −30 in this subnet | 0 |
| Siphon | 3 / cycle | sustained | P ≥ 40 | Skim 20 % of the sector's yield (¤; **Compute** if Arcology) | +2 / cycle |
| Jam | 5 / cycle | sustained | P ≥ 40 | +10 % loss on every rival link segment through this hex (stacks with L_cont) | +3 / cycle |
| Root | 12 | instant | P ≥ 70 | You become co-operator: see everything, F → 0, sector demand +4 for owner, you may claim on brownout with no further P requirement. Owner's Purge resets it. | +6 |
| DDoS | 8 | instant | Backbone reach | Target node output −50 % next cycle | +8 |

### 8.2 Physical layer

| Op | ¤ | Requires | Effect | Exposure |
|---|---|---|---|---|
| Lay / upgrade link | per §3.2 | conduit rights | completes next cycle | 0 (+2 if laid in view of a rival Surveillance Array) |
| Build structure | per §3 | hex control (or neutral + rights for Repeater) | completes after build cycles | 0 |
| Repair segment | 30 | — | clears L_dmg next cycle | 0 |
| **Raid** | 60 | Outpost within 2; *Field Teams* | Roll: 60 % base −30 % per defending Outpost within 1, +10 % per 25 P you hold in the hex. Success: sever one link segment (3 cycles) **or** darken a substation (2 cycles) **or** seize conduit rights (3 cycles). | +10 |
| Seize orphan | 50 % build cost | hex control, orphan timer expired | Inherit the structure | +2 |

### 8.3 Reach rule

You can stage an op against a sector if **any** of:
- it is adjacent to a sector where you hold C ≥ 25 or a structure,
- you already have P ≥ 1 in its subnet (footholds spread),
- it is within 4 hops of an Exchange you are connected to (+50 % BW cost).

No reach → no op. Long-range action is bought with Exchange access, which is why
Exchanges are the most contested hexes on the map.

## 9. Attention: action slots

Each syndicate may **issue** at most `3 + (number of Executives)` new operations per
cycle (sustained ops keep running without a slot). Executives (2 at start, max 5) are
hired for Capital, each with a specialty (+1 free slot for ops of one class). Slots are
the "attention tax": a sprawling empire has the same number of hands as a small one.

## 10. Exposure and the Bureau

```
X gains: per op table (§8) + 6 per takeover + 5 per insolvency cycle
X cools: −3 / cycle (−5 with Shell Companies); ops staged from Undercity hexes add half
```

| X threshold | Bureau reaction |
|---|---|
| 40 — *Notice* | Silhouettes of all your nodes appear on every rival's map. |
| 60 — *Audit* | 15 % of Capital frozen for 3 cycles; +1 demand in all sectors; a rival's Scan on you costs 0. |
| 80 — *Enforcement* | Grid power cut; Bureau raid darkens one random Substation for 2 cycles each cycle while ≥ 80. |
| 100 — *Public Enemy* | Every rival gains +25 P in all your border subnets, immediately. |

The Bureau also **guards Exchanges** (Bureau-owned firewall 60, cannot be claimed) and
holds the **Kill Switch** used against a Singularity launch (03).

## 11. Worked cycle — a chain that reaches too far

Setup: a Core Node at hex **A** (Corporate Campus, demand 6, powered by a Substation
in-hex) with Trunk Fiber (cap 20) laid in a straight chain through **B** Industrial (4),
**C** Sprawl (2), **D** Arcology (6) to **E** Financial District (8). No rivals present.

```
Raw output at A ........................ 30 BW
A's own demand, 0 hops ................. −6   → 24 available to send
Trunk capacity out of A ................ 20   → 20 sent, 4 stranded at A (→ Buffer 3, decay 1)

Hop 1  A→B   h=1  L_base 3 %, util 100 % → L_cong 5 %  → 8 %   arrive 18.40   B takes 4 → 14.40
Hop 2  B→C   h=2  L_base 4 %, util 72 %  → 0 %          → 4 %   arrive 13.82   C takes 2 → 11.82
Hop 3  C→D   h=3  L_base 5 %                             → 5 %   arrive 11.23   D takes 6 →  5.23
Hop 4  D→E   h=4  L_base 6 %                             → 6 %   arrive  4.92   E needs 8

E:  r = 4.92 / 8 = 0.61   → C −= 9.75 / cycle, yield = 40 × (C/100) × 0.61
                             one more point of loss and E is in brownout
```

The richest sector is the one dying. Three fixes, in the order the game should teach
them:

| Fix | Cost | Result at E | Lesson |
|---|---|---|---|
| Repeater at C | 80 ¤ + 3 ¤/cycle | Hop 3 → 3 %, hop 4 → 4 %: E receives **5.25** (r 0.66). Still degraded. | Repeaters fix *length*, not *starvation*. |
| Upgrade A→B to Backbone (cap 60) | 70 ¤ + 2 ¤/cycle extra | 24 sent, no congestion: E receives **9.10** (r 1.14). Full control, 1.1 surplus. | The bottleneck was the first hop, not the last. |
| Edge Node at D | 120 ¤ + 6 ¤ upkeep + **2 MW** (A's substation is 3 hexes away → grid 10 ¤/cycle, or a second Substation 200 ¤) | D and E fed locally at 0–1 hop; A's chain now only carries B and C. | Nodes are the strongest anti-loss tool, and **power** is what makes them expensive. |

Now add a rival with Presence 60 in **D**:

```
D demand 6 → 8 (contested)             L_cont at D = 6 %
Hop 3 arrive 11.23,  D takes 8 → 3.23
Hop 4  L = 6 % + 6 % = 12 %            arrive 2.84   E: r = 0.36 → BROWNOUT
```

The rival never touched E. They pushed Presence into the hex *before* it, and the
richest sector on the line browned out two cycles later. **That is the dual-layer
battlefield working as intended**, and it is the moment the tutorial should be built
around.

## 12. Tuning targets

| Cycle | Sectors | Nodes | Net BW | Capital / cycle | Notes |
|---|---|---|---|---|---|
| 1 | 4 | 1 (Crown) | 30 | ~60 | |
| 30 | 12–15 | 2–3 | 60–80 | 150–250 | First Exchange contact |
| 80 | 30–40 | 5–7 | 200–300 | 500–800 | First takeovers; doctrines T2 |
| 150 | 50–70 | 9–12 | 500+ | 1200+ | Victory phases active |

Guardrails to hold in balancing:

- **No sector should be sustainable beyond 6 hops from a source** without redundancy.
- **A Core Node's upkeep should be ≤ 25 % of the yield it protects** at cycle 30.
- **A single Raid should never end a game**, but two coordinated Raids on an
  unredundant power + link pair should brown out ~20 % of a mid-game network for 3 cycles.
- **Exposure from a full offensive posture** (2 Intrusions + 1 Siphon + 1 Jam running)
  should hit *Notice* in ~7 cycles and *Audit* in ~14 without Shell Companies.
