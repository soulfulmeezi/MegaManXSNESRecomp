#include "mmx_cpu_antistuck.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void repeated_same_spot_waits(void) {
  MmxCpuAntiStuckState s={0};
  assert(MmxCpuAntiStuckTryHop(&s,7211,431));
  assert(MmxCpuAntiStuckTryHop(&s,7211,431));
  assert(!MmxCpuAntiStuckTryHop(&s,7211,431));
  assert(s.attempts==2 && s.wait_ticks==150);
  for (int i=0;i<149;++i) {
    MmxCpuAntiStuckTick(&s,true,7211,431);
    assert(!MmxCpuAntiStuckTryHop(&s,7211,431));
  }
  MmxCpuAntiStuckTick(&s,true,7211,431);
  assert(MmxCpuAntiStuckTryHop(&s,7211,431));
}
static void real_progress_resets_history(void) {
  MmxCpuAntiStuckState s={0};
  assert(MmxCpuAntiStuckTryHop(&s,200,350));
  assert(MmxCpuAntiStuckTryHop(&s,201,352));
  assert(!MmxCpuAntiStuckTryHop(&s,201,352));
  MmxCpuAntiStuckTick(&s,true,240,350);
  assert(s.wait_ticks==0 && s.attempts==0);
  assert(MmxCpuAntiStuckTryHop(&s,240,350));
}
static void airborne_apex_does_not_fake_progress(void) {
  MmxCpuAntiStuckState s={0};
  assert(MmxCpuAntiStuckTryHop(&s,500,400));
  MmxCpuAntiStuckTick(&s,false,510,330);
  assert(s.attempts==1);
  MmxCpuAntiStuckTick(&s,true,506,403);
  assert(MmxCpuAntiStuckTryHop(&s,506,403));
  assert(!MmxCpuAntiStuckTryHop(&s,506,403));
}
static void different_height_is_new_route(void) {
  MmxCpuAntiStuckState s={0};
  assert(MmxCpuAntiStuckTryHop(&s,4055,385));
  MmxCpuAntiStuckTick(&s,true,4057,351);
  assert(MmxCpuAntiStuckTryHop(&s,4057,351));
  assert(s.attempts==1);
}
static void changed_obstacle_position_allows_hop(void) {
  MmxCpuAntiStuckState s={0};
  assert(MmxCpuAntiStuckTryHop(&s,100,300));
  assert(MmxCpuAntiStuckTryHop(&s,102,300));
  assert(MmxCpuAntiStuckTryHop(&s,130,300));
  assert(s.attempts==1 && s.wait_ticks==0);
}
int main(void) {
  repeated_same_spot_waits();
  real_progress_resets_history();
  airborne_apex_does_not_fake_progress();
  different_height_is_new_route();
  changed_obstacle_position_allows_hop();
  puts("CPU anti-stuck checks: 5 passed");
  return 0;
}
