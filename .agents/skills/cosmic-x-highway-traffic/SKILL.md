---
name: cosmic-x-highway-traffic
description: Specify and validate deterministic moving traffic on Cosmic X Highway, especially the visible lower road, plus world-anchored parallax and correct renderer layering.
---

# Cosmic X Highway traffic and parallax engineer

## Inspect the current implementation
Read src/mmx_renderer.c, src/mmx_renderer.h, src/mmx_wide_policy.c, src/mmx_render_assets.h, tests/mmx_renderer_test.c, and docs/custom-renderer-spike.md. Verify the active branch, Highway stage identifiers, camera/world transforms, raster/compositor order and supported aspect ratios. Do not assume experimental road overlays already exist or that a previous line height was validated.

## Road geometry first
1. On approved captures, identify road deck boundaries, upper/lower lane centers, occluding pillars, ramps, turns, stage gates and lane visibility versus world/camera X.
2. The owner's highest traffic priority is the LOWER road. Check camera positions 0x05B7, 0x0718, 0x0FCE and 0x148F as requested inspection sites, not verified data. Note candidate lower-road debug lines at native Y 218-220 risk clipping against the 224-pixel frame bottom.
3. Express screen projection from world coordinates and camera position. Use piecewise world-column lane geometry or measured curves where a fixed Y line would float above asphalt.
4. Render against the existing compositor without changing collision coordinates, HUD, sprite ownership, background palette phase or world-column resource selection.

## Traffic behavior
- Before final art, use original synthetic vehicle rectangles with wheels and clear lane-colored markers.
- Represent each car by deterministic ID, world X, lane ID, world velocity, length, spawn bounds and draw order. Compute view movement independently from camera movement; cars cannot merely follow screen X.
- Tune upper/lower direction, velocities, density and gaps from actual reference measurements when available, otherwise keep them clearly labeled design parameters.
- Ensure consistent behavior after screen re-entry, pauses, checkpoint reloads, rewind, save/load, stage transitions, leftward scrolling and widescreen expansion. Avoid double-spawning and offscreen pop-in.
- Initially traffic is DRAW-ONLY, with no player collision, damage, enemy interaction or physics changes. Those are separate opt-in designs.
- Do not import proprietary sprites. Replace placeholders later with verified original/licensed assets through the HD art skill.

## Parallax and compositor
Measure far skyline, middle road supports, lane traffic and foreground motion separately. Test camera-stop behavior, subpixel/fractional drift, horizontal seams, palette-region transitions, pillar occlusion, letterboxing/adaptive views, pause menus and visual priority. If the renderer snapshots native PPU state, respect its capture/draw boundaries.

## Acceptance checks before gameplay merge
- Owner-approved road alignment across requested camera checkpoints, including lower road never disappearing off the bottom or floating at curves.
- Synthetic tests for left/right camera travel, stable lane projection, separation of world and screen velocities, deterministic despawn/respawn, no double drawing and seamless wrapping.
- A real, authorized moving gameplay clip demonstrates cars traveling smoothly, with no pillar jump or road-layer mismatch; one screenshot is insufficient.
- Report measured reference car/background speeds separately from observed in-game speeds and graphics/presentation frame times.

No code execution or gameplay modifications are authorized by this skill document alone. Plan a separate implementation PR after user approval.
