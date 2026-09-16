# Gridlock: Silicon Syndicate — Game Design Document

A cyberpunk grand-strategy 4X about corporate espionage, dual-layer (physical + cyber)
infrastructure warfare, territory expansion through fiber-optic networks, and a
bandwidth-driven macro-economy. Target engine: Unreal Engine 5.

## Documents

| # | File | What it covers |
|---|---|---|
| 00 | [Vision & Core Loop](docs/00-vision-and-core-loop.md) | Pitch, the three pillars as rules, glossary, map, cycle structure, core loop |
| 01 | [Macro-Economy & Network Expansion](docs/01-macro-economy-network.md) | Capital / Bandwidth / Power model, sector & infrastructure tables, flow solver and per-hop packet loss, territory as reach, the Exchange market, operations, Exposure, a fully worked cycle |
| 02 | [Corporate Ideologies & Tech Trees](docs/02-corporate-ideologies-tech.md) | Compute-funded shared tech tree (4 branches × 3 tiers); Hardware Hegemony, Ghost Protocol, Algorithmic Hive — passives, uniques, signature mechanics, 9-policy Doctrine trees, AI personas, matchup matrix |
| 03 | [Asymmetric Victory Conditions](docs/03-victory-conditions.md) | Hostile Takeover, Singularity, Blackout / Ghost Coup, The Charter, Valuation fallback — triggers, multi-cycle contests, telegraphing, failure states; the AI Threat Board, postures and Cartels |
| A | [Technical Notes (UE5)](docs/A-technical-notes-ue5.md) | Deterministic headless sim, flow solver, fog projection, Nanite/Lumen/Niagara presentation, UI, data-driven content, AI planner, vertical-slice scope |

## Reading order

Start with 00 for terms, then 01 — every later system is priced in Bandwidth and
Capital as defined there. 02 and 03 can be read independently. The worked example in
01 §11 is the single best five-minute introduction to how the game feels.

## Status

v0.1, 15 Sep 2026. Numbers are first-pass tuning targets meant to be validated in the
headless balance harness described in Appendix A §7.
