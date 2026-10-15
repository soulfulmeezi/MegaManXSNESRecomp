# Cosmic X — preservation-first v33 foundation

**Base:** backup/cosmic-x-v33-preserve-20261015 at c067ac6666f6cda2b8e30d9e97429e9fd6ca763b.
**State:** Draft, additive work only; no live gameplay feature enabled.

## Working state we must keep

- The checkpoint includes v20–v24 committed development plus a selected set of more recent modified and untracked v25–v33-era source files. There are additional v25–v33k backups on the owner's PC that have NOT been integrated or validated by this commit. Do not delete or replace those archives.
- Earth Wave and Jupiter Storm already exist in src/mmx_cosmic.h and src/mmx_weapon_cosmic.inc, using the native per-player projectile state. Their runtime identifiers are cosmic weapon page 3, Earth Wave 1 and Jupiter Storm 2. Preserve attack damage, collision, native input, actor ownership and save/rewind behavior.
- Preserve Zero's native physics, CPU companion navigation, co-op control and Jupiter material options; preserve the widescreen renderer, road cars, environment layers, shader pipeline, audio and guest clock.
- In this checkpoint the renderer uses MMX_NEAR_ROAD_CARS default 0 (near-road cars disabled) and MMX_TRAFFIC_DEBUG_LANES default 1 (debug overlays enabled). Do not change those defaults without controlled moving playtest verification of masks, clipping and animation.
- Existing lower-road traffic and native BG1/BG2 lane-detection code must be enhanced in place, not replaced with a new renderer.

## Additive contents of this branch

1. The ten original agent skills and routing documents from draft PR #4. These are reusable instructions, not installed tools or running features.
2. The original ROM-free Constellation state prototype and six synthetic tests from draft PR #5, now rebased onto the newer correct checkpoint.
3. CMake and CI changes that add only an independent test target. The Cosmic core is deliberately NOT linked to either playable game executable, and existing native gameplay code is unchanged.

## Important mismatch before enabling powers

The isolated prototype's proposed Aspect ID Storm=0 / Gravity=1 does NOT equal existing runtime weapon IDs Jupiter Storm=2 / Earth Wave=1. It is a completely separate progression ID space. The proposed twelve-Aspect roster also omits the already-implemented Earth Wave. NEVER wire the two state machines together by casting or assuming IDs align. First specify a versioned translation including Earth Wave and Jupiter Storm, with backward-compatible weapon availability and opt-out.

The next gameplay milestone requires user-approved behavior and tests for old saves, co-op player switching, native projectile hit/damage, Resonance events and deterministic rollback. Only then add original Gravity and one Storm+Gravity fusion without changing Earth Wave or the original Jupiter Storm.

## Ordered acceptance gates

- Gate 0: Keep the remote backup branch unchanged; verify this PR diff contains only skills, docs, isolated core, unit-test target and CI edits.
- Gate 1: Get an independent CI build plus owner ROM-free and authorized actual Windows-game test results. Do not claim gameplay smoothness from synthetic tests.
- Gate 2: Implement an opt-in adapter that maps existing Earth Wave and Jupiter Storm to unlocks; preserve all default controls and native weapon saves.
- Gate 3: Add a small original Gravity prototype and bounded Thunderwell fusion with visible telegraph, reduced-flash option and finite projectile/collision rules.
- Gate 4: Study new reference footage and enhance existing Highway vehicles, curve geometry, near-road masks and parallax. Prioritize lower-road visibility at camera X 0x05B7, 0x0718, 0x0FCE and 0x148F.
- Gate 5: Profile true frame presentation p50/p95/p99, audio, co-op/solo behavior and Zero's ground/wall/safe-follow logic; compare against preserved playtest recordings.

## Scope and rights

No existing gameplay files or proprietary ROM/game assets changed, no external code installed, no unknown scripts executed, no branch merged or deployed. The project is PolyForm Noncommercial; commercial IP/asset clearances remain a separate requirement.
