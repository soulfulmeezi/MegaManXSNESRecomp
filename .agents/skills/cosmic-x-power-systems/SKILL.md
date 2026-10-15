---
name: cosmic-x-power-systems
description: Design and test original Cosmic X buster charge tiers, energy projectiles, volley weapons, ammo, damage rules and special abilities from verified source-code hooks and authorized combat references.
---

# Cosmic X power and weapon-system engineer

Use when the owner wants new powers, charging, repeating projectiles, special attacks, or combat upgrades. Start with docs/cosmic-x-combat-reference-2026-10-14.md, src/mmx_weapons.c, src/mmx_weapon_combat.c, src/mmx_zero.c, src/mmx_coop.c, existing weapon .inc files, docs/x-weapons-port.md and relevant tests. Verify the active gameplay branch and available hooks.

## Separate inspiration from evidence
This new 79.6-second reference clip visually contains large blue shots, repeated cylindrical energy projectiles, ring-shaped objects and impact bursts around 00:05, 00:19, 00:27 and 00:50. Precise buttons, charging duration, hitboxes, attack origin, energy cost and damage are NOT established. Use only general mechanics inspiration. Do not reuse character artwork, exact weapon names, animation strips, code or proprietary data.

## Describe each original power
Specify: stable ID, character/weapon mode, slot/unlock and weapon energy rules; press/hold/release input, charge thresholds, cooldown and cancellation; spawn origin, size and facing; projectile type (ordinary, charged, volley, spread, orbit, beam, returning, area, shield); velocity, lifetime, steering, cap and despawn; active hitbox distinct from trail; damage, pierce, hit cooldown, invulnerability, guard/weakness and boss interactions; co-op controller/ownership; deterministic save/load/rewind requirements; original VFX/audio hooks and host frame budget.

## Iteration plan
1. Inventory existing weapon helpers and regression tests before designing an alternate shot framework.
2. Propose ONE original synthetic power with regular and charged variants. Provide adjustable parameters, acceptance criteria and a balance table rather than invented damage figures.
3. Create ROM-free tests for input edges, charge/reset, projectiles traveling left/right, simultaneous players, caps, blocked/immune enemies, weapon swaps and determinism.
4. Implement a single opt-in vertical slice only on a separately authorized gameplay branch; compare actual game feel to the owner's preference and profile under combat stress.
5. Expand to more powers only after the first demonstrates correct collision, timing and readable feedback.

## Safety
Changes to gameplay code, running builds, installing external tools or accessing copyrighted ROM assets need explicit approval beyond this document. Do not describe a synthetic test as successful real gameplay.


## Progression and power fusion
The owner selected BOTH numerous collectibles and permanent evolving powers. Use [Cosmic Constellation proposal](../../../docs/cosmic-x-constellation-system.md) and [cosmic-x-constellation-progression](../cosmic-x-constellation-progression/SKILL.md) for unlock design, two-active-power loadouts, curated Conjunctions, Resonance, X/Zero expressions, tests and save-state compatibility. These are proposed mechanics until individually implemented, approved and playtested.
