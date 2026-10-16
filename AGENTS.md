# Cosmic X agent instructions

When the task involves Cosmic X artwork, battle effects, game rendering, stage behavior, or reverse engineering, inspect the relevant project-local skill in `.agents/skills/` before proposing or editing code. Do not assume these instructions are themselves installed as ChatGPT plugins.

## Source-of-truth and no-substitution rule
- The user's original `space-battle-sky.js` is the source of truth for fighter rows, both palettes, cruiser hull generation, city generation, and simulation choreography. It may be provided separately from this repository. Locate the actual file and compare symbol-by-symbol; NEVER invent missing art data.
- The latest working Cosmic X Windows project may differ from this repository. Check the local active source, executable build label, paths, relevant commits, and binding tables before editing. Never call a GitHub change an applied local build unless it was actually built and launched.
- If a renderer feature is not possible, explain exactly which API or measured performance limitation prevents it BEFORE any change of design. Atlas frame swapping at faithful dimensions is the accepted fallback. Substituting tiny fixed ship sprites is forbidden.
- Work in isolated rounds. Do not silently add Round 2–4 features to Round 1.

## Acceptance and verification
- Every round must expose on-screen build label, visible fighter count, visible cruiser count, frame time in milliseconds; provide working on-demand cruiser, flyby, crash controls that do not collide with existing keybindings.
- Every round includes reproducible BEFORE/AFTER instructions and logs actual build, runtime, and observed frame-time numbers. If not executed, mark each result **UNTESTED**.
- Simulation: 60 fixed ticks/s. Round 1: 8 active fighters (4/team), >=3 visible on screen, and a ~90-px close flyby with brief shake at least once every 300 ticks, at 256 reference width. Preserve all three original quadratic Bézier path modes and left-facing upright correction.
- Do not shrink the art to solve performance. Profile before optimizing. Frame budget for 60 FPS: 16.67 ms/frame.
- Prefer a deterministic test seed and automated assertions, plus full-resolution capture comparisons.
- Never distribute proprietary ROMs, reverse-engineered third-party assets, or third-party executable contents. Study legitimately provided material for behavior and technique; create original assets.

## Select skills by task
- Exact original artwork/logic comparison: `cosmicx-reference-fidelity`
- Fighter pixels/rotations/scale: `cosmicx-sprite-engineering`
- Paths, pursuit, density, scheduling: `cosmicx-battle-choreography`
- Native renderer/atlas: `cosmicx-renderer-integration`
- Frame profiling and regression evidence: `cosmicx-performance-qa`
- Reference EXE and symbols: `cosmicx-reverse-engineering`
- City, roads, parallax, traffic: `cosmicx-level-design`
- Combat hit effects, sounds, smoke, collisions: `cosmicx-effects-animation`

If an important repeatable failure appears, propose a focused new or revised skill with a reproducible example, a pass/fail criterion, and a small regression test. Review the skill's source before using any third-party scripts.
