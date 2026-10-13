#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mmx_future_zero_color.h"
#include "mmx_zero_jupiter_stripes.h"
#include "mmx_cpu_safe_follow.h"
static uint16_t rgb(unsigned r,unsigned g,unsigned b) {
  return (uint16_t)(r|(g<<5)|(b<<10));
}
static void jupiter_palette_is_selective(void) {
  /* All seven red shades use Jupiter's cloudy SNES BGR555 ramp. */
  assert(MmxFutureZeroJupiter(rgb(31,5,3))==rgb(29,28,26)); /* cream white */
  assert(MmxFutureZeroJupiter(rgb(28,4,3))==rgb(27,25,22)); /* pale cloud */
  assert(MmxFutureZeroJupiter(rgb(25,4,3))==rgb(25,21,16)); /* sand */
  assert(MmxFutureZeroJupiter(rgb(22,3,4))==rgb(23,16,10)); /* orange belt */
  assert(MmxFutureZeroJupiter(rgb(19,3,2))==rgb(19,12,8)); /* bronze */
  assert(MmxFutureZeroJupiter(rgb(16,2,1))==rgb(13,9,6));  /* deep brown */
  assert(MmxFutureZeroJupiter(rgb(13,1,2))==rgb(6,5,4));   /* charcoal */
  /* Do not recolor blonde hair, skin, gem, neutral undersuit,
   * blue charge effects or other sprites' near-red shading. */
  assert(MmxFutureZeroJupiter(rgb(31,26,4))==rgb(31,26,4)); /* hair */
  assert(MmxFutureZeroJupiter(rgb(31,18,13))==rgb(31,18,13)); /* skin */
  assert(MmxFutureZeroJupiter(rgb(2,21,5))==rgb(2,21,5)); /* gem */
  assert(MmxFutureZeroJupiter(rgb(3,3,3))==rgb(3,3,3)); /* outline */
  assert(MmxFutureZeroJupiter(rgb(3,4,24))==rgb(3,4,24)); /* charge blue */
  assert(MmxFutureZeroJupiter(rgb(10,0,0))==rgb(10,0,0)); /* too dark */
}
static void jupiter_cloud_belts_are_pose_local(void) {
  uint16_t red=rgb(25,4,3), base=MmxFutureZeroJupiter(red);
  /* Stripe coloring really changes individual sprite pixels: neighboring
   * horizontal bands get different cloud-belt colors from the same index. */
  uint16_t cream=MmxZeroJupiterStripePixel(red,base,0,30,30);
  uint16_t rust=MmxZeroJupiterStripePixel(red,base,0,38,30);
  assert(cream!=rust);
  assert(cream!=base);
  assert(rust!=base);
  /* The pattern is anchored to each pose's armor-top instead of screen Y. */
  assert(cream!=MmxZeroJupiterStripePixel(red,base,0,30,27));
  /* Only actual red armor is eligible, independent of pose and body shape. */
  assert(MmxZeroJupiterStripePixel(rgb(31,26,4),base,0,38,30)==base);
  assert(MmxZeroJupiterStripePixel(rgb(31,18,13),base,0,38,30)==base);
  assert(MmxZeroJupiterStripePixel(rgb(2,21,5),base,0,38,30)==base);
  assert(MmxZeroJupiterStripePixel(rgb(13,1,2),base,0,38,30)==base);
  assert(MmxZeroJupiterStripePixel(red,base,0,29,30)==base);
  assert(MmxZeroJupiterStripePixel(red,base,128,38,30)==base);
  assert(MmxZeroJupiterStripePixel(red,base,0,128,30)==base);
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
  /* Wait only when X is testing the next route and Zero's own
   * platform is still safely supported. Fragile road must not wait. */
  assert(MmxCpuSafeFollowWaitForLanding(true,true,true,true,false,false,false,false));
  assert(!MmxCpuSafeFollowWaitForLanding(false,true,true,true,false,false,false,false));
  assert(!MmxCpuSafeFollowWaitForLanding(true,true,true,true,true,false,false,false));
  assert(!MmxCpuSafeFollowWaitForLanding(true,true,true,true,false,true,false,false));
  assert(!MmxCpuSafeFollowWaitForLanding(true,true,true,false,false,false,false,false));
  assert(!MmxCpuSafeFollowWaitForLanding(true,true,true,true,false,false,true,false));
  assert(!MmxCpuSafeFollowWaitForLanding(true,true,true,true,false,false,false,true));
}
static void vertical_jump_has_no_horizontal_input(void) {
  assert(MmxCpuSafeFollowJumpPad(0,0x40,0x80,1)==1);
  assert(MmxCpuSafeFollowJumpPad(1,0x40,0x80,1)==0x81);
  assert(MmxCpuSafeFollowJumpPad(-1,0x40,0x80,1)==0x41);
}
int main(void) {
  jupiter_palette_is_selective();
  jupiter_cloud_belts_are_pose_local();
  no_rush_and_camera_are_continuous();
  vertical_jump_has_no_horizontal_input();
  puts("Jupiter Zero stripes & safe-follow: 4 test groups passed");
  return 0;
}
