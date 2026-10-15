# Cosmic v25 — stage-wide hover traffic (Task 1 continuation)

Base: local v24 commit `611e316` on `experiment/hover-traffic-v24-20261013`.
Scope: **traffic coverage only**. No art, player/Zero physics, weapon waves, foreground ships or miniboss dodging changes.

## Frames, states and layers (planned before implementation)

- **Frames:** No new sprites or sprite frames. Retain all 20 model/direction art sprites and the existing 8 engine/exhaust animation phases from v24.
- **States:** Preserve enter → cruise → exit, with deterministic staggered phases. Vehicles are derived from the combat tick in world coordinates, independent of scrolling direction.
- **Layers:** Keep the original lower-road BG1 road-surface color override only. Use the original asphalt color test to obscure traffic at road boundaries, lane markings and bridge pillars; native sprites and HUD retain their priority.

## Implemented

- Keep the two original four-lane v24 corridors **bit-for-bit identical**, including their world anchors, phases and model assignments.
- Supply five additional four-lane **candidate** corridor groups covering the beginning and later stretches of the intro highway. The original two recorded corridor geometries are reused at offsets of -896, +1792 and +3584 world pixels. These are tentative placement coordinates, not traced against a full-stage ROM map.
- Continue to generate deterministic lane cars and camera-transform positions only at presentation. Pre-cull whole lanes outside the viewport. Restrict the number of visible cars with a 64-entry bounded array; original maximum was 32.
- Preserve three cars per lane, the same fixed perspective scale, direction variants, road-material pixel mask and engine timing.
- Add test coverage for v24 anchors remaining unchanged and for world-space stage coverage across the new candidate paths.

## Verification performed in this environment

- The ROM-free `mmx_renderer_test` builds and passes on Linux with original renderer sources and the v25 changes.
- A standalone viewport sweep checks all 28 candidate lane paths: all 28 are seen at some camera position from x=0 to x=5600; the 910-pixel simulated viewport never required more than 17 simultaneous active cars (under the cap of 64). This test measures geometric visibility, **not** actual asphalt occupancy.
- `git diff --check` passes apart from benign Windows CRLF conversion notices.

## Needed Windows verification / limitation

A real ROM-backed Windows launch, traffic animation recording and inspection of the **entire stage** were **not possible** here. v24 provided only two measured road entrances, and the new locations are projections based on their repeating geometry. In particular, cars in new sections may not align with the actual road shape, and the asphalt mask may cause partial clipping or no visible traffic. Do not label v25 production-ready until the user reviews the running game at the opening, middle, end and when backtracking. Adjust candidate corridor anchors based on those screenshots/captures.

**X4–X6 comparison: 7/10, provisional.** The vehicle art/animations are unchanged from v24: body silhouettes are rigid during cruise, distant sprites lose detail and scale moves in pixel increments. Coverage has been expanded geometrically, but stage-wide visual fidelity has not yet been confirmed.

Do **not** start Task 2 (foreground ships) or Task 3 (Zero miniboss dodge) until the user has approved the full traffic pass.
