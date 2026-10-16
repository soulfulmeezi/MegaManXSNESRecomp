---
name: cosmicx-reference-fidelity
description: Use when porting or reviewing Cosmic X space-battle features against the user's original space-battle-sky.js. Prevent substitute art, missing logic, and wrong dimensions.
---

# Source fidelity gate
1. Obtain the actual user-supplied `space-battle-sky.js`; it is not assumed to exist in this repo. Capture a source hash, revision, and source location. If absent, stop the port and mark blocked rather than reproducing from memory.
2. Inventory source symbols: `FIGHTER_HALF`, `FIGHTER_ROWS`, `FIGHTER_PALETTES`, `HULL_PALETTES`, `buildHull`, `buildCity`, `bezier`, `newPath`, `step`, and `draw`. Map each to the active native code path.
3. Assert 10 authored fighter half-rows reflected to 20 rows, each padded to 36 columns; two palettes with every character key preserved; transparent dots and nearest-neighbor pixels. Compare every pixel and RGBA palette value, not visual similarity.
4. Compare native hull pixels to source `buildHull` outputs, including 8 horizontal and 14 vertical padding: body 190x52 => canvas 198x66, body 232x62 => canvas 240x76.
5. Trace all three path modes, the 60% pursuit branch, facing correction, target acquisition, projectile scaling, crash, sparks, city, and traffic. Record `exact / intentionally extended / missing` with reasons. New deterministic scheduler is an explicit user-requested addition, not license to change artwork.
6. Compare reference and game footage at equal reference-relative coordinates, matching frame or seeded scenario. Log nonmatches and stop before claiming fidelity.

## Required output
Feature-to-source-to-target mapping, dimensions/palette diff, screenshots or pixel diffs, measured behavior, build identifier, and explicit **UNTESTED** flags. Never use a reduced asset as an expedient fallback.
