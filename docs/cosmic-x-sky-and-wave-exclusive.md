# Cosmic X — faster Highway sky and exclusive Earth Wave

**Based on** upgrade/cosmic-x-v33-phase1-powers-vfx. Separate draft change.
The captured 28-second 970x774 / 30fps preview showed beautiful but mostly fixed
purple Highway cloud silhouettes and Earth Wave coexisting with X-Buster fire.
Encoded video frame rate alone cannot establish actual 60Hz presentation.

## Gameplay

- The native R2+Shoot action is exclusive: while an X-owned Earth Wave is
  active, native Shoot-held and Shoot-edge bits are suppressed each frame.
- A second R2+Shoot may still create another wave if one of the two allowed
  wave slots is available, but full slots never fall back to a normal X shot.
- When the last wave retires, normal X-Buster fire resumes. Other pad inputs
  and Zero's separate storm command are unaffected. The lifetime comes only
  from the existing per-player combat state; no new save/rollback fields.
- No changes to wave velocity, damage, terrain collision, sound or host clock.
  Native shot creation at alternate co-op code paths still requires gameplay
  verification; this is specifically a pre-native-fire input guard.

## Sky

- New original, deterministic purple-cloud wind-overlay artwork at 1–3
  pixels per simulation tick with a 4 px/tick gust accent, using only the
  original BG2 color classified as Highway sky.
- Preserve native cloud tiles, original background scroll, actors, lighting,
  sprite priority, palette fades, and HUD. No ROM graphics copied.
- Preview toggle: MMX_COSMIC_SKY_MOTION=1, default OFF for comparison.
  Use MMX_COSMIC_SKY_MOTION=0 to recover the old cloud look.
- Original earth/storm optional sparks remain MMX_COSMIC_UPGRADE_VFX=1.

## Acceptance

Build and run mmx_renderer_test / mmx_cosmic_core_test, inspect real footage
with both toggle states. Verify R2 cast, Shoot held/re-pressed during wave,
full pool, wave retirement, co-op actor selection, rewind, stage transitions,
HUD readability, and full-game performance before considering a default.
Cloud motion should be visually faster without degrading 60Hz frame pacing.

This branch does not merge, deploy, delete archived local experiments, or
alter the protected Cosmic X preservation checkpoint.
