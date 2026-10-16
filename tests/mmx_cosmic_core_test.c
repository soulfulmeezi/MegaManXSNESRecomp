#include "mmx_cosmic_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void new_state_is_opt_in(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  assert(MmxCosmicStateValid(&s));
  assert(s.unlocked == 0 && s.resonance == 0);
  assert(s.equipped[0] == MMX_COSMIC_NONE && s.equipped[1] == MMX_COSMIC_NONE);
  assert(MmxCosmicSelectedFusion(&s) == MMX_COSMIC_NO_FUSION);
  assert(!MmxCosmicActivateFusion(&s));
  MmxCosmicTick(&s);
}
static void unlock_and_equip_are_checked(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  assert(!MmxCosmicEquip(&s, 0, MMX_COSMIC_STORM));
  assert(!MmxCosmicUnlock(&s, MMX_COSMIC_ASPECT_COUNT));
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_STORM));
  assert(!MmxCosmicUnlock(&s, MMX_COSMIC_STORM));
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_GRAVITY));
  assert(MmxCosmicEquip(&s, 0, MMX_COSMIC_STORM));
  assert(!MmxCosmicEquip(&s, 1, MMX_COSMIC_STORM));
  assert(MmxCosmicEquip(&s, 1, MMX_COSMIC_GRAVITY));
  assert(MmxCosmicSelectedFusion(&s) == MMX_COSMIC_THUNDERWELL);
  assert(MmxCosmicEquip(&s, 0, MMX_COSMIC_NONE));
  assert(MmxCosmicSelectedFusion(&s) == MMX_COSMIC_NO_FUSION);
  assert(!MmxCosmicEquip(&s, 2, MMX_COSMIC_GRAVITY));
  assert(!MmxCosmicSpecialize(&s, MMX_COSMIC_FROST, MMX_COSMIC_SHAPE));
  assert(MmxCosmicSpecialize(&s, MMX_COSMIC_STORM, MMX_COSMIC_FLOW));
  assert(!MmxCosmicSpecialize(&s, MMX_COSMIC_STORM, 4));
}
static void resonance_fusion_and_cooldown(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_STORM));
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_GRAVITY));
  assert(MmxCosmicEquip(&s, 1, MMX_COSMIC_STORM));
  assert(MmxCosmicEquip(&s, 0, MMX_COSMIC_GRAVITY));
  assert(!MmxCosmicActivateFusion(&s));
  assert(MmxCosmicReward(&s, MMX_COSMIC_PRECISION_DODGE) == 12);
  assert(MmxCosmicReward(&s, MMX_COSMIC_PRECISION_DODGE) == 0);
  assert(MmxCosmicReward(&s, 300) == 0);
  for (unsigned i = 0; i < 20; ++i) {
    (void)MmxCosmicReward(&s, MMX_COSMIC_GUARD_COUNTER);
    (void)MmxCosmicReward(&s, MMX_COSMIC_PRECISION_DODGE);
  }
  assert(s.resonance == MMX_COSMIC_RESONANCE_MAX);
  assert(MmxCosmicActivateFusion(&s));
  assert(s.resonance == 0 && s.fusion_activations == 1);
  assert(s.fusion_cooldown == MMX_COSMIC_FUSION_COOLDOWN);
  assert(!MmxCosmicActivateFusion(&s));
  for (unsigned i = 0; i < MMX_COSMIC_FUSION_COOLDOWN; ++i) MmxCosmicTick(&s);
  assert(s.fusion_cooldown == 0);
  assert(!MmxCosmicActivateFusion(&s));
}
static void unknown_pairs_do_not_auto_fuse(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_SOLAR));
  assert(MmxCosmicUnlock(&s, MMX_COSMIC_PRISM));
  assert(MmxCosmicEquip(&s, 0, MMX_COSMIC_SOLAR));
  assert(MmxCosmicEquip(&s, 1, MMX_COSMIC_PRISM));
  assert(MmxCosmicSelectedFusion(&s) == MMX_COSMIC_NO_FUSION);
  assert(!MmxCosmicActivateFusion(&s));
}
static void corrupted_state_is_rejected(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  s.equipped[0] = MMX_COSMIC_VOID;
  assert(!MmxCosmicStateValid(&s));
  MmxCosmicReset(&s);
  s.resonance = 101;
  assert(!MmxCosmicStateValid(&s));
  MmxCosmicReset(&s);
  s.specialization[0] = MMX_COSMIC_FORCE;
  assert(!MmxCosmicStateValid(&s));
  MmxCosmicReset(&s);
  s.unlocked = (uint16_t)(1u << MMX_COSMIC_ASPECT_COUNT);
  assert(!MmxCosmicStateValid(&s));
  MmxCosmicReset(&s);
  s.last_reward = 7;
  assert(!MmxCosmicStateValid(&s));
}
static void copied_state_replays_deterministically(void) {
  MmxCosmicState a, b;
  MmxCosmicReset(&a);
  assert(MmxCosmicUnlock(&a, MMX_COSMIC_STORM));
  b = a;
  for (int i = 0; i < 90; ++i) {
    MmxCosmicTick(&a);
    MmxCosmicTick(&b);
    (void)MmxCosmicReward(&a, (unsigned)(i % MMX_COSMIC_EVENT_COUNT));
    (void)MmxCosmicReward(&b, (unsigned)(i % MMX_COSMIC_EVENT_COUNT));
  }
  assert(memcmp(&a, &b, sizeof(a)) == 0);
}
int main(void) {
  new_state_is_opt_in();
  unlock_and_equip_are_checked();
  resonance_fusion_and_cooldown();
  unknown_pairs_do_not_auto_fuse();
  corrupted_state_is_rejected();
  copied_state_replays_deterministically();
  puts("Cosmic Constellation prototype: 6 synthetic tests passed");
  return 0;
}
