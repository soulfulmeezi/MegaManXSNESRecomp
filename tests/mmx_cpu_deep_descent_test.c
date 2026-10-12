#include "mmx_cpu_deep_descent.h"
#include <assert.h>
#include <stdio.h>

static void lower_verified_floor(void) {
  /* From the recorded gameplay: Zero's feet ~367; X >200 px lower. */
  assert(MmxCpuDeepDropAllowed(true,true,true,true,true,
                               224,72,0));
}
static void require_safe_ground_and_visible_target(void) {
  assert(!MmxCpuDeepDropAllowed(false,true,true,true,true,224,72,0));
  assert(!MmxCpuDeepDropAllowed(true,true,false,true,true,224,72,0));
  assert(!MmxCpuDeepDropAllowed(true,true,true,false,true,224,72,0));
  assert(!MmxCpuDeepDropAllowed(true,true,true,true,false,224,72,0));
  assert(!MmxCpuDeepDropAllowed(true,false,true,true,true,224,72,0));
}
static void reject_impossible_geometry(void) {
  assert(!MmxCpuDeepDropAllowed(true,true,true,true,true,270,72,0));
  assert(!MmxCpuDeepDropAllowed(true,true,true,true,true,90,72,0));
  assert(!MmxCpuDeepDropAllowed(true,true,true,true,true,224,176,0));
  assert(!MmxCpuDeepDropAllowed(true,true,true,true,true,224,72,50));
}
static void intermediate_ledge_when_direct_floor_unreachable(void) {
  assert(MmxCpuIntermediateDropAllowed(
      true,true,true,true,true,80,72,144,true));
  assert(!MmxCpuIntermediateDropAllowed(
      true,true,true,true,true,80,72,144,false));
  assert(!MmxCpuIntermediateDropAllowed(
      true,true,true,false,true,80,72,144,true));
  assert(!MmxCpuIntermediateDropAllowed(
      true,true,true,true,true,80,120,144,true));
  assert(!MmxCpuIntermediateDropAllowed(
      true,true,true,true,true,80,72,20,true));
}
static void committed_direction_and_no_backtracking(void) {
  MmxCpuDeepDrop route={.target_x=2604,.target_feet=591,
                         .direction=1,.ticks=150};
  assert(MmxCpuDeepDropSteer(&route,2532,true)==1);
  assert(MmxCpuDeepDropTick(&route,false,true));
  assert(MmxCpuDeepDropSteer(&route,2578,false)==1);
  assert(MmxCpuDeepDropSteer(&route,2598,false)==0);
  assert(MmxCpuDeepDropSteer(&route,2620,false)==0);
  assert(!MmxCpuDeepDropTick(&route,true,true));
  assert(route.ticks==0);
}
static void invalidated_floor_hands_back_recovery(void) {
  MmxCpuDeepDrop route={.target_x=2604,.target_feet=591,
                         .direction=1,.ticks=150};
  assert(MmxCpuDeepDropTick(&route,false,true));
  assert(!MmxCpuDeepDropTick(&route,false,false));
  assert(MmxCpuDeepDropSteer(&route,2575,false)==0);
}
static void abandon_lip_that_never_drops(void) {
  MmxCpuDeepDrop route={.target_x=2604,.target_feet=591,
                         .direction=1,.ticks=150};
  for(int i=0;i<45;++i)
    assert(MmxCpuDeepDropTick(&route,true,true));
  assert(!MmxCpuDeepDropTick(&route,true,true));
  assert(route.ticks==0);
}
int main(void) {
  lower_verified_floor();
  require_safe_ground_and_visible_target();
  reject_impossible_geometry();
  intermediate_ledge_when_direct_floor_unreachable();
  committed_direction_and_no_backtracking();
  invalidated_floor_hands_back_recovery();
  abandon_lip_that_never_drops();
  puts("CPU deep descent checks: 7 passed");
  return 0;
}
