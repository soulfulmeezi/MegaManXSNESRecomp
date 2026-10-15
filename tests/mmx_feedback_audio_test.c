#include "mmx_feedback_audio.h"
#include "mod_audio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static unsigned generation, registrations, swings, lands;
static int clip, voice;
SNESModAudioClip snes_mod_audio_register_pcm_s16(const int16_t *p,uint32_t n,uint32_t rate,uint32_t channels) {
  assert(rate==22050 && channels==1 && n>2000 && n<4000);
  assert(p[0]==0 && abs(p[n-1])<8);
  unsigned nonzero=0; for(unsigned i=0;i<n;i++){assert(abs(p[i])<16000);nonzero+=p[i]!=0;}
  assert(nonzero>n/2);registrations++;return ++clip;
}
SNESModAudioVoice snes_mod_audio_play_voice(SNESModAudioClip c,int gain) {
  assert(gain>0 && gain<=100);assert(c==1 || c==2);
  if(c==1)swings++;else lands++;return ++voice;
}
void snes_mod_audio_stop_voice(SNESModAudioVoice v){(void)v;}
int snes_mod_audio_set_voice_pitch(SNESModAudioVoice v,int p){assert(v && p>=1024 && p<=1204);return 1;}
uint32_t snes_mod_audio_reset_generation(void){return generation;}
int main(void) {
  MmxFeedbackActor a[2]={{0,100,100,0,1,1},{0}};unsigned f=1;
  MmxFeedbackAudioFrame(a,f,0,true);assert(registrations==2 && !swings && !lands);
  a[0].slash=5;MmxFeedbackAudioFrame(a,++f,0,true);
  a[0].slash=7;MmxFeedbackAudioFrame(a,++f,0,true);assert(swings==1);
  MmxFeedbackAudioFrame(a,f,0,true);assert(swings==1);
  a[0].slash=9;MmxFeedbackAudioFrame(a,++f,0,true);assert(swings==1);
  a[0].slash=1;a[0].combo=1;MmxFeedbackAudioFrame(a,++f,0,true);
  a[0].slash=7;MmxFeedbackAudioFrame(a,++f,0,true);assert(swings==2);
  a[0].grounded=0;
  for(int i=0;i<4;i++){a[0].y+=3;MmxFeedbackAudioFrame(a,++f,0,true);}
  a[0].grounded=1;MmxFeedbackAudioFrame(a,++f,0,true);assert(lands==1);
  MmxFeedbackAudioFrame(a,++f,0,true);assert(lands==1);
  a[0].grounded=0;MmxFeedbackAudioFrame(a,++f,0,true);
  a[0].grounded=1;MmxFeedbackAudioFrame(a,++f,0,true);assert(lands==1);
  /* Load/replay discontinuity primes without manufacturing cues. */
  generation++;a[0].combo=2;MmxFeedbackAudioFrame(a,++f,0,true);assert(swings==2);
  a[0].combo=0;MmxFeedbackAudioFrame(a,1,0,true);assert(swings==2);
  a[0].combo=1;MmxFeedbackAudioFrame(a,2,1,true);assert(swings==2);
  MmxFeedbackAudioFrame(a,3,1,false);assert(swings==2);
  MmxFeedbackAudioFrame(a,4,1,true);assert(swings==2);
  /* Second actor has independent cue state. */
  a[1]=(MmxFeedbackActor){1,200,100,0,1,1};MmxFeedbackAudioFrame(a,5,1,true);
  a[1].slash=7;MmxFeedbackAudioFrame(a,6,1,true);assert(swings==3);
  /* Teleport is not a landing or an attack edge. */
  a[1].y+=200;a[1].combo=1;MmxFeedbackAudioFrame(a,7,1,true);assert(swings==3);
  puts("feedback: synthesized PCM, combos, co-op, landing, jitter, mute and reset passed");
}
