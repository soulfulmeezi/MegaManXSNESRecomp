---
name: cosmic-x-frame-pacing
description: Investigate smooth 60Hz presentation in Cosmic X with reproducible Highway timing, frame-tail metrics and separate SNES simulation, audio, rendering and presentation measurements.
---

# Cosmic X smooth frame-pacing engineer

Read docs/highway-pacing-2026-09-30.md, docs/highway-pacing-2026-09-30.json, tools/scripts/highway-coop-pacing.txt, tests/mmx_graphics_pacing_test.inc and src/mmx_rtl.c. The historical Highway stutters were traced to graphics decompression/scheduling and received a yield-checkpoint fix. Check whether the active branch contains that fix before hypothesizing new root causes.

## Measurement recipe
1. Record branch/build hash, pinned framework SHA, operating system, CPU/GPU, renderer backend, window size, aspect ratio, monitor refresh and VRR, VSync, audio, player mode and input route. Do not include local ROMs or save payloads in an evidence PR.
2. Gate the route on real Highway gameplay and allow stable warm-up. The historic report explicitly rejected measurements mistakenly taken at boot/title screens.
3. Keep the exact gameplay interval comparable for solo X, co-op and optional renderer changes. Treat any actor death or unpaired route segment as a confounder.
4. Distinguish SNES guest simulation duration and frame-clock behavior from audio work, decompression, CPU composition, GPU submission, present blocking and observed display intervals. A recorded movie frame count does not replace instrumented host timing.
5. Report p50, p95, p99, maximum, frame count and missed intervals relative to approximately 16.67 ms presentation at 60Hz. Investigate individual repeated spikes rather than only averages.
6. Do not force original guest logic to exactly 60Hz, skip emulated work, change character physics or desynchronize audio to make an FPS label read 60.

## Tool boundaries
Tracy is a separate optional profiler, NOT an agent skill. Add it only for a proven instrumentation need with license/version and hook review. Any execution, third-party installation or ROM-backed measurement requires separate explicit permission.

## Acceptance
Compare matched baseline and candidate runs on identical settings. Preserve graphics resource outputs, co-op determinism, rewind/save/rollback, audio continuity, background motion and native game timing. A claim of smooth 60fps requires actual measured presentation/visible gameplay; a passing synthetic test or encoded 60fps clip alone cannot establish it.

Provide a bottleneck hypothesis table with evidence, confidence, regression test and the smallest proposed code change. Keep profiling opt-in and free of overhead in release builds where possible.
