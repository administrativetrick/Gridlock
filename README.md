# Gridlock: Silicon Syndicate

A cyberpunk grand-strategy 4X about corporate espionage, dual-layer (physical + cyber)
infrastructure warfare, territory expansion through fiber-optic networks, and a
bandwidth-driven macro-economy. Unreal Engine 5.8 client over an engine-agnostic C++20 simulation.

## Layout

| Path | What |
|---|---|
| `docs/` | The design document (00 vision, 01 economy, 02 archetypes, 03 victories, A technical notes) |
| `sim/` | **GridlockSim** — deterministic, headless C++20 library. Map generation, the ten-step cycle resolution, per-hop-loss flow solver, dual-layer control, archetypes, tech and doctrine trees, four victory paths, fog-limited AI rivals. Public API: `sim/include/gridlock/Game.h`. |
| `ue5/` | **The game.** UE 5.8 project: 3D hex city, fiber links with visible packet loss, fog, structure markers, click-to-order, wireframe HUD. Links `gridlock_sim.lib`. |
| `cli/` | Developer harness: a console front end over the same library. Used for headless balance runs (`--headless <seed> <cycles>`) and scripted checks. Not the game. |
| `tests/` | Fail-fast test suite (worked example from docs/01 §11, determinism, validation, decay, fog, exchange peering, 80-cycle AI game). Runs on every `build.bat`. |

## Build

Prerequisites on this machine: Visual Studio 2022 Build Tools (MSVC 14.44), CMake + Ninja from
the 2019 Build Tools, Unreal Engine 5.8 at `D:\Program Files\Epic Games\UE_5.8`.

```bat
build.bat
```

Configures and compiles the sim library, the console harness and the tests with MSVC/Ninja
(Release), then runs the tests. The build aborts on any failing check.

```bat
"D:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GridlockEditor Win64 Development -Project="%CD%\ue5\Gridlock.uproject"
```

Compiles the UE5 game module against the freshly built `build\gridlock_sim.lib`.

## Play

```bat
run.bat hegemony
```

`run.bat [hegemony|ghost|hive] [seed]` launches the UE5 client as a standalone window. Press
**F1** in game for the key map; **V** opens the Board Room; **Space** ends the cycle.

The design's core loop in play: click a hex in your network, press **L**, click a destination
to lay fiber (rights are bought along the route and the projected survival is shown). Watch the
white packets fall off long links. Build a node (**1**/**2**) where a sector browns out. Press
**I** and click a rival hex to push presence into its subnet; **C** scans; **H** hardens your own.

## Status

Vertical slice per docs/A §8: all systems of docs 01–03 are implemented and exercised by the
AI in headless runs (Hostile Takeover and Valuation endings observed; Singularity, Blackout and
Charter paths are wired and validated but rarer). Presentation uses procedural hex prisms and
engine basic shapes with dynamic colours, Lumen with a single key light, and a drawn-text HUD.
The Nanite brutalist kitbash, Niagara flow ribbons and the CommonUI Board Room are the next art
layer. Balance is first-pass; see `--headless` traces.

Two deliberate deviations from the design text, both found in implementation:

- Within a priority class the flow solver serves sinks **nearest-first**, not richest-first
  (docs/01 §4.1 now says so); otherwise the worked example in §11 does not hold.
- The Exchange peering fee is served **before Normal sectors** rather than after all sectors,
  so a large saturated empire can still peer. Sales still come last.
