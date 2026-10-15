# CosmicFX atmosphere engine: reference-driven prototype

Base: backup/cosmic-x-v33-preserve-20261015
Branch: experiment/cosmicfx-atmosphere-20261015

## Visual direction from October 14, 2026 reference recordings

- Clip A, 00:00-00:15: layered light blue sky and dense, broad cloudbanks behind a metal airship. Storm-like windows and machinery later in the clip use a distinct depth/priority hierarchy.
- Clip A, 00:45-01:45: richly colored energy shots, illuminated industrial backgrounds and glowing prop surfaces. These are a VFX and art production target, not proof that the current engine contains those sprites.
- Clip B, 00:05-00:37: large radial, curved and beam projectiles from boss combat. Strong silhouettes communicate attack purpose.
- Clip B, 00:40-01:19: rapid contrast changes, luminous attack trails, and blue/white/red flashes. Keep gameplay readability and avoid gratuitous seizure-risk strobing.

## v1 implementation

The existing native PPU/host RGB555 renderer already has composition layers. This experiment adds a *self-contained* optional CosmicFX atmosphere module for three depth planes rather than rewriting engine internals.

1. Distant wisps follow camera x / 12 and drift at 1 px per 20 simulation ticks.
2. Mid-range cloudbanks follow camera x / 6 and drift at 1 px per 11 ticks.
3. Larger, close clouds follow camera x / 3 and drift at 1 px per 7 ticks.
4. Cloud masses consist of overlapping ellipse lobes; blend with the native BG2 sky color.
5. The Intro Highway uses a distinct **dark-night cloud profile** with the existing city's lavender sky palette, so the first stage receives visible motion as well. Only the original skyline blue/purple color ranges qualify.
6. Only eligible bright blue/night-sky BG2 sky pixels with no BG1 foreground are modified. Mechanics, enemies, player sprites, HUD, palette fades, PPU priorities, windows, saves and netplay protocol are untouched.
7. Cosmetic time uses existing saved `frame_weapon_combat.tick`; no wall-clock time, thread or RNG.
7. Set `MMX_CLOUD_FX=0` in the environment to disable effects while A/B testing.

## Verification before preview release

- Build `mmx_environment_fx_test`; run `ctest -R mmx_cosmicfx_environment`.
- Build and run the renderer and Zero unit tests.
- In a stage with visibly bright blue sky, capture moving/fixed-camera frames at 0, 60, 120 ticks, measure cloud displacement independently from BG1 structures.
- Check title/menu, underground/dark scenes, fades, splitscreen, multiplayer, camera margins and frames with multiple enemies.
- Profile native resolution, wide modes and 60-FPS host presentation.

The separate \`MMX_WEAPON_FX=0\` environment setting disables additional Earth-wave wake sparks and Jupiter electrical halos while leaving the original projectile sprite intact.

## Remaining work

This is only a cloud system starter, **not a second playable engine or a completed art upgrade**. Full reference fidelity needs hand-crafted/layered clouds, detailed new stage art, a sprite/animation authoring pipeline, bigger weapon atlas, particles, hit-stop, camera feedback, and new Zero/CPU animation sheets.

The sprite and sound effects depicted in third-party reference footage are not distributed as game assets. Use original Cosmic X artwork or appropriately licensed resources.

## Architecture recommendation

Next build a reusable `CosmicFxStageProfile` with weather/cloud/lighting parameters for different stages, and a GPU (shader) pass only if native CPU composition proves too expensive. Preserve the existing SNES recompiled gameplay simulation.
