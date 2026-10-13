#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mmx_future_zero_color.h"
#include "mmx_zero_jupiter_stripes.h"
#include "mmx_zero_jupiter_shell.h"
#include "mmx_zero_jupiter_planet.h"
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
static void jupiter_shell_is_clouds_not_metal(void) {
  uint16_t silver=rgb(24,24,24), red=rgb(25,4,3);
  uint16_t white=rgb(29,28,26);
  assert(MmxJovianMaterial(silver,60,37)); /* silver plate becomes cloud */
  assert(MmxJovianMaterial(red,60,37));
  assert(MmxJovianMaterial(rgb(2,21,5),60,37)); /* green chest gem */
  assert(!MmxJovianMaterial(rgb(31,18,13),60,37)); /* facial skin */
  assert(!MmxJovianMaterial(rgb(31,26,4),32,37)); /* blond hair */
  assert(MmxJovianMaterial(rgb(31,26,4),90,37)); /* golden boot */
  assert(!MmxJovianMaterial(rgb(3,3,3),60,37)); /* black suit */
  assert(!MmxJovianMaterial(rgb(3,4,24),60,37)); /* blue eye / face */
  uint16_t cloud=MmxJovianShellColor(silver,silver,50,30,20,37);
  uint16_t storm=MmxJovianShellColor(silver,silver,50,37,20,37);
  assert(cloud!=silver && storm!=silver && cloud!=storm);
  /* Two different metallic colors on a single material band are equal:
   * the original panel-by-panel silver shading has been replaced. */
  assert(cloud==MmxJovianShellColor(white,white,50,30,20,37));
  assert(MmxJovianShellColor(rgb(31,18,13),rgb(31,18,13),50,30,20,37)==rgb(31,18,13));
  assert(MmxJovianShellColor(rgb(3,3,3),rgb(3,3,3),50,30,20,37)==rgb(3,3,3));
  /* Same Zero-pose coordinate gives same color, independently of camera. */
  assert(cloud==MmxJovianShellColor(silver,silver,50,30,20,37));
}
static void jupiter_planet_masks_replace_sprite_panels(void) {
  uint8_t sprite[MMX_JUPITER_POSE_PIXELS]={0},mask[MMX_JUPITER_POSE_PIXELS];
  uint16_t palette[128]={0};
  palette[16]=rgb(29,28,26); /* silver metal */
  palette[17]=rgb(23,15,4);  /* golden armor / hair */
  palette[18]=rgb(31,18,13); /* skin */
  palette[19]=rgb(3,3,3);    /* black undersuit */
  palette[20]=rgb(2,21,5);   /* green gem */
  palette[21]=rgb(24,24,24); /* secondary metallic panel */
  palette[22]=rgb(3,4,24);   /* blue visor / eye */
  palette[23]=rgb(15,15,15); /* metallic seam */
  /* A tiny fictional Zero sprite: a head, golden hair and a torso with
   * two different metal source shades and a black vertical suit panel. */
  for(unsigned y=20;y<=23;++y) for(unsigned x=58;x<=67;++x)
    sprite[y*128+x]=17;
  for(unsigned y=24;y<=29;++y) for(unsigned x=60;x<=65;++x)
    sprite[y*128+x]=18;
  sprite[26*128+64]=22;
  for(unsigned y=38;y<70;++y) for(unsigned x=45;x<81;++x)
    sprite[y*128+x]=x<62?16:21;
  for(unsigned y=38;y<70;++y) sprite[y*128+62]=23;
  for(unsigned y=38;y<70;++y) sprite[y*128+80]=19;
  sprite[44*128+55]=20;
  for(unsigned y=82;y<95;++y)for(unsigned x=50;x<60;++x)
    sprite[y*128+x]=17;
  unsigned top=0,spot_x=0,spot_y=0;
  MmxJupiterPlanetMask(sprite,palette,mask,&top,&spot_x,&spot_y);
  assert(top==20);
  assert(mask[20*128+60]==0); /* hair remains original */
  assert(mask[26*128+62]==0); /* skin remains original */
  assert(mask[26*128+64]==0); /* eye remains original */
  assert(mask[45*128+80]==0); /* black undersuit remains original */
  assert(mask[45*128+55]==1); /* green chest detail becomes planet */
  assert(mask[45*128+50]==1); /* silver metal is now cloud matter */
  assert(mask[45*128+75]==1); /* both plate shades use one material */
  assert(mask[45*128+62]==2); /* thin internal metallic seam erased */
  assert(mask[87*128+54]==1); /* gold boots become Jupiter */
  uint16_t old1=palette[sprite[45*128+50]],old2=palette[sprite[45*128+75]];
  uint16_t planet1=MmxJupiterPlanetColor(old1,mask[45*128+50],50,45,top,spot_x,spot_y);
  uint16_t planet2=MmxJupiterPlanetColor(old2,mask[45*128+75],50,45,top,spot_x,spot_y);
  assert(planet1==planet2); /* identical clouds despite old plate shades */
  assert(planet1!=old1);
  assert(MmxJupiterPlanetColor(palette[19],0,80,45,top,spot_x,spot_y)==palette[19]);
  assert(MmxJupiterPlanetColor(palette[18],0,62,26,top,spot_x,spot_y)==palette[18]);
  assert(MmxJupiterPlanetColor(palette[17],0,60,20,top,spot_x,spot_y)==palette[17]);
  assert(MmxJupiterPlanetColor(0,1,50,45,top,spot_x,spot_y)!=
         MmxJupiterPlanetColor(0,1,50,53,top,spot_x,spot_y)); /* belts */
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
  jupiter_shell_is_clouds_not_metal();
  jupiter_planet_masks_replace_sprite_panels();
  no_rush_and_camera_are_continuous();
  vertical_jump_has_no_horizontal_input();
  puts("Jupiter planet shell & safe-follow: 6 test groups passed");
  return 0;
}
