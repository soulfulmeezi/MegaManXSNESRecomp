#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mmx_future_zero_color.h"
#include "mmx_cpu_safe_follow.h"
static uint16_t rgb(unsigned r,unsigned g,unsigned b) {
  return (uint16_t)(r|(g<<5)|(b<<10));
}
static void yellow_black_is_selective(void) {
  uint16_t highlight=rgb(31,5,3);
  uint16_t medium=rgb(22,3,4);
  uint16_t shadow=rgb(13,1,2);
  assert(MmxFutureZeroYellowBlack(highlight)==rgb(31,28,2));
  assert(MmxFutureZeroYellowBlack(medium)==rgb(22,19,2));
  assert(MmxFutureZeroYellowBlack(shadow)==rgb(4,4,4));
  assert(MmxFutureZeroYellowBlack(rgb(31,26,4))==rgb(31,26,4)); /* hair */
  assert(MmxFutureZeroYellowBlack(rgb(31,18,13))==rgb(31,18,13)); /* skin */
  assert(MmxFutureZeroYellowBlack(rgb(2,21,5))==rgb(2,21,5)); /* gem */
  assert(MmxFutureZeroYellowBlack(rgb(3,3,3))==rgb(3,3,3)); /* outline */
  assert(MmxFutureZeroYellowBlack(rgb(3,4,24))==rgb(3,4,24)); /* charge blue */
}
static void no_rush_and_camera_are_continuous(void) {
  assert(MmxCpuSafeFollowLimit(false)==224);
  assert(MmxCpuSafeFollowLimit(true)==288);
  assert(MmxCpuSafeFollowClamp(500,250,false)==474);
  assert(MmxCpuSafeFollowClamp(500,250,true)==500);
  assert(MmxCpuSafeFollowClamp(0,400,true)==112);
  assert(MmxCpuSafeFollowCameraX(200,0,false)==100);
  assert(MmxCpuSafeFollowCameraX(200,0,true)==120);
  assert(MmxCpuSafeFollowCameraX(0,200,true)==80);
  assert(MmxCpuSafeFollowCameraX(1000,840,true)==920);
  assert(MmxCpuSafeFollowCameraX(1000,839,true)==920);
  assert(MmxCpuSafeFollowCameraX(1000,700,true)==920);
  assert(MmxCpuSafeFollowCameraX(1000,900,true)==950);
}
static void vertical_jump_has_no_horizontal_input(void) {
  assert(MmxCpuSafeFollowJumpPad(0,0x40,0x80,1)==1);
  assert(MmxCpuSafeFollowJumpPad(1,0x40,0x80,1)==0x81);
  assert(MmxCpuSafeFollowJumpPad(-1,0x40,0x80,1)==0x41);
}
int main(void) {
  yellow_black_is_selective();
  no_rush_and_camera_are_continuous();
  vertical_jump_has_no_horizontal_input();
  puts("Future Zero color & safe-follow: 3 test groups passed");
  return 0;
}
