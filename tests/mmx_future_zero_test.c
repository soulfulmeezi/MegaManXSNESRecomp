#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mmx_future_zero_color.h"
#include "mmx_zero_jupiter_stripes.h"
#include "mmx_zero_jupiter_shell.h"
#include "mmx_zero_jupiter_planet.h"
#include "mmx_cpu_safe_follow.h"
#include "mmx_x_earth_sections.h"
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
  MmxJupiterPlanetSection sections[MMX_JUPITER_MAX_SECTIONS]={0};
  palette[16]=rgb(29,28,26); /* silver metal */
  palette[17]=rgb(23,15,4);  /* golden armor / hair */
  palette[18]=rgb(31,18,13); /* skin */
  palette[19]=rgb(3,3,3);    /* black undersuit */
  palette[20]=rgb(2,21,5);   /* green gem */
  palette[21]=rgb(24,24,24); /* secondary metallic panel */
  palette[22]=rgb(3,4,24);   /* blue visor / eye */
  palette[23]=rgb(12,12,12); /* interior metallic seam */
  palette[24]=rgb(12,4,0);  /* darkest original red armor shade */
  /* A tiny fictional Zero pose: separate helmet, chest, shoulder, forearm,
   * thigh and boot plates, all in the original indexed 16-bit style. */
  for(unsigned y=20;y<=23;++y) for(unsigned x=58;x<=67;++x)
    sprite[y*128+x]=17;
  for(unsigned y=24;y<=29;++y) for(unsigned x=60;x<=65;++x)
    sprite[y*128+x]=18;
  sprite[26*128+64]=22;
  for(unsigned y=21;y<=25;++y) for(unsigned x=52;x<=56;++x)
    sprite[y*128+x]=16; /* helmet */
  for(unsigned y=38;y<=46;++y) for(unsigned x=40;x<=49;++x)
    sprite[y*128+x]=16; /* left shoulder */
  for(unsigned y=38;y<=46;++y) for(unsigned x=79;x<=88;++x)
    sprite[y*128+x]=21; /* right shoulder */
  for(unsigned y=38;y<=69;++y) for(unsigned x=53;x<=74;++x)
    sprite[y*128+x]=x<62?16:21;
  sprite[45*128+57]=24; /* dark armor still receives the plate texture */
  for(unsigned y=38;y<=69;++y) sprite[y*128+63]=23;
  for(unsigned y=38;y<=69;++y) sprite[y*128+80]=19;
  sprite[44*128+55]=20;
  for(unsigned y=52;y<=64;++y) for(unsigned x=38;x<=45;++x)
    sprite[y*128+x]=16; /* left forearm */
  for(unsigned y=52;y<=64;++y) for(unsigned x=83;x<=90;++x)
    sprite[y*128+x]=21; /* right forearm */
  for(unsigned y=73;y<=82;++y) for(unsigned x=52;x<=60;++x)
    sprite[y*128+x]=16; /* left thigh */
  for(unsigned y=73;y<=82;++y) for(unsigned x=66;x<=74;++x)
    sprite[y*128+x]=21; /* right thigh */
  for(unsigned y=87;y<=98;++y)for(unsigned x=51;x<=60;++x)
    sprite[y*128+x]=17;
  for(unsigned y=87;y<=98;++y)for(unsigned x=66;x<=75;++x)
    sprite[y*128+x]=17;
  unsigned section_count=0;
  MmxJupiterPlanetMask(sprite,palette,mask,sections,&section_count);
  assert(mask[20*128+60]==0); /* hair remains original */
  assert(mask[26*128+62]==0); /* skin remains original */
  assert(mask[26*128+64]==0); /* eye remains original */
  assert(mask[45*128+80]==0); /* black undersuit remains original */
  unsigned chest=mask[45*128+55];
  unsigned helmet=mask[23*128+54],shoulder=mask[41*128+44];
  unsigned forearm=mask[58*128+40],thigh=mask[77*128+56];
  unsigned boot=mask[91*128+55];
  assert(chest && helmet && shoulder && forearm && thigh && boot);
  assert(mask[45*128+53]==chest && mask[45*128+70]==chest);
  assert(mask[45*128+57]==chest); /* dark red shade has no charcoal hole */
  assert(mask[45*128+63]==chest); /* enclosed seam joins its plate */
  assert(mask[41*128+80]!=chest && mask[58*128+86]!=chest &&
         mask[77*128+70]!=chest && boot!=chest);
  assert(section_count>=10); /* separated armor patches have separate IDs */
  assert(sections[chest-1].flags&MMX_JUPITER_SECTION_CHEST);
  assert(!(sections[helmet-1].flags&MMX_JUPITER_SECTION_CHEST));
  assert(sections[chest-1].pattern_seed!=sections[shoulder-1].pattern_seed);
  assert(sections[boot-1].flags&MMX_JUPITER_SECTION_BOOT);
  assert(sections[boot-1].flags&MMX_JUPITER_SECTION_SWIRL);
  uint16_t old1=palette[16],old2=palette[21];
  uint16_t planet1=MmxJupiterPlanetColor(old1,mask[45*128+55],55,45,sections);
  uint16_t planet2=MmxJupiterPlanetColor(old2,mask[45*128+55],55,45,sections);
  assert(planet1==planet2); /* identical clouds despite old plate shades */
  assert(planet1!=old1);
  assert(MmxJupiterPlanetColor(palette[19],0,80,45,sections)==palette[19]);
  assert(MmxJupiterPlanetColor(palette[18],0,62,26,sections)==palette[18]);
  assert(MmxJupiterPlanetColor(palette[17],0,60,20,sections)==palette[17]);
  assert(MmxJupiterPlanetColor(0,chest,60,45,sections)!=
         MmxJupiterPlanetColor(0,chest,60,53,sections)); /* local belts */
  assert(MmxJupiterPlanetColor(0,chest,64,53,sections)==
         MmxFutureZeroBgr555(30,9,3)); /* red-orange Great Red Spot */
  assert(MmxJupiterPlanetColor(0,chest,65,53,sections)==
         MmxFutureZeroBgr555(30,24,18)); /* lighter spot swirl */
  assert(sections[shoulder-1].flags&MMX_JUPITER_SECTION_SWIRL);
  assert(MmxJupiterPlanetColor(0,shoulder,sections[shoulder-1].center_x,
      sections[shoulder-1].center_y-2,sections)==
      MmxFutureZeroBgr555(30,29,27)); /* white storm on another plate */
  bool dark_boot_band=false;
  for(unsigned y=sections[boot-1].top;y<=sections[boot-1].bottom;++y)
    for(unsigned x=sections[boot-1].left;x<=sections[boot-1].right;++x)
      if(mask[y*128+x]==boot && MmxJupiterPlanetColor(0,boot,x,y,sections)==
          MmxFutureZeroBgr555(11,6,4)) dark_boot_band=true;
  assert(dark_boot_band); /* boots use darker brown belts */
  const unsigned armor_sections[]={helmet,chest,shoulder,forearm,thigh,boot};
  for(unsigned n=0;n<sizeof(armor_sections)/sizeof(armor_sections[0]);++n) {
    unsigned id=armor_sections[n];
    bool is_boot=(sections[id-1].flags&MMX_JUPITER_SECTION_BOOT)!=0;
    uint16_t cream=is_boot?MmxFutureZeroBgr555(27,26,24):
        MmxFutureZeroBgr555(29,28,26);
    uint16_t tan=is_boot?MmxFutureZeroBgr555(24,20,15):
        MmxFutureZeroBgr555(26,22,17);
    uint16_t rust=is_boot?MmxFutureZeroBgr555(22,13,7):
        MmxFutureZeroBgr555(24,16,10);
    uint16_t brown=is_boot?MmxFutureZeroBgr555(11,6,4):
        MmxFutureZeroBgr555(15,9,5);
    bool has_cream=false,has_tan=false,has_rust=false,has_brown=false;
    const MmxJupiterPlanetSection *s=&sections[id-1];
    for(unsigned y=s->top;y<=s->bottom;++y)
      for(unsigned x=s->left;x<=s->right;++x) if(mask[y*128+x]==id) {
        uint16_t c=MmxJupiterPlanetColor(0,id,x,y,sections);
        if(c==cream)has_cream=true;
        if(c==tan)has_tan=true;
        if(c==rust)has_rust=true;
        if(c==brown)has_brown=true;
      }
    assert(has_cream && has_tan && has_rust && has_brown);
  }
  assert(MmxJupiterPlanetPaintable(24,palette));
  assert(!MmxJupiterPlanetBrightArmor(palette[19])); /* black undersuit */
}
static void x_earth_armor_is_section_local(void) {
  uint16_t blue=rgb(4,10,27);
  assert(MmxEarthXArmorPixel(blue,MMX_EARTH_CHEST,0,0)!=blue);
  assert(MmxEarthXArmorPixel(rgb(24,16,11),MMX_EARTH_CHEST,0,0)==rgb(24,16,11)); /* skin */
  assert(MmxEarthXArmorPixel(rgb(3,3,4),MMX_EARTH_CHEST,0,0)==rgb(3,3,4)); /* undersuit */
  assert(MmxEarthXArmorPixel(rgb(27,27,29),MMX_EARTH_CHEST,0,0)==rgb(27,27,29)); /* trim */
  assert(MmxEarthXArmorPixel(rgb(3,14,4),MMX_EARTH_CHEST,0,0)==rgb(3,14,4)); /* green detail */
  assert(MmxEarthXArmorPixel(blue,MMX_EARTH_SECTION_COUNT,0,0)==blue);
  assert(MmxEarthXFaceDetail(0xba8,MMX_EARTH_HELMET,7,-18));
  assert(!MmxEarthXFaceDetail(0xba8,MMX_EARTH_HELMET,2,-18));
  assert(!MmxEarthXFaceDetail(0xc38,MMX_EARTH_HELMET,7,-18));

  /* Every native-size plate has one connected continent, an ocean majority,
   * a small tan interior and at most 5% clouds. Large maps used to pass broad
   * area scans while leaving actual plates blue or almost entirely white. */
  for(unsigned section=0;section<MMX_EARTH_SECTION_COUNT;++section) {
    unsigned land=0,cloud=0,dry=0,head=0,tail=0;
    bool seen[144]={false}; int queue[144];
    for(int y=0;y<12;++y) for(int x=0;x<12;++x) {
      char m=MmxEarthXMaterial(section,x,y);
      if(m=='G' || m=='T') {
        ++land;
        if(!tail) { queue[tail++]=y*12+x; seen[y*12+x]=true; }
      }
      dry+=m=='T'; cloud+=m=='C';
      assert(m=='.' || m=='G' || m=='T' || m=='C');
      assert(!(m=='C' && MmxEarthXMaterial(section,x+1,y)=='C' &&
          MmxEarthXMaterial(section,x,y+1)=='C' && MmxEarthXMaterial(section,x+1,y+1)=='C'));
      /* Material placement stays identical across the native lighting ramp. */
      const uint16_t shades[]={rgb(0,4,20),rgb(4,10,27),rgb(12,23,31)};
      unsigned previous=0;
      for(unsigned s=0;s<3;++s) {
        uint16_t c=MmxEarthXArmorPixel(shades[s],section,x,y);
        unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
        assert((m=='.' && b>g && g>r) || (m=='G' && g>r && g>b) ||
            (m=='T' && r>g && g>b) || (m=='C' && r>=17 && g>=23 && b>=25));
        assert(r+g+b>previous); previous=r+g+b;
      }
    }
    assert(land>=36 && land<=70 && dry>=2 && dry<=8 && cloud>=3 && cloud<=7);
    /* Flood-fill the periodic chart so an origin offset does not split a
     * connected coastline at the sample window boundary. */
    while(head<tail) {
      int cell=queue[head++],x=cell%12,y=cell/12;
      const int neighbors[]={y*12+(x+1)%12,y*12+(x+11)%12,
          ((y+1)%12)*12+x,((y+11)%12)*12+x};
      for(unsigned n=0;n<4;++n) {
        int next=neighbors[n]; char m=MmxEarthXMaterial(section,next%12,next/12);
        if(!seen[next] && (m=='G' || m=='T')) { seen[next]=true;queue[tail++]=next; }
      }
    }
    assert(tail==land);
    for(unsigned other=0;other<section;++other) {
      unsigned differences=0;
      for(int y=0;y<12;++y) for(int x=0;x<12;++x)
        differences+=MmxEarthXMaterial(section,x,y)!=MmxEarthXMaterial(other,x,y);
      assert(differences>=12);
    }
  }
  /* Protect every non-blue SNES color, not only a few example swatches. */
  for(unsigned c=0;c<32768;++c) if(!MmxEarthIsBlueArmor((uint16_t)c))
    for(unsigned s=0;s<MMX_EARTH_SECTION_COUNT;++s)
      assert(MmxEarthXArmorPixel((uint16_t)c,s,-3,-20)==c);
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
  x_earth_armor_is_section_local();
  no_rush_and_camera_are_continuous();
  vertical_jump_has_no_horizontal_input();
  puts("Jupiter planet shell, Earth X & safe-follow: 7 test groups passed");
  return 0;
}
