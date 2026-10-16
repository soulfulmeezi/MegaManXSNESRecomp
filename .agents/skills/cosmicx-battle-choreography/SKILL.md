---
name: cosmicx-battle-choreography
description: Use for Cosmic X background battle animation, Bézier paths, chasing, visible ship counts, close flyby scheduling, and fixed-step simulation.
---

# Simulation/choreography
Read `space-battle-sky.js` before modifying motion. Match the quadratic Bézier position and tangent, `atan2(dy,dx)`, interpolated depth `s0+(s1-s0)*u`, and the source's path-state lifecycle.

Three original path modes:
- 0: rush toward camera, start 0.25–0.45x, finish 2.0–2.6x.
- 1: recede, start 1.6–2.4x, finish 0.25–0.45x.
- 2: cross edge-to-edge, start 0.7–1.2x, finish 0.7–1.4x.
- Where a suitable opposite-team leader is progressing through 0.05–0.5, reference `newPath` follows with ~60% chance by copying endpoints, scales and duration with a perturbed control point and delayed start. Keep that behavior and add explicit guarantees.

Guarantees required by the project, stronger than original random JS:
- Maintain exactly 8 active fighters, 4 per side; use deterministic spawn/recovery scheduling to keep at least 3 visible at every rendered tick, excluding genuine transition frames only if separately approved.
- Schedule a controlled close flyby at least once per 300 fixed 60-Hz ticks, rising from far depth to approximately 2.5–2.6x (about 90–94 px wide at width 256), exiting screen with brief shake.
- Use seeded tests over >=3600 ticks covering min-visible, max-flyby-gap, team counts, curve continuity, direction, left-facing correction, and no permanent stalled paths.
- Never measure 'active' as equivalent to 'visible'. Overlay the actual after-clipping screen count.
- Test fixed-step accumulator under 30/60/120Hz display; do not tie simulation speed to variable render FPS.

In Round 1, debug keys for cruiser and crash can be no-op placeholders only if visibly marked `UNIMPLEMENTED (future round)`, never falsely represented as functional.
