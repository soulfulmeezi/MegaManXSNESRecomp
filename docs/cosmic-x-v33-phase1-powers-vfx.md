# Cosmic X Phase 1 — original-power bridge and opt-in energy sparks

Preserved basis: upgrade/cosmic-x-v33-foundation. No live combat, collision or save-state changes.

1. Add a distinct Constellation EARTH progression ID 12 (total 13) without changing the existing native weapons (page 3, Earth Wave 1 and Jupiter Storm 2).
2. Add pure mapping in src/mmx_cosmic_legacy_bridge.h. The speculative Gravity Aspect is deliberately not mapped to Earth Wave.
3. Add optional original deterministic spark graphics to existing Earth Wave and Jupiter Storm. In the native custom renderer, MMX_COSMIC_UPGRADE_VFX defaults to 0; set MMX_COSMIC_UPGRADE_VFX=1 in the environment before launching an approved test build to see the optional two-spark orbit. No projectile age/position, hitboxes, damage, controls or rewind state change. Native opaque projectile pixels take precedence.
4. Register ROM-free bridge and visual tests alongside the existing Constellation core tests. Validate real host build, co-op rendering, correct fade/HUD order and p50/p95/p99 performance before enabling by default.

Not yet implemented: gameplay progression HUD, Gravity ability, Thunderwell combo, unlock persistence, increased 60fps or new Highway cars. Existing v33 cars, Jupiter material and Zero AI remain unchanged.

Keep this work on a separate draft PR. Do not merge or deploy without explicit owner approval and genuine game validation.
