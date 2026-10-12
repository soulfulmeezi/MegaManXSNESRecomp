#pragma once
#include <stdint.h>
/* Small ROM-free policy for opt-in offline CPU companion freedom.
 * X's native authored stage bounds and collision remain mandatory.
 * Shared couch/online play keeps its exact stock 224px separation. */
static inline int MmxCpuSafeFollowLimit(bool eligible) {
  return eligible ? 288 : 224;
}
static inline int MmxCpuSafeFollowClamp(int player_x,int other_x,bool eligible) {
  int max=MmxCpuSafeFollowLimit(eligible);
  return player_x<other_x-max ? other_x-max :
         player_x>other_x+max ? other_x+max : player_x;
}
/* Camera transitions smoothly from ordinary midpoint for near actors
 * toward the human leader once distance exceeds 160px. At the maximum
 * 288px distance the human is ~208px from the left edge, rather than
 * offscreen. X can still physically progress until the new limit. */
static inline int MmxCpuSafeFollowCameraX(
    int human_x,int follower_x,bool eligible) {
  int delta=human_x-follower_x;
  if (!eligible || (delta>=-160 && delta<=160))
    return follower_x+delta/2;
  return human_x-(delta>0?80:-80);
}
/* A defensive vertical jump must not quietly hold LEFT during B sustain. */
static inline uint16_t MmxCpuSafeFollowJumpPad(
    int direction,uint16_t left,uint16_t right,uint16_t jump) {
  return (uint16_t)(jump | (direction>0?right:direction<0?left:0));
}
