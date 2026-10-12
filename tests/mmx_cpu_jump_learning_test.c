#include "mmx_cpu_jump_learning.h"
#include <assert.h>
#include <stdio.h>

static const MmxCpuJumpDemo *takeoff(
    MmxCpuJumpLearning *s,int x,int y,unsigned cues) {
  return MmxCpuJumpLearnObserve(s,0,0,x,y,true,true,true,true,1,true,false,cues);
}
static void record_real_success_and_match(void) {
  MmxCpuJumpLearning s={0};
  assert(!takeoff(&s,3681,383,MMX_CPU_DEMO_GAP));
  for(int i=0;i<19;++i)
    assert(!MmxCpuJumpLearnObserve(&s,0,0,3681+i*4,383-i,true==false,
                                  false,false,1,true,false,MMX_CPU_DEMO_GAP));
  assert(!MmxCpuJumpLearnObserve(&s,0,0,3770,333,false,false,false,1,false,false,0));
  const MmxCpuJumpDemo *d=MmxCpuJumpLearnObserve(
      &s,0,0,3815,373,true,true,true,1,false,false,0);
  assert(d && d->valid && d->start_x==3681 && d->end_x==3815);
  assert(d->jump_hold>=16 && d->airborne_ticks==20);
  assert(d->cues==MMX_CPU_DEMO_GAP);
  assert(MmxCpuJumpLearnMatch(&s,0,3675,383,1,3810,373,true)==d);
  assert(MmxCpuJumpLearnHold(d,20)>=16);
  assert(!MmxCpuJumpLearnMatch(&s,0,3675,383,1,3810,373,false));
  assert(!MmxCpuJumpLearnMatch(&s,1,3675,383,1,3810,373,true));
  assert(!MmxCpuJumpLearnMatch(&s,0,3675,383,-1,3810,373,true));
}
static void no_airborne_or_unsafe_landing_is_not_learning(void) {
  MmxCpuJumpLearning s={0};
  assert(!takeoff(&s,100,350,MMX_CPU_DEMO_GAP));
  for(int i=0;i<12;++i)
    MmxCpuJumpLearnObserve(&s,0,0,100,350,true,true,true,1,false,false,0);
  for(int i=0;i<MMX_CPU_DEMO_MAX;++i) assert(!s.history[i].valid);
  assert(!takeoff(&s,200,350,MMX_CPU_DEMO_GAP));
  for(int i=0;i<6;++i)
    MmxCpuJumpLearnObserve(&s,0,0,200+i*8,290,false,false,false,1,true,false,0);
  assert(!MmxCpuJumpLearnObserve(&s,0,0,270,350,true,false,false,1,false,false,0));
  for(int i=0;i<MMX_CPU_DEMO_MAX;++i) assert(!s.history[i].valid);
}
static void not_learning_stationary_bounces_or_obstacles_as_gaps(void) {
  MmxCpuJumpLearning s={0};
  assert(!takeoff(&s,400,300,MMX_CPU_DEMO_OBSTACLE));
  for(int i=0;i<8;++i)
    MmxCpuJumpLearnObserve(&s,0,0,402,270,false,false,false,1,true,false,0);
  const MmxCpuJumpDemo *d=MmxCpuJumpLearnObserve(
      &s,0,0,450,300,true,true,true,1,false,false,0);
  assert(d && d->valid);
  assert(!MmxCpuJumpLearnMatch(&s,0,400,300,1,450,300,true));
}
static void direction_and_session_are_respected(void) {
  MmxCpuJumpLearning s={0};
  assert(!takeoff(&s,900,350,MMX_CPU_DEMO_GAP));
  for(int i=0;i<8;++i)
    MmxCpuJumpLearnObserve(&s,0,0,900+i*8,315,false,false,false,1,true,false,0);
  MmxCpuJumpLearnObserve(&s,0,0,980,351,true,true,true,1,false,false,0);
  assert(s.history[0].valid);
  MmxCpuJumpLearnObserve(&s,1,0,1000,351,true,true,true,1,false,false,0);
  assert(!s.history[0].valid);
}
static void fragile_takeoff_stable_landing_can_be_learned(void) {
  MmxCpuJumpLearning s={0};
  /* Fragile road under human feet is still a legitimate jump source.
   * Only a stable, verified landing should be stored. */
  assert(!MmxCpuJumpLearnObserve(
      &s,0,0,3681,383,true,true,false,1,true,false,
      MMX_CPU_DEMO_GAP|MMX_CPU_DEMO_FRAGILE));
  for (int i=0;i<8;++i)
    MmxCpuJumpLearnObserve(&s,0,0,3681+i*10,335,
                           false,false,false,1,true,false,0);
  const MmxCpuJumpDemo *d=MmxCpuJumpLearnObserve(
      &s,0,0,3790,382,true,true,true,1,false,false,0);
  assert(d && d->valid);
  assert((d->cues&MMX_CPU_DEMO_FRAGILE)!=0);
}
static void clamp_cross_character_holds(void) {
  MmxCpuJumpDemo d={.valid=true,.jump_hold=2};
  assert(MmxCpuJumpLearnHold(&d,20)==16);
  d.jump_hold=31;
  assert(MmxCpuJumpLearnHold(&d,20)==24);
  assert(MmxCpuJumpLearnHold(NULL,20)==20);
}
int main(void) {
  record_real_success_and_match();
  no_airborne_or_unsafe_landing_is_not_learning();
  not_learning_stationary_bounces_or_obstacles_as_gaps();
  direction_and_session_are_respected();
  fragile_takeoff_stable_landing_can_be_learned();
  clamp_cross_character_holds();
  puts("CPU human jump demonstration: 6 tests passed");
  return 0;
}
