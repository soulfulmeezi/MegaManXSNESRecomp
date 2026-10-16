---
name: cosmicx-performance-qa
description: Use to profile Cosmic X frame timing, fixed-step cadence, draw cost, atlas fallback thresholds, debug overlay, and repeatable before/after tests.
---

# Performance and QA gate
- 60-Hz frame budget is 16.67ms, but assess actual frame durations (p50/p95/p99/max and dropped frames), simulation update ms, native draw ms, and CPU/GPU bottlenecks separately. State the observed values and machine/build/viewport.
- Overlay on screen at every implementation round: precise build label, fighters on screen, cruisers on screen, latest frame time in ms. Do not substitute FPS for ms.
- Record before and after on the same camera point, viewport size, simulation seed, and 10-second timeline. Capture a full-resolution screenshot and ideally 60fps video; list exact hotkeys used and confirm them against existing bindings.
- Automated tests: at least three visible fighters at each eligible simulation tick, eight active split 4+4, close-pass separation <=300 ticks, palette pixel equivalence, hull canvas dimensions, and debug actions for cruiser, flyby, crash.
- If frame time exceeds 16.67ms, report exact measurement and first optimize unnecessary work; then switch sprite rendering to faithful pre-rendered atlas while preserving the original 36x20 source and 0.25–2.6 scale endpoints. Never reduce size, omit cruisers, or suppress effects to get under budget without explicit user consent.
- Unavailable GPU, Windows executable or runtime => `UNTESTED`, not 'PASS'. Debug hooks not yet wired => `UNIMPLEMENTED`, not 'TESTED'.

Required deliverable: a compact measured before/after table, controls, screenshots/video paths, test command/output, deviations, and round-specific acceptance verdict.
