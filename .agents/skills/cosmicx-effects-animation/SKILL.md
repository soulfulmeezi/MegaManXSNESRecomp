---
name: cosmicx-effects-animation
description: Use for Cosmic X laser bolts, projectile collisions, crashes, explosions, sparks, shake, engine trails, and responsive sound effects.
---

# Combat VFX and animation
- Port the source's collision lifecycle without scripted substitutes: scaled heading-fired bolts, enemy hit check, tumble velocities, angular spin, gravity, smoke/fire trail, impact at skyline, large explosion, spark ring, debris and camera shake.
- Preserve source-relative bolt dimensions: length max(4, floor(7*scale)); width 1, 2 or 3 based on thresholds 0.6 and 1.3; white leading pixel and team-specific bolt color. Validate 0.25x and 2.6x.
- Source big explosions: 24-tick lifetime, expanding colored core, ten radial spark points, secondary debris, flash and six-tick shake; use code-accurate timing before aesthetic extensions.
- Control shake only in the target background/camera according to renderer constraints; avoid gameplay collision or HUD displacement. A flyby triggers ~5 ticks of shake after scale >2.1; city crash triggers the big effect.
- Sound changes (such as Zero's saber swing/landing) must be keyed to actual animation state transitions, verified against available assets and audio API. Do not double-play X's normal buster when special wave is active.
- Add direct test triggers for cruiser, close flyby and crash only after binding collision check. Capture each trigger from a cold start in seconds.
- Test 60Hz cadence, VFX overdraw, emitter caps, collision edge cases and co-op/gameplay interference. Mark sound or VFX not executed as `UNTESTED`.

Required output: event state diagram, source-to-native behavior mapping, measured effect timeline, sound-trigger table when relevant, and 60fps before/after verification.
