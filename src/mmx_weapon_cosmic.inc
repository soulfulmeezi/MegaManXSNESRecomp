/* Cosmic attacks use the existing per-player projectile snapshots, collision
 * dispatch and save/rewind data. No separate unsaved timers or enemy HP writes. */
static bool cosmic_spawn(uint8_t *r,unsigned kind) {
  if(!MmxZeroEnabled() || !collision_patch) return false;
  unsigned count=0;
  for(unsigned i=0;i<8;++i)
    count+=combat.shots[i].active && combat.shots[i].page==MMX_COSMIC_PAGE;
  unsigned d=free_slot(r);
  if(!d || count>=2) return false;
  int x=(int)word(r+0xbad)+((r[0xc11]&64)?18:-18);
  int y=(int)word(r+0xbb0)+(kind==MMX_EARTH_WAVE?6:-10);
  if(x<0 || x>8192 || y<0 || y>8192) return false;
  MmxWeaponShot *s=combat.shots+slot_index(d);
  memset(s,0,sizeof(*s));
  s->active=1;s->page=MMX_COSMIC_PAGE;s->weapon=(uint8_t)kind;s->age=1;
  s->facing=r[0xc11]&64;s->vx=s->facing?1024:-1024;
  s->x=x*256;s->y=y*256;
  s->born=combat.tick;
  memset(r+d,0,64);++r[0xbdd];native_object(r,d,s);
  sound(r,kind==MMX_EARTH_WAVE?2:0x0f);
  return true;
}
static void cosmic_player(uint8_t *r) {
  if(!MmxZeroEnabled() || MmxZeroActive() || MmxWeaponsCombatActive() ||
      r[0xbdb] || r[0xd1]!=2 || r[0xd2]!=4 || r[0xd3]!=4 ||
      r[0xba9]!=2 || !(r[0xbcf]&127) || r[0x1f0c] || r[0x1f23] ||
      r[0x1f48] || !(r[0xbd3]&4) || r[0xbaa]>10) return;
  /* Hold physical R2 + new Shoot press, not Down. Normal fire remains native. */
  if(MmxZeroCosmicTriggerHeld() && (r[0xbe3]&64) && cosmic_spawn(r,MMX_EARTH_WAVE)) {
    stop_charge(r);r[0xbdf]&=(uint8_t)~64;r[0xbe3]&=(uint8_t)~64;
    r[0xbf8]=12;
  }
}
void MmxWeaponsJupiterStorm(uint8_t *r) {
  if(r && MmxZeroActive()) cosmic_spawn(r,MMX_JUPITER_STORM);
}
static void cosmic_tick(uint8_t *r,unsigned d,MmxWeaponShot *s) {
  if(s->age==1 && s->born==combat.tick) return;
  if(++s->age>48 || r[d+1]>=8) {retire(r,d);return;}
  /* Four one-pixel steps avoid tunneling through a thin wall or gap. */
  for(unsigned step=0;step<4;++step) {
    int x=(s->x>>8)+(s->vx>0?1:-1),y=s->y>>8;
    if(x<0 || x>8192) {retire(r,d);return;}
    if(s->weapon==MMX_EARTH_WAVE) {
      int surface=0;
      if(!terrain_solid(r,x,y+14,true,&surface) || surface<y+5 ||
          surface>y+14 || terrain_solid(r,x,surface-12,false,NULL)) {
        retire(r,d);return;
      }
      s->y=(surface-10)*256;
    } else if(terrain_solid(r,x,y,false,NULL)) {retire(r,d);return;}
    s->x=x*256;
  }
  if(s->x/256<(int)camera_x(r)-48 || s->x/256>(int)camera_x(r)+304) {
    retire(r,d);return;
  }
  native_object(r,d,s);
}
