---
name: cosmic-x-video-reference
description: Measure cars, parallax, camera movement, scale and perceived frame pacing from an accessible, authorized Cosmic X reference video without inventing values.
---

# Cosmic X video reference analyzer

## When to use
The owner supplies gameplay footage and asks to recreate the background speed, visible highway traffic, motion smoothness or enhanced visual detail. Use this skill before specifying traffic velocities or background scroll coefficients. Do not assert that a file from an earlier conversation is currently accessible.

## Input checks
1. Identify filename, duration, encoded frame rate, dimensions, video timestamps, playback speed, any cropped game viewport and source/build provenance when known.
2. Exclude desktop borders and overlays. Record crop and per-axis scaling into game-logical pixels. Preserve pixel-aspect-ratio uncertainty; never assume 5x scaling means native artwork gained detail.
3. Separate scenes with a moving/stationary camera, fades, deaths, menus and teleports. Record which source frames were individually examined and which intervals were skipped.
4. Inspect full-resolution individual frames at known frame indices or timestamps. Do not treat a contact-sheet estimate as a measurement. For fast vehicles, sample sufficiently close frames to maintain reliable object identity.

## Motion measurement
- Track named landmarks and object IDs: upper-road cars, lower-road cars, lane markings, stationary road pillars, far buildings, foreground fence and X/Zero.
- For every tracked point report source timestamp, screen X/Y within the viewport, logical X/Y after crop/scale correction, bbox, lane/layer, uncertainties and visible occlusion.
- Compute screen velocity from displacement over *actual timestamp delta*. Note that for foreground world features, screen displacement reflects both world movement and camera movement. Infer world speed only after camera motion is independently estimated.
- For distant layers estimate the observed relationship between layer displacement and camera displacement; if the camera cannot be measured, report screen-relative speed only.
- Compare straight road stretches separately from curves; note acceleration, wrap, interval changes and partial masking.
- Check repeated visual frames/dropped presentation frames. A 60fps-encoded video does not prove 60 unique game updates or stable 60Hz host presentation.

## Output and handoff
Deliver an evidence table with fields: source, interval, frame, seconds, crop, track_id, lane_or_layer, x, y, bbox, camera_dx, screen_px_per_sec, inferred_world_px_per_sec, uncertainty, evidence_status. Use UNKNOWN rather than an invented zero. Produce: lower/upper car lane map, visible car density, speed ranges, per-layer scroll factors, scale/crop notes and a prioritized mismatch list relative to an actually accessible Cosmic X capture.

Pass observed ranges and uncertainties to cosmic-x-highway-traffic and motion/presentation observations to cosmic-x-frame-pacing. A source video cannot replace the game's own instrumented timing data.

## Safety
Only inspect recordings actually supplied or authorized. No ROM/asset extraction, model installation, video redistribution or automatic addition of screenshot frames to public Git. State explicitly when no capture could be opened or no values were measured.


## Combat powers extension — additional October 14 video
When the uploaded footage emphasizes abilities rather than highway movement, additionally record actor identity (player/boss/unknown), effect shape and color, apparent charge buildup, attack direction, repeated shot count, movement state, visible contact feedback, slash trajectory, particle/flash duration and any uncertain arena transition. Distinguish direct visual evidence from unconfirmed input, damage, hitbox or attack ownership. The submitted combat clip is a 30fps-encoded montage, not an authoritative 60Hz runtime trace.

Use [the combat-reference observation log](../../../docs/cosmic-x-combat-reference-2026-10-14.md) and hand off confirmed needs to cosmic-x-power-systems, cosmic-x-combat-mobility, cosmic-x-boss-patterns or cosmic-x-combat-vfx. Never hard-code attack timings from sparse cuts or use another game's exact art or weapon naming.
