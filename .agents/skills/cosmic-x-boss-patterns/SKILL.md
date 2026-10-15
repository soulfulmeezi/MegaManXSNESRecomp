---
name: cosmic-x-boss-patterns
description: Design original readable boss attack patterns, telegraphs, radial projectile fields, blade rushes, arena hazards and phase transitions as deterministic state machines.
---

# Cosmic X boss attack-pattern director

Use when the owner requests visually powerful original boss battles inspired by footage. Inspect existing boss/stage logic and renderer boundaries on the actual gameplay branch, plus docs/cosmic-x-combat-reference-2026-10-14.md and the power-system skill. Never copy characters, proprietary hitbox data or stage resources from the reference.

## What the new clip shows
A green bladed opponent (about 00:05-00:16), airborne mechanical opponent (00:21-00:27), clustered energy spheres (00:29-00:35), dramatic branching lightning (00:36-00:39), flames on the ground (00:46-00:47), a broad sword arc (00:49-00:56), and distinct background changes (00:59-00:64). Ownership of each effect, HP thresholds and exact gameplay rules are not verified.

## Original encounter contract
Define the arena and safe lanes, boss silhouette and readable tell, AI target choice and vulnerability, and a deterministic finite-state machine: intro, observe/position, telegraph, active attack, recovery, damage reaction, phase transition, defeat and cleanup. Specify each attack's startup frames, movement curve, collider active interval, velocity, count cap, safe dodge route, cooldown and interrupt rules. Use explicit random seeds when randomness affects progression.

Prototype original synthetic patterns: arcing/radial projectile wave with safe gaps, airborne aimed shots, lunging sword attack with visible wind-up and recovery, and a floor hazard that players can evade. Avoid unfair simultaneous unavoidable patterns. Cover one-player and co-op threat targeting, widescreen exposure, stage reset and rollback.

## Acceptance
ROM-free tests for legal state transitions, hitboxes active only during the specified windows, bounded projectile counts, safe cleanup on death/restart, reproducible seeded behavior, and save/rewind. Later real gameplay should be evaluated for readable timing and player counterplay, not just spectacle. Gameplay code/asset changes require separate owner approval.
