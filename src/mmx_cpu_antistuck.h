#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Host-only memory for repeated ordinary, unproductive obstacle jumps.
 * Do not apply it to a committed gap crossing or native wall climb. */
typedef struct {
  int last_takeoff_x, last_takeoff_y;
  uint8_t attempts, wait_ticks;
} MmxCpuAntiStuckState;

/* Native jumps must have physically useful progress between takeoffs.
 * Do not count an airborne apex as progress; only stable ground matters. */
static inline void MmxCpuAntiStuckTick(MmxCpuAntiStuckState *s,
                                       bool grounded, int x, int y) {
  if (!s) return;
  if (s->wait_ticks) {
    --s->wait_ticks;
    if (!s->wait_ticks) s->attempts=0; /* allow a later re-test */
  }
  if (grounded && s->attempts &&
      (x-s->last_takeoff_x>28 || s->last_takeoff_x-x>28 ||
       y-s->last_takeoff_y>32 || s->last_takeoff_y-y>32)) {
    s->attempts=0;
    s->wait_ticks=0;
  }
}

/* Called ONLY when grounded and about to execute an ordinary obstacle/stall
 * jump with no verified pit crossing, elevated wall or explicit landing.
 * Permit two nearby attempts; wait 150 frames before trying again if neither
 * attempt produced meaningful progress. Walking and planned routes can still
 * proceed while the ordinary hop is throttled. */
static inline bool MmxCpuAntiStuckTryHop(MmxCpuAntiStuckState *s,
                                         int x, int y) {
  if (!s) return true;
  if (s->wait_ticks) return false;
  bool same=s->attempts &&
      x-s->last_takeoff_x<=16 && s->last_takeoff_x-x<=16 &&
      y-s->last_takeoff_y<=24 && s->last_takeoff_y-y<=24;
  if (same) {
    if (s->attempts>=2) {
      s->wait_ticks=150;
      return false;
    }
    ++s->attempts;
  } else {
    s->last_takeoff_x=x;
    s->last_takeoff_y=y;
    s->attempts=1;
  }
  return true;
}
