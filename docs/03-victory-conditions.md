# 03 — Asymmetric Victory Conditions

**Module 3 of 3.** Four ways to end the game, plus a fallback. For each: the metric,
the phases, how it **telegraphs** (what rivals can see, and only if their network
reaches it), how it is **contested**, and its failure state. Then the model AI
syndicates use to decide when and how to fight a leader.

Design rules for every ending:

1. **A victory has a hidden build-up and a loud finish.** The build-up rewards
   Information as Munition: rivals who invested in vision see it early. The finish is
   announced to everyone, so nobody loses to a surprise.
2. **Every finish is a multi-cycle contest, never a threshold check.** The leader has
   to *hold* something under pressure for N cycles. Rivals get real verbs to spend.
3. **Every counter has a counter.** The leader can prepare for the contest; a rival
   who reads the build-up can prepare their counter; the leader can read *that*.
4. **Failure has a price but is not elimination.** A failed launch sets a syndicate
   back 15–25 cycles. That is the cost of going early.

---

## 1. Overview

| # | Ending | Fantasy | Metric | Contest length | Natural archetype | Primary counter |
|---|---|---|---|---|---|---|
| 2 | **Hostile Takeover** | Buy the city | Valuation share ≥ 51 % | 8 cycles (Proxy Fight) | Hegemony | Buy shares, poison pills, devalue their assets |
| 3 | **Singularity** | Upload a god | Coherence 100 | 6 cycles (Launch) after 10 (Training) | Hive | Cut the Exchange feed, root the Seed, Bureau Kill Switch |
| 4 | **Blackout / Ghost Coup** | Cut every light at once | Root every Crown Node; hold the dark | 3 cycles (Blackout) | Ghost | Purge, honeypots, redundant Crown power |
| 5 | **The Charter** | Make it legal | 5 of 9 District seats; pass the vote | 4 cycles (Vote) | Any; favours wide, calm players | Bribe seats back, expose bribery, contest districts |
| 6 | **Valuation (fallback)** | Highest close | Valuation at cycle 200 | — | — | — |

Only **one victory phase may be active per syndicate at a time**. Two syndicates may be
in phases simultaneously — the second announcement is the most dramatic moment the game
can produce and should be scored accordingly.

---

## 2. Hostile Takeover — economic & physical dominance

### 2.1 Valuation

Every cycle, each syndicate's **Valuation** is computed publicly (the ticker on the
top bar). Public means *every* syndicate sees every Valuation — money is the one thing
this city does not hide.

```
Valuation_s = Σ over controlled sectors  ( Capital base × C/100 × 5 )
            + Σ over nodes  ( build cost × 0.5 × powered fraction )
            + Σ over links  ( build cost × 0.25 )
            + Capital × 0.10
            + Σ over Siphons & Taps  ( 5 cycles of their current skim )
            + 200 per Exchange peered

Share_s     = Valuation_s / Σ Valuation over all syndicates
```

Sector value is Integrity-weighted, so browning out a rival's Financial District takes
Valuation off them the *same cycle*. Nodes count only while powered. **Dark
infrastructure is worthless on paper**, which is exactly why Substations are raided
during a Proxy Fight.

### 2.2 Trigger — Tender Offer

Requires *Tender Offer Authority* (Corporate T3) and, at the moment of trigger:

- Share ≥ **40 %** (35 % with Hegemony's *Monopoly Mandate*),
- peered at ≥ **2 Exchanges**,
- Exposure < 60 (the market does not trust a syndicate under Audit).

Announcement: **global**, with a countdown: "**[Syndicate]** has filed a Tender Offer.
Proxy Fight resolves in 8 cycles."

### 2.3 Contest — the Proxy Fight (8 cycles)

Each cycle a **Share Ledger** updates. The bidder wins if Share ≥ **51 %** (45 % with
Monopoly Mandate) at the end of cycle 8. Share moves by Valuation as normal *plus*
these Proxy Fight verbs, available to everyone:

| Verb | Who | Cost | Effect |
|---|---|---|---|
| **Buy Shares** | anyone | 100 ¤ per 1 % | Adds 1 % to your Share, removes it pro rata from all others. −30 % cost with *Buyback Program*. Bidder buying is the offensive tool; rivals buying is the defensive one. |
| **Poison Pill** | rivals | 250 ¤, once per rival per fight | Your own Valuation ×1.25 for the remainder of the fight (dilutes bidder's share) |
| **Regulatory Complaint** | rivals | 150 ¤ + 5 Exposure to filer | Bidder Exposure +15. If bidder reaches Audit (60), **the fight pauses** for the Audit's 3 cycles (countdown resumes after) |
| **Greenmail** | bidder | 300 ¤ to a chosen rival | That rival (AI) abstains from Proxy Fight verbs for 2 cycles (AI accepts if its own Share < 15 %) |
| **Devalue** | rivals | ordinary ops | Brownouts, darkened nodes and Roots against the bidder all reduce Valuation the same cycle |

### 2.4 Telegraphing

| Signal | Who sees it |
|---|---|
| Valuation ticker | Everyone, always |
| Tender Offer Authority researched | Anyone with Deep Packet Inspection who Scans a bidder node hex (Labs display current research) |
| Second Exchange peering | Anyone connected to that Exchange |
| Capital stockpile ≥ 2000 ¤ (war chest for Buy Shares) | Anyone with *Backbone Sniffing* (¤ flows through Exchanges) |

### 2.5 Failure

Share < threshold at cycle 8: **Offer lapses.** Bidder's Exposure +20, all Buy Shares
spent are lost, and a **15-cycle cooldown** before another Tender Offer. Rivals'
Poison Pills expire.

### 2.6 Why it works with the pillars

Valuation is the only *fully public* number in the game, so the Takeover is the ending
that tests the **Bandwidth Economy** most directly: the bidder must keep every sector
lit and every node powered for 8 cycles while every rival is trying to brown them out.
Hegemony's thick redundant networks are built for it; a thin greedy network falls apart
in the fight it started.

---

## 3. Singularity — launch a rogue AI into the global net

### 3.1 Requirements

- *Seed Protocol* doctrine (Hive) **or** the tech *Kill Chain* + *Hyperscale Cooling*
  + *Backbone Sniffing* (any archetype; the "hard way").
- **Seed cluster:** 3 Tier 3 nodes (2 Inference Farms for Hive with Seed Protocol)
  arranged so that **each has two edge-disjoint paths** to at least one peered
  Exchange (a ring, not a star).
- Exposure < 80 at Training start (the Bureau's grid cut would end it anyway).

### 3.2 Phase 1 — Training (10 cycles, covert)

Allocate **≥ 150 Compute / cycle** across the Seed cluster for 10 consecutive cycles.
A cycle below 150 does not reset the count but does not advance it. Training is
**not announced.** The Seed grows a **Coherence** value from 0 to 40 over the phase.

### 3.3 Phase 2 — Launch (6 cycles, global announcement)

Coherence starts the launch at 40 and must reach **100**.

```
each Launch cycle:
  fed        = BW delivered to the Exchange hexes from the Seed cluster (target 200)
  Coherence += 20 × min(1, fed / 200)                 // full feed: +20 → 100 by cycle 3 of 6
  Coherence −= 10 × (Seed nodes currently Rooted or dark)
  Coherence −= 15 if the Bureau Kill Switch fires this cycle
  Coherence −= 5  per DDoS landing on a Seed node this cycle
  with Alignment Waiver: all Coherence gains × 0.75

win  : Coherence ≥ 100 at the end of any Launch cycle
fail : Coherence ≤ 0, or fewer than 2 Seed nodes lit, or Launch cycle 6 ends below 100
```

A perfect launch finishes in 3 cycles. A contested one has 6 cycles of runway, so the
defenders have to *sustain* pressure, not land one lucky Raid.

### 3.4 Contest — what rivals do

| Counter | How |
|---|---|
| **Starve the feed** | Jam, Sever, Raid or Tap the links from Seed to Exchange. The ring requirement means they need two breaks per node. |
| **Root the Seed** | Intrusion on Seed subnets during *Training* (they are Tier 3, F 50+, adaptive). A Root landing during Launch is −10 Coherence per cycle held. |
| **Darken the Seed** | Raid Substations. Hive nodes need +30 % power; a Seed cluster is ~55 MW of Substations. |
| **DDoS** | 8 BW each via Backbone reach, −5 Coherence apiece. Cheap and stackable — the mass-participation counter. |
| **Bureau Kill Switch** | Any syndicate holding **3 District seats** (§5) may petition; the Bureau fires the Kill Switch once per launch, −15 Coherence, cycle 2 or later. |
| **Blackout the Exchange** | The nuclear counter: brown out or DDoS the Exchange hex itself. Bureau firewall 60 makes it expensive and hits everyone peered there. |

### 3.5 Failure states

- **Aborted** (Coherence ≤ 0 or Seed lost): all Seed nodes **burn out** — dark for 10
  cycles, firewall 0. Hive loses 30 M. Exposure +25.
- **Misaligned launch** (Hive only): if M < 70 at Launch start, each Launch cycle has a
  10 % chance the Seed **defects**: the launch ends and the Seed becomes the
  **Emergent Intelligence**, a neutral fifth faction owning the Seed hexes, that spawns
  daemons into every syndicate's subnets and DDoSes the highest-Valuation syndicate each
  cycle until its nodes are rooted or darkened. The Hive keeps playing, diminished.
  This is the game's best story generator and must never be a mystery: the Launch button
  shows the defect chance.

### 3.6 Telegraphing

| Signal | Who sees it |
|---|---|
| Three Tier 3 nodes in a ring | Anyone whose vision touches them (Scan, Array, adjacency) |
| Compute ≥ 150 / cycle | *Compute rings* on the nodes to anyone who can see them; volume via *Backbone Sniffing* |
| Seed Protocol taken | **Global** (doctrines are public) |
| Launch | **Global** with Coherence bar visible to all |

A Hive that hides its Seed behind a Ghost-style Cutout ring and never peers where its
enemies peer can keep Training dark for the full 10 cycles. That is the intended
reward for investing in the Information pillar *defensively*.

---

## 4. Blackout / Ghost Coup — infiltration

### 4.1 Requirements

- *Blackout Protocol* (Black Ops T3, or Ghost doctrine) — the **Blackout** variant, or
- *Shadow Board* (Ghost doctrine T3) — the **Ghost Coup** variant.

### 4.2 Trigger

Hold **Root** simultaneously on the **Crown Node subnet of every rival syndicate**
(Crown Nodes need *Kill Chain* to root). Roots expire if the owner Purges, so the
trigger is a race to land the last one while holding the rest. The cycle you hold all
of them, you may declare **Blackout** (global announcement).

### 4.3 Contest — the Blackout (3 cycles)

```
Blackout cycles 1–3:
  every rival Crown Node outputs 0                     (you are root; you shut it)
  every rival node within 3 hops of its Crown outputs −50 %
  rivals cannot Buy BW at Exchanges
  YOU may claim any rival sector in brownout that is adjacent to your network
       or in which you hold P ≥ 50 — no C < 50 requirement, no takeover Exposure

each cycle a rival may:
  Purge its Crown subnet (cost 6 BW — but its Crown is dark; must come from another node
       or the Buffer, which is what the Buffer is for)                     → ends the Blackout for that rival
  Raid your nearest Outpost/Node (Bureau grants a free Raid roll to every rival — the city hates blackouts)
  Fail-over: if a rival's Crown has a Substation-independent power source (Private Grid, or
       a second Substation ≥ 3 hexes away), its Crown outputs 50 % instead of 0
```

**Blackout victory:** at the end of cycle 3, you control **≥ 35 % of all sectors on
the map**, *or* every rival syndicate's Crown Node is still dark (a total decapitation).

**Ghost Coup variant (Shadow Board):** instead of claiming sectors, each Blackout cycle
you may **install a Shadow Director** on one rival Board for 400 ¤. A syndicate with a
Shadow Director becomes your **subsidiary**: its Valuation counts as yours, its AI stops
contesting you, and its sectors are neither lost nor gained — they are *owned*. Ghost
Coup victory: **every rival is a subsidiary** at the end of cycle 3, or the sum of your
own and subsidiary Valuation ≥ 60 %.

### 4.4 Telegraphing

| Signal | Who sees it |
|---|---|
| Kill Chain researched | Deep Packet Inspection Scan of a Lab hex |
| Presence climbing in a Crown subnet | The owner's Surveillance Array, or a Honeypot hit |
| Simultaneous Roots on multiple Crowns | Each victim knows *their own* Crown is rooted (F drops to 0, an unmissable alarm). They do **not** know the others are, unless they share vision (Cartel, §7.5) — the Blackout's lead-up is the least visible of any ending, by design. |
| Blackout declared | **Global** |

### 4.5 Failure

If the Blackout ends without victory: all your Roots are cleared, Exposure **+40**
(the city knows who turned the lights off), Bureau *Enforcement* for 5 cycles
regardless of Exposure, and a 25-cycle cooldown. A Ghost that fails a Blackout is
found, which is the one thing Ghost cannot survive: the intended stakes.

---

## 5. The Charter — regulatory capture

The legitimacy ending, for the player who would rather own the rulebook than the map.
It rewards **wide, calm, population-heavy** play and offers the non-Hive, non-Ghost
syndicate a second path besides the Takeover.

### 5.1 Districts and seats

The city is divided into **9 Districts** (contiguous regions of 40–70 hexes, drawn at
map generation, shown as faint boundaries). Each has a **Municipal Board seat**.

A seat is **held** by a syndicate that, for **10 consecutive cycles**, controls sectors
comprising ≥ **60 % of the District's population** at C ≥ 70. It stays held while the
condition holds and flips after 3 consecutive cycles below it.

Alternatively, a seat can be **bought** (*Lobbying Office* tech): 40 ¤/cycle sustained
bribe, +2 Exposure/cycle. A bought seat is lost the cycle the bribe stops, or when a
rival files an **Ethics Complaint** (100 ¤, requires P ≥ 30 in *any* sector of the
District) — the complaint has a 50 % chance to void the bribe and add +15 Exposure to
the briber.

### 5.2 Trigger — Consolidation Motion

Requires *Consolidation Lobby* (Corporate T3) and **5 of 9 seats held or bought**.
Global announcement. **Vote resolves in 4 cycles.**

### 5.3 Contest — the Vote (4 cycles)

Each cycle the motion's tally = seats held by the mover. Rivals may:

| Verb | Cost | Effect |
|---|---|---|
| **Counter-bribe** | 80 ¤/cycle per seat | Contest a bought seat: highest sustained bribe holds it; ties keep the incumbent |
| **Ethics Complaint** | as §5.1 | Void a bought seat (50 %) |
| **Contest the District** | ordinary ops | Brown out enough of the mover's sectors to drop them below 60 % population — the seat flips only after 3 cycles below, so this must start *before* the motion |
| **Bureau Referral** | 200 ¤, one per rival per vote | Mover's Exposure +10; at Audit the vote **pauses** for 3 cycles |

**Charter victory:** ≥ 5 seats at the end of cycle 4 → the Consolidation Charter passes
and your syndicate becomes the city's licensed network utility.

### 5.4 Telegraphing

Seat holdings are **public** (the Municipal Board sits in the Bureau's building), so
the Charter is the *most* visible build-up of any ending — its balance comes from how
slow and expensive it is to contest, not from secrecy. The intended rhythm: AIs tolerate
a 3-seat player, hedge at 4, and contest seats hard at 5.

### 5.5 Failure

Motion fails: bought seats are voided, Exposure +15, 20-cycle cooldown.

---

## 6. Fallback — Valuation close

If no victory has resolved by **cycle 200**, the highest Valuation wins. Tiebreak:
sectors controlled, then Exchanges peered. The game shows "cycles to close" from
cycle 150 onward so the late game has a clock everyone can plan around.

---

## 7. How rival AI syndicates contest a leader

### 7.1 The Threat Board

Every AI keeps a **Threat Board**: for each rival `r` and each ending `e`, a
**Victory Proximity** `VP[r][e] ∈ [0,100]`, computed **only from information that AI's
own network can see**. This is the single most important AI design rule in the game:
**AIs are not omniscient, and denying them vision denies them a reaction.**

| Ending | VP inputs | Visibility |
|---|---|---|
| Takeover | Share (public); Exchanges peered (if AI peers there too); Tender Offer Authority (if scanned) | Mostly public → hardest to sneak |
| Singularity | Tier 3 count seen; Compute rings seen; Seed Protocol (public); Backbone volumes (if sniffing) | Requires investment → sneakable |
| Blackout | Own Crown P observed; Kill Chain seen; other victims' reports (Cartel only) | Requires Array/Honeypot → very sneakable |
| Charter | Seats (public) | Public |

```
VP is a weighted sum of observed inputs, each mapped to 0–100 against the trigger
threshold, times a confidence factor = (fraction of inputs actually observed).
Stale observations decay 5 points per cycle since last seen.
```

### 7.2 Postures

The AI's posture toward rival `r` is set by `max_e VP[r][e]`:

| VP | Posture | Behaviour |
|---|---|---|
| < 40 | **Expand** | Ignore; pursue own plan |
| 40–65 | **Hedge** | Buy vision on `r` (Scan, Arrays on border, Backbone Sniffing); pre-position counters (Honeypots if Blackout suspected; save Capital if Takeover) |
| 65–85 | **Contain** | Active counters from the table for the suspected ending; propose a **Cartel** to other AIs with VP ≥ 50 on the same target |
| > 85 or phase active | **Desperation** | Spend down to upkeep on counters; accept Exposure up to 75; ignore own victory progress |

### 7.3 Counter selection by ending

| Suspected ending | Preferred counters (in order) |
|---|---|
| Takeover | Stockpile Capital for Buy Shares; brown out highest-value border sectors; Regulatory Complaint if bidder X > 40 |
| Singularity | Push Presence into any Tier 3 subnet seen; Raid Substations near them; DDoS on Launch; petition Kill Switch if 3 seats |
| Blackout | Harden Crown every cycle; build Honeypot in Crown hex; second Substation for Crown; Purge Crown on any P ≥ 30 |
| Charter | Counter-bribe the cheapest contested seat; Ethics Complaints; brown out sectors in the 5th District |

### 7.4 Archetype flavour on top

| AI archetype | Prefers | Avoids |
|---|---|---|
| Bulldozer (Hegemony) | Sever, Raid, Eminent Domain into the leader's corridors; Buy Shares | Intrusion |
| Parasite (Ghost) | Root Seed/Crown; Tap the leader's feed; **frame the leader** for ops against a third AI to pull it into the Cartel | Anything physical |
| Oracle (Hive) | DDoS spam via Backbone; Oracle to read the leader's orders; daemons into Seed subnets | Raid |

### 7.5 Cartels

When **two or more AIs** hold Contain posture toward the same target they form a
**Cartel** (announced: "Anti-trust consortium formed against **[Syndicate]**"):

- **Shared vision** among members (this is how a Blackout gets seen — victims compare
  notes on Crown intrusions).
- **Exchange sanctions:** at any Exchange a member peers, the target's sell price is
  −50 % and buy price +100 %.
- **Right-of-Way Denial** on all target links through member hexes.
- **Coordinated cycle:** members alternate who spends on counters so the target faces
  pressure every cycle.

A Cartel dissolves when the target's max VP has been < 50 for 5 cycles, or when a
**member's own VP > 65** (the others turn on it; the leader can engineer this with
Ghost's attribution or by Greenmailing a member into a Takeover of its own).

**Players may join Cartels** against an AI leader through the Board Room screen, on the
same terms, and may be offered membership by AIs.

### 7.6 Fairness rules

- **Every AI counter has a one-cycle tell.** Capital stockpiling shows in the ticker;
  Cartel formation is announced; a Raid needs an Outpost within 2 hexes built the cycle
  before; DDoS needs Backbone reach, visible at the Exchange. A player who is watching
  can always see the punch coming.
- **No hidden-information cheating.** The AI's Threat Board uses the same fog the
  player has. Debug overlay shows the AI's VP matrix so designers can verify this.
- **Rubber-banding is forbidden.** No bonus resources for trailing AIs. Catch-up comes
  from Cartel economics (sanctions and shared vision), which the leader can break.
- **Desperation is capped.** An AI never spends below its own upkeep, so it cannot
  suicide its economy to stop you — it can only make you earn it.

## 8. The Board Room screen

A single UI surface, opened from the top bar, that shows:

- your five paths as progress bars with the exact unmet requirements listed,
- **what each rival can see of your progress**, derived from their observed inputs
  (so the player experiences the information pillar as "they know about the Seed but
  not the ring"),
- every rival's *estimated* proximity as your own network sees it, with confidence,
- active phases (yours or theirs) with the cycle countdown and every verb you can spend,
- Cartel status and invitations.

The Board Room is where the last 40 cycles of the game are played. It should look like
the villain's war room the fantasy promises: holographic city, five glowing paths, and a
ticker that never stops.
