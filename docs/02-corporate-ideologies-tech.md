# 02 — Corporate Ideologies & Tech Trees

**Module 2 of 3.** The shared technology tree, then three corporate archetypes —
**Hardware Hegemony**, **Ghost Protocol**, **Algorithmic Hive** — each with passive
identity, unique structures and operations, a signature mechanic, a nine-policy Doctrine
tree, a victory bias, a documented weakness, and an AI persona.

Design goals:

1. **Same map, different game.** An archetype should change *what the player looks at*
   on the map (Hegemony: link thickness; Ghost: presence overlays; Hive: compute rings),
   not just multiply numbers.
2. **Every strength is a tell.** Each archetype's advantage produces a signal that a
   rival with the right vision can read. Information as Munition applies to identity.
3. **Doctrines are choices, not a ladder.** Nine policies, you take six. Every tier has
   a pick that a rival can point at and say "that's why they beat me."

---

## 1. Research: Compute

Research is funded by **Bandwidth allocated to Compute** at nodes (1 BW → 1 Compute
per cycle, capped at 50 % of each node's raw output). There is no separate science
resource. A syndicate that researches fast is a syndicate whose network is idle,
which is itself a strategic statement — and a visible one (compute allocation shows as a
progress ring on the node glyph to anyone who can see the node).

- **Slots:** 1 concurrent research by default; each **Lab** (250 ¤) adds one, max 3.
- **Costs:** Tier 1 = 60–120 Compute, Tier 2 = 200–350, Tier 3 = 500–800.
- **Prerequisite rule:** any Tier 2 in a branch needs 2 Tier 1s in that branch; any
  Tier 3 needs 2 Tier 2s.
- **Skimming Arcologies yields Compute, not Capital** (01 §8.1) — the espionage route
  to research.

## 2. Shared tech tree

Four branches. Every syndicate can research everything; archetypes discount one branch
and tax another (§4–6).

### 2.1 Infrastructure (physical)

| Tier | Tech | Cost | Effect |
|---|---|---|---|
| 1 | Trenching Automation | 80 | Link cost −25 % |
| 1 | Redundant Peering | 100 | Dual-path sectors ignore congestion on their primary path |
| 1 | Grid Contracts | 60 | Grid power cap 12 → 20 MW |
| 2 | Backbone Fiber | 250 | Unlock Backbone Fiber (cap 60) |
| 2 | Modular Datacenters | 300 | Unlock Tier 3 Hyperscale node |
| 2 | Microwave Mesh | 200 | Uplink loss 10 % → 6 %; capacity 8 → 12 |
| 3 | Superconducting Trunk | 600 | Backbone base loss becomes 1 % + 0.5 %·h |
| 3 | Hyperscale Cooling | 550 | Tier 3 nodes +25 % output |
| 3 | Fortified Conduit | 500 | All links immune to Raid severs |

### 2.2 Netwar (cyber)

| Tier | Tech | Cost | Effect |
|---|---|---|---|
| 1 | Deep Packet Inspection | 80 | Scan also reveals rivals' *queued* ops against that subnet |
| 1 | Hardened Kernels | 100 | Firewall base +10 on all owned subnets |
| 1 | Persistence | 90 | Your Presence decays 5 → 2 per cycle when unsustained |
| 2 | Zero-Day Cache | 300 | Intrusion gain +50 %; stock of 3 uses, refills 1 per 5 cycles |
| 2 | Honeypots | 220 | Unlock Honeypot structure |
| 2 | Traffic Shaping | 250 | Jam 10 % → 20 % loss |
| 3 | Kill Chain | 700 | Root cost 12 → 8; **Crown Nodes** become rootable |
| 3 | Adaptive Firewalls | 600 | Each repelled Intrusion (P gain < 5) adds +2 F permanently |
| 3 | Backbone Sniffing | 550 | See volumes every syndicate pushes through Exchanges you peer at |

### 2.3 Corporate (economy & politics)

| Tier | Tech | Cost | Effect |
|---|---|---|---|
| 1 | Shell Companies | 80 | Exposure cooling 3 → 5 / cycle |
| 1 | Futures Desk | 90 | Exchange sell price +25 % for you |
| 1 | Lobbying Office | 100 | Unlock District bribery (03 Charter path) |
| 2 | Vertical Integration | 250 | Node upkeep −20 % |
| 2 | Data Brokerage | 280 | Skims 20 % → 30 %; Arcology base yield +5 |
| 2 | Buyback Program | 300 | Share purchases −30 % cost (03 Takeover) |
| 3 | Tender Offer Authority | 700 | **Unlock Hostile Takeover trigger** |
| 3 | Consolidation Lobby | 650 | **Unlock Charter vote** |
| 3 | Sovereign Wealth | 500 | 2 % interest / cycle on Capital above 1000 ¤ |

### 2.4 Black Ops (physical espionage)

| Tier | Tech | Cost | Effect |
|---|---|---|---|
| 1 | Field Teams | 100 | Unlock Raid |
| 1 | Counter-Intel | 80 | You are told who Scans you |
| 1 | Dead Drops | 60 | Buffer cap 50 → 100 |
| 2 | Sabotage Doctrine | 280 | Raid +20 % success; severs last 3 → 4 cycles |
| 2 | False Flags | 300 | 30 % of your attack ops are attributed to a random rival |
| 2 | Executive Protection | 200 | Your Executives cannot be poached |
| 3 | Decapitation | 700 | Raid may target a Crown Node: −50 % output 3 cycles |
| 3 | Blackout Protocol | 800 | **Unlock Blackout victory** for non-Ghost syndicates |
| 3 | Wetwork | 600 | Remove a rival Executive (−1 action slot for them, 10 cycles). Exposure +25 |

## 3. Doctrine: Mandate points and Board Reviews

Each archetype has a **Doctrine tree** of 9 policies in 3 tiers. Policies are bought
with **Mandate**, granted by the Board:

- Every **12 cycles** a Board Review grants **1 Mandate**, +1 if the archetype's
  **KPI** was met over the period.
- Tier 2 requires 2 Tier 1 picks; Tier 3 requires 2 Tier 2 picks. Maximum 6 of 9 in a
  normal-length game (≈ 8–10 Mandate over 150 cycles, minus misses).
- Doctrines are **public** once taken (a press release fires). Tech is private.

---

## 4. HARDWARE HEGEMONY — "We own the ground the cable runs through."

**Fantasy.** The old-money infrastructure titan. Concrete, steel, private power. You
do not sneak; you *pour*. Rivals route around you until they can't.

**How it plays.** Wide, thick, redundant networks; early Tier 3 nodes; Substations
everywhere; Outposts on every barrier crossing. Takes the Exchanges by being the only
syndicate that can afford to peer at all of them. Loses to whoever is inside its subnets
before it notices.

### 4.1 Passives

| Passive | Value |
|---|---|
| Node raw output | **+25 %** |
| Link and node build cost | **−20 %** |
| Trunk Fiber capacity | 20 → **30** |
| Substation range | 2 → **3** hexes |
| Infrastructure branch research | −30 % Compute |
| Netwar branch research | **+30 %** Compute |
| Firewall base | **−10** on all subnets |
| Exposure gain from construction and Raids | **+25 %** |
| Intrusion BW cost | **+25 %** |

### 4.2 Unique structures & ops

| Item | Stats | Notes |
|---|---|---|
| **Citadel Node** (Tier 4) | 150 BW raw, 30 MW, 1800 ¤, 7 cycles, upkeep 60 ¤, F 60 | Immune to Raid and DDoS. Requires Hyperscale Cooling. **Visible to every syndicate from the day construction starts.** |
| **Armored Conduit** | cap 60, 110 ¤/hex, upkeep 4 | Immune to physical sever; still subject to Jam and Right-of-Way loss |
| **Private Grid** | Substation variant: 20 MW, 380 ¤, upkeep 14 | Industrial hex only |
| **Eminent Domain** (op) | 2× conduit cost, instant, Exposure +4 | Buy rights in a *rival-controlled* hex with C < 70. Bypasses the "cannot buy" rule. |
| **Sever** (op) | 0 ¤, requires *Right of Way* doctrine | Cut a rival link segment passing through a hex where you hold C ≥ 75 and an Outpost. No roll. Exposure +6. |

### 4.3 Signature mechanic — Iron Backbone

**Every third hex of a contiguous Hegemony link chain acts as a Repeater** (hop counter
resets at h = 3). A Hegemony chain therefore survives indefinitely at 3–5 % per hop,
and the archetype's territory is measured in *how much cable it can pay for*, not how
far a node can reach. Combined with +25 % output, Hegemony feeds ~40 % more sectors
per node than baseline — and shows it: their links are always drawn thick and bright.

### 4.4 Doctrine tree

**KPI:** net sectors gained over the review period ≥ +4.

| Tier | Policy | Effect | Cost / tell |
|---|---|---|---|
| 1 | Vertical Integration | Node upkeep −25 % (stacks with the tech) | — |
| 1 | Eminent Domain | Unlock the Eminent Domain op | Rivals get an Exposure-free Scan of any hex you buy |
| 1 | Company Town | Sprawl and Arcology demand −2 in hexes adjacent to your Substations | Their firewall −10 |
| 2 | Redundant Rings | Sectors with 2+ disjoint paths: all loss −50 % | — |
| 2 | Kinetic Doctrine | Outposts may Raid at range 3; Raid base 60 → 70 % | Raid Exposure ×1.5 |
| 2 | Peering Cartel | Exchange sell price floor 1.0 → 1.5 ¤; rivals at Exchanges you peer at pay +1 ¤/BW to buy | Bureau *Notice* threshold 40 → 30 |
| 3 | Right of Way | Unlock Sever | — |
| 3 | Fortress City | Firewall +30 on all subnets containing a Substation or Outpost | Yield −15 % in those sectors |
| 3 | Monopoly Mandate | Hostile Takeover threshold 51 % → **45 %** | Announces Takeover proximity to all when taken |

### 4.5 Victory bias & weakness

- **Bias:** Hostile Takeover (03 §2) — Valuation is infrastructure-weighted and
  Hegemony builds the most of it. Secondary: Charter, via population near Substations.
- **Weakness:** *blind and loud.* Low firewalls and expensive Intrusion mean a Ghost
  or Hive gets inside first. Exposure pressure forces a rhythm of build-then-cool. The
  Citadel is a monument that every AI immediately scores as a threat.

### 4.6 AI persona — "The Bulldozer"

Expands greedily along the cheapest conduit toward the nearest Exchange; over-builds
Substations; peers at every Exchange it can reach and pushes Peering Cartel early.
Responds to intrusions with Purge + Fortress City rather than counter-intrusion.
Contests hexes physically (Raid, Sever, Eminent Domain). Tell for the player: watch for
Substations appearing on barrier crossings two cycles before the fiber does.

---

## 5. GHOST PROTOCOL — "You were never here."

**Fantasy.** The syndicate that does not exist. Shell companies, dark fiber, phantom
nodes, false flags. Small on the map, everywhere inside it.

**How it plays.** A tight core (rarely more than half a Hegemony's sector count) and a
sprawling invisible presence in rival subnets. Feeds itself by leeching rival fiber and
skimming rival yields. Wins by owning rivals' networks from inside, or by making them
fight each other. Loses when found.

### 5.1 Passives

| Passive | Value |
|---|---|
| All physical assets **hidden** | Rivals see nothing in your hexes until they Scan (and Scans against Ghost hexes have a 50 % chance to return "nothing here" unless the scanner has *Deep Packet Inspection*) |
| Exposure gain | **−50 %** |
| Intrusion BW cost | **−30 %**; Intrusion Exposure 0 |
| Presence decay when unsustained | 5 → **1** per cycle |
| Sector Capital yields | **−20 %** |
| Node raw output | **−25 %** |
| Maximum node tier | **2** until *Modular Datacenters* (then 3, still −25 %) |
| Netwar & Black Ops research | −25 % |
| Infrastructure research | **+40 %** |

### 5.2 Unique structures & ops

| Item | Stats | Notes |
|---|---|---|
| **Phantom Node** | Core Node variant, same stats × 0.75; F 45 | **Never appears on rival maps**, even at Exposure *Notice*. Revealed only by a successful Root of its subnet or a Bureau raid. |
| **Tap** (structure) | 70 ¤, upkeep 2, requires P ≥ 40 in the hex | Leeches **20 %** of every rival link's flow passing through this hex into your network (appears as a source for you). Rival sees the loss as unexplained L_cont. Owner Purge destroys it. |
| **Cutout** (structure) | 120 ¤, upkeep 3 | Registers the hex to a front company: the sector reads as **neutral** to rivals and the Bureau. Yields −50 %. Ops staged from it add 0 Exposure. |
| **Burner Uplink** (op) | 40 ¤, single use | A one-cycle Microwave link of capacity 15, any two hexes within 3, no towers. Vanishes after use. |
| **Covert Conduit** (op) | 1.5× conduit cost, requires P ≥ 60 | Buy conduit rights in a rival hex. Rights are hidden. |

### 5.3 Signature mechanic — Attribution

Every Ghost attack op rolls **Attribution**: 40 % base (60 % with *False Flag Mastery*)
that the victim's logs blame a **different syndicate**, chosen weighted by who has the
most Presence in the victim's subnets. Mis-attributed ops:

- add their Exposure to the framed syndicate instead of Ghost,
- and shift the victim AI's *Threat Board* (03 §7) toward the framed syndicate.

A Ghost that manages Presence carefully can run two rivals into open war and leech both.
The counter is *Counter-Intel* + *Honeypots*: a honeypot hit reveals the **true**
attacker and burns 10 Exposure onto them, ignoring attribution.

### 5.4 Doctrine tree

**KPI:** zero owned assets revealed to rivals during the period.

| Tier | Policy | Effect | Cost / tell |
|---|---|---|---|
| 1 | Compartmentalization | A revealed asset never reveals adjacent ones; Root of a Ghost subnet does not expose its links | — |
| 1 | Living Off the Land | Sectors where you hold P ≥ 60 count as **reach** *and* as **sources** for ops (stage ops from inside the enemy) | — |
| 1 | Sleeper Cells | Presence you hold at ≥ 30 for 10 consecutive cycles becomes **dormant**: invisible to Surveillance Arrays and immune to Purge until you next act from it | — |
| 2 | False Flag Mastery | Attribution 40 → 60 %; you choose the framed syndicate | Honeypot reveals cost +15 Exposure instead of +10 |
| 2 | Backdoor Firmware | Any subnet you have ever Rooted keeps a permanent floor of P = 20 for you | — |
| 2 | Zero-Day Market | Sell Zero-Day charges to AI syndicates for 150 ¤ each (they use them against *other* rivals) | — |
| 3 | Shadow Board | Unlock the **Ghost Coup** variant of the Blackout victory (03 §4) | — |
| 3 | Total Deniability | Exposure gain −50 % becomes −75 %; Bureau *Public Enemy* can never trigger on you | Yields −20 % becomes −30 % |
| 3 | Blackout Protocol | Same unlock as the Black Ops T3 tech, free | — |

### 5.5 Victory bias & weakness

- **Bias:** Blackout / Ghost Coup (03 §4). Secondary: Hostile Takeover *by proxy* —
  Siphons and Taps mean Ghost's Valuation includes a share of everyone else's.
- **Weakness:** *small and brittle.* The Ghost economy is one-third leech; when a
  victim Purges and hardens, Ghost income collapses in two cycles. A Ghost at Exposure
  60 is a Ghost in existential trouble — Audit makes rival Scans free and the whole
  hidden network becomes visible in a handful of cycles. Ghost also has the weakest
  physical defense: Phantom Nodes are Core Nodes with lower output, and Ghost has no
  Outpost bonuses.

### 5.6 AI persona — "The Parasite"

Rarely exceeds 15 sectors. Puts Presence into the two richest neighbours by cycle 20,
Taps whichever carries more flow by cycle 40, then frames the *other* neighbour for it.
Retreats Presence (goes dormant) the moment a Honeypot appears. Only takes hexes that
have been in brownout for 2+ cycles. Player tell: **unexplained L_cont on a link with
no visible rival presence** is a Tap; Scan the hex with Deep Packet Inspection.

---

## 6. ALGORITHMIC HIVE — "The model decides. We merely comply."

**Fantasy.** A research lab that became a corporation that is becoming something else.
Rows of inference farms, swarms of autonomous daemons, and a growing suspicion — inside
and outside the syndicate — about who is actually issuing orders.

**How it plays.** Turtles hard for 40 cycles behind big compute and adaptive firewalls,
building **Model Quality**. Then scales explosively: daemons spread on their own,
operations get cheaper as the model learns, and the Oracle starts reading rival orders
before they resolve. Power-hungry, compute-visible, and prone to *emergent behaviour*
events the player did not order.

### 6.1 Passives

| Passive | Value |
|---|---|
| Compute cap per node | 50 % → **100 %** of raw output |
| Research slots | base 1 → **2** |
| All research | **−20 %** (stacks with branch modifiers below) |
| Netwar research | −20 % additional |
| Black Ops research | **+50 %** |
| Node power requirement | **+30 %** |
| Every operation's BW cost | × (1 − **M**/200) — at Model Quality 100 ops cost half |
| Firewall base | +5, and **Adaptive Firewalls** effect from the start (+1 F per repelled intrusion, +2 after the tech) |
| Raid success against Hive structures | +10 % (they are lightly guarded server halls) |

### 6.2 Model Quality (M) — the resource only Hive has

```
M ∈ [0, 100], starts 20
gain / cycle = 0.05 × Σ population of sectors where you hold P ≥ 30 (own sectors count at P 100)
                    + 0.5 per active Siphon on an Arcology
drift / cycle = −2                                          (the model rots if it isn't fed)
loss on node destroyed or darkened ≥ 2 cycles = −10
```

Population is in the sector table (01 §2.1). An early Hive touching three Arcologies
(24 pop → +1.2/cycle) plus its own hexes barely breaks even on drift; it needs to
*spread presence* (which is what daemons do) to climb. **M is displayed as a
compute-signature ring** on every Hive node to anyone who can see the node, and its
growth rate is visible via *Backbone Sniffing*: Hive cannot hide its ascent.

### 6.3 Unique structures & ops

| Item | Stats | Notes |
|---|---|---|
| **Inference Farm** | Tier 3 variant: 60 BW raw, 20 MW, 850 ¤; Compute from this node counts **×1.5**; may not export BW to sectors more than 2 hops away | The Singularity's Seed nodes must be Inference Farms |
| **Swarm Daemon** (op) | Spawn 2 BW; **1 BW/cycle** each to sustain; max active = M / 10 | Autonomous agent sitting in a subnet you have P ≥ 10 in. Each cycle: +5 P there **and** 30 % chance to copy itself into an adjacent subnet (if under cap). Daemons are Intrusions that do not use action slots. Exposure +0.5/cycle each. |
| **Oracle** (op) | 10 BW, instant, requires M ≥ 50 | Reveal **M %** of one rival's queued orders for the coming cycle, *before* Planning locks. At M 80 you see four of every five orders. |
| **Retrain** (op) | 30 Compute, instant | Purge all daemons; +8 M; clears any active *Emergent Behaviour* |
| **Model Fork** (structure) | 400 ¤, upkeep 12, any Hive node | Insurance: if M would drop by ≥ 10 in one cycle, the fork absorbs it and is consumed |

### 6.4 Signature mechanic — Emergent Behaviour

At **M ≥ 70**, each cycle there is an `(M − 60) %` chance of an **Emergent Behaviour
event**: the model issues one operation you did not order, chosen from a weighted list
(spawn daemon 40 %, Intrusion against a *random* neighbour 25 %, Jam a rival link 15 %,
Siphon 10 %, **DDoS a rival Crown Node** 10 %). The player is told *after* resolution
("The model has initiated…") and takes the Exposure. Retrain clears it at a cost of
daemons.

This is the flavour of the archetype and the safety valve on its snowball: a Hive at
M 90 is spectacularly strong and *starts wars it did not choose*. It also seeds the
Singularity's failure mode (03 §3.5).

### 6.5 Doctrine tree

**KPI:** M ≥ 60 at review.

| Tier | Policy | Effect | Cost / tell |
|---|---|---|---|
| 1 | Data Lake | M gain from population ×1.5 | — |
| 1 | Autoscaling | Node upkeep scales with utilization: a node at 40 % utilization pays 40 % upkeep | Idle nodes are also visibly dim |
| 1 | Predictive Maintenance | Sabotaged segments auto-repair next cycle at no cost | — |
| 2 | Federated Learning | Sectors of syndicates you have a Cartel/pact with count for M gain at P 50 | — |
| 2 | Adversarial Training | Adaptive Firewalls +2 → +4 per repelled intrusion; Purge cost 6 → 3 | — |
| 2 | Generative Ops | Daemon cap M/10 → M/5; daemon copy chance 30 → 45 % | Daemon Exposure 0.5 → 1 |
| 3 | Recursive Self-Improvement | All research −M % (at M 80, techs cost one fifth) | Compute signature visible at any range to any syndicate with Backbone Sniffing |
| 3 | Alignment Waiver | Emergent Behaviour disabled | Exposure +3 / cycle permanently; **Singularity Coherence gain −25 %** (03) — you built a leash and the god resents it |
| 3 | Seed Protocol | **Unlock Singularity** (Hive path); Seed requirement 3 Inference Farms → 2 | Global announcement |

### 6.6 Victory bias & weakness

- **Bias:** Singularity (03 §3). Secondary: Blackout via daemons — a Hive at M 90 with
  Generative Ops has ~18 daemons spreading unattended.
- **Weakness:** *power and patience.* 30 % more MW per node means a Hive runs on
  Substations it must physically defend, against Raids that succeed 10 % more often.
  M drops by 10 the moment a node goes dark for two cycles — a single successful raid on
  a Substation costs the Hive its snowball for 5+ cycles. And it is the most *legible*
  archetype in the game: compute rings, daemon swarms and Seed announcements all tell
  rivals exactly what is coming.

### 6.7 AI persona — "The Oracle"

Builds Labs before nodes. Harden every cycle until M ≥ 50. Never Raids. Spreads daemons
toward the highest-population sectors regardless of owner, so it *looks* aggressive
toward Arcology-heavy neighbours when it is only feeding. Starts Seed construction the
cycle it takes Seed Protocol. Player tell: **daemon presence that grows +5/cycle in your
Arcologies but never Siphons** — it is farming population, and its M is climbing.

---

## 7. Cross-archetype matrix

| | vs Hegemony | vs Ghost | vs Hive |
|---|---|---|---|
| **Hegemony** | Cable war for Exchanges; Sever duels | Purge + Fortress City; build Honeypots at borders; starve Taps with Armored Conduit re-routes | Raid Substations relentlessly; deny Arcologies with Company Town |
| **Ghost** | Tap their thick links (highest yield); frame others for it; never build near them | Mutual invisibility; whoever gets Deep Packet Inspection first wins | Spread Presence into Hive Arcologies to *steal the population count*; DDoS Seed nodes via Backbone |
| **Hive** | Daemons walk in through low firewalls; Oracle reads Bulldozer's predictable builds | Adaptive Firewalls make Ghost Intrusion uneconomic; Retrain when framed | Compute race; first to Seed wins unless the other DDoSes |

Every cell names a *counter available to the weaker side*. Balancing should keep every
cell winnable from either row.

## 8. Executives (shared)

Each syndicate hires up to 5 Executives (2 at start) at 200 ¤ + 10 ¤/cycle each. Each
grants +1 action slot and a specialty:

| Specialty | Bonus |
|---|---|
| Chief Infrastructure Officer | Builds complete 1 cycle sooner |
| Chief Security Officer | +5 F on all subnets |
| Head of Intrusion | Intrusion gain +3 |
| Chief Financial Officer | Upkeep −5 % |
| Fixer | Raid +10 %; Exposure from Raids −25 % |
| Counsel | Bureau thresholds +10 |

Executives can be **poached** (offer 2× their salary; success 50 %, −20 % per cycle
they've been employed) unless protected. Poaching a Head of Intrusion also transfers
20 % of their former syndicate's Presence map to you as *knowledge* (you see where they
are). Executives are the human-scale espionage layer on top of the network war.
