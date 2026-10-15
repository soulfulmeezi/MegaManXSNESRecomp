#pragma once
#include "mmx_zero.h"
#include "mmx_weapons.h"
#include "mmx_weapon_combat.h"
#include <string.h>

enum { MMX_COOP_X, MMX_COOP_ZERO };
enum { MMX_COOP_ABSENT, MMX_COOP_ALIVE, MMX_COOP_FALLEN };

/* Native routines keep their original addresses. Only these owned ranges are
 * projected into WRAM when a different player runs; world/task RAM stays live.
 * $1F83..86 are shared subtanks, not projected. Weapons start at $1F87. */
typedef struct MmxCoopPlayer {
  uint8_t body[0x90];                /* $0BA8..0C37 */
  uint8_t auxiliaries[0x1e0];        /* armor/charge objects $0C38..0E17 */
  uint8_t shots[0x200];              /* $1228..1427 */
  uint8_t energy[16];               /* $1F87..96, shared unlock bits excluded */
  MmxZeroState zero;
  MmxWeaponsState weapons;
  MmxWeaponCombatState combat;
  uint16_t input, pressed;          /* engine's 12-bit seat input */
  uint8_t character, status;
  uint8_t shot_command, hud_state;  /* $1F0D and $1F12 */
} MmxCoopPlayer;

typedef struct MmxCoopState {
  MmxCoopPlayer players[2];
  uint8_t initialized, current, controller_pass, stage;
  /* Post-P1 registers survive P2's controller pass, including a save taken
   * during an interpreter deadline. Guest cycles are never rolled back. */
  uint16_t return_a, return_x, return_y, return_s;
  uint8_t return_p, return_db;
  uint8_t time_tick, effect_return; /* Previous seat + 1 during a dash-effect update; formerly reserved. */
  uint16_t object_a, object_x, object_y, object_s, object_d, object_entry;
  uint8_t object_p, object_db, object_pass, object_reserved;
  uint16_t contact_a, contact_x, contact_y, contact_s, contact_d, contact_entry;
  uint8_t contact_p, contact_db, contact_pass, platform_riders; /* Pending .2C bits; formerly reserved. */
  uint8_t enrolled, select_hold, select_armed, stage_pending;
  uint8_t menu_owner, menu_last, p1_select_hold, p1_select_armed; /* Last two bytes were reserved. */
  uint8_t pickup_owner[16]; /* Native item slots $1628 + index*$30. */
  uint16_t pickup_s, pickup_d;
  uint8_t pickup_pass, pickup_reserved[3];
  uint8_t anchor, solo_death[2], respawn_pending; /* Seat bits; formerly reserved. */
  uint8_t death_flags[2][8]; /* Native freeze flags around a partner's death. */
  uint8_t scene_owner, scene_phase, door_pass, scene_reserved;
  uint16_t door_s, door_d, door_entry;
  uint16_t slime_p2; /* Capture owner bits for $1428 + slot*$40; formerly reserved. */
} MmxCoopState;

enum { MMX_COOP_LEGACY_STATE_SIZE = 4648 };
/* v14 saves / v13 captures predate Modern's per-player extension. Keep
 * every original body, weapon and scheduler byte at its corresponding field. */
static inline void MmxCoopImportLegacy(MmxCoopState *out, const uint8_t *bytes) {
  const size_t extra = sizeof(MmxZeroModernState);
  const size_t old_player = sizeof(MmxCoopPlayer) - extra;
  const size_t prefix = offsetof(MmxCoopPlayer, zero) + MMX_ZERO_HEALTH_STATE_SIZE;
  memset(out, 0, sizeof(*out));
  for (unsigned i = 0; i < 2; ++i) {
    uint8_t *p = (uint8_t *)&out->players[i];
    memcpy(p, bytes + i * old_player, prefix);
    memcpy(p + prefix + extra, bytes + i * old_player + prefix, old_player - prefix);
  }
  memcpy(&out->initialized, bytes + old_player * 2,
      MMX_COOP_LEGACY_STATE_SIZE - old_player * 2);
}

/* Follow the controlled character's POSITION rather than their d-pad.
 * Wall-kick steering and committed pit routes can temporarily override. */
static inline int MmxCoopCpuFollowDirection(int target_dx) {
  return target_dx>40 ? 1 : target_dx< -40 ? -1 : 0;
}
/* Once a terrain-verified crossing is launched, the midair flight follows
 * its committed takeoff heading until actual landing or wall contact.
 * The ordinary 40px proximity deadzone must not cancel a jump in flight. */
static inline int MmxCoopCpuAirRouteDirection(
    int target_dx, int committed_dir, bool airborne) {
  if (airborne && committed_dir)
    return committed_dir>0 ? 1 : -1;
  return MmxCoopCpuFollowDirection(target_dx);
}
/* Bound the committed wall approach through the entire native dash,
 * jump, descent and possible long wall approach. Wall contact or landing
 * still terminates the route immediately. Keep within uint8_t range. */
static inline uint8_t MmxCoopCpuPitApproachTicks(int distance) {
  return distance>=96 ? 220 : 180;
}
/* Zero's X3 small buster requires 21 charge ticks. Reserve the full
 * 201-tick saber-ready charge for bosses and minibosses. */
static inline unsigned MmxCoopCpuZeroChargeGoal(bool boss) {
  return boss ? 201u : 21u;
}
/* Short, falling highway road sections warrant an EARLY jump, but only
 * toward solid support beyond the first missing span. The native jump
 * still controls the actual crossing; a wide gap is never presumed safe. */
static inline bool MmxCoopCpuCrumbleJumpAllowed(
    bool grounded,int first_gap,int landing_distance,bool headroom) {
  return grounded && headroom && first_gap>=20 && first_gap<=72 &&
         landing_distance>=first_gap+24 && landing_distance<=176;
}
/* A vanishing platform with no verified stable landing may use a real
 * opposite wall. Do not invent a catch when the scanner sees none. */
static inline bool MmxCoopCpuCrumbleWallAllowed(
    bool grounded,int first_gap,bool stable_landing,bool verified_wall) {
  return grounded && first_gap>=20 && first_gap<=72 &&
         !stable_landing && verified_wall;
}
/* A route toward a verified higher platform needs the full native
 * wall-kick budget, even when the leader is only 32-95px above.
 * Accidental wall slides without an upper destination still use 2 kicks. */
static inline bool MmxCoopCpuFullWallClimb(bool pit_wall_arrival,
                                            bool verified_upper_goal) {
  return pit_wall_arrival || verified_upper_goal;
}
/* A long climb may need a brief OUTWARD kick, just like a player.
 * Keep ordinary slides and the verified wall climb as the default. */
static inline uint8_t MmxCoopCpuWallPushFrames(bool tall, bool arc_mode) {
  return tall ? (arc_mode ? 5u : 0u) : 4u;
}
static inline bool MmxCoopCpuWallApex(int current_y,int previous_y,
                                      bool arc_mode) {
  /* Integer pixel coordinates can briefly repeat DURING ascent.
   * Wide-arc mode waits for genuine downward motion. */
  return arc_mode ? current_y>previous_y : current_y>=previous_y;
}

/* Pure controller decision for one local companion terrain scan.
 * Sensor inputs belong to the CPU's OWN position, never the human jump pad.
 * WAIT forbids walking into unverified void; CLIMB requests takeoff against
 * a confirmed wall and then native wall-slide/wall-kick input takes over.
 * Inline makes the decision independently testable without owning a ROM. */
typedef enum {
  MMX_CPU_MOVE_WALK, MMX_CPU_MOVE_WAIT,
  MMX_CPU_MOVE_JUMP, MMX_CPU_MOVE_CLIMB, MMX_CPU_MOVE_DROP
} MmxCpuMoveDecision;
static inline MmxCpuMoveDecision MmxCoopCpuChooseMove(
    bool ground_missing, bool safe_landing, bool climb_route,
    bool wall_near, bool safe_drop, bool obstacle_or_stall) {
  if (ground_missing)
    return safe_drop ? MMX_CPU_MOVE_DROP :
           climb_route ? MMX_CPU_MOVE_CLIMB :
           safe_landing ? MMX_CPU_MOVE_JUMP : MMX_CPU_MOVE_WAIT;
  if (climb_route && wall_near) return MMX_CPU_MOVE_CLIMB;
  if (obstacle_or_stall) return MMX_CPU_MOVE_JUMP;
  return MMX_CPU_MOVE_WALK;
}

/* X can stand nearly directly above the CPU, so horizontal follow has
 * no direction. Choose a nearby, physically sensed wall to climb rather
 * than idling. Prefer the wall toward X when a horizontal preference
 * exists; when directly underneath, use the nearest visible wall face.
 * Do not invent a direction if no solid wall was detected. */
static inline int MmxCoopCpuChooseClimbDirection(int target_dx,
                                                  int wall_right,
                                                  int wall_left) {
  if (target_dx>=12) return wall_right ? 1 : 0;
  if (target_dx<=-12) return wall_left ? -1 : 0;
  if (!wall_right) return wall_left ? -1 : 0;
  if (!wall_left) return 1;
  return wall_right<=wall_left ? 1 : -1;
}

/* The trusted co-op plugin prepares the owner-supplied X3 ROM, then enables
 * the chosen roster. This mode excludes single-player character exchange. */
bool MmxCoopEnable(unsigned p1_character);
/* Host-only option: synthesize P2 gamepad input during local co-op. */
void MmxCoopSetCpuCompanion(bool active);
/* Desktop host: physical L2 analog state, independent of the 12 SNES buttons. */
void MmxCoopSetSwitchTrigger(bool held);
void MmxCoopDisable(void);
bool MmxCoopEnabled(void);
void MmxCoopReset(void);
MmxCoopState MmxCoopGetState(void);
bool MmxCoopValidState(const MmxCoopState *state);
void MmxCoopSetState(const MmxCoopState *state);
void MmxCoopInitialize(uint8_t ram[0x20000]);
void MmxCoopCapture(uint8_t ram[0x20000]);
/* Developer mod opt-in; host-only logging, outside save/netplay state. */
bool MmxCoopDiagnosticsEnabled(void);
void MmxCoopSetDiagnosticsEnabled(bool active);
void MmxCoopDiagnosticFrame(const uint8_t ram[0x20000]);
bool MmxCoopSelect(uint8_t ram[0x20000], unsigned player);
void MmxCoopPoll(uint16_t p1, uint16_t p2);
void MmxCoopApplyInput(uint8_t ram[0x20000]);
bool MmxCoopFrameTick(uint8_t ram[0x20000]);
bool MmxCoopTransitionActive(void);
bool MmxCoopFindLanding(const uint8_t ram[0x20000],uint16_t *x,uint16_t *y);
/* Caller must first validate the landing space. This does not grant re-entry
 * to a fallen player, and must not become the public join path by itself. */
bool MmxCoopPlacePartner(uint8_t ram[0x20000], uint16_t x, uint16_t y);
void MmxCoopRegisterHooks(void);
/* Host side, before each frame: ROM-dependent hook setup. */
void MmxCoopHostFrame(void);
void MmxCoopLiftCarry(uint8_t *ram);
void MmxCoopSyncPriority(uint8_t *ram);
/* End-of-frame hook for --coop-trace; no effect unless tracing. */
void MmxCoopTraceFrame(const uint8_t ram[0x20000]);
