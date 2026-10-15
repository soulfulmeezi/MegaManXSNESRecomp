#include "mmx_cosmic_legacy_bridge.h"
#include "mmx_cosmic_upgrade_vfx.h"
#include <assert.h>
#include <stdio.h>
static void mapping_test(void) {
  unsigned p=0,w=0;
  assert(MmxCosmicAspectForNative(3,1)==MMX_COSMIC_EARTH);
  assert(MmxCosmicAspectForNative(3,2)==MMX_COSMIC_STORM);
  assert(MmxCosmicAspectForNative(2,1)==MMX_COSMIC_NONE);
  assert(MmxCosmicAspectForNative(3,0)==MMX_COSMIC_NONE);
  assert(MmxCosmicNativeForAspect(MMX_COSMIC_EARTH,&p,&w));
  assert(p==3 && w==1);
  assert(MmxCosmicNativeForAspect(MMX_COSMIC_STORM,&p,&w));
  assert(p==3 && w==2);
  assert(!MmxCosmicNativeForAspect(MMX_COSMIC_GRAVITY,&p,&w));
  assert(p==3 && w==2);
  assert(!MmxCosmicNativeForAspect(255,&p,&w));
  assert(!MmxCosmicNativeForAspect(MMX_COSMIC_STORM,0,&w));
  assert(!MmxCosmicNativeForAspect(MMX_COSMIC_STORM,&p,0));
}
static void progression_test(void) {
  MmxCosmicState s;
  MmxCosmicReset(&s);
  assert(MmxCosmicUnlock(&s,MMX_COSMIC_EARTH));
  assert(MmxCosmicUnlock(&s,MMX_COSMIC_STORM));
  assert(MmxCosmicEquip(&s,0,MMX_COSMIC_EARTH));
  assert(MmxCosmicEquip(&s,1,MMX_COSMIC_STORM));
  assert(MmxCosmicSelectedFusion(&s)==MMX_COSMIC_NO_FUSION);
  assert(!MmxCosmicUnlock(&s,MMX_COSMIC_ASPECT_COUNT));
}
static void visual_test(void) {
  assert(MmxCosmicUpgradeSparkPixel(0,8,0,14)==-1);
  assert(MmxCosmicUpgradeSparkPixel(MMX_JUPITER_STORM,2,0,14)==-1);
  assert(MmxCosmicUpgradeSparkPixel(MMX_JUPITER_STORM,45,0,14)==-1);
  assert(MmxCosmicUpgradeSparkPixel(MMX_JUPITER_STORM,8,0,14)>=0);
  for(unsigned kind=MMX_EARTH_WAVE;kind<=MMX_JUPITER_STORM;++kind) {
    unsigned spark=0;
    for(int y=-28;y<=19;++y) for(int x=-24;x<=24;++x)
      if(MmxCosmicShotPixel(kind,9,x,y)<0 &&
         MmxCosmicUpgradeSparkPixel(kind,9,x,y)>=0) ++spark;
    assert(spark>0);
  }
}
int main(void) {
  mapping_test(); progression_test(); visual_test();
  puts("Cosmic Phase 1 native mapping + opt-in VFX tests passed");
  return 0;
}
