#pragma once
/* Five original 16-bit-style silhouettes, 20x9; 0 is transparent.
 * Scout, interceptor, gunship, cargo transport, command cruiser. */
static const char mmx_fleet[5][9][21]={
 {"00000000100000000000","00000001210000000000","00000112221100000000","00111123333211100000","01444445555444441000","00116666666611000000","00000177110000000000","00000011000000000000","00000000000000000000"},
 {"00010000000000100000","00121000000001210000","00122111111112210000","01222333333332221000","14444555555554444100","01116666666661110000","00001177117711000000","00000110011000000000","00000000000000000000"},
 {"00000011111100000000","00000122332210000000","00111223333221110000","01222225555222221000","14444445555444444100","01116616666166111000","00017710000177100000","00001100000011000000","00000000000000000000"},
 {"00011111111110000000","00122222222221100000","01224424424422310000","01222222222223331000","14444444444445555410","01666666666666666100","00117711111177110000","00001100000011000000","00000000000000000000"},
 {"00000011111100000000","00000122332210000000","00111222222221111000","01222233333322222100","14444445555444444441","01666666666666666610","00111771111771111000","00000110000110000000","00000000000000000000"}
};
static inline int MmxFleetPixel(unsigned model,int x,int y,unsigned tick,int hostile) {
 if(model>=5 || x<0 || x>=20 || y<0 || y>=9)return -1;
 unsigned k=(unsigned)(mmx_fleet[model][y][x]-'0');
 if(!k)return -1;
 static const unsigned short colors[8]={0,0x1042,0x398c,0x6b51,0x520e,0x7f97,0x28c6,0x7fe8};
 if(k==7)return (tick/6)%2?0x7fcb:0x5684;
 if(hostile && (k==2 || k==4))return k==2?0x1953:0x10ce;
 return colors[k];
}
static inline int MmxFleetScenePixel(int x,int y,int camera,unsigned tick) {
 /* Continuous paths wrap outside the supported widescreen view. Increasing
  * world position reveals later encounters without spawning ships onscreen. */
 unsigned beat=camera<1200?0:camera<2500?1:camera<3900?2:3;
 for(unsigned i=0;i<5;i++) {
   int dir=(i&1)?-1:1;
   int travel=(int)((tick/(i==4?4:2)+i*211)%1600);
   int sx=dir>0?travel-700:900-travel;
   sx-=camera/(i==4?12:8);
   /* Keep wrap seam beyond view for all stage camera positions. */
   while(sx < -700)sx+=1600;
   int sy=26+(int)i*17+(int)((tick/18+i*7)%24);
   if(((tick/18+i*7)/24)&1)sy=26+(int)i*17+23-(int)((tick/18+i*7)%24);
   int lx=x-sx,ly=y-sy;
   int c=MmxFleetPixel(i,dir>0?lx:19-lx,ly,tick,i&1);
   if(c>=0)return c;
   /* Weapons travel from the nose; bounded flight followed by a small
    * expanding impact ring. Never creates a gameplay hitbox. */
   unsigned age=(tick+i*53)%240;
   if(beat && i<4 && age<90) {
     int bx=sx+(dir>0?20:0)+dir*(int)age*2;
     if(y==sy+4 && x>=bx-2 && x<=bx+2)return i&1?0x329f:0x7fe8;
   }
   if(beat>=2 && i==3 && age>=90 && age<126) {
     int dx=x-(sx-180),dy=y-(sy+4),r=(int)(age-90)/4+1;
     int d=dx*dx+dy*dy;
     if(d>=r*r && d<(r+1)*(r+1))return age<108?0x3bff:0x1911;
   }
   if(beat==3 && i==3 && lx>20 && lx<42 && ly>=3 && ly<=6 && ((lx+ly+tick/8)%5)<2)
     return 0x28c6;
 }
 return -1;
}
