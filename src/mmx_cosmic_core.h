#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Standalone, ROM-free Cosmic Constellation prototype. This is not connected
 * to the game or any serialized save ABI. Do not embed it in MmxCoopState. */
enum {
  MMX_COSMIC_ASPECT_COUNT = 12,
  MMX_COSMIC_NONE = 255,
  MMX_COSMIC_RESONANCE_MAX = 100,
  MMX_COSMIC_FUSION_COST = 100,
  MMX_COSMIC_FUSION_COOLDOWN = 90
};
typedef enum MmxCosmicAspect {
  MMX_COSMIC_STORM = 0, MMX_COSMIC_GRAVITY, MMX_COSMIC_SOLAR,
  MMX_COSMIC_PRISM, MMX_COSMIC_FROST, MMX_COSMIC_ORBIT,
  MMX_COSMIC_ECHO, MMX_COSMIC_RIFT, MMX_COSMIC_EMBER,
  MMX_COSMIC_MAGNET, MMX_COSMIC_TIDE, MMX_COSMIC_VOID
} MmxCosmicAspect;
typedef enum MmxCosmicSpecialization {
  MMX_COSMIC_BASE = 0, MMX_COSMIC_FORCE,
  MMX_COSMIC_SHAPE, MMX_COSMIC_FLOW
} MmxCosmicSpecialization;
typedef enum MmxCosmicEvent {
  MMX_COSMIC_PRECISION_DODGE = 0,
  MMX_COSMIC_GUARD_COUNTER,
  MMX_COSMIC_WEAKPOINT_HIT,
  MMX_COSMIC_ASPECT_TRANSITION,
  MMX_COSMIC_COOP_TIMING,
  MMX_COSMIC_EVENT_COUNT
} MmxCosmicEvent;
typedef enum MmxCosmicFusion {
  MMX_COSMIC_NO_FUSION = 0,
  MMX_COSMIC_THUNDERWELL = 1
} MmxCosmicFusion;
typedef struct MmxCosmicState {
  uint16_t unlocked;
  uint8_t specialization[MMX_COSMIC_ASPECT_COUNT];
  uint8_t equipped[2];
  uint8_t last_reward; /* MMX_COSMIC_NONE, or MmxCosmicEvent. */
  uint8_t reserved;
  uint16_t resonance;
  uint16_t fusion_cooldown;
  uint16_t fusion_activations;
} MmxCosmicState;

void MmxCosmicReset(MmxCosmicState *state);
bool MmxCosmicStateValid(const MmxCosmicState *state);
bool MmxCosmicUnlock(MmxCosmicState *state, unsigned aspect);
bool MmxCosmicEquip(MmxCosmicState *state, unsigned slot, unsigned aspect);
bool MmxCosmicSpecialize(MmxCosmicState *state, unsigned aspect, unsigned choice);
unsigned MmxCosmicReward(MmxCosmicState *state, unsigned event);
MmxCosmicFusion MmxCosmicSelectedFusion(const MmxCosmicState *state);
bool MmxCosmicActivateFusion(MmxCosmicState *state);
void MmxCosmicTick(MmxCosmicState *state);
