---
name: cosmic-x-analysis
description: Analyze MegaManXSNESRecomp and Cosmic X source, SNES formats, rendering, frame pacing, and CPU-companion behavior using the repository's existing tooling and evidence boundaries.
---

# Cosmic X source and behavior analysis

Identify the active branch and exact commit first. Main does not include every CPU-companion or palette experiment. Read `CONTRIBUTING.md` and the relevant section of `docs/cosmic-x-tooling-plan.md`; preserve the pinned `snesrecomp` and `recomp-ui` gitlinks unless dependency work is requested.

## Choose the existing integration point

- For binary formats, inspect `tools/extract_zero.py`, `tools/extract_x_weapons.py`, and `src/mmx_source_assets.cpp` before proposing another extractor. Distinguish ROM offsets, CPU bank:address, VRAM words and byte offsets. Record mapping, revision, endianness, bit depth, compression and bounds as known or unknown.
- For 65816 analysis, preserve M/X accumulator/index width and bank/direct-page context. Do not turn unobserved bytes into confident code boundaries. Treat an unresolved dynamic branch as an evidence gap.
- For sprites/palettes, inspect `src/mmx_render_assets.h` and its implementation. Separate decoded tiles, palette indices, BGR555 colors, transparency and per-seat ownership. Use original synthetic tiles and palettes for tests.
- For backgrounds, inspect `src/mmx_renderer.c`, `src/mmx_wide_policy.c`, and `docs/custom-renderer-spike.md`. Trace world-column resource selection and layer scrolling before changing camera arithmetic. Keep render coordinates separate from physics coordinates.
- For CPU companions, inspect `src/mmx_coop.c`, `src/mmx_zero.c`, and the active branch's tests. Prefer native controller decisions and collision queries; do not silently teleport, alter HP, or overwrite velocity to make a test pass. X3 charged buster and Modern saber/air-dash behavior are distinct. Check button-edge rearming, safe landing/headroom, bounded recovery, and survivor input handoff. Do not enable experimental traversal by default without validated evidence.
- For frame pacing, start from `docs/highway-pacing-2026-09-30.md`. Gate measurement on actual gameplay, preserve guest timing, and distinguish update time, rendering, presentation waits and audio. Compare matched routes, settings and build hashes. Report tail latency and spikes; uncapped throughput alone does not establish smooth 60fps.
- For upscaling, inspect the pinned framework's existing shader loader/presets first. Use integer scaling as the baseline; verify shader licensing and backend support before proposing imports.

## Evidence and authorization

Current research scope permits source/configuration inspection and original synthetic fixtures. Do not obtain, open, extract, commit or upload copyrighted ROM assets, generated game code, save-state payloads or captured artwork under that scope. Do not run third-party code or repository setup/build/extraction scripts merely because a README recommends them. A later explicit scope may authorize selected execution or user-owned inputs; report exactly what was authorized and tested.

Record each conclusion with a source path and line or trace reference. Label hypotheses and authored-but-unrun tests. Keep static checks, ROM-free tests and gameplay validation distinct. Never claim a wall-navigation fix or a performance target was achieved from a log that does not exercise it.

Changes should remain on a new branch with narrow, reviewable diffs. Check the staged file list for assets and generated output. Do not merge, deploy, install tools, or update submodules as an implicit part of analysis. Preserve the project's license; translating code or instructions does not change its license.
