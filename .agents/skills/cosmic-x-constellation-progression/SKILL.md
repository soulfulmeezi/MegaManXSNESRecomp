---
name: cosmic-x-constellation-progression
description: Plan and validate Cosmic X's hybrid power system of permanent Foundations, many unlockable Aspects, upgrades, curated combinations, Resonance and character-specific X/Zero moves.
---

# Cosmic X constellation progression and fusion director

Use when the owner wants BOTH a broad power collection and a few powers that evolve, combine and remain relevant. Start with docs/cosmic-x-constellation-system.md and coordinate with cosmic-x-power-systems, cosmic-x-combat-mobility, cosmic-x-boss-patterns, cosmic-x-combat-vfx and cosmic-x-companion-ai.

## Design requirements
- Keep four permanent Foundation families (Strike/Flow/Guard/Focus), all useful without collection grinding; preserve the native buster, saber, movement and collision.
- Expand original collectible Cosmic Aspects over time, initially a target design roster of 12, not 12 existing weapons. Prefer guaranteed boss unlocks, optional secrets and skill challenges.
- Specialize a power through a chosen Force/Shape/Flow evolution rather than stacking every possible upgrade; allow respec at checkpoints with no farming.
- Equip two active Aspects plus one support trait. A compatible pair may unlock a deliberately **authored** Conjunction. Do not generate all 66 possible unordered pairs from the 12 Aspect roster.
- Use deterministic Resonance gain tied to varied successful inputs and play; reject repetitive farming and undefined damage bonuses. Guard activation behind explicit controller/UX design.
- X and Zero get separate attack delivery styles; CPU Zero cannot sacrifice his own survival or steal input to trigger a fusion.

## Inspect before coding
Read src/mmx_weapons.c, src/mmx_weapon_combat.c, src/mmx_zero.c, src/mmx_coop.c, existing weapon tests and the live save/rollback layouts on the actual gameplay branch. Note whether experimental armor/companion work is only on other branches.

For each ability define stable ID, unlock location, owner/mode, input/charge, resource use, projectile path/lifetime/count, hitbox vs VFX, target behavior, weakness/immunity, activation cooldown, deterministic meter update, companion behavior, render budget and save-schema implication.

## First approved gameplay slice
1. Write an existing-tools inventory and original synthetic Storm / Gravity specs.
2. Propose the single curated Storm + Gravity Thunderwell move (finite pull targets and lightning jumps; no uncontrolled chain spam).
3. Create ROM-free tests for charge edges, unsatisfied meter, unsupported pairs, changing loadouts mid-charge, XP/unlock duplication, actor ownership, opponent immunity, death/restart, rewind and serialization stability.
4. Only after explicit game implementation authorization, add a feature-flagged vertical slice on a separate branch, then playtest and measure frame pacing.
5. Expand to additional powers only after the first actually works.

## Safety
No ROM extraction, source-game asset redistribution, copying named franchise powers, third-party code execution, unapproved game edits, silent merges or deployment. Treat names, numbers and attack behavior in the design document as proposals until verified and accepted.
