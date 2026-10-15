#ifndef MMX_LANE_CAPTURE_H
#define MMX_LANE_CAPTURE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Read-only presentation diagnostics. No guest memory or controller changes. */
static unsigned mmx_lane_capture_done;
static int MmxLaneCaptureTarget(unsigned x) {
  static const unsigned targets[]={0x05b7,0x0718,0x0fce,0x148f};
  for(int i=0;i<4;i++) if(!(mmx_lane_capture_done&(1u<<i)) &&
      x+8>=targets[i] && x<=targets[i]+8) return i;
  return -1;
}
static uint32_t MmxLaneCaptureRgb(unsigned c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return (((r<<3)|(r>>2))<<16)|(((g<<3)|(g>>2))<<8)|((b<<3)|(b>>2));
}
static void MmxLaneCaptureLe(unsigned char *p,unsigned v,int n) {
  for(int i=0;i<n;i++) p[i]=(unsigned char)(v>>(8*i));
}
static int MmxLaneCaptureBmp(const char *path,const uint32_t *rgb,unsigned w,unsigned h) {
  unsigned stride=(w*3+3)&~3u; unsigned char head[54]={0};
  head[0]='B';head[1]='M';MmxLaneCaptureLe(head+2,54+stride*h,4);
  MmxLaneCaptureLe(head+10,54,4);MmxLaneCaptureLe(head+14,40,4);
  MmxLaneCaptureLe(head+18,w,4);MmxLaneCaptureLe(head+22,h,4);
  MmxLaneCaptureLe(head+26,1,2);MmxLaneCaptureLe(head+28,24,2);
  MmxLaneCaptureLe(head+34,stride*h,4);
  FILE *f=fopen(path,"wb");if(!f)return 0;
  int ok=fwrite(head,1,54,f)==54;
  unsigned char row[3072];
  if(w>1024){fclose(f);return 0;}
  for(unsigned y=h;y>0 && ok;y--) {
    memset(row,0,stride);
    for(unsigned x=0;x<w;x++) {
      uint32_t c=rgb[(y-1)*w+x];
      row[x*3]=(unsigned char)c;row[x*3+1]=(unsigned char)(c>>8);row[x*3+2]=(unsigned char)(c>>16);
    }
    ok=fwrite(row,1,stride,f)==stride;
  }
  if(fclose(f))ok=0;
  return ok;
}
static void MmxLaneCaptureSave(const char *dir,uint32_t *data,const uint32_t *composite,
    unsigned width,unsigned camera,int target) {
  static const char *names[]={"BG1","BG2","BG3","paths-unmasked","mask-reasons","composite"};
  const size_t count=(size_t)width*224; int ok=1;
  for(int i=0;i<6;i++) {
    char path[2048];int n=snprintf(path,sizeof(path),"%s/cam-%04X-%s.bmp",dir,camera,names[i]);
    if(n<0 || (size_t)n>=sizeof(path) || !MmxLaneCaptureBmp(path,i==5?composite:data+i*count,width,224))ok=0;
  }
  /* Avoid disk retries every frame after a failure; relaunch retries. */
  mmx_lane_capture_done|=1u<<target;
  fprintf(stderr,"v33e layer capture camera %04X: %s\n",camera,ok?"SAVED":"FAILED - check capture directory");
  free(data);
}
#endif
