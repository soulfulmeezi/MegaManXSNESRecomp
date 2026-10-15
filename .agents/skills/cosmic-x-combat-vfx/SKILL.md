---
name: cosmic-x-combat-vfx
description: Plan readable, performant original combat VFX for Cosmic X: energy blasts, charge glows, slash arcs, lightning, floor flames, hit flashes and boss-phase transitions.
---

# Cosmic X combat effects and feedback engineer

Review src/mmx_renderer.c, src/mmx_render_assets.c, src/mmx_wide_policy.c, tests/mmx_renderer_test.c and existing shader/preset support. Keep native SNES tiles/palettes separate from optional HD effects.

## Observed inspirations
The combat reference shows blue/cyan shots and grouped rings (00:05, 00:19, 00:50), green blade trails (00:09-00:16), broad white/pink sword arcs (00:49-00:56), electrical branches over neon scenery (00:36-00:39), orange floor eruptions (00:46-00:47), and major light/silhouette/background changes (00:29-00:35, 00:59-00:64, 00:73-00:75). Some are boss actions and some player actions; precise effect ownership and damage behavior are unknown.

## Per-effect design
Record: unique effect ID, trigger, source actor, world-space or screen-space anchor, compositor layer, spawn angle, animation duration and frame sequence, palette/alpha/glow/trail, particle cap, offscreen policy, rewind/pause behavior and audiovisual cues. Default VFX-only elements to NO damaging hitbox; when gameplay collision exists, document it independently from visible glow bounds.

Prefer original synthetic fixtures in the current renderer before any artwork pack. Keep draw order consistent across widescreen, moving camera, palette transitions, foreground pillars, player sprites and HUD. Reuse existing graphics helpers rather than introducing a second unreviewed effects engine.

## Smoothness and accessibility
Limit particles, light passes, texture uploads and frame allocations. Diagnose p95/p99 game and presentation timings with cosmic-x-frame-pacing. Avoid strobe-like flashes or prolonged full-screen whiteouts during active hazards. Provide reduced-flash, reduced-shake and high-contrast effect modes, not color-only warning tells.

## Validate
Inspect screenshots for occlusion and animation alignment, use synthetic renderer tests for bounds/priority, and require authorized actual gameplay/video before claiming match or smooth motion. No third-party shaders, game code changes, copied footage frames or executable installs are part of this skill.
