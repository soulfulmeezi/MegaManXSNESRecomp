#include "mmx_feedback_audio.h"
#include "mod_audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum { RATE=22050, SWING=3308, LAND=2646 };
typedef struct CueLedger {
  MmxFeedbackActor previous;
  unsigned airborne, descending;
} CueLedger;
static CueLedger ledger[2];
static SNESModAudioClip swing, landing;
static SNESModAudioVoice voices[2][2];
static uint32_t last_frame, generation;
static unsigned last_stage;
static bool initialized;

/* Original synthesized PCM. No reference-game audio or runtime files.
 * Filtered noise + a descending triangle tone make a short energy whoosh;
 * a low triangle and noise transient make the footfall. */
static void register_clips(void) {
  int16_t pcm[SWING];
  for(unsigned kind=0;kind<2;kind++) {
    unsigned n=kind?LAND:SWING, phase=0;
    uint32_t noise=0x7843a215u; int low=0;
    for(unsigned i=0;i<n;i++) {
      noise^=noise<<13; noise^=noise>>17; noise^=noise<<5;
      int raw=(int)(noise&65535u)-32768;
      low+=(raw-low)/4;
      unsigned hz=kind?75u+100u*(n-i)/n:380u+1100u*(n-i)/n;
      phase=(phase+hz*65536u/RATE)&65535u;
      int triangle=(phase<32768? (int)phase:65535-(int)phase)*2-32768;
      int envelope=(int)((uint64_t)(n-i)*(n-i)*1024u/((uint64_t)n*n));
      /* Short attack avoids a click; all amplitudes have mixer headroom. */
      if(i<64)envelope=envelope*(int)i/64;
      int value=kind?triangle/5+low/4:(raw-low)/3+triangle/7;
      pcm[i]=(int16_t)(value*envelope/1024);
    }
    SNESModAudioClip clip=snes_mod_audio_register_pcm_s16(pcm,n,RATE,1);
    if(kind)landing=clip;else swing=clip;
  }
}
void MmxFeedbackAudioReset(void) {
  for(unsigned a=0;a<2;a++)for(unsigned c=0;c<2;c++) {
    snes_mod_audio_stop_voice(voices[a][c]);voices[a][c]=0;
  }
  memset(ledger,0,sizeof(ledger)); initialized=false;
}
void MmxFeedbackAudioFrame(const MmxFeedbackActor actors[2],
                           uint32_t frame, unsigned stage, bool enabled) {
  const char *toggle=getenv("MMX_ZERO_SFX");
  if(toggle && !strcmp(toggle,"0"))enabled=false;
  uint32_t reset=snes_mod_audio_reset_generation();
  if(!enabled || reset!=generation) {
    MmxFeedbackAudioReset();generation=reset;
    if(!enabled)return;
  }
  if(initialized && frame==last_frame)return;
  bool prime=!initialized || frame!=last_frame+1 || stage!=last_stage;
  if(prime)MmxFeedbackAudioReset();
  if(!swing && !landing)register_clips();
  for(unsigned a=0;a<2;a++) {
    CueLedger *l=&ledger[a];MmxFeedbackActor now=actors[a],prev=l->previous;
    int dx=now.x-prev.x,dy=now.y-prev.y;
    bool continuous=!prime && now.active && prev.active &&
                    dx>-64 && dx<64 && dy>-64 && dy<64;
    if(continuous) {
      bool new_swing=now.slash>=7 &&
        (prev.slash<7 || now.combo!=prev.combo || now.slash<prev.slash);
      if(new_swing) {
        snes_mod_audio_stop_voice(voices[a][0]);
        voices[a][0]=snes_mod_audio_play_voice(swing,100);
        snes_mod_audio_set_voice_pitch(voices[a][0],1024+(int)now.combo*90);
        if(getenv("MMX_AUDIO_TRACE"))
          fprintf(stderr,"Cosmic X swing: actor=%u frame=%u clip=%d voice=%d\n",
              a,frame,swing,voices[a][0]);
      }
      if(!now.grounded) {
        if(l->airborne<255)l->airborne++;
        if(dy>0 && l->descending<255)l->descending++;
      } else {
        if(!prev.grounded && l->airborne>=3 && l->descending>=2) {
          snes_mod_audio_stop_voice(voices[a][1]);
          voices[a][1]=snes_mod_audio_play_voice(landing,65);
        }
        l->airborne=l->descending=0;
      }
    } else l->airborne=l->descending=0;
    l->previous=now;
  }
  last_frame=frame;last_stage=stage;initialized=true;
}
