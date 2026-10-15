# Cosmic X — Cosmic Constellation System

**Status: proposal only, not implemented or playtested.** The owner's direction is BOTH a large roster of collectible powers and a small set of upgradable fundamentals that can combine into unexpected techniques. Use original art and mechanics, preserving the SNES-recompiled host's guest clock, co-op, movement and renderer boundaries.

## Architecture: breadth plus depth

**Four permanent Foundations:** these are progression families rather than four new buttons.

| Foundation | Always-useful identity | Upgrade direction |
| --- | --- | --- |
| Strike | Native X buster / Zero saber | Charge control, follow-ups, situational combo transitions |
| Flow | Native jumping, wall moves and dash | More tactical attack-after-dodge options without overriding physics |
| Guard | Defense and damage recovery | Timed counter or controlled protection |
| Focus | Charging and weapon-energy strategy | Energy efficiency and Resonance-generating precision |

**Collectible Cosmic Aspects:** proposed target roster of 12, not implemented. Each changes how a weapon behaves, and is usable throughout the game rather than being discarded after a stronger unlock.

| Aspect | Role | Example original mechanic |
| --- | --- | --- |
| Storm | Electrical chain | Bounded jumps between grouped targets |
| Gravity | Placement / crowd control | Short attractor field with clear limits |
| Solar | Precision energy | Narrow charged lance |
| Prism | Reflection | Splits a shot at a defined angle |
| Frost | Area control | Short-lived slowing field |
| Orbit | Spacing / defense | Capped orbiting satellites |
| Echo | Delayed repetition | Repeat an attack using recorded intent, NOT guest-time rewinding |
| Rift | Repositioning | Collision-checked short movement |
| Ember | Ground effect | Brief visible flame path |
| Magnet | Redirection | Attraction to tagged objects |
| Tide | Rhythm | Pushing wave projectile |
| Void | Tactical interruption | Limited absorb/cancel field |

**Acquisition suggestion:** eight guaranteed boss rewards plus two exploration secrets and two optional skill challenges. Core progression never requires a hidden collectible or a repetitive grind. Actual boss mapping, numbers and unlock chronology still require narrative/design decisions.

## Depth: three upgrade routes per Aspect

- **Force:** riskier charged form or higher single-target impact.
- **Shape:** new spatial action such as branching, field shaping or curved trajectory.
- **Flow:** faster deployment, greater movement compatibility or energy efficiency.

A chosen specialization changes behavior, not simply a percent damage increase. A player can switch between earned specializations at a checkpoint/menu with no respec currency cost. Don't stack all routes into an uncapped power-creep tree.

## Loadout and controls

Native buster/saber remains available. Equip **two active Aspects (A/B)** and **one non-damaging support trait**. Compatible A+B pairings provide a unique **Conjunction** move powered by the **Resonance** meter. Unsupported A+B pairs remain two useful powers rather than invoking an automatically generated combination.

Do not add new mandatory inputs until the controller mapping is reviewed. Save equipped Aspects, earned upgrades and meter state only after separately reviewing existing save layouts, old-save migration, rewind and rollback.

## Curated pair recipes

12 Aspects allow 66 possible pairs, but start with **1-2 implemented recipes** and plan roughly 6-10 handcrafted Conjunctions, not all 66.

| Two Aspects | Conjunction | Original behavior concept |
| --- | --- | --- |
| Storm + Gravity | **Jupiter's Thunderwell** | Attractor orb groups targets; bounded lightning arcs end in a burst |
| Solar + Prism | **Dawn Lancer** | A focused beam divides into two weaker angled rays |
| Frost + Orbit | **Halo Shards** | Timed crystal satellites launch when the player dodges |
| Echo + Rift | **Afterimage Gate** | Collision-safe reposition leaves one delayed attack echo |
| Ember + Tide | **Steamwake** | Moving steam field interferes with enemy targeting without hiding hazards from the player |
| Magnet + Storm | **Ion Rail** | A short conductive path channels one piercing charged shot |

These are pitches, not verified damage, animation counts or exact moves taken from the reference video. Keep every hitbox distinct from its particles. The existing Jupiter-themed armor idea can optionally receive a matching original banded/storm energy effect, without making armor visuals modify combat damage.

## Resonance: a skill-rewarded fusion meter

Earn meter through varied successful actions: precision dodges, correctly timed defensive counters, safe weapon transitions, weaknesses exploited and co-op timing. Diminishing gains for repeating the identical move discourage spam; standing still or taking damage should not farm it. A Conjunction costs explicit meter, observes a cooldown and never silently interrupts a human teammate.

X emphasizes precise projectiles and area setup; Zero emphasizes close-range control and saber follow-ups. The *same* Aspect has character-specific delivery. CPU Zero participates only when his path is safe and the player has opted into such cooperation. Do not overwrite his native controller or give the wrong Zero mode unavailable movement abilities.

## Example vertical slice: Jupiter's Thunderwell

On the Highway stage, X throws a Gravity field at two **synthetic** enemy test shapes (traffic cars remain draw-only); Zero safely follows with an original Storm slash. A successful dodge/Aspect alternation raises Resonance. When manually activated, Thunderwell groups targets and creates capped lightning arcs and a brief ending pulse. The lightning is readable and reduced-flash compatible, and wide-view traffic/parallax continues unchanged.

This is a proposed scenario, **not a gameplay result**.

## Implementation gates

1. Identify actual gameplay branch; inspect src/mmx_weapons.c, src/mmx_weapon_combat.c, src/mmx_zero.c, src/mmx_coop.c, original weapon tests, save/rollback and renderer before changes.
2. Define synthetic Storm and Gravity ability descriptors: IDs, charge timing, cooldown, damage policy, target cap, ownership, projectile life, energy, meter and collision vs VFX. Numbers remain tunable design parameters.
3. Write ROM-free tests for press/hold/release, weapon change, actor ownership, immunity/hit cooldown, max projectile count, deterministic meter gains, unsupported pair fallback, save/load/rewind and old-save handling.
4. On a **separately authorized gameplay branch**, implement one original fusion behind a feature flag, playtest it with approved resources and compare real frame-pacing p50/p95/p99 to the baseline.
5. Only after this vertical slice succeeds, add a third Aspect, a second combination and later broader unlocks. Avoid premature 66-pair complexity.

## Intellectual property and scope

Design ideas are original and abstract. Do not copy game characters, copyrighted sprites, other titles' exact attacks, source code or ROM assets from the reference. This document does not authorize installing tools, running the game, changing code, merging, deploying or shipping commercially. The repository's PolyForm Noncommercial license and external franchise rights require separate review.
