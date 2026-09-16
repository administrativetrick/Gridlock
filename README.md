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

```bat
"D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\ue5\Gridlock.uproject" -run=GLAssets
```

Regenerates every asset from code: the tileable PBR textures (`ue5/Content/Textures/T_*`,
synthesised by `GLTextureKit` from gradient noise with domain warping and cellular noise: 1024²
concrete colour / masks / normal with two-level panels, bolt dents, chipped seams, cracks and rust
streaks; a riveted, scratched metal set with hazard stripes; grime with puddle masks; a window-cell
atlas with frames, spandrels, blinds, curtains and three hue families; neon signage glyph bars; and a
datacenter floor plan drawn as a circuit board for the tile tops: rack rows with hot and cold
aisles, cable trays, 45°-routed traces into a core switch pad, vias, silkscreen labels and a
ground-plane via grid, with a build-order channel so a sector's plan fills in as it develops:
core pad on claim, trays with integrity, racks as nodes, substations, racks and labs are built),
the materials (`ue5/Content/Materials/M_Neon`, `M_NeonInst`,
`M_GlowInst`, `M_Holo`, whose surface is a custom HLSL node blending concrete and metal tri-planar in
world space with a world-space normal, puddle wetness, flickering lit windows, animated signage bands
and owner-coloured traces) and the procedural kitbash library (`ue5/Content/Meshes/SM_*`: towers, slabs, arcologies,
factories, docks, sprawl, undercity, the Exchange spire, every infrastructure prop, the fiber cable
and the bevelled hex tile, all built by `GLMeshKit`). The assets are committed, so this is only
needed after editing `GLTextureKit.cpp`, `GLMeshKit.cpp` or `GLMaterialCommandlet.cpp`.

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
Charter paths are wired and validated but rarer). Presentation is generated from code, no
hand-authored content: a procedural kitbash library gives every sector type its own brutalist
architecture (setback towers, twin shafts with skybridges, finned slabs, stepped arcologies,
factories with tanks and chimneys, dock cranes with containers, sprawl clusters, undercity
pipework, the Exchange obelisk) and every structure its own prop (finned server towers,
transformers, pylons, bunkers, dishes, racks), instanced with per-instance syndicate glow and
neon rims traced on every edge, surfaced with generated concrete, grime and normal maps sampled
tri-planar in world space, lit window cells on every wall tinted toward the owner, and glowing
circuit traces on the tile tops. Fiber is emissive cable geometry with packet spheres that drop
away in proportion to loss; presence shows as holographic discs; the ground is a wet reflective
plane under Lumen, with height fog and a bloom / fringe / grain / vignette grade. The interface is
Slate built in C++: gauged top bar, clickable command toolbar with build and ops menus, dual-layer
inspector with integrity, delivery, firewall and presence bars, research and doctrine windows, the
Board Room, and the log. Balance is first-pass; see `--headless` traces.

Two deliberate deviations from the design text, both found in implementation:

- Within a priority class the flow solver serves sinks **nearest-first**, not richest-first
  (docs/01 §4.1 now says so); otherwise the worked example in §11 does not hold.
- The Exchange peering fee is served **before Normal sectors** rather than after all sectors,
  so a large saturated empire can still peer. Sales still come last.
