---
name: cosmic-x-combat-mobility
description: Design responsive dash, jump, aerial firing, momentum, movement-and-attack transitions and cancel windows while preserving native Cosmic X collision and character-specific controls.
---

# Cosmic X combat-mobility and input-feel engineer

Use when matching fluid movement during shooting and melee effects. Study source player control in src/mmx_zero.c, src/mmx_coop.c, src/mmx_rtl.c and tests; inspect the real active branch. In the Oct 14 clip ground running/dash-like trails are visible around 00:09 and airborne attacks appear around 00:05, 00:23 and 00:27. The actual button inputs and active frames cannot be established from a 30fps recording of a separate game.

## Model motion explicitly
Create per-character state charts for idle/run, ascent/apex/fall, wall slide/climb/jump, dash, charging, attack startup/active/recovery, hitstun, death and scripted states. Distinguish X, X3 Zero, Modern Zero and CPU-companion capability and input ownership.

For every transition specify physical momentum, speed/acceleration, collision, input press/release/edge rules, facing, attack spawn frame, compatible charging, interrupt/cancel windows and recovery frames. Keep visual motion trails separate from actual velocities and hitbox extents.

## Required regressions
- Firing while running and jumping, charged release in midair, wall contact during charge, dash into wall/pit, aerial attack landing on one-way platforms.
- Direction change mid-attack, interrupted attack while damaged, repeated jump input edges, rapid weapon/character switching and co-op simultaneous controls.
- AI companion must value safe ground and headroom over firing at a boss; retain native collision, health and controller ownership.
- Separate synthetic unit timing, gameplay instrumentation and subjective owner feedback.

Do not replace native physics, set velocities artificially or disable collision just to make movement visually faster. A gameplay branch and explicit authorization are required for implementation.
