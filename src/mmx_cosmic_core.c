#include "mmx_cosmic_core.h"
#include <string.h>

static bool cosmic_unlocked(const MmxCosmicState *state, unsigned aspect) {
  return aspect < MMX_COSMIC_ASPECT_COUNT &&
      (state->unlocked & (uint16_t)(1u << aspect)) != 0;
}

void MmxCosmicReset(MmxCosmicState *state) {
  if (!state) return;
  memset(state, 0, sizeof(*state));
  state->equipped[0] = MMX_COSMIC_NONE;
  state->equipped[1] = MMX_COSMIC_NONE;
  state->last_reward = MMX_COSMIC_NONE;
}

bool MmxCosmicStateValid(const MmxCosmicState *state) {
  if (!state || state->reserved || state->resonance > MMX_COSMIC_RESONANCE_MAX ||
      (state->unlocked & ~(uint16_t)((1u << MMX_COSMIC_ASPECT_COUNT) - 1u)))
    return false;
  if (state->last_reward != MMX_COSMIC_NONE &&
      state->last_reward >= MMX_COSMIC_EVENT_COUNT) return false;
  for (unsigned i = 0; i < MMX_COSMIC_ASPECT_COUNT; ++i)
    if (state->specialization[i] > MMX_COSMIC_FLOW ||
        (state->specialization[i] && !cosmic_unlocked(state, i))) return false;
  for (unsigned slot = 0; slot < 2; ++slot) {
    unsigned aspect = state->equipped[slot];
    if (aspect != MMX_COSMIC_NONE && !cosmic_unlocked(state, aspect)) return false;
  }
  return state->equipped[0] == MMX_COSMIC_NONE ||
      state->equipped[0] != state->equipped[1];
}

bool MmxCosmicUnlock(MmxCosmicState *state, unsigned aspect) {
  if (!state || !MmxCosmicStateValid(state) ||
      aspect >= MMX_COSMIC_ASPECT_COUNT || cosmic_unlocked(state, aspect))
    return false;
  state->unlocked |= (uint16_t)(1u << aspect);
  return true;
}

bool MmxCosmicEquip(MmxCosmicState *state, unsigned slot, unsigned aspect) {
  if (!state || !MmxCosmicStateValid(state) || slot >= 2 ||
      (aspect != MMX_COSMIC_NONE && !cosmic_unlocked(state, aspect)) ||
      (aspect != MMX_COSMIC_NONE && state->equipped[1 - slot] == aspect))
    return false;
  state->equipped[slot] = (uint8_t)aspect;
  return true;
}

bool MmxCosmicSpecialize(MmxCosmicState *state, unsigned aspect, unsigned choice) {
  if (!state || !MmxCosmicStateValid(state) ||
      !cosmic_unlocked(state, aspect) || choice > MMX_COSMIC_FLOW) return false;
  state->specialization[aspect] = (uint8_t)choice;
  return true;
}

unsigned MmxCosmicReward(MmxCosmicState *state, unsigned event) {
  static const uint8_t value[MMX_COSMIC_EVENT_COUNT] = {12, 10, 8, 6, 8};
  if (!state || !MmxCosmicStateValid(state) || event >= MMX_COSMIC_EVENT_COUNT)
    return 0;
  /* Consecutive identical events earn nothing; no passive idle increment. */
  if (state->last_reward == event) return 0;
  unsigned room = MMX_COSMIC_RESONANCE_MAX - state->resonance;
  unsigned amount = value[event] < room ? value[event] : room;
  state->resonance += (uint16_t)amount;
  state->last_reward = (uint8_t)event;
  return amount;
}

MmxCosmicFusion MmxCosmicSelectedFusion(const MmxCosmicState *state) {
  if (!MmxCosmicStateValid(state)) return MMX_COSMIC_NO_FUSION;
  unsigned a = state->equipped[0], b = state->equipped[1];
  if ((a == MMX_COSMIC_STORM && b == MMX_COSMIC_GRAVITY) ||
      (a == MMX_COSMIC_GRAVITY && b == MMX_COSMIC_STORM))
    return MMX_COSMIC_THUNDERWELL;
  return MMX_COSMIC_NO_FUSION;
}

bool MmxCosmicActivateFusion(MmxCosmicState *state) {
  if (!state || MmxCosmicSelectedFusion(state) == MMX_COSMIC_NO_FUSION ||
      state->resonance < MMX_COSMIC_FUSION_COST || state->fusion_cooldown)
    return false;
  state->resonance -= MMX_COSMIC_FUSION_COST;
  state->fusion_cooldown = MMX_COSMIC_FUSION_COOLDOWN;
  ++state->fusion_activations;
  return true;
}

void MmxCosmicTick(MmxCosmicState *state) {
  if (state && MmxCosmicStateValid(state) && state->fusion_cooldown)
    --state->fusion_cooldown;
}
