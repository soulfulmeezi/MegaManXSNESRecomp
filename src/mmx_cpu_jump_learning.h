#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Session-only observation. The CPU learns WHAT X/Zero successfully did;
 * terrain validation remains authoritative, not a button macro/replay.
 * No ROM writes, save-format changes or cross-session persistence. */
enum { MMX_CPU_DEMO_GAP=1, MMX_CPU_DEMO_OBSTACLE=2,
       MMX_CPU_DEMO_FRAGILE=4 };
enum { MMX_CPU_DEMO_MAX=12 };
typedef struct {
  int16_t start_x,start_y,end_x,end_y;
  uint8_t stage, character, cues, jump_hold, airborne_ticks;
  int8_t direction;
  bool dash_used, valid;
} MmxCpuJumpDemo;
typedef struct {
  MmxCpuJumpDemo history[MMX_CPU_DEMO_MAX], pending;
  uint8_t next, stage, character;
  bool initialized, last_grounded, previous_jump,
       tracking, went_airborne, hold_released;
  uint8_t grounded_arm_ticks;
} MmxCpuJumpLearning;

static inline void MmxCpuJumpLearnReset(MmxCpuJumpLearning *s) {
  if (s) memset(s,0,sizeof(*s));
}

/* Sample ONCE per game input frame. The native character's grounded state
 * and genuine landing are necessary; a B press by itself isn't a success.
 * Inputs: dir -1/+1/0, independent of the follower's own decisions. */
static inline const MmxCpuJumpDemo *MmxCpuJumpLearnObserve(
    MmxCpuJumpLearning *s,unsigned stage,unsigned character,
    int x,int y,bool grounded,bool safe_ground,
    int dir,bool jump,bool dash,unsigned cues) {
  if (!s) return NULL;
  if (!s->initialized || s->stage!=(uint8_t)stage) {
    MmxCpuJumpLearnReset(s);
    s->stage=(uint8_t)stage;
    s->character=(uint8_t)character;
    s->initialized=true;
    s->last_grounded=grounded;
  }
  if (s->character!=(uint8_t)character) {
    s->tracking=false;
    s->went_airborne=false;
    s->character=(uint8_t)character;
    s->last_grounded=grounded;
    s->previous_jump=jump;
    return NULL;
  }
  const MmxCpuJumpDemo *recorded=NULL;
  if (s->tracking) {
    if (!grounded) {
      s->went_airborne=true;
      s->grounded_arm_ticks=0;
      if (s->pending.airborne_ticks<140) ++s->pending.airborne_ticks;
      if (!jump) s->hold_released=true;
      if (!s->hold_released && jump && s->pending.jump_hold<32)
        ++s->pending.jump_hold;
      if (dash) s->pending.dash_used=true;
      if (!s->pending.direction && dir) s->pending.direction=(int8_t)dir;
    } else if (s->went_airborne) {
      /* Only a genuine safe landing, with measurable displacement, can
       * become a demonstration. A same-spot bounce learns nothing. */
      int dx=x-(int)s->pending.start_x;
      int dy=y-(int)s->pending.start_y;
      int adx=dx<0?-dx:dx,ady=dy<0?-dy:dy;
      if (safe_ground && s->pending.airborne_ticks>=4 &&
          s->pending.airborne_ticks<=110 &&
          (adx>=24 || ady>=24) &&
          (!s->pending.direction || dx*s->pending.direction>=0)) {
        MmxCpuJumpDemo *slot=&s->history[s->next];
        *slot=s->pending;
        slot->end_x=(int16_t)x;
        slot->end_y=(int16_t)y;
        if (!slot->direction) slot->direction=(int8_t)(dx>0?1:dx<0?-1:0);
        slot->valid=true;
        recorded=slot;
        s->next=(uint8_t)((s->next+1)%MMX_CPU_DEMO_MAX);
      }
      s->tracking=false;
      s->went_airborne=false;
    } else if (++s->grounded_arm_ticks>8) {
      /* A jump input that the game never accepted is not training. */
      s->tracking=false;
    }
  }
  /* Arm a takeoff only on an actual NEW player B edge, on safe ground.
   * Collision context is captured when the player chooses to jump. */
  if (!s->tracking && grounded && safe_ground && jump &&
      !s->previous_jump) {
    memset(&s->pending,0,sizeof(s->pending));
    s->pending.start_x=(int16_t)x;
    s->pending.start_y=(int16_t)y;
    s->pending.stage=(uint8_t)stage;
    s->pending.character=(uint8_t)character;
    s->pending.direction=(int8_t)dir;
    s->pending.cues=(uint8_t)cues;
    s->pending.jump_hold=1;
    s->pending.dash_used=dash;
    s->tracking=true;
    s->grounded_arm_ticks=0;
    s->hold_released=false;
  }
  s->previous_jump=jump;
  s->last_grounded=grounded;
  return recorded;
}

/* Find a same-stage, same-direction successful landing near this real
 * verified native crossing. This is a hint for B hold length ONLY; it
 * never authorizes a jump or redirects into missing ground. */
static inline const MmxCpuJumpDemo *MmxCpuJumpLearnMatch(
    const MmxCpuJumpLearning *s,unsigned stage,int x,int y,
    int dir,int planned_x,int planned_y,bool verified_route) {
  if (!s || !verified_route || !dir) return NULL;
  const MmxCpuJumpDemo *best=NULL;
  int best_score=100000;
  for (unsigned i=0;i<MMX_CPU_DEMO_MAX;++i) {
    const MmxCpuJumpDemo *d=&s->history[i];
    if (!d->valid || d->stage!=(uint8_t)stage ||
        d->direction!=dir || !(d->cues&MMX_CPU_DEMO_GAP))
      continue;
    int tx=x-(int)d->start_x,ty=y-(int)d->start_y;
    int lx=planned_x-(int)d->end_x,ly=planned_y-(int)d->end_y;
    int ax=tx<0?-tx:tx,ay=ty<0?-ty:ty;
    int bx=lx<0?-lx:lx,by=ly<0?-ly:ly;
    if (ax>48 || ay>24 || bx>48 || by>28) continue;
    int score=ax+ay*2+bx+by*2;
    if (score<best_score) {
      best_score=score;
      best=d;
    }
  }
  return best;
}

/* Cross-character physics differ. Never blindly replay the human's
 * duration; adopt only a small adjustment around the proven default. */
static inline uint8_t MmxCpuJumpLearnHold(
    const MmxCpuJumpDemo *d,uint8_t fallback) {
  if (!d || !d->valid) return fallback;
  unsigned n=d->jump_hold;
  if (n<16) n=16;
  if (n>24) n=24;
  return (uint8_t)n;
}
