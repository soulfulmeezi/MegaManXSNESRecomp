#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Entirely host-side input planning. Actual position/velocity/collision stay
 * native to the game; this does not add persistent or save-state fields. */
typedef struct {
  int target_x, target_feet;
  int8_t direction;
  uint8_t ticks, ground_ticks;
  bool was_airborne;
} MmxCpuDeepDrop;

/* Deep drops are different from the existing 20..112px short descents.
 * Permit only a grounded human, a grounded CPU, a verified stable lower
 * shelf, a free vertical shaft and a landing within the visible camera.
 * Deep descents never imply that a blind jump across the void is safe. */
static inline bool MmxCpuDeepDropAllowed(
    bool human_grounded, bool cpu_grounded, bool floor_verified,
    bool clear_shaft, bool visible, int depth, int horizontal_distance,
    int leader_floor_offset) {
  return human_grounded && cpu_grounded && floor_verified &&
      clear_shaft && visible &&
      depth>=128 && depth<=256 &&
      horizontal_distance>=40 && horizontal_distance<=120 &&
      leader_floor_offset>=-24 && leader_floor_offset<=24;
}

/* Once airborne, steer toward the COMMITTED landing, not the moving player.
 * Never reverse after overshooting the target: that might cross another
 * void and recreates the original gap backtracking bug. */
static inline int MmxCpuDeepDropSteer(
    const MmxCpuDeepDrop *route,int x,bool grounded) {
  if (!route || !route->ticks || !route->direction) return 0;
  if (grounded && !route->was_airborne) return route->direction;
  return (route->target_x-x)*route->direction>12 ?
      route->direction : 0;
}

/* Successful landings reset the route. A stale/unverified destination must
 * hand control back to the normal native wall safety system. */
static inline bool MmxCpuDeepDropTick(
    MmxCpuDeepDrop *route,bool grounded,bool target_safe) {
  if (!route || !route->ticks) return false;
  if (!target_safe) {
    route->ticks=0;
    return false;
  }
  if (!grounded) {
    route->was_airborne=true;
    route->ground_ticks=0;
  } else if (route->was_airborne) {
    route->ticks=0;
    return false;
  } else if (++route->ground_ticks>45) {
    route->ticks=0;
    return false;
  }
  if (!--route->ticks) return false;
  return true;
}
