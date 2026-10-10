#include "mmx_coop.h"
#include "mmx_coop_view.h"
#include "mmx_renderer.h"
#include "mmx_coop_trace.h"
#include "mmx_rtl.h"
#include "mmx_wide_policy.h"
_Static_assert(sizeof(MmxCoopState) == MMX_COOP_LEGACY_STATE_SIZE + 2 * sizeof(MmxZeroModernState),
               "Update the legacy co-op importer when its layout changes");
#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include "common_rtl.h"
#if SNESRECOMP_NET
#include "snes_netplay.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

extern uint8_t g_ram[0x20000];
extern Snes *g_snes;
extern int snes_frame_counter;

static MmxCoopState state = {.players = {{.character = MMX_COOP_X}, {.character = MMX_COOP_ZERO}}};
static bool enabled;
static bool cpu_companion; /* Opt-in local mode; never stored in guest save state. */
static uint8_t cpu_jump_hold_frames, cpu_jump_cooldown_frames;
static uint8_t cpu_cliff_dash_frames;
static bool cpu_jump_seen_airborne;
/* Host-only high-priority recovery: from first native wall slide until
 * verified landing; no normal follow inputs may interrupt this sequence. */
static uint8_t cpu_wall_recovery_phase, cpu_wall_recovery_jumps;
static uint8_t cpu_wall_recovery_ticks;
/* A new native wall-slide must follow a real departure from the previous
 * slide before the next kick is allowed. Prevents double-pressing B while
 * the original slide action is still latched in a snapshot. */
static bool cpu_wall_recovery_left_slide;
/* Track buffered pre-slide wall-kicks separately from confirmed native kicks,
 * so an ignored early B press never consumes one of the two climb attempts. */
static bool cpu_wall_buffer_pending, cpu_wall_buffer_attempted;
static uint8_t cpu_wall_jump_hold_frames;
/* Tall ascents can require many successive native wall-kicks. Ordinary
 * accidental slides keep the original two-kick safety limit. */
static bool cpu_tall_wall_climb;
static uint16_t cpu_tall_wall_start_y, cpu_tall_wall_best_y;
static uint16_t cpu_tall_wall_kick_x, cpu_tall_wall_kick_y;
/* Count wall-jump opportunities from native engine state, not a timer.
 * Once the kick's real upward arc peaks, release B proactively so the
 * first wall-slide frame can receive a fresh B edge with no lost frame. */
static uint8_t cpu_tall_wall_trace_ticks;
static bool cpu_wall_y_valid;
static uint16_t cpu_wall_last_y;
static uint8_t cpu_human_seat, cpu_swap_trigger_down, cpu_rescue_cooldown;
static bool cpu_l2_trigger_held;
static uint8_t cpu_stall_ticks;
static uint8_t cpu_zero_melee_cooldown;
static bool cpu_zero_dash_was_active;
static bool cpu_wall_escape_reported;
/* Host-only route commitment for an intentional drop to an opposite wall.
 * This never changes native physics, guest memory or save-state layout. */
static uint8_t cpu_pit_wall_ticks;
static int8_t cpu_pit_wall_direction;
static bool cpu_pit_wall_was_airborne;
static uint16_t cpu_last_x;
static int8_t cpu_jump_direction, cpu_wall_direction, cpu_stall_direction;
static unsigned starting_character;
_Static_assert(sizeof(MmxCoopPlayer) == 2276, "Co-op player save ABI");
_Static_assert(sizeof(MmxCoopState) == 4664, "Co-op save ABI");
static bool join_tick(uint8_t *r);
static void place_other(uint8_t *r,uint16_t x,uint16_t y,bool preserve);
static bool floor_below(const uint8_t *r,const uint8_t *b);
static void cpu_companion_rescue(uint8_t *r);
/* Registers the item routine passed into $84:AB81/AB56; see platform_hook. */
static struct {bool valid;uint16_t d,s,a,x,y;uint8_t p,db;} platform_entry;
/* --coop-trace: observation only; see mmx_coop_trace.h. */
static void trace_event(unsigned kind,uint32_t pc,unsigned a,unsigned b,const CpuState *cpu) {
  MmxCoopTraceEvent e={(uint32_t)snes_frame_counter,pc,cpu?cpu->S:0,cpu?cpu->D:0,
      (uint8_t)kind,(uint8_t)a,(uint8_t)b,state.current,state.anchor,state.controller_pass,
      state.object_pass,state.contact_pass,state.pickup_pass,state.door_pass};
  MmxCoopTraceRecord(&e);
}
#define TRACE(kind,pc,a,b,cpu) do { if (g_mmx_coop_trace) trace_event(MMX_COOP_EV_##kind,pc,a,b,cpu); } while (0)
#define TRACE_MARK(seat,flags) do { if (g_mmx_coop_trace) MmxCoopTraceMark(seat,MMX_COOP_RAN_##flags); } while (0)
static bool scene_tick(uint8_t *r);
static unsigned word(const uint8_t *p) {return p[0]|p[1]<<8;}
static void putword(uint8_t *p,unsigned v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static unsigned weapon_view(const uint8_t *r,bool vertical) {
  if(!MmxCoopViewsOnline() || !state.initialized || state.menu_owner || state.scene_owner)
    return word(r+(vertical?0x1e50:0x1e4d));
  MmxCoopView v=MmxCoopViewForPlayer(r,&state,state.current);
  return (unsigned)(vertical?v.y:v.x);
}
static void sound(uint8_t *r,unsigned command) {
  unsigned i=r[0xba3]&30;r[0xb72+i]=(uint8_t)command;r[0xb73+i]=0;r[0xba3]=(uint8_t)((i+2)&30);
}
/* Host-only opt-in trace. Never changes WRAM, scheduling or serialized state.
 * Rotates through numbered 32 MiB CSV segments and keeps the newest
 * DIAGNOSTIC_SEGMENTS, so a full playthrough survives but disk use is bounded.
 * Rollback/replayed frames retain their original guest counters and a distinct
 * host sequence number rather than being mistaken for consecutive simulation. */
enum { DIAGNOSTIC_SEGMENTS = 32 };
typedef struct DiagnosticFile {
  FILE *file;
  char stem[2048], path[2064];
  unsigned segment;
  long bytes;
  bool checked;
} DiagnosticFile;
static DiagnosticFile physics_file, netplay_file;
static char diagnostic_stamp[32];
static unsigned long diagnostic_pid;
static bool diagnostic_enabled;
static unsigned diagnostic_session;
static unsigned diagnostic_sequence, diagnostic_rows;
static unsigned select_line; /* source line of the pending MmxCoopSelect, 0 if none */
extern int snes_frame_counter;
bool MmxCoopDiagnosticsEnabled(void) {return diagnostic_enabled;}
static void diagnostic_close(DiagnosticFile *t) {
  if(t->file) fclose(t->file);
  t->file=NULL;t->checked=false;t->bytes=0;
}
void MmxCoopSetDiagnosticsEnabled(bool active) {
  diagnostic_close(&physics_file);diagnostic_close(&netplay_file);
  diagnostic_stamp[0]=0;
  diagnostic_sequence=diagnostic_rows=0;
  diagnostic_enabled=active;
}
/* Segment <segment> of a trace: <stem>-001.csv, <stem>-002.csv, ... */
static void diagnostic_segment_path(const DiagnosticFile *t,unsigned segment,char *out,size_t cap) {
  snprintf(out,cap,"%s-%03u.csv",t->stem,segment);
}
/* Both traces of one session share a stem, so they sort and pair together:
 * logs/coop-physics-<time>-<pid>-<n>-<segment>.csv and logs/coop-netplay-<same>. */
static FILE *diagnostic_open(DiagnosticFile *t,const char *kind) {
  if (t->checked) return t->file;
  t->checked=true;
  if(!diagnostic_stamp[0]) {
    time_t now=time(NULL);struct tm *local=localtime(&now);
    snprintf(diagnostic_stamp,sizeof(diagnostic_stamp),"unknown-time");
    if(local) strftime(diagnostic_stamp,sizeof(diagnostic_stamp),"%Y%m%d-%H%M%S",local);
#ifdef _WIN32
    _mkdir("logs");diagnostic_pid=(unsigned long)_getpid();
#else
    mkdir("logs",0755);diagnostic_pid=(unsigned long)getpid();
#endif
    ++diagnostic_session;
  }
  snprintf(t->stem,sizeof(t->stem),"logs/coop-%s-%s-%lu-%u",
      kind,diagnostic_stamp,diagnostic_pid,diagnostic_session);
  t->segment=1;diagnostic_segment_path(t,t->segment,t->path,sizeof(t->path));
  t->file=fopen(t->path,"wb");
  if (!t->file) {fprintf(stderr,"[coop-%s] cannot open %s\n",kind,t->path);return NULL;}
  fprintf(stderr,"[coop-%s] recording %s (32 MiB per segment, newest %u kept)\n",
      kind,t->path,(unsigned)DIAGNOSTIC_SEGMENTS);
  return t->file;
}
/* Start the next numbered segment; drop the one that leaves the window. */
static FILE *diagnostic_ready(DiagnosticFile *t,const char *kind,const char *header) {
  if (!diagnostic_open(t,kind)) return NULL;
  if (t->bytes>=32L*1024*1024) {
    fclose(t->file);t->file=NULL;
    ++t->segment;
    if (t->segment>DIAGNOSTIC_SEGMENTS) {
      /* Every segment of this stem belongs to this explicitly requested trace. */
      char old[2064];
      diagnostic_segment_path(t,t->segment-DIAGNOSTIC_SEGMENTS,old,sizeof(old));
      remove(old);
    }
    diagnostic_segment_path(t,t->segment,t->path,sizeof(t->path));
    t->file=fopen(t->path,"wb");
    if (!t->file) {fprintf(stderr,"[coop-%s] cannot open %s; recording stopped\n",kind,t->path);return NULL;}
    t->bytes=0;
  }
  if (!t->bytes) t->bytes=fprintf(t->file,"%s",header);
  return t->file;
}
/* Every live slot of an object pool: slot, class (+0A), X (+05), Y (+08),
 * state bytes +00..02 and the rider latch +2C. Items ($1628 + slot*$30) hold
 * the $0E..$10/$13/$14 platforms; enemies ($0E68 + slot*$40) hold scripted
 * riders such as Storm Eagle's lift ($48). */
static void diagnostic_pool(const uint8_t *r,unsigned base,unsigned stride,unsigned slots,
                            char *out,size_t cap) {
  size_t n=0;out[0]=0;
  for(unsigned i=0;i<slots && n<cap;++i) {
    const uint8_t *d=r+base+i*stride;
    if(!d[0]) continue;
    int w=snprintf(out+n,cap-n,"%s%u:%02x:%04x:%04x:%02x%02x%02x:%02x:%02x:%02x",n?";":"",
        i,d[10],word(d+5),word(d+8),d[0],d[1],d[2],d[0x2c],d[0x27],d[0x0e]);
    if(w<0) break;
    n+=(size_t)w;
  }
}
static void diagnostic_items(const uint8_t *r,char *out,size_t cap) {
  diagnostic_pool(r,0x1628,48,16,out,cap);
}
/* Per-byte snprintf dominated the trace cost (body/scratch/items on ~16 rows
 * a frame with Storm Eagle's columns); encode with a table instead. */
static void diagnostic_hex(char *out,const uint8_t *p,size_t n) {
  static const char digits[]="0123456789abcdef";
  for(size_t i=0;i<n;++i) {out[i*2]=digits[p[i]>>4];out[i*2+1]=digits[p[i]&15];}
  out[n*2]=0;
}
static const char kPhysicsHeader[]=
    "sequence,host_frame,world_tick,event,pc,cpu_d,cpu_s,stage,mode,submode,phase,"
    "current,anchor,controller_pass,object_pass,contact_pass,pickup_pass,pickup_d,"
    "menu_owner,scene_owner,camera_x,camera_y,freeze_flags,pickup_owners,seat,"
    "character,status,input,x,y,previous_x,previous_y,vx,vy,hp,ground,"
    "terrain_above_feet,solid_above_feet,terrain_feet,solid_feet,body,scratch,"
    "upgrades,caller,items,regs,slot,enemies,shots\n";
static void diagnostic_event(const uint8_t *r,const CpuState *cpu,uint32_t pc,const char *event) {
  if (!diagnostic_enabled || !enabled || !state.initialized) return;
  FILE *out=diagnostic_ready(&physics_file,"physics",kPhysicsHeader);
  if (!out) return;
  bool frame_end=!strcmp(event,"frame-end");
  bool platform=!strncmp(event,"platform",8) || !strncmp(event,"contact",7) ||
      !strcmp(event,"lift-contact") || !strcmp(event,"body-moved") || !strcmp(event,"ghost-moved");
  char flags[15],owners[33],scratch[129],items[16*39+1]={0},caller[12]={0},regs[24]={0},slot[64*2+1]={0},enemies[23*39+3]={0},shots[12*39+1]={0};
  diagnostic_hex(flags,r+0x1f13,7);
  diagnostic_hex(owners,state.pickup_owner,16);
  diagnostic_hex(scratch,r,64);
  /* A:X:Y:P:DB at the hook, before co-op changes them (P from the flag mirrors). */
  if(cpu) {
    CpuState c=*cpu;cpu_mirrors_to_p(&c);
    snprintf(regs,sizeof(regs),"%04x:%04x:%04x:%02x:%02x",c.A,c.X,c.Y,c.P,c.DB);
  }
  if(frame_end || platform) diagnostic_items(r,items,sizeof(items));
  bool switching=!strcmp(event,"select-before");
  if(switching && select_line) snprintf(caller,sizeof(caller),"L%u",select_line);
  /* Storm Eagle's elevator ($59, parts $58/$5A) keeps one rider latch in .2C;
   * log the pool at every seat switch while it exists to see who flips it. */
  bool elevator=false;
  for(unsigned i=0;i<15 && switching;++i) {
    const uint8_t *e=r+0xe68+i*64;
    if(e[0] && e[10]>=0x58 && e[10]<=0x5a) elevator=true;
  }
  if(frame_end || elevator || platform) {
    diagnostic_pool(r,0xe68,64,15,enemies,sizeof(enemies));
    /* Enemy projectiles and effects ($1428, eight 64-byte slots), after "p". */
    size_t used=strlen(enemies);
    if(used+2<sizeof(enemies)) {
      char *tail=enemies+used;tail[0]='p';
      diagnostic_pool(r,0x1428,64,8,tail+1,sizeof(enemies)-used-1);
      if(!tail[1]) tail[0]=0;
    }
  }
  /* The current seat's shots ($0C98, twelve 32-byte slots). */
  if(frame_end || platform) diagnostic_pool(r,0xc98,32,12,shots,sizeof(shots));
  /* The contacted item's whole 48-byte slot: a byte the first seat's call
   * writes and the second seat's call reads shows up between their rows. */
  if(platform && cpu && cpu->D>=0x1628 && cpu->D<0x1928 && !((cpu->D-0x1628)%48))
    diagnostic_hex(slot,r+cpu->D,48);
  else if(platform && cpu && ((cpu->D>=0xe68 && cpu->D<0x1228 && !((cpu->D-0xe68)%64)) ||
      (cpu->D>=0x1428 && cpu->D<0x1628 && !((cpu->D-0x1428)%64))))
    diagnostic_hex(slot,r+cpu->D,64);
  /* Entry to a long subroutine: the JSL return address names the item code. */
  if(platform && cpu && (!strcmp(event,"platform-enter") || !strcmp(event,"lift-contact")) &&
      cpu->S<0x1ffd)
    snprintf(caller,sizeof(caller),"%06x",
        (unsigned)((r[cpu->S+1]|r[cpu->S+2]<<8|r[cpu->S+3]<<16)+1)&0xffffff);
  unsigned sequence=++diagnostic_sequence;
  for(unsigned seat=0;seat<2;++seat) {
    /* Other-seat history is sampled once at frame end; repeated copies at
     * every interpreter boundary add volume without new observations. */
    if(seat!=state.current && !frame_end) continue;
    const MmxCoopPlayer *p=&state.players[seat];
    const uint8_t *b=seat==state.current ? r+0xba8 : p->body;
    char body[0x90*2+1]={0};
    if(frame_end || platform || !strcmp(event,"pickup"))
      diagnostic_hex(body,b,0x90);
    int x=word(b+5),y=word(b+8);
    int count=fprintf(out,
        "%u,%d,%u,%s,%06x,%04x,%04x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%04x,"
        "%u,%u,%u,%u,%s,%s,%u,%u,%u,%04x,%d,%d,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%s,%s,"
        "%02x,%s,%s,%s,%s,%s,%s\n",
        sequence,snes_frame_counter,r[0xb9c],event,(unsigned)pc,
        cpu?(unsigned)cpu->D:0,cpu?(unsigned)cpu->S:0,r[0x1f7a],r[0xd1],r[0xd2],r[0xd3],
        state.current,state.anchor,state.controller_pass,state.object_pass,state.contact_pass,
        state.pickup_pass,state.pickup_d,state.menu_owner,state.scene_owner,word(r+0x1e4d),word(r+0x1e50),
        flags,owners,seat+1,p->character,p->status,p->input,x,y,word(b+0x22),word(b+0x24),
        (int16_t)word(b+0x1a),(int16_t)word(b+0x1c),b[0x27]&127,b[0x2b],
        MmxWeaponsTerrainClass(r,x,y+8),MmxWeaponsTerrainSolid(r,x,y+8,true,NULL),
        MmxWeaponsTerrainClass(r,x,y+16),MmxWeaponsTerrainSolid(r,x,y+16,true,NULL),body,scratch,
        r[0x1f99],caller,items,regs,slot,enemies,shots);
    if(count<0) {diagnostic_close(&physics_file);physics_file.checked=true;return;}
    physics_file.bytes+=count;
  }
  if (++diagnostic_rows%60==0) fflush(out);
}
static uint32_t diagnostic_hash(const void *data,size_t size) {
  const uint8_t *p=data;uint32_t h=2166136261u;
  for(size_t i=0;i<size;++i) h=(h^p[i])*16777619u;
  return h;
}
/* Netplay companion: one row per simulated frame while a match runs. Rollback
 * re-simulation repeats a world tick; the last row for a tick is what was
 * kept. Compare wram_hash/coop_hash between both players' files for the first
 * tick they disagree. Observation only, like the physics trace. */
static const char kNetplayHeader[]=
    "sequence,host_frame,world_tick,sim_tick,frames_finished,speculative,rollback,"
    "slot,host,input_player,transport,ice_failed,remote_lead,input_delay,"
    "published_inputs,active_mask,quiesced,draining,state_barrier,desync_tick,"
    "desync_local,desync_remote,p1_input,p2_input,current,anchor,stage,mode,"
    "submode,phase,wram_hash,coop_hash,p1_hash,p2_hash,shared_hash,p1_body,p2_body\n";
static void diagnostic_netplay(const uint8_t *r) {
#if SNESRECOMP_NET
  if (!diagnostic_enabled || !snes_netplay_active()) return;
  FILE *out=diagnostic_ready(&netplay_file,"netplay",kNetplayHeader);
  if (!out) return;
  uint32_t tick=0,local=0,remote=0;
  char desync[12]="";
  if(snes_netplay_input_desync(&tick,&local,&remote)) snprintf(desync,sizeof(desync),"%u",(unsigned)tick);
  else local=remote=0;
  const char *transport=snes_netplay_transport_name();
  /* Which part of the co-op state forked: each seat's stored copy (body,
   * auxiliaries, shots, weapons, combat, Zero state) and everything after
   * the two seats. The stored bodies are written out whole. */
  char bodies[2][0x90*2+1];
  for(unsigned i=0;i<2;++i) diagnostic_hex(bodies[i],state.players[i].body,0x90);
  const uint8_t *shared=(const uint8_t *)&state+sizeof(state.players);
  int count=fprintf(out,
      "%u,%d,%u,%u,%u,%u,%d,%d,%d,%d,%s,%d,%d,%d,%u,%x,%d,%d,%d,%s,%08x,%08x,"
      "%04x,%04x,%u,%u,%u,%u,%u,%u,%08x,%08x,%08x,%08x,%08x,%s,%s\n",
      diagnostic_sequence,snes_frame_counter,r[0xb9c],(unsigned)snes_netplay_sim_tick(),
      (unsigned)snes_netplay_frames_finished(),RtlSpeculativeFrame(),snes_netplay_rollback_active(),
      snes_netplay_local_slot(),snes_netplay_is_host(),snes_netplay_input_player(),
      transport?transport:"",snes_netplay_ice_failed(),snes_netplay_remote_lead(),
      snes_netplay_input_delay(),(unsigned)snes_netplay_published_inputs(),
      (unsigned)snes_netplay_active_mask(),snes_netplay_quiesced(),snes_netplay_draining(),
      snes_netplay_state_barrier(),desync,local,remote,
      state.players[0].input,state.players[1].input,state.current,state.anchor,
      r[0x1f7a],r[0xd1],r[0xd2],r[0xd3],
      diagnostic_hash(r,0x20000),diagnostic_hash(&state,sizeof(state)),
      diagnostic_hash(&state.players[0],sizeof(state.players[0])),
      diagnostic_hash(&state.players[1],sizeof(state.players[1])),
      diagnostic_hash(shared,sizeof(state)-sizeof(state.players)),bodies[0],bodies[1]);
  if(count<0) {diagnostic_close(&netplay_file);netplay_file.checked=true;return;}
  netplay_file.bytes+=count;
  if (diagnostic_rows%60==0) fflush(out);
#else
  (void)r;
#endif
}
void MmxCoopDiagnosticFrame(const uint8_t *r) {
  diagnostic_event(r,NULL,0,"frame-end");
  diagnostic_netplay(r);
}
bool MmxCoopTransitionActive(void) {
  return enabled && state.initialized && (state.players[0].zero.swap_phase || state.players[1].zero.swap_phase);
}

bool MmxCoopEnabled(void) { return enabled; }
/* No native shared-screen catch-up beam while the locally controlled
 * companion is attempting parkour. Online/human co-op retains stock scene
 * handoffs. Scripted boss/capsule/door handoffs remain separate. */
static bool cpu_traversal_active(void) {
  if (!enabled || !cpu_companion) return false;
#if SNESRECOMP_NET
  if (snes_netplay_active()) return false;
#endif
  return true;
}
static void shot_ghost_reset(void);
static void lift_reset(void);
static void cpu_companion_reset_motion(void) {
  cpu_jump_hold_frames=cpu_jump_cooldown_frames=cpu_cliff_dash_frames=0;
  cpu_jump_seen_airborne=false;
  cpu_wall_recovery_phase=cpu_wall_recovery_jumps=cpu_wall_recovery_ticks=0;
  cpu_wall_recovery_left_slide=cpu_wall_buffer_pending=cpu_wall_buffer_attempted=false;
  cpu_wall_jump_hold_frames=0;cpu_wall_y_valid=false;cpu_wall_last_y=0;
  cpu_tall_wall_climb=false;cpu_tall_wall_start_y=cpu_tall_wall_best_y=0;
  cpu_tall_wall_kick_x=cpu_tall_wall_kick_y=0;
  cpu_tall_wall_trace_ticks=0;
  cpu_jump_direction=cpu_wall_direction=0;
  cpu_stall_ticks=0;cpu_last_x=0;cpu_stall_direction=0;
  cpu_zero_melee_cooldown=0;cpu_zero_dash_was_active=false;
  cpu_wall_escape_reported=false;
  cpu_pit_wall_ticks=0;cpu_pit_wall_direction=0;
  cpu_pit_wall_was_airborne=false;
}
void MmxCoopReset(void) {
  cpu_companion_reset_motion();cpu_human_seat=cpu_swap_trigger_down=cpu_rescue_cooldown=0;
  cpu_l2_trigger_held=false;
  platform_entry.valid=false;lift_reset();shot_ghost_reset();
  MmxCoopViewsResetWorld();
  MmxWeaponsCameraQuery(enabled?weapon_view:NULL);
  platform_entry.valid=false;
  memset(&state, 0, sizeof(state));
  MmxWeaponsPartnerCombat(NULL);
  state.players[0].character = (uint8_t)starting_character;
  state.players[1].character = (uint8_t)(starting_character ^ 1);
  if (enabled) {
    MmxZeroState z = {0}; z.active_x = starting_character == MMX_COOP_X;
    MmxZeroSetState(z);
  }
}
bool MmxCoopEnable(unsigned character) {
  if (character > MMX_COOP_ZERO || !MmxZeroEnabled()) return false;
  starting_character = character; enabled = true; MmxCoopReset(); return true;
}
void MmxCoopDisable(void) { enabled = false; cpu_companion = false; starting_character = 0; MmxCoopReset(); }
void MmxCoopSetCpuCompanion(bool active) {
  if (cpu_companion!=active) {
    cpu_companion_reset_motion();
    cpu_human_seat=cpu_swap_trigger_down=cpu_rescue_cooldown=0;
    cpu_l2_trigger_held=false;
  }
  cpu_companion=active;
}
MmxCoopState MmxCoopGetState(void) { return state; }
bool MmxCoopValidState(const MmxCoopState *s) {
  if (!s || s->initialized > 1 || s->current > 1 || s->controller_pass > 2 ||
      s->effect_return>2 || s->object_reserved || s->object_pass > 2 ||
      s->platform_riders > 3 || s->contact_pass > 2 || s->enrolled>1 ||
      s->select_hold>180 || s->select_armed>1 || s->stage_pending>2 || /* Accept older 3-second hold saves. */
      s->menu_owner>2 || s->menu_last>1 || s->p1_select_hold>90 || s->p1_select_armed>1 ||
      s->pickup_pass>2 || s->pickup_reserved[0] || s->pickup_reserved[1] || s->pickup_reserved[2] ||
      s->anchor>1 || s->solo_death[0]>1 || s->solo_death[1]>1 || s->respawn_pending>3 ||
      s->scene_owner>2 || s->scene_phase>3 || s->door_pass>2 || s->scene_reserved || s->slime_p2>255 ||
      (s->scene_phase && !s->scene_owner) ||
      (s->door_pass && s->door_entry!=0xe70d && s->door_entry!=0xec98 && s->door_entry!=0xc0ae)) return false;
  for(unsigned i=0;i<16;++i) if(s->pickup_owner[i]>2) return false;
  if(s->pickup_pass && (s->pickup_d<0x1628 || s->pickup_d>=0x1928 || (s->pickup_d-0x1628)%48)) return false;
  if (s->object_pass && s->object_entry != 0xd2bd && s->object_entry != 0xd3dd &&
      s->object_entry != 0xd3fa && s->object_entry != 0xd43a && s->object_entry != 0xd457 &&
      s->object_entry != 0x9d67) return false;
  if (s->contact_pass && s->contact_entry != 0x9b03 && s->contact_entry != 0x9b43 &&
      s->contact_entry != 0xab81 && s->contact_entry != 0xab56) return false;
  for (unsigned i = 0; i < 2; ++i) {
    const MmxCoopPlayer *p = &s->players[i];
    if (p->character > MMX_COOP_ZERO || p->status > MMX_COOP_FALLEN ||
        p->input > 4095 || p->pressed > 4095 ||
        !MmxWeaponsValidState(&p->weapons) || !MmxWeaponsValidCombatState(&p->combat) ||
        !MmxZeroValidState(&p->zero)) return false;
    if (s->initialized && p->zero.active_x != (p->character == MMX_COOP_X)) return false;
    for (unsigned n = 1; n < 16; n += 2) if (p->energy[n] > 28) return false;
  }
  return s->players[0].character != s->players[1].character;
}
void MmxCoopSetState(const MmxCoopState *s) {
  platform_entry.valid=false;lift_reset();shot_ghost_reset();
  if (enabled && MmxCoopValidState(s)) {
    state = *s;
    MmxWeaponsPartnerCombat(state.initialized ? &state.players[state.current^1].combat : NULL);
  }
  else MmxCoopReset();
}
static bool ghost_active(void);
void MmxCoopCapture(uint8_t *r) {
  /* A ghost replay projects the other seat's body without selecting it. */
  if (!enabled || !state.initialized || !r || ghost_active()) return;
  MmxCoopPlayer *p = &state.players[state.current];
  memcpy(p->body, r + 0xba8, sizeof(p->body));
  memcpy(p->auxiliaries, r + 0xc38, sizeof(p->auxiliaries));
  memcpy(p->shots, r + 0x1228, sizeof(p->shots));
  memcpy(p->energy, r + 0x1f87, sizeof(p->energy));
  for (unsigned n = 1; n < 16; n += 2) p->energy[n] &= 63;
  p->zero = MmxZeroGetState();
  p->weapons = MmxWeaponsGetState();
  p->combat = MmxWeaponsGetCombatState();
  /* Fractional damage belongs to the world enemy, not to the attacker.
   * Keep the two serialized copies synchronized before projecting either. */
  memcpy(state.players[state.current^1].combat.enemies,p->combat.enemies,sizeof(p->combat.enemies));
  p->shot_command = r[0x1f0d]; p->hud_state = r[0x1f12];
}
bool MmxCoopSelect(uint8_t *r, unsigned player) {
  if (!enabled || !state.initialized || !r || player > 1 || ghost_active()) return false;
  if (player == state.current) return true;
  TRACE(SELECT,0,state.current,player,NULL);
  diagnostic_event(r,NULL,0,"select-before");
  select_line=0;
  MmxCoopCapture(r);
  state.current = (uint8_t)player;
  const MmxCoopPlayer *p = &state.players[player];
  memcpy(r + 0xba8, p->body, sizeof(p->body));
  memcpy(r + 0xc38, p->auxiliaries, sizeof(p->auxiliaries));
  memcpy(r + 0x1228, p->shots, sizeof(p->shots));
  for (unsigned n = 0; n < 16; ++n)
    r[0x1f87 + n] = p->energy[n] | ((n & 1) ? r[0x1f87 + n] & 0xc0 : 0);
  MmxZeroSetState(p->zero);
  MmxWeaponsSetState(p->weapons);
  MmxWeaponsSetCombatState(p->combat);
  MmxWeaponsPartnerCombat(&state.players[player^1].combat);
  r[0x1f0d] = p->shot_command; r[0x1f12] = p->hud_state;
  if (g_snes && g_snes->cart)
    MmxZeroSetCollisionRom(g_snes->cart->rom, g_snes->cart->romSize);
  return true;
}
/* Every later call records its source line, so the physics trace can name the
 * co-op hook behind each seat switch (select-before rows, caller column). */
#define MmxCoopSelect(r,player) (select_line=__LINE__,MmxCoopSelect((r),(player)))
static void select_world_survivor(uint8_t *r) {
  unsigned other=state.anchor^1;
  if(state.current==state.anchor && !(r[0xbcf]&127) &&
      state.players[other].status==MMX_COOP_ALIVE && (state.players[other].body[0x27]&127)) {
    /* World scripts must see the living actor during the other seat's death
     * countdown, not only after its orbs have finished spawning. */
    state.anchor=(uint8_t)other;MmxCoopSelect(r,other);
  }
}
/* A collector parked in action $18 by an item that sets neither $1F19 nor
 * the Heart Tank pause (a Sub Tank) froze only that player; the other kept
 * moving. Park the other player's update too, while the collector's own
 * native state runs the fill. */
static bool partner_parked(const uint8_t *r) {
  if(!state.initialized || state.menu_owner || state.scene_owner) return false;
  const MmxCoopPlayer *o=&state.players[state.current^1];
  return o->status==MMX_COOP_ALIVE && (o->body[0x27]&127) && o->body[2]==0x18 &&
      r[0xbaa]!=0x18 && r[0xbaa]!=12;
}
static bool refill_paused(const uint8_t *r) {
  if(state.menu_owner || state.scene_owner) return false;
  /* The native death controller also sets $1F19 while its countdown runs.
   * It must keep running even though live movement/terrain is paused. */
  if(r[0xbaa]==12) return false;
  if(r[0x1f19]) return true;
  /* $81:EAFB pauses tasks $1F13..18 for a Heart Tank, leaving $1F19
   * clear. Its saved airborne action/$1F3B can also resemble a cutscene.
   * Park both actors while the collected tank's native upgrade runs. */
  if(r[0x1f13] && r[0x1f16])
    for(unsigned d=0x1628;d<0x1928;d+=48)
      if(r[d] && r[d+10]==11 && r[d+1]==2 && r[d+2]==6) return true;
  return false;
}
void MmxCoopInitialize(uint8_t *r) {
  if (!enabled || state.initialized || !r || r[0xd1] != 2 || r[0xd2] != 4 || r[0xba9] != 2) return;
  state.initialized = 1; state.stage = r[0x1f7a];
  state.players[0].status = MMX_COOP_ALIVE;
  MmxCoopCapture(r);
  MmxCoopPlayer *partner = &state.players[1];
  /* Inventory starts full for a new partner. Shared unlocks and subtanks
   * remain in world WRAM; joining never creates another stored-healing pool. */
  for (unsigned n = 1; n < 16; n += 2) partner->energy[n] = 28;
  partner->weapons.initialized = 1; memset(partner->weapons.energy, 28, 16);
  partner->zero.active_x = partner->character == MMX_COOP_X;
  partner->body[0x27]=r[0x1f9a]|128;
  /* Co-op starts with both players enrolled. The ordinary arrival path waits
   * for gameplay and a safe landing, then uses the original teleport art. */
  state.enrolled=1;state.stage_pending=2;
  MmxWeaponsPartnerCombat(&partner->combat);
}
static void lift_close(void);
bool MmxCoopFrameTick(uint8_t *r) {
  if (!enabled || !state.initialized) return MmxWeaponsFrameTick(r);
  lift_close(); /* an elevator query never spans a frame */
  if (g_mmx_coop_trace) {MmxCoopTraceFrameBegin(&state);TRACE(FRAME,0,0,0,NULL);}
  /* The stage-clear weapon demonstration reuses the native player/shot
   * pools. It owns that single scripted actor; projecting either stored
   * co-op body here overwrites its weapon and recorded fire input. */
  if(r[0xd1]==2 && r[0xd2]==4 && r[0xd3]>=10) {
    state.stage_pending=1;state.select_hold=state.p1_select_hold=0;return false;
  }
  if (state.menu_owner) {
    if (state.current==1) MmxCoopApplyInput(r);
    state.select_hold=state.p1_select_hold=0;return false;
  }
  /* A changed stage requests initialization; it must not keep returning
   * before the block below can adopt the new stage and revive the roster. */
  if (state.stage!=r[0x1f7a]) state.stage_pending=1;
  if (r[0xd1]!=2 || r[0xd2]!=4 || r[0xba9]!=2) {
    state.stage_pending=1;state.select_hold=state.p1_select_hold=0;
    /* A new stage/checkpoint is initialized for the configured P1, even
     * when P2 was the last survivor driving the preceding world tasks. */
    if(state.anchor && (r[0xd1]!=2 || r[0xd2]!=4)) {MmxCoopSelect(r,0);state.anchor=0;}
    return false;
  }
  if (state.stage_pending==1 && r[0xd3]==4 && !r[0x1f0c]) {
    MmxCoopViewsResetWorld();
    /* Native stage entry/reset has rebuilt P1. Preserve enrollment and
     * reserves, refill P2 as agreed, and wait for safe ground to arrive. */
    MmxWeaponsState weapons=MmxWeaponsGetState();
    weapons.page=weapons.weapon=weapons.menu_page=0;weapons.charge=weapons.cooldown=0;
    MmxWeaponsSetState(weapons);
    MmxCoopCapture(r);state.stage=r[0x1f7a];state.players[0].status=MMX_COOP_ALIVE;
    memset(state.pickup_owner,0,sizeof(state.pickup_owner));state.pickup_pass=0;
    memset(state.solo_death,0,sizeof(state.solo_death));state.respawn_pending=0;
    state.scene_owner=state.scene_phase=state.door_pass=0;
    MmxCoopPlayer *p=&state.players[1];p->status=MMX_COOP_ABSENT;
    /* Match native P1's buster reset, for X1 and imported selections alike.
     * Voluntary withdrawal and scene transport retain their own selection. */
    p->body[0x33]=0;p->weapons.page=p->weapons.weapon=p->weapons.menu_page=0;
    p->body[0x27]=r[0x1f9a]|128;
    memset(p->energy,0,sizeof(p->energy));
    for(unsigned i=1;i<16;i+=2)p->energy[i]=28;
    memset(p->weapons.energy,28,sizeof(p->weapons.energy));
    memset(p->weapons.fraction,0,sizeof(p->weapons.fraction));p->weapons.charge=p->weapons.cooldown=0;
    memset(&p->combat,0,sizeof(p->combat));memset(p->shots,0,sizeof(p->shots));
    memset(&p->zero,0,sizeof(p->zero));p->zero.active_x=p->character==MMX_COOP_X;
    state.stage_pending=state.enrolled?2:0;
  }
  MmxCoopCapture(r);
  /* Boarding stores action $2C in the pilot's own context ($83:8605).
   * Keep that seat driving the native armor, including old saves made
   * after a partner return selected the wrong world actor. */
  if(!state.stage_pending && r[0xd3]==4 && r[0xe18] && (r[0xe22]&0x40)) {
    unsigned riders=0;
    for(unsigned seat=0;seat<2;++seat)
      if(state.players[seat].status==MMX_COOP_ALIVE &&
          (state.players[seat].body[0x27]&127) && state.players[seat].body[2]==0x2c)
        riders|=1u<<seat;
    if(riders==1 || riders==2) {
      state.anchor=(uint8_t)(riders==2);MmxCoopSelect(r,state.anchor);
      if(state.current==1) MmxCoopApplyInput(r);
    }
  }
  select_world_survivor(r);
  if((r[0xbcf]&127) && !(state.players[state.anchor^1].body[0x27]&127)) {
    /* Repair old co-op saves made after Penguin saw a dead world actor.
     * His combat state writes .30 only at $81:B6ED (player-dead latch).
     * Ordinary hit immunity uses .35 and its damage row; preserve those. */
    for(unsigned d=0xe68;d<0x1228;d+=64)
      if(r[d] && r[d+10]==2 && r[d+1]==4 && (r[d+0x27]&127) && r[d+0x30]==1)
        r[d+0x30]=0;
  }
  /* Keep both death animations, with only the world actor responsible for
   * the life decrement/checkpoint transition. Retiring the other seat here
   * would cancel its sound/orbs if the survivor dies during its countdown. */
  if(state.players[0].status==MMX_COOP_ALIVE && state.players[1].status==MMX_COOP_ALIVE &&
      !(state.players[0].body[0x27]&127) && !(state.players[1].body[0x27]&127)) {
    state.solo_death[state.anchor^1]=1;
    state.solo_death[state.anchor]=0;
  }
  /* Retail refills park their collector in action $18 while the item task
   * advances HP/energy. $00:D263 skips terrain collision when $1F19 is set.
   * The other actor must park too; running its motion through that pause
   * lets it fall through the floor. Keep running the native refill task. */
  if(refill_paused(r)) return false;
  if (scene_tick(r)) return true;
  if (!state.scene_owner && join_tick(r)) return true;
  /* In CPU mode, the last-chance pit safety runs only near native death;
   * it must never mask ordinary wall-slide navigation. */
  if (cpu_traversal_active()) cpu_companion_rescue(r);
  if (r[0x1f10]>=6) return false;
  unsigned phases[2]={0,0};
  for (unsigned seat=0;seat<2;++seat) if (state.players[seat].status==MMX_COOP_ALIVE &&
      (!state.scene_owner || seat==state.anchor)) {
    MmxCoopSelect(r,seat);
    MmxWeaponsFrameTick(r);
    MmxCoopCapture(r);
    phases[seat]=MmxWeaponsTimePhase(&state.players[seat].combat);
  }
  MmxCoopSelect(r,state.anchor);
  /* Two staggered half-speed effects must not alternate into a permanent
   * freeze. Advance one shared display-frame cadence, while both ages tick. */
  ++state.time_tick;
  bool frozen=phases[0]==1 || phases[1]==1 ||
      ((phases[0]==2 || phases[1]==2) && !(state.time_tick&1));
  if (frozen) r[0xb9d]=r[0xba0]=0;
  return frozen;
}
/* Offline companion controls are the 12-bit SNES pad masks. Use explicit
 * constants here: this module does not include the host UI's SNES_PAD_* macros. */
enum {
  MMX_CPU_JUMP = 1u << 0, MMX_CPU_FIRE = 1u << 1,
  MMX_CPU_LEFT = 1u << 6, MMX_CPU_RIGHT = 1u << 7,
  MMX_CPU_DASH = 1u << 8
};
/* X3 Zero is charge-capable: his own MmxZeroPlayerTick increments charge
 * while Y is HELD and fires on release. Modern Zero replaces that buster
 * with a direct saber and has his own shorter melee attack cadence.
 * Keep this CPU-only selection outside the traversal paths: wall/jump
 * returns otherwise bypass attack decisions on nearly every busy tick. */
enum {
  MMX_CPU_WALL_IDLE, MMX_CPU_WALL_SEEK, MMX_CPU_WALL_PUSH,
  MMX_CPU_WALL_RETURN, MMX_CPU_WALL_FINISHED
};
/* Both X and Zero stand about 16 pixels above their feet. Probe several pixels
 * below the feet so a small step down does not register as a bottomless pit.
 * A probe is terrain only; moving platforms and scripted geometry need a
 * separate later pass. The game's actual collision map is queried live. */
static bool cpu_companion_supported(const uint8_t *ram, int x, int feet) {
  for (int depth=0; depth<=20; depth+=4)
    if (MmxWeaponsTerrainSolid(ram,x,feet+depth,true,NULL)) return true;
  return false;
}
/* Short, directional ground-level rays from the follower's OWN feet.
 * Checking both 20 and 28 pixels ahead lets Zero notice an edge before
 * his sprite crosses it. Never sample P1's position or jump input here. */
static bool cpu_companion_ground_missing(const uint8_t *ram,int x,int feet,int dir) {
  if (!ram || !dir) return false;
  bool here=cpu_companion_supported(ram,x,feet);
  bool ahead20=cpu_companion_supported(ram,x+dir*20,feet);
  bool ahead28=cpu_companion_supported(ram,x+dir*28,feet);
  /* Instrument the real per-tick sensor even when it finds NO edge.
   * This proves that the AI checks terrain before the walking decision. */
  if (getenv("MMX_CPU_TRACE")) {
    static int last_trace=-1000;
    if (snes_frame_counter-last_trace>=120) {
      fprintf(stderr,"[cpu-sense] x=%d feet=%d dir=%d ground=%d ahead20=%d ahead28=%d\n",
              x,feet,dir,(int)here,(int)ahead20,(int)ahead28);
      last_trace=snes_frame_counter;
    }
  }
  return here && (!ahead20 || !ahead28);
}
/* A pit jump needs a plausible *landing*, not just a nearby drop.
 * Sample only solid walkable terrain near the follower's current foot level.
 * This is a conservative short-jump planner, not a proof of a clear arc or a
 * replacement for native moving-platform contacts. */
static bool cpu_companion_walkable(unsigned tile) {
  return tile==0x13 || (tile>=1 && tile<=12) ||
         (tile>=0x34 && tile<=0x38) ||
         (tile>=0x3b && tile<=0x3d);
}
static bool cpu_companion_landing(const uint8_t *ram,int x,int feet) {
  for (int delta=-20;delta<=24;delta+=4) {
    int py=feet+delta;
    unsigned tile=MmxWeaponsTerrainClass(ram,x,py);
    if (!cpu_companion_walkable(tile)) continue;
    int surface=0;
    if (MmxWeaponsTerrainSolid(ram,x,py,true,&surface) &&
        surface>=feet-20 && surface<=feet+24) return true;
  }
  return false;
}
/* Look ahead for a safe floor at a different elevation, not only an
 * equal-height landing inside the first 96 pixels. Highway repeatedly
 * alternates medium gaps and higher roofs: the old fixed 40..96px and
 * +/-20px check returned false at a real edge (ground=1 ahead28=0), then
 * the WAIT decision suppressed every jump forever.
 *
 * Native jump physics and wall-slide recovery remain responsible for the
 * actual crossing. A candidate must be walkable terrain, have head space
 * above it, and have solid support slightly farther into the landing.
 * Distances beyond 176px are NOT treated as safe native jumps. */
static bool cpu_companion_gap_reachable(const uint8_t *ram,int x,int feet,
                                         int direction,int *distance_out,
                                         int *rise_out) {
  if (!ram || !direction) return false;
  for (int d=40;d<=176;d+=8) {
    int landing_x=x+direction*d;
    for (int rise=0;rise<=88;rise+=8) {
      int py=feet-rise,surface=0;
      unsigned type=MmxWeaponsTerrainClass(ram,landing_x,py);
      if (!cpu_companion_walkable(type) ||
          !MmxWeaponsTerrainSolid(ram,landing_x,py,true,&surface) ||
          surface<feet-88 || surface>feet+24 ||
          MmxWeaponsTerrainSolid(ram,landing_x,surface-16,true,NULL) ||
          MmxWeaponsTerrainSolid(ram,landing_x,surface-32,true,NULL) ||
          !cpu_companion_supported(ram,landing_x+direction*8,surface+2))
        continue;
      /* Do not infer a high unreachable roof across a full screen as a
       * safe landing for a single undashed jump. */
      if (d>144 && feet-surface>40) continue;
      if (distance_out) *distance_out=d;
      if (rise_out) *rise_out=feet-surface;
      return true;
    }
    /* Some safe short ledges are slightly below the starting ground. */
    if (d<=112 && cpu_companion_landing(ram,landing_x,feet) &&
        !MmxWeaponsTerrainSolid(ram,landing_x,feet-28,false,NULL)) {
      if (distance_out) *distance_out=d;
      if (rise_out) *rise_out=0;
      return true;
    }
  }
  return false;
}
/* At a descending junction, X may be on verified LOWER terrain.
 * A roof-level-only search previously returned no landing and WAIT locked
 * the CPU at the lip, even with X standing safely on the tier below.
 * Match the lower candidate's surface to X's grounded foot elevation, and
 * require support and free headroom on that candidate; never drop toward
 * an X who is still falling or to an unseen void.
 * A nearby lower floor is a step-off (DROP), a distant one needs a jump. */
static bool cpu_companion_lower_landing(const uint8_t *ram,
                                        int x,int feet,int dir,int leader_feet,
                                        int *distance_out,int *drop_out) {
  if (!ram || !dir) return false;
  for (int d=32;d<=176;d+=8) {
    int sx=x+dir*d;
    for (int lower=24;lower<=112;lower+=8) {
      int floor=feet+lower,surface=0;
      if (!cpu_companion_walkable(MmxWeaponsTerrainClass(ram,sx,floor)) ||
          !MmxWeaponsTerrainSolid(ram,sx,floor,true,&surface) ||
          surface<feet+20 || surface>feet+112 ||
          abs(surface-leader_feet)>24 ||
          (d>128 && surface-feet>72) ||
          !cpu_companion_supported(ram,sx+dir*8,surface+2) ||
          MmxWeaponsTerrainSolid(ram,sx,surface-20,true,NULL) ||
          MmxWeaponsTerrainSolid(ram,sx,surface-32,true,NULL))
        continue;
      if (distance_out) *distance_out=d;
      if (drop_out) *drop_out=surface-feet;
      return true;
    }
  }
  return false;
}

/* Deliberate pit traverse: locate a physical wall BELOW the far lip.
 * Demand open terrain through the gap and an exposed, continuous wall
 * face within an ordinary jump/dash approach range. The leader must separately
 * be standing beyond this wall. This is not a guaranteed safe trajectory. */
static bool cpu_companion_opposite_pit_wall(
    const uint8_t *ram,int x,int feet,int dir,
    int *distance_out,int *depth_out) {
  if (!ram || !dir) return false;
  int visible_bottom=(int)word(ram+0x1e5c)+224;
  for (int distance=40;distance<=160;distance+=4) {
    int wx=x+dir*distance;
    /* There must be a void before the target. Solid walkways are handled
     * by the existing regular movement and landing planners. */
    if (cpu_companion_supported(ram,x+dir*(distance/2),feet) ||
        cpu_companion_supported(ram,wx-dir*12,feet))
      continue;
    for (int depth=24;depth<=80;depth+=8) {
      int wy=feet+depth;
      /* Never approve a catch already below the camera's death area. */
      if (wy+40>visible_bottom+16) continue;
      if (!MmxWeaponsTerrainSolid(ram,wx,wy,false,NULL) ||
          !MmxWeaponsTerrainSolid(ram,wx,wy+24,false,NULL) ||
          !MmxWeaponsTerrainSolid(ram,wx,wy+40,false,NULL) ||
          MmxWeaponsTerrainSolid(ram,wx-dir*12,wy,false,NULL) ||
          MmxWeaponsTerrainSolid(ram,wx-dir*12,wy+24,false,NULL))
        continue;
      bool clear=true;
      for (int step=12;step<distance-12;step+=12)
        if (MmxWeaponsTerrainSolid(
                ram,x+dir*step,feet-12,false,NULL)) {
          clear=false;break;
        }
      if (!clear) continue;
      if (distance_out) *distance_out=distance;
      if (depth_out) *depth_out=depth;
      return true;
    }
  }
  return false;
}

/* CPU wall climb: detect a reachable upper platform.
 * Native movement and collision remain in control. */
static bool cpu_companion_upper_lip(
    const uint8_t *ram, int x, int feet, int dir)
{
  if (!ram || !dir) return false;

  for (int distance = 24; distance <= 56; distance += 8) {
    int lx = x + dir * distance;

    for (int depth = 4; depth <= 28; depth += 4) {
      int surface = 0;
      int sample = feet + depth;

      if (!cpu_companion_walkable(
              MmxWeaponsTerrainClass(ram, lx, sample)))
        continue;

      if (!MmxWeaponsTerrainSolid(
              ram, lx, sample, true, &surface))
        continue;

      if (surface < feet + 4 || surface > feet + 28)
        continue;

      if (MmxWeaponsTerrainSolid(
              ram, lx, surface - 20, false, NULL))
        continue;

      if (MmxWeaponsTerrainSolid(
              ram, lx, surface - 36, false, NULL))
        continue;

      if (!cpu_companion_supported(
              ram, lx + dir * 8, surface + 2))
        continue;

      return true;
    }
  }

  return false;
}

/* A normal-height landing check deliberately rejects elevated platforms.
 * A gap ending at a higher vertical wall is different: Zero may leap to the
 * wall, latch onto its native slide and climb with successive wall kicks.
 * Require BOTH a solid wall in front of the void and walkable, open terrain
 * at the top of it. Scan only short local distances, not across whole maps;
 * this is an attempt plan, not a guarantee the jump trajectory succeeds. */
static bool cpu_companion_raised_wall(const uint8_t *ram,int x,int feet,
                                      int direction,int *distance_out) {
  if (!ram || !direction) return false;
  for (int distance=24;distance<=192;distance+=4) {
    int wx=x+direction*distance;
    if (!MmxWeaponsTerrainSolid(ram,wx,feet-16,false,NULL) ||
        !MmxWeaponsTerrainSolid(ram,wx,feet-40,false,NULL) ||
        MmxWeaponsTerrainSolid(ram,wx-direction*12,feet-24,false,NULL))
      continue;
    /* Tall Highway support columns are roughly 170+ game pixels high.
     * A 144px ceiling on wall detection used to make Zero freeze at their
     * bases even though native repeated wall kicks can reach their tops. */
    for (int rise=32;rise<=256;rise+=4) {
      int surface=0;
      int sample=feet-rise;
      if (!cpu_companion_walkable(MmxWeaponsTerrainClass(ram,wx,sample)) ||
          !MmxWeaponsTerrainSolid(ram,wx,sample,true,&surface) ||
          surface>feet-32 || surface<feet-256 ||
          MmxWeaponsTerrainSolid(ram,wx,surface-20,true,NULL))
        continue;
      int landing_x=wx+direction*20;
      if (!cpu_companion_walkable(MmxWeaponsTerrainClass(ram,landing_x,surface+4)) ||
          !MmxWeaponsTerrainSolid(ram,landing_x,surface+4,true,NULL) ||
          MmxWeaponsTerrainSolid(ram,landing_x,surface-24,true,NULL))
        continue;
      if (distance_out) *distance_out=distance;
      return true;
    }
  }
  return false;
}
/* On tall structures the top can be off-screen and outside a useful
 * terrain scan. A solid vertical FACE across a short approach is still a
 * wall-climb target if the leader is on its far, higher side. This does NOT
 * copy the human's jump input: the wall and goal must both be present. */
static int cpu_companion_wall_face(const uint8_t *ram,int x,int feet,int dir) {
  if (!ram || !dir) return 0;
  for (int d=12;d<=144;d+=4) {
    int wx=x+dir*d;
    if (MmxWeaponsTerrainSolid(ram,wx,feet-16,false,NULL) &&
        MmxWeaponsTerrainSolid(ram,wx,feet-36,false,NULL) &&
        !MmxWeaponsTerrainSolid(ram,wx-dir*12,feet-32,false,NULL))
      return d;
  }
  return 0;
}
/* Detect a low or tall solid obstacle directly ahead at chest/leg height,
 * without depending on P1 jumping or standing on a higher platform. Probe
 * non-floor collision so a nearby slope or harmless decoration is not a wall. */
static bool cpu_companion_obstacle_ahead(const uint8_t *ram,int x,int y,int direction) {
  if (!direction) return false;
  for (int look=14;look<=44;look+=6) {
    int wall=x+direction*look;
    if (MmxWeaponsTerrainSolid(ram,wall,y+5,false,NULL) ||
        MmxWeaponsTerrainSolid(ram,wall,y-5,false,NULL) ||
        MmxWeaponsTerrainSolid(ram,wall,y-14,false,NULL)) return true;
  }
  return false;
}
/* Enemy bodies occupy $0E68..$1227, one 64-byte record per slot.
 * Choose a living target roughly level with the companion and in the current
 * travel/facing direction. Avoid attacking every decorative/scripted object.
 * Shots are brief Y-button taps rather than holding charge indefinitely.
 * This is deliberately simple; visibility/obstacle-aware aiming is future work. */
static bool cpu_companion_enemy_ahead(const uint8_t *r, int x, int y, int direction) {
  if (!direction) return false;
  for (unsigned d=0xe68;d<0x1228;d+=64) {
    if (!r[d] || !r[d+14] || !(r[d+0x27]&127)) continue;
    int dx=(int)word(r+d+5)-x, dy=(int)word(r+d+8)-y;
    if (dx*direction>=12 && dx*direction<=152 && abs(dy)<=40) return true;
  }
  return false;
}
/* Modern saber reach is short; a regular buster can address enemies
 * across far more of the shared screen. Use game object collision data
 * instead of firing at any decorative/scripted sprite. */
static bool cpu_companion_enemy_melee(const uint8_t *r,int x,int y,int dir) {
  if (!r || !dir) return false;
  for (unsigned d=0xe68;d<0x1228;d+=64) {
    if (!r[d] || !r[d+14] || !(r[d+0x27]&127)) continue;
    int dx=(int)word(r+d+5)-x,dy=(int)word(r+d+8)-y;
    if (dx*dir>=0 && dx*dir<=56 && abs(dy)<=36) return true;
  }
  return false;
}
/* Highest-priority native motion is decided by cpu_companion_input() before
 * this attack/dash postprocessor. Holding charge never overrides jump or
 * wall-recovery directions, and any charged blast waits until a safe time
 * when the native weapon can fire. */
static uint16_t cpu_companion_zero_combat(const uint8_t *r,
                                          const MmxCoopPlayer *f,
                                          const MmxCoopPlayer *leader,
                                          uint16_t input) {
  if (!r || !f || !leader || f->character!=MMX_COOP_ZERO ||
      f->status!=MMX_COOP_ALIVE || !(f->body[0x27]&127) ||
      f->body[2]==12) return input;
  int x=(int)word(f->body+5),y=(int)word(f->body+8);
  int dx=(int)word(leader->body+5)-x;
  int dir=(input&MMX_CPU_RIGHT) ? 1 : (input&MMX_CPU_LEFT) ? -1 :
          (f->body[0x69]&64 ? 1 : -1);
  bool target=cpu_companion_enemy_ahead(r,x,y,dir);
  bool recovering=cpu_wall_recovery_phase!=MMX_CPU_WALL_IDLE;
  if (f->zero.modern.enabled) {
    /* Modern mode: direct saber, not a chargeable X3 buster. */
    input&=(uint16_t)~MMX_CPU_FIRE;
    if (cpu_zero_melee_cooldown) --cpu_zero_melee_cooldown;
    if (!recovering && !cpu_zero_melee_cooldown &&
        cpu_companion_enemy_melee(r,x,y,dir)) {
      input|=MMX_CPU_FIRE;
      cpu_zero_melee_cooldown=22;
    }
  } else {
    /* X3 mode: build a powerful shot while running and jumping. Fire on
     * release only if an enemy is in front AND the native player isn't
     * busy with a wall-recovery or an earlier charged burst. With no
     * target, hold a full ready charge rather than waste it at empty air. */
    bool ready=f->zero.charge>=141;
    bool busy=f->zero.burst || f->zero.slash || f->zero.combo ||
              f->zero.swap_phase;
    if (ready && target && !recovering && !busy &&
        (f->input&MMX_CPU_FIRE)) {
      /* Native X3 fires on RELEASE, not on a continuous unheld pad.
       * If a charge release was suppressed by an action/animation, rearm
       * Y for one tick so we can try again with a real input edge. */
      input&=(uint16_t)~MMX_CPU_FIRE;
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,"[cpu-attack] Zero X3 charged release charge=%u x=%d y=%d\n",
                (unsigned)f->zero.charge,x,y);
    } else input|=MMX_CPU_FIRE;
  }
  /* Ground dash on VERIFIED clear terrain for fast catching up, including
   * run-up toward a far raised climb. The native game decides the actual
   * dash speed and animation; never force dash across an unverified gap
   * or through a solid wall. Air dash is exclusively Modern mode and
   * requires separate terrain-safe flight planning. */
  bool safe_dash=false;
  if ((f->body[0x2b]&4) && dx*dir>112 &&
      (input&(dir>0?MMX_CPU_RIGHT:MMX_CPU_LEFT)) &&
      cpu_companion_supported(r,x,y+16) &&
      cpu_companion_supported(r,x+dir*28,y+16) &&
      cpu_companion_supported(r,x+dir*48,y+16) &&
      !cpu_companion_obstacle_ahead(r,x,y,dir) &&
      !cpu_companion_enemy_melee(r,x,y,dir)) {
    input|=MMX_CPU_DASH;
    safe_dash=true;
  }
  if (safe_dash && !cpu_zero_dash_was_active && getenv("MMX_CPU_TRACE"))
    fprintf(stderr,"[cpu-dash] Zero ground dash x=%d y=%d leader_dx=%d\n",
            x,y,dx);
  cpu_zero_dash_was_active=safe_dash;
  return input;
}
/* A physical wall can be contacted during air action 6/8 before the
 * native wall-slide action 0x10 appears. Prepare recovery by steering INTO
 * the sensed face, with B released, so a later slide frame can start a
 * real wall jump. Contact alone must not count toward the two kicks. */
static int cpu_companion_air_wall_contact(const uint8_t *ram,int x,int y,
                                           int prefer_dir) {
  if (!ram) return 0;
  int dirs[2]={prefer_dir?prefer_dir:1,prefer_dir?-prefer_dir:-1};
  for (unsigned i=0;i<2;++i) {
    int d=dirs[i];
    /* 19px begins preparing the B-release before the sprite actually
     * enters the wall's native slide state. Require two vertical samples
     * to avoid treating isolated floor tiles as a climbable wall. */
    if ((MmxWeaponsTerrainSolid(ram,x+d*11,y-4,false,NULL) ||
         MmxWeaponsTerrainSolid(ram,x+d*15,y-4,false,NULL) ||
         MmxWeaponsTerrainSolid(ram,x+d*19,y-4,false,NULL)) &&
        (MmxWeaponsTerrainSolid(ram,x+d*11,y-16,false,NULL) ||
         MmxWeaponsTerrainSolid(ram,x+d*15,y-16,false,NULL) ||
         MmxWeaponsTerrainSolid(ram,x+d*19,y-16,false,NULL)))
      return d;
  }
  return 0;
}
/* Native wall kicks can be buffered while approaching within ~7 px of
 * a wall. Keep the buffered sensor STRICTER than the early 19 px steering
 * probe, and attempt it only during visually confirmed descent.
 * A rejected buffer releases B immediately, so the first native slide
 * frame can still be used without wasting a recovery jump. */
static bool cpu_companion_wall_jump_near(const uint8_t *ram,int x,int y,int dir) {
  if (!ram || !dir) return false;
  return (MmxWeaponsTerrainSolid(ram,x+dir*11,y-4,false,NULL) ||
          MmxWeaponsTerrainSolid(ram,x+dir*15,y-4,false,NULL)) &&
         (MmxWeaponsTerrainSolid(ram,x+dir*11,y-16,false,NULL) ||
          MmxWeaponsTerrainSolid(ram,x+dir*15,y-16,false,NULL));
}
/* A failed wall ascent should retain a safe exit: scan REAL walkable
 * terrain a short distance AWAY from the collision wall, BELOW Zero's
 * airborne feet. This is never a synthetic teleport or an invented dash
 * target. Solid headroom and additional support farther into the shelf
 * are required before committing to an escape direction. */
static int cpu_companion_wall_escape(const uint8_t *r,int x,int y,
                                      int wall_dir,int *distance_out) {
  if (!r || !wall_dir) return 0;
  int away=-wall_dir,feet=y+16;
  for (int d=24;d<=96;d+=8) {
    int lx=x+away*d;
    for (int drop=16;drop<=88;drop+=8) {
      int sample=feet+drop,surface=0;
      if (!cpu_companion_walkable(MmxWeaponsTerrainClass(r,lx,sample)) ||
          !MmxWeaponsTerrainSolid(r,lx,sample,true,&surface) ||
          surface<feet+12 || surface>feet+88 ||
          MmxWeaponsTerrainSolid(r,lx,surface-20,true,NULL) ||
          MmxWeaponsTerrainSolid(r,lx,surface-36,true,NULL) ||
          !cpu_companion_supported(r,lx+away*8,surface+2))
        continue;
      if (distance_out) *distance_out=d;
      return away;
    }
  }
  return 0;
}
/* Native and buffered wall-kick input is highest priority.
 * Ordinary recovery is limited to two kicks. A genuine upper-platform goal
 * invokes a taller eight-kick climb instead, using *real* renewed native
 * wall-slide states rather than timed synthetic teleports. The away-then-
 * toward steering is shorter for tall climbs so Zero can reattach higher. */
static uint16_t cpu_companion_wall_recovery(const MmxCoopPlayer *f,
                                            bool wall_slide,bool near_wall,
                                            bool descending,bool at_kick_apex) {
  uint16_t toward=cpu_wall_direction>0 ? MMX_CPU_RIGHT : MMX_CPU_LEFT;
  uint16_t away=cpu_wall_direction>0 ? MMX_CPU_LEFT : MMX_CPU_RIGHT;
  unsigned max_kicks=cpu_tall_wall_climb ? 8u : 2u;
  if (!wall_slide && cpu_wall_recovery_jumps)
    cpu_wall_recovery_left_slide=true;
  /* A fast wall climb must keep B through the upward motion, then have
   * B UP before contact returns. Detect the native apex from successive
   * Y samples rather than wasting 4+9 steering frames or guessing a
   * fixed button hold. If the apex and slide happen on the same tick,
   * releasing here restores a fresh B edge for the NEXT available tick. */
  if (cpu_tall_wall_climb && at_kick_apex && cpu_wall_jump_hold_frames) {
    cpu_wall_jump_hold_frames=0;
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] apex B-release kick=%u x=%u y=%u frame=%d\n",
              (unsigned)cpu_wall_recovery_jumps,
              (unsigned)word(f->body+5),(unsigned)word(f->body+8),
              snes_frame_counter);
  }

  /* A buffered press has to result in the game's wall-kick action ($12),
   * or it was only a speculative press against nearby collision. Count
   * only CONFIRMED wall-kicks, never the attempted pre-slide B edge. */
  if (cpu_wall_buffer_pending) {
    cpu_wall_buffer_pending=false;
    if (f->body[2]==0x10) {
      ++cpu_wall_recovery_jumps;
      cpu_wall_recovery_left_slide=false;
      cpu_wall_recovery_phase=MMX_CPU_WALL_PUSH;
      if (cpu_tall_wall_climb) {
        cpu_tall_wall_kick_x=(uint16_t)word(f->body+5);
        cpu_tall_wall_kick_y=(uint16_t)word(f->body+8);
        cpu_tall_wall_trace_ticks=0;
      }
      cpu_wall_recovery_ticks=cpu_tall_wall_climb ? 0 : 4;
      cpu_wall_jump_hold_frames=cpu_tall_wall_climb ? 24 : 17;
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,"[cpu-wall] buffered kick confirmed %u/%u action=16 frame=%d\n",
                (unsigned)cpu_wall_recovery_jumps,max_kicks,snes_frame_counter);
      return (cpu_tall_wall_climb ? toward : away)|MMX_CPU_JUMP;
    }
    /* Native code did not accept the buffer. Release B to re-arm the
     * next opportunity; do not spend a jump or wait for a cooldown. */
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] buffered press not accepted action=%u frame=%d\n",
              (unsigned)f->body[2],snes_frame_counter);
    return toward;
  }

  /* If Zero is sliding and B is still held from the kick ascent, release
   * it NOW; next frame is the earliest possible new B press edge. The
   * original 4+9-frame steering timers never delay a reattached slide. */
  if (wall_slide && (f->input&MMX_CPU_JUMP) &&
      (cpu_wall_recovery_jumps==0 || cpu_wall_recovery_left_slide)) {
    /* Native wall jumps are EDGE-triggered. This matters on the very
     * FIRST contact too: an ordinary ground jump can still hold B when
     * Zero enters action $10. Writing B again would not generate a new
     * press and the old code counted a nonexistent kick. Release once,
     * keep contact, then press on the next truly eligible native frame. */
    cpu_wall_jump_hold_frames=0;
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] rearm B before kick=%u action=%u frame=%d\n",
              (unsigned)(cpu_wall_recovery_jumps+1),
              (unsigned)f->body[2],snes_frame_counter);
    return toward;
  }
  if (wall_slide && cpu_wall_recovery_jumps<max_kicks &&
      (cpu_wall_recovery_jumps==0 || cpu_wall_recovery_left_slide)) {
    ++cpu_wall_recovery_jumps;
    cpu_wall_recovery_left_slide=false;
    cpu_wall_recovery_phase=MMX_CPU_WALL_PUSH;
    if (cpu_tall_wall_climb) {
      cpu_tall_wall_kick_x=(uint16_t)word(f->body+5);
      cpu_tall_wall_kick_y=(uint16_t)word(f->body+8);
      cpu_tall_wall_trace_ticks=0;
    }
    /* A long climb needs to get back to its wall quickly. One kick-off
     * frame creates a clean release, then hold INTO the same wall while
     * still rising; otherwise Zero drifts away and lands back at the base. */
    cpu_wall_recovery_ticks=cpu_tall_wall_climb ? 0 : 4;
    /* Hold B for the whole native upward arc on a tall climb, up to
     * 24 frames as a safety ceiling. Release at the measured apex,
     * not at a guessed six-frame delay; the next slide must see a
     * FRESH press edge to allow another immediate native wall-jump. */
    cpu_wall_jump_hold_frames=cpu_tall_wall_climb ? 24 : 18;
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] jump=%u/%u frame=%d dir=%d tall=%d best_y=%u start_y=%u\n",
              (unsigned)cpu_wall_recovery_jumps,max_kicks,
              snes_frame_counter,(int)cpu_wall_direction,(int)cpu_tall_wall_climb,
              (unsigned)cpu_tall_wall_best_y,(unsigned)cpu_tall_wall_start_y);
    return toward|MMX_CPU_JUMP;
  }

  /* First opportunity can precede the dedicated wall-slide action.
   * This is a ONE-SHOT buffered attempt per airborne recovery, not
   * continuous B spam; the $12 transition is required to count success. */
  if (!cpu_tall_wall_climb && !wall_slide && near_wall && descending &&
      !cpu_wall_buffer_attempted &&
      cpu_wall_recovery_jumps==0 &&
      !(f->input&MMX_CPU_JUMP) &&
      (f->body[2]==6 || f->body[2]==8)) {
    cpu_wall_buffer_attempted=true;
    cpu_wall_buffer_pending=true;
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] buffered near-wall B action=%u frame=%d\n",
              (unsigned)f->body[2],snes_frame_counter);
    return toward|MMX_CPU_JUMP;
  }

  if (cpu_wall_recovery_jumps>=max_kicks && !cpu_wall_jump_hold_frames) {
    cpu_wall_recovery_phase=MMX_CPU_WALL_FINISHED;
    return toward;
  }

  /* Maintaining B through the wall-kick ascent provides its full height,
   * unlike the old one-frame kick. Release early on new wall slide above.
   * Continue to steer away briefly, then back toward the same wall. */
  uint16_t steer=toward;
  if (cpu_wall_recovery_phase==MMX_CPU_WALL_PUSH) {
    /* Native wall-kick already provides a strong push-off.
     * Pressing AWAY again prevented high-wall reattachment. */
    steer=cpu_tall_wall_climb ? toward : away;
    if (cpu_wall_recovery_ticks) --cpu_wall_recovery_ticks;
    if (!cpu_wall_recovery_ticks) {
      cpu_wall_recovery_phase=MMX_CPU_WALL_RETURN;
      cpu_wall_recovery_ticks=cpu_tall_wall_climb ? 1 : 9;
    }
  } else if (cpu_wall_recovery_phase==MMX_CPU_WALL_RETURN) {
    if (cpu_wall_recovery_ticks) --cpu_wall_recovery_ticks;
    if (!cpu_wall_recovery_ticks) cpu_wall_recovery_phase=MMX_CPU_WALL_SEEK;
  } else cpu_wall_recovery_phase=MMX_CPU_WALL_SEEK;
  if (cpu_wall_jump_hold_frames) {
    --cpu_wall_jump_hold_frames;
    steer|=MMX_CPU_JUMP;
  }
  return steer;
}
/* Feed native co-op pad input rather than moving sprites directly.
 * Offline-only host timers preserve variable-height ground jumps and allow
 * fresh B press edges for wall kicks. No co-op save ABI or netplay changes. */
static uint16_t cpu_companion_input(const uint8_t *ram,unsigned controlled_seat) {
  const MmxCoopPlayer *leader=&state.players[controlled_seat],
                      *follower=&state.players[controlled_seat^1];
  if (!ram || !state.initialized || state.menu_owner || state.scene_owner ||
      state.stage_pending || leader->status!=MMX_COOP_ALIVE ||
      follower->status!=MMX_COOP_ALIVE ||
      !(leader->body[0x27]&127) || !(follower->body[0x27]&127)) {
    cpu_companion_reset_motion();
    return 0;
  }
  int dx=(int)word(leader->body+5)-(int)word(follower->body+5);
  /* Follow the leader's ACTUAL position, not their controller direction.
   * Briefly tapping left while X is still to Zero's right should not
   * reverse Zero's approach. Wall-kick recovery is a separate state. */
  int direction=MmxCoopCpuFollowDirection(dx);
  int x=(int)word(follower->body+5),y=(int)word(follower->body+8);
  /* Horizontal follow reaches its dead zone even when P1 is standing on
   * a high platform. Search BOTH sides for an actual close wall before
   * choosing a route: this is not a reaction to P1's B/jump input.
   * Run ahead of the early !direction exit below. */
  /* The human must already be standing on the higher tier. A midair X
   * jumping in place is not a climb destination and must not steer Zero. */
  bool elevated_goal=(leader->body[0x2b]&4)!=0 &&
                      (int)word(leader->body+8)<y-48;
  bool climb_from_below=false;
  int climb_face=0;
  if (!direction && elevated_goal) {
    int right=cpu_companion_wall_face(ram,x,y+16,1);
    int left=cpu_companion_wall_face(ram,x,y+16,-1);
    /* Don't start crossing a distant gap merely because a wall exists
     * somewhere far away. Use a wall within 80 native stage pixels. */
    if (right>80) right=0;
    if (left>80) left=0;
    direction=MmxCoopCpuChooseClimbDirection(dx,right,left);
    if (direction) {
      climb_from_below=true;
      climb_face=direction>0 ? right : left;
    }
  }
  bool grounded=(follower->body[0x2b]&4)!=0;
  /* Keep targeting the opposite wall through descent, even if the
   * usual horizontal follow deadzone is crossed in midair. */
  if (cpu_pit_wall_ticks) {
    if (!grounded) cpu_pit_wall_was_airborne=true;
    if (grounded && cpu_pit_wall_was_airborne) {
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,"[cpu-pit] landed before wall contact x=%d y=%d\n",x,y);
      cpu_pit_wall_ticks=0;cpu_pit_wall_was_airborne=false;
      cpu_pit_wall_direction=0;
    } else {
      direction=cpu_pit_wall_direction;
      --cpu_pit_wall_ticks;
      if (!cpu_pit_wall_ticks && getenv("MMX_CPU_TRACE"))
        fprintf(stderr,"[cpu-pit] approach expired x=%d y=%d\n",x,y);
    }
  }
  bool wall_slide=!grounded && follower->body[2]==0x12;
  /* Use actual Y-position change across game frames for descent, rather
   * than mistaking an upward wall-kick action $12 for a downward slide. */
  bool descending=!grounded && cpu_wall_y_valid &&
                  y>(int)cpu_wall_last_y;
  /* Measured apex: Zero has already climbed >=6 pixels since kick
   * takeoff and his Y has stopped decreasing. A Y equality is enough;
   * it is the last opportunity to release B before descent and slide. */
  bool kick_apex=!grounded && cpu_tall_wall_climb &&
      cpu_wall_recovery_jumps>0 && cpu_wall_y_valid &&
      (int)cpu_tall_wall_kick_y-y>=6 &&
      y>=(int)cpu_wall_last_y;
  if (grounded) cpu_wall_y_valid=false;
  else cpu_wall_y_valid=true;
  cpu_wall_last_y=(uint16_t)y;
  int touching_wall=!grounded &&
      (wall_slide || descending) ?
      cpu_companion_air_wall_contact(ram,x,y,direction) : 0;
  /* Detect genuine obstruction by observing lack of horizontal progress
   * while repeatedly driving a direction. Useful for moving gates and
   * objects that are not represented in the static terrain map. */
  if (grounded && direction && direction==cpu_stall_direction &&
      abs(x-(int)cpu_last_x)<=1) {
    if (cpu_stall_ticks<24) ++cpu_stall_ticks;
  } else cpu_stall_ticks=0;
  cpu_last_x=(uint16_t)x;cpu_stall_direction=(int8_t)direction;
  uint16_t input=direction>0 ? MMX_CPU_RIGHT : direction<0 ? MMX_CPU_LEFT : 0;
  if (cpu_jump_cooldown_frames) --cpu_jump_cooldown_frames;
  /* This is the highest-priority locomotion state. A landing exits
   * immediately, even when only one wall jump has been performed. It
   * is checked BEFORE any recovery B press or phase change. */
  if (grounded && cpu_wall_recovery_phase!=MMX_CPU_WALL_IDLE) {
    if (getenv("MMX_CPU_TRACE"))
      fprintf(stderr,"[cpu-wall] landed after %u/%u kick(s) start_y=%u best_y=%u landed_y=%d gain=%d\n",
              (unsigned)cpu_wall_recovery_jumps,
              cpu_tall_wall_climb ? 8u : 2u,
              (unsigned)cpu_tall_wall_start_y,(unsigned)cpu_tall_wall_best_y,
              y,(int)cpu_tall_wall_start_y-(int)cpu_tall_wall_best_y);
    cpu_wall_recovery_phase=cpu_wall_recovery_jumps=cpu_wall_recovery_ticks=0;
    cpu_wall_recovery_left_slide=cpu_wall_buffer_pending=cpu_wall_buffer_attempted=false;
    cpu_wall_jump_hold_frames=0;
    cpu_tall_wall_climb=false;cpu_tall_wall_start_y=cpu_tall_wall_best_y=0;
    cpu_tall_wall_trace_ticks=0;
    cpu_tall_wall_kick_x=cpu_tall_wall_kick_y=0;
    cpu_wall_direction=0;
    cpu_wall_escape_reported=false;
    cpu_jump_cooldown_frames=0;
  }
  /* Wall contact starts a PREPARATION phase even before the native
   * animation switches to slide. The jump itself is only pressed on a
   * genuine slide frame: this avoids using a recovery kick too early. */
  if (!grounded && (wall_slide || touching_wall ||
                    cpu_wall_recovery_phase!=MMX_CPU_WALL_IDLE)) {
    if (cpu_wall_recovery_phase==MMX_CPU_WALL_IDLE) {
      cpu_wall_direction=(int8_t)(
          touching_wall ? touching_wall :
          direction ? direction : (follower->body[0x69]&64 ? 1 : -1));
      cpu_wall_recovery_phase=MMX_CPU_WALL_SEEK;
      cpu_wall_recovery_jumps=cpu_wall_recovery_ticks=0;
      cpu_wall_escape_reported=false;
      cpu_wall_recovery_left_slide=cpu_wall_buffer_pending=cpu_wall_buffer_attempted=false;
      cpu_wall_jump_hold_frames=0;
      cpu_jump_hold_frames=cpu_cliff_dash_frames=0;
      cpu_jump_seen_airborne=false;
      /* X is a route GOAL only when settled on higher solid ground.
       * Use the longer climb budget while that goal is at least 96px up;
       * do not extend recovery merely because the human jumps in place. */
      bool pit_wall_arrival=cpu_pit_wall_ticks &&
          cpu_pit_wall_was_airborne &&
          cpu_wall_direction==cpu_pit_wall_direction;
      cpu_tall_wall_climb=pit_wall_arrival ||
          ((leader->body[0x2b]&4)!=0 &&
           ((int)word(leader->body+8) <= y-96));
      if (pit_wall_arrival && getenv("MMX_CPU_TRACE"))
        fprintf(stderr,
            "[cpu-pit] opposite wall reached x=%d y=%d dir=%d\n",
            x,y,(int)cpu_wall_direction);
      cpu_pit_wall_ticks=0;cpu_pit_wall_direction=0;
      cpu_pit_wall_was_airborne=false;
      cpu_tall_wall_start_y=cpu_tall_wall_best_y=(uint16_t)y;
      cpu_tall_wall_trace_ticks=0;
      cpu_tall_wall_kick_x=(uint16_t)x;
      cpu_tall_wall_kick_y=(uint16_t)y;
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,"[cpu-wall] contact dir=%d action=%u vy=%d slide=%d tall=%d leader_above=%d\n",
                (int)cpu_wall_direction,(unsigned)follower->body[2],
                (int16_t)word(follower->body+0x1c),(int)wall_slide,
                (int)cpu_tall_wall_climb,y-(int)word(leader->body+8));
    }
    if (cpu_tall_wall_climb && y<(int)cpu_tall_wall_best_y)
      cpu_tall_wall_best_y=(uint16_t)y;
    bool near=cpu_companion_wall_jump_near(ram,x,y,cpu_wall_direction);
    bool upper_lip=cpu_tall_wall_climb &&
        (leader->body[0x2b]&4) &&
        abs((int)word(leader->body+8)-y)<=48 &&
        cpu_companion_upper_lip(
            ram,x,y+16,cpu_wall_direction);

    uint16_t wall_input;

    if (upper_lip) {
      wall_input=cpu_wall_direction>0 ?
          MMX_CPU_RIGHT : MMX_CPU_LEFT;
      cpu_wall_jump_hold_frames=0;

      if (getenv("MMX_CPU_TRACE") &&
          snes_frame_counter%30==0)
        fprintf(stderr,
            "[cpu-lip] upper ledge transfer x=%d y=%d dir=%d\\n",
            x,y,(int)cpu_wall_direction);
    } else {
      wall_input=cpu_companion_wall_recovery(
          follower,wall_slide,near,descending,kick_apex);
    }
    /* Last-chance retreat: after a confirmed kick has already peaked
     * and Zero has dropped back to its starting height without finding
     * another wall slide, direct him to an ACTUALLY SUPPORTED lower shelf.
     * A Modern-mode Zero can add one native air dash for a distant
     * landing. X3 mode has a charged buster but no Modern air dash. */
    if (!upper_lip && cpu_wall_recovery_jumps && !wall_slide && !near && descending &&
        y>=(int)cpu_tall_wall_kick_y &&
        (follower->body[2]==6 || follower->body[2]==8)) {
      int escape_dist=0;
      int escape_dir=cpu_companion_wall_escape(
          ram,x,y,cpu_wall_direction,&escape_dist);
      if (escape_dir) {
        wall_input=(uint16_t)(escape_dir>0?MMX_CPU_RIGHT:MMX_CPU_LEFT);
        if (follower->character==MMX_COOP_ZERO &&
            follower->zero.modern.enabled &&
            !follower->zero.modern.dash_used &&
            !(follower->input&MMX_CPU_DASH) && escape_dist>=48)
          wall_input|=MMX_CPU_DASH;
        if (!cpu_wall_escape_reported && getenv("MMX_CPU_TRACE"))
          fprintf(stderr,"[cpu-safety] wall-kick retreat x=%d y=%d escape_dir=%d floor_dist=%d modern_airdash=%d\n",
                  x,y,escape_dir,escape_dist,
                  (int)((wall_input&MMX_CPU_DASH)!=0));
        cpu_wall_escape_reported=true;
      }
    }
    if (cpu_tall_wall_climb && getenv("MMX_CPU_TRACE") &&
        cpu_wall_recovery_jumps && cpu_tall_wall_trace_ticks<48) {
      unsigned tick=cpu_tall_wall_trace_ticks++;
      if (tick%3==0 || wall_slide) {
        fprintf(stderr,
            "[cpu-climb] t=%u x=%d y=%d kickx=%u kicky=%u offx=%d rise=%d action=%u vy=%d slide=%d wallL=%d wallR=%d input=%c%c hold=%u kicks=%u\n",
            tick,x,y,(unsigned)cpu_tall_wall_kick_x,
            (unsigned)cpu_tall_wall_kick_y,
            x-(int)cpu_tall_wall_kick_x,
            (int)cpu_tall_wall_kick_y-y,
            (unsigned)follower->body[2],
            (int16_t)word(follower->body+0x1c),(int)wall_slide,
            (int)cpu_companion_wall_jump_near(ram,x,y,-1),
            (int)cpu_companion_wall_jump_near(ram,x,y,1),
            (wall_input&MMX_CPU_LEFT)?'L':(wall_input&MMX_CPU_RIGHT)?'R':'-',
            (wall_input&MMX_CPU_JUMP)?'B':'-',
            (unsigned)cpu_wall_jump_hold_frames,
            (unsigned)cpu_wall_recovery_jumps);
      }
    }
    return wall_input;
  }
  /* Preserve takeoff direction and hold B long enough for a useful ascent,
   * even if P1 changes direction while the CPU is already jumping. */
  if (cpu_jump_hold_frames) {
    if (!grounded) cpu_jump_seen_airborne=true;
    if (grounded && cpu_jump_seen_airborne) {
      /* Native jump has already landed: stop holding B immediately. */
      cpu_jump_hold_frames=cpu_cliff_dash_frames=0;
      cpu_jump_seen_airborne=false;
    } else {
      --cpu_jump_hold_frames;
      uint16_t jump=(cpu_jump_direction>0 ? MMX_CPU_RIGHT : MMX_CPU_LEFT)|MMX_CPU_JUMP;
      if (cpu_cliff_dash_frames) {
        jump|=MMX_CPU_DASH;
        --cpu_cliff_dash_frames;
      }
      return jump;
    }
  }
  if (cpu_cliff_dash_frames && !grounded && !wall_slide) {
    /* Preserve momentum briefly toward the raised wall, even when B is
     * released to restore a future wall-jump press edge. */
    input|=MMX_CPU_DASH;
    --cpu_cliff_dash_frames;
  } else if (grounded) cpu_cliff_dash_frames=0;
  /* Releasing dash must not release the route's horizontal steering.
   * The confirmed pit-wall direction is maintained until actual wall
   * contact, landing, or a bounded route timeout. */
  if (cpu_pit_wall_ticks && !grounded && cpu_pit_wall_direction) {
    input&=(uint16_t)~(MMX_CPU_LEFT|MMX_CPU_RIGHT);
    input|=cpu_pit_wall_direction>0 ? MMX_CPU_RIGHT : MMX_CPU_LEFT;
    if (getenv("MMX_CPU_TRACE") && !cpu_cliff_dash_frames &&
        snes_frame_counter%24==0)
      fprintf(stderr,
          "[cpu-pit] forward glide x=%d y=%d dir=%d action=%u\n",
          x,y,(int)cpu_pit_wall_direction,(unsigned)follower->body[2]);
  }
  /* Shoot nearby enemies outside jump takeoff/kick phases. */
  int facing=direction ? direction : (follower->body[0x69]&64 ? 1 : -1);
  if (ram[0xb9c]%12==0 && cpu_companion_enemy_ahead(ram,x,y,facing))
    input|=MMX_CPU_FIRE;

  if ((!direction || !grounded) && getenv("MMX_CPU_TRACE")) {
    static int last_idle_trace=-1000;
    if (snes_frame_counter-last_idle_trace>=120) {
      fprintf(stderr,
          "[cpu-state] x=%d y=%d dx=%d dir=%d high=%d climb_side=%d action=%u grounded=%d wall=%d recovery=%u hold=%u\n",
          x,y,dx,direction,(int)elevated_goal,(int)climb_from_below,
          (unsigned)follower->body[2],(int)grounded,touching_wall,
          (unsigned)cpu_wall_recovery_phase,(unsigned)cpu_jump_hold_frames);
      last_idle_trace=snes_frame_counter;
    }
  }
  /* Pit approaches use the standard held native jump immediately below.
   * Do not override that hold with a walk-off-only return on the lip. */
  if (!grounded || !direction) return input;
  int feet=y+16;
  bool edge=cpu_companion_ground_missing(ram,x,feet,direction);
  bool obstacle=cpu_companion_obstacle_ahead(ram,x,y,direction);
  bool blocked=cpu_stall_ticks>=10;
  /* P1's Y position and B button must NOT influence CPU jump decisions.
   * These checks use the follower's own ground and obstruction state only.
   * If the ground ends ahead and there is no reachable landing, wait.
   * Do this before obstruction/stall reactions to avoid walking into void. */
  int landing_distance=0, landing_rise=0;
  bool short_landing=edge && cpu_companion_gap_reachable(
      ram,x,feet,direction,&landing_distance,&landing_rise);
  /* When X is already standing on a lower tier, a verified downward
   * landing may be more useful than a blind horizontal jump or WAIT. */
  int drop_distance=0,drop_depth=0;
  bool lower_goal=edge && (leader->body[0x2b]&4)!=0 &&
      dx*direction>40 &&
      (int)word(leader->body+8)-y>=20 &&
      (int)word(leader->body+8)-y<=112;
  bool lower_landing=lower_goal && cpu_companion_lower_landing(
      ram,x,feet,direction,(int)word(leader->body+8)+16,
      &drop_distance,&drop_depth);
  bool safe_drop=lower_landing && drop_distance<=72;
  /* Farther lower ledges call for a normal jump (and perhaps dash),
   * so the CPU has the lateral flight time to reach the floor. */
  if (lower_landing && !short_landing && !safe_drop) {
    short_landing=true;
    landing_distance=drop_distance;
    landing_rise=-drop_depth;
  }
  int wall_distance=0;
  /* Look for a tall climbable wall while STILL on safe ground, not only
   * after the cliff sensor fires. Once within ~48px, proactively jump at
   * the wall. At a lip, allow a longer native dash-jump approach if the
   * wall's top has a valid, walkable landing. */
  bool high_goal=elevated_goal && (dx*direction>40 || climb_from_below);
  /* Only scan high columns when the CPU is near a gap, touching a
   * possible obstacle or has a higher destination ahead. A full-height
   * scan every ordinary walking frame needlessly burns CPU. */
  bool raised_wall=(edge || obstacle || high_goal) && !short_landing &&
      cpu_companion_raised_wall(ram,x,feet,direction,&wall_distance);
  bool goal_wall=false;
  if (!raised_wall && high_goal && !short_landing) {
    int face=climb_from_below ? climb_face :
             cpu_companion_wall_face(ram,x,feet,direction);
    /* High destination plus a real solid wall is an actionable route even
     * when horizontal follow distance is zero. The wall must be within
     * sensor range, never inferred solely from X's height. */
    goal_wall=face && (climb_from_below || dx*direction>face);
    if (goal_wall) wall_distance=face;
  }
  bool climb_takeoff=(raised_wall || goal_wall) &&
                     (edge || wall_distance<=48);
  /* Opt-in field diagnostics make real stage geometry inspectable without
   * assuming a screenshot reveals the actual SNES collision classes.
   * This never affects controller input or deterministic guest state. */
  if ((edge || climb_takeoff || blocked || obstacle) && getenv("MMX_CPU_TRACE")) {
    static int last_log=-1000;
    if (snes_frame_counter-last_log>=90) {
      fprintf(stderr,"[cpu-nav] edge=%d x=%d feet=%d dir=%d p1dx=%d p1dy=%d landing=%d dist=%d rise=%d drop=%d drop_dist=%d drop_depth=%d raised=%d goalwall=%d wall_dist=%d grounded=%d cooldown=%u\n",
              (int)edge,x,feet,direction,dx,
              (int)word(leader->body+8)-y,(int)short_landing,
              landing_distance,landing_rise,(int)safe_drop,drop_distance,
              drop_depth,(int)raised_wall,(int)goal_wall,wall_distance,
              (int)grounded,(unsigned)cpu_jump_cooldown_frames);
      last_log=snes_frame_counter;
    }
  }
  /* Optional first-pass pit route, separate from the normal floor/jump
   * planner. All other unverified pits retain their WAIT behavior. */
  int pit_wall_distance=0,pit_wall_depth=0;
  bool pit_enabled=getenv("MMX_CPU_PIT_WALL")!=NULL;
  bool pit_wall_candidate=pit_enabled && edge &&
      !short_landing && !safe_drop &&
      cpu_companion_opposite_pit_wall(
          ram,x,feet,direction,&pit_wall_distance,&pit_wall_depth);
  bool pit_can_dash=follower->character==MMX_COOP_ZERO ||
      (ram[0x1f99]&8)!=0;
  /* Without native dash ability use a shorter, conservative approach.
   * A wall alone is not proof that every character can fly far enough. */
  bool pit_wall_route=pit_wall_candidate &&
      !raised_wall && !goal_wall &&
      pit_wall_distance<=(pit_can_dash?152:104) &&
      (leader->body[0x2b]&4)!=0 &&
      dx*direction>pit_wall_distance+24;
  if (pit_enabled && edge && getenv("MMX_CPU_TRACE")) {
    static int last_pit_scan=-1000;
    if (snes_frame_counter-last_pit_scan>=90) {
      fprintf(stderr,
          "[cpu-pit-scan] x=%d feet=%d dir=%d candidate=%d dist=%d depth=%d dash=%d leader_ground=%d leader_dx=%d route=%d\n",
          x,feet,direction,(int)pit_wall_candidate,
          pit_wall_distance,pit_wall_depth,(int)pit_can_dash,
          (int)((leader->body[0x2b]&4)!=0),dx,(int)pit_wall_route);
      last_pit_scan=snes_frame_counter;
    }
  }
  if (pit_wall_route) {
    bool headroom=true;
    for (int offset=-6;offset<=6;offset+=6)
      if (MmxWeaponsTerrainSolid(ram,x+offset,y-25,false,NULL) ||
          MmxWeaponsTerrainSolid(ram,x+offset,y-35,false,NULL))
        headroom=false;
    if (headroom && !cpu_jump_cooldown_frames) {
      cpu_pit_wall_ticks=pit_wall_distance>=96 ? 100 : 72;
      cpu_pit_wall_direction=(int8_t)direction;
      cpu_pit_wall_was_airborne=false;
      /* Standard crossing holds native B for a high dash-jump.
       * Optional drop test taps B only on takeoff for a low arc:
       * descend toward the far wall, not over the upper platform. */
      const char *pit_style=getenv("MMX_CPU_PIT_STYLE");
      bool low_arc=pit_style && strcmp(pit_style,"drop")==0;
      cpu_jump_hold_frames=low_arc ? 0 : 20;
      cpu_jump_direction=(int8_t)direction;
      cpu_jump_seen_airborne=false;
      cpu_jump_cooldown_frames=36;
      cpu_cliff_dash_frames=pit_can_dash && pit_wall_distance>=88 ? 20 : 0;
      uint16_t takeoff=(input&(uint16_t)~MMX_CPU_FIRE)|MMX_CPU_JUMP;
      if (cpu_cliff_dash_frames) {
        takeoff|=MMX_CPU_DASH;
        --cpu_cliff_dash_frames;
      }
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,
            "[cpu-pit] %s toward opposite wall x=%d feet=%d dir=%d wall_dist=%d wall_depth=%d dash=%d\n",
            low_arc?"low-arc drop":"jump",
            x,feet,direction,pit_wall_distance,pit_wall_depth,
            (int)((takeoff&MMX_CPU_DASH)!=0));
      return takeoff;
    }
  }
  /* Terrain sensing takes priority over ordinary follow: a missing floor
   * triggers a deliberate jump toward an identified landing/wall, never
   * an extra walking frame into unknown void. A wall below an elevated
   * destination initiates a climb without waiting for X's jump timing. */
  MmxCpuMoveDecision move=MmxCoopCpuChooseMove(
      edge,short_landing,(raised_wall || goal_wall),climb_takeoff,
      safe_drop,obstacle || blocked);
  if (move==MMX_CPU_MOVE_WAIT)
    return input&(uint16_t)~(MMX_CPU_LEFT|MMX_CPU_RIGHT|MMX_CPU_JUMP|MMX_CPU_DASH);
  /* A short safe DROP is deliberately NOT a jump: keep moving toward
   * the floor X is standing on, and let the native fall/land logic run. */
  if (move==MMX_CPU_MOVE_WALK || move==MMX_CPU_MOVE_DROP) return input;

  bool headroom=true;
  for (int offset=-6;offset<=6;offset+=6) {
    if (MmxWeaponsTerrainSolid(ram,x+offset,y-25,false,NULL) ||
        MmxWeaponsTerrainSolid(ram,x+offset,y-35,false,NULL)) {
      headroom=false;break;
    }
  }
  if (!headroom || cpu_jump_cooldown_frames) {
    if (edge) input&=(uint16_t)~(MMX_CPU_LEFT|MMX_CPU_RIGHT);
    return input;
  }
  /* Hold native jump for 21 input frames in total (this frame plus
   * 20 polls), allowing the game's variable-height jump to finish its
   * ascent. Release earlier if native ground contact returns or a real
   * airborne wall contact takes priority. */
  cpu_jump_hold_frames=20;
  cpu_jump_seen_airborne=false;
  if (getenv("MMX_CPU_TRACE"))
    fprintf(stderr,"[cpu-jump] takeoff x=%d y=%d mode=%d hold=21 landing_dist=%d rise=%d\n",
            x,y,(int)move,landing_distance,landing_rise);
  /* A climb attempt can repeat shortly after a failed takeoff; ordinary
   * short-hop/ledge navigation retains the longer anti-spam cooldown. */
  cpu_jump_cooldown_frames=(move==MMX_CPU_MOVE_CLIMB) ? 24 : 48;
  cpu_jump_direction=(int8_t)direction;
  /* An elevated wall >~72px away can need more horizontal range than a
   * normal hop. Ask Zero to dash-jump as part of his own route choice;
   * the game's native equipment/movement code still controls the result. */
  /* Long safe crossings need dash momentum too, not only climbs.
   * A 100+px landing previously triggered an ordinary short ground jump
   * and often failed even when the route scan found a sound landing. */
  cpu_cliff_dash_frames=((follower->character==MMX_COOP_ZERO ||
      (ram[0x1f99]&8)) &&
      ((move==MMX_CPU_MOVE_CLIMB && wall_distance>72) ||
       (edge && short_landing && landing_distance>80))) ? 20 : 0;
  uint16_t takeoff=(input&(uint16_t)~MMX_CPU_FIRE)|MMX_CPU_JUMP;
  if (cpu_cliff_dash_frames) {
    takeoff|=MMX_CPU_DASH;
    --cpu_cliff_dash_frames;
  }
  return takeoff;
}
/* Real DualSense L2 is an analog trigger with no SNES pad bit; the desktop
 * host samples it separately. Tests can drive this host-only signal too. */
void MmxCoopSetSwitchTrigger(bool held) { cpu_l2_trigger_held=held; }
void MmxCoopPoll(uint16_t p1, uint16_t p2) {
  if (!enabled) return;
  uint16_t inputs[2] = {p1 & 4095, p2 & 4095};
  /* Offline CPU mode: a new press of the physical L2 analog trigger
   * switches the controlled character. The L2 signal is NOT a SNES button,
   * so Start/Select and native L/R weapon changes still work normally.
   * One press swaps once; holding L2 never oscillates the active seat. */
  bool offline_cpu = cpu_companion
#if SNESRECOMP_NET
      && !snes_netplay_active()
#endif
      ;
  if (offline_cpu) {
    /* Physical P1 must NEVER remain bound to the fallen seat while the
     * other is alive. The L2 swap used to require both alive and ignored
     * the lost seat until the next level, leaving X uncontrollable if the
     * human had swapped to Zero before Zero died. Follow the live player
     * on this very input poll, before AI input routing or pressed edges. */
    unsigned survivor=cpu_human_seat^1;
    const MmxCoopPlayer *controlled=&state.players[cpu_human_seat];
    const MmxCoopPlayer *remaining=&state.players[survivor];
    if (state.initialized &&
        (controlled->status!=MMX_COOP_ALIVE ||
         !(controlled->body[0x27]&127)) &&
        remaining->status==MMX_COOP_ALIVE &&
        (remaining->body[0x27]&127)) {
      cpu_human_seat=(uint8_t)survivor;
      cpu_companion_reset_motion();
      if (getenv("MMX_CPU_TRACE"))
        fprintf(stderr,
            "[cpu-switch] controlled seat fallen; DualSense handed to surviving %s (seat %u)\n",
            state.players[survivor].character==MMX_COOP_ZERO?"Zero":"X",
            survivor);
    }
    bool held=cpu_l2_trigger_held;
    if (held && !cpu_swap_trigger_down && state.initialized &&
        !state.menu_owner && !state.scene_owner && !state.stage_pending &&
        state.players[0].status==MMX_COOP_ALIVE &&
        state.players[1].status==MMX_COOP_ALIVE &&
        (state.players[0].body[0x27]&127) &&
        (state.players[1].body[0x27]&127)) {
      cpu_human_seat^=1;
      cpu_companion_reset_motion();
    }
    cpu_swap_trigger_down=held;
    inputs[cpu_human_seat]=p1&4095;
    unsigned ai=cpu_human_seat^1;
    inputs[ai]=cpu_companion_input(g_ram,cpu_human_seat);
    if (state.initialized && !state.menu_owner && !state.scene_owner &&
        !state.stage_pending)
      inputs[ai]=cpu_companion_zero_combat(g_ram,&state.players[ai],
                                           &state.players[cpu_human_seat],
                                           inputs[ai]);
  } else {
    cpu_swap_trigger_down=0;
  }
  for (unsigned i = 0; i < 2; ++i) {
    state.players[i].pressed = inputs[i] & ~state.players[i].input;
    state.players[i].input = inputs[i];
  }

  if (offline_cpu && state.initialized &&
      getenv("MMX_HUMAN_TRACE")) {
    static int previous_action=-1,previous_seat=-1;
    const MmxCoopPlayer *h=&state.players[cpu_human_seat];
    int action=h->body[2];

    bool button_edge=
        (h->pressed&(MMX_CPU_JUMP|MMX_CPU_DASH))!=0;
    bool climbing=action==6 || action==8 ||
                  action==0x10 || action==0x12 ||
                  action==0x14;

    if (action!=previous_action ||
        cpu_human_seat!=previous_seat ||
        button_edge ||
        (climbing && snes_frame_counter%4==0)) {
      fprintf(stderr,
          "[human-move] frame=%d player=%s x=%u y=%u "
          "action=%d ground=%d pad=%03x pressed=%03x\\n",
          snes_frame_counter,
          h->character==MMX_COOP_ZERO?"Zero":"X",
          word(h->body+5),word(h->body+8),
          action,(h->body[0x2b]&4)!=0,
          (unsigned)h->input,(unsigned)h->pressed);
    }

    previous_action=action;
    previous_seat=cpu_human_seat;
  }
}
void MmxCoopApplyInput(uint8_t *r) {
  if (!enabled || !state.initialized || !r) return;
  unsigned input = state.players[state.current].input, native = 0;
  for (unsigned bit = 0; bit < 12; ++bit) if (input & (1u << bit)) native |= 0x8000u >> bit;
  /* $00:E543..E5F6: preserve X1's configurable button masks at $7E:FFC0..5.
   * Both seats use the game's action layout, after independent host bindings. */
  unsigned buttons = ((native & 255) >> 2 & 0x3c) | (native >> 8 & 0xc0) | (native >> 12 & 3);
  static const uint16_t action_bits[6] = {0x4000, 0x8000, 0x0080, 0x0020, 0x0010, 0x1000};
  unsigned actions = native & 0x0f00;
  for (unsigned i = 0; i < 6; ++i) {
    unsigned mask = r[0xffc0 + i];
    if (mask && (buttons & mask) == mask) actions |= action_bits[i];
  }
  /* Native input mapping writes port 1 into the projected body. Seat 2 takes
   * its previous actions from its own preceding frame snapshot. */
  unsigned previous = word(state.players[state.current].body+0x36);
  r[0xbe0] = (uint8_t)previous; r[0xbe1] = (uint8_t)(previous >> 8);
  r[0xbde] = (uint8_t)actions; r[0xbdf] = (uint8_t)(actions >> 8);
  r[0xbe2] = (uint8_t)(actions & ~previous); r[0xbe3] = (uint8_t)((actions & ~previous) >> 8);
}
static void place_other(uint8_t *r,uint16_t x,uint16_t y,bool preserve) {
  MmxCoopCapture(r);
  MmxCoopPlayer *p = &state.players[state.current^1];
  const MmxCoopPlayer *source=&state.players[state.current];
  unsigned hp=preserve ? p->body[0x27]&127 : r[0x1f9a];
  unsigned weapon=preserve ? p->body[0x33] : 0;
  /* Use native player setup fields, but never inherit P1's hurt, charge,
   * movement or weapon counters. Personal inventory survives withdrawal. */
  memcpy(p->body, source->body, sizeof(p->body));
  /* $0C38..$0C97 are the three armor-part objects. Only the same character
   * can share them: X returning beside a world-driving Zero (who opened the
   * boss door) took Zero's slots and lost his helmet/arm/boot sprites. */
  uint8_t armor[0x60];memcpy(armor,p->character==source->character ? source->auxiliaries :
      p->auxiliaries,sizeof(armor));
  memset(p->auxiliaries,0,sizeof(p->auxiliaries));memset(p->shots,0,sizeof(p->shots));
  memcpy(p->auxiliaries,armor,sizeof(armor));
  memset(&p->combat,0,sizeof(p->combat));memset(&p->zero,0,sizeof(p->zero));
  p->zero.active_x=p->character==MMX_COOP_X;
  memset(p->body+0x28,0,sizeof(p->body)-0x28);
  memset(p->body+0x1a,0,6); /* velocities, ending before the collision pointer */
  p->body[2]=p->body[3]=0;p->body[14]=1;
  p->body[0x2b]=4; /* solid ground; native idle initializer runs on resume */
  /* Constants from $81:819F..825A. The idle initializer does not rebuild
   * these action-table pointers, so clearing them disables firing entirely. */
  putword(p->body+0x31,0xa597);putword(p->body+0x5f,0xfa80);
  p->body[0x2f]=8;p->body[0x66]=255;p->body[0x67]=3;
  p->body[0x69]=r[0x1f7f]?0:64;p->body[0x78]=4;
  p->body[4] = p->body[7] = 0;
  p->body[5] = (uint8_t)x; p->body[6] = (uint8_t)(x >> 8);
  p->body[8] = (uint8_t)y; p->body[9] = (uint8_t)(y >> 8);
  p->body[0x27] = (uint8_t)(hp|128);p->body[0x33]=(uint8_t)weapon;
  putword(p->body+0x22,x);putword(p->body+0x24,y);
  p->shot_command=0;p->hud_state=2;
  p->status = MMX_COOP_ALIVE;
}
bool MmxCoopPlacePartner(uint8_t *r,uint16_t x,uint16_t y) {
  if (!enabled || !state.initialized || !r ||
      state.players[state.current].status!=MMX_COOP_ALIVE ||
      state.players[state.current^1].status!=MMX_COOP_ABSENT) return false;
  place_other(r,x,y,state.enrolled!=0);state.enrolled=1;return true;
}

bool MmxCoopFindLanding(const uint8_t *r,uint16_t *out_x,uint16_t *out_y) {
  if (!r || !out_x || !out_y) return false;
  int px=word(r+0xbad),py=word(r+0xbb0),camera_x=word(r+0x1e4d),camera_y=word(r+0x1e50);
  int height=state.players[state.current^1].character==MMX_COOP_ZERO ? 44 : 36;
  const int gaps[]={32,-32,48,-48,64,-64,16,-16,0};
  /* A scene may finish on a narrow ledge just inside a door. The players
   * can overlap, so returning beside/on the driver is preferable to being
   * stranded across the door. Voluntary joins keep the wider clear space. */
  unsigned gap_count=state.scene_owner ? 9 : 6;
  for (unsigned i=0;i<gap_count;++i) {
    int x=px+gaps[i];if (x-12<camera_x || x+12>=camera_x+256) continue;
    for (int offset=-16;offset<=24;++offset) {
      int floor=py+16+offset,surface=0;
      unsigned type=MmxWeaponsTerrainClass(r,x,floor);
      /* $84:961C: ordinary ground, slopes and solid conveyors ($37/$38)
       * support a landing. Mammoth's arena can offer only conveyor tiles.
       * Spikes and transient/one-way surfaces remain ineligible. */
      if (!(type==0x13 || (type>=1 && type<=12) ||
            (type>=0x34 && type<=0x38) || (type>=0x3b && type<=0x3d)) ||
          !MmxWeaponsTerrainSolid(r,x,floor,true,&surface) || surface!=floor ||
          floor-height<camera_y || floor>=camera_y+224) continue;
      bool clear=true;
      /* A clear destination across a shut door is not a usable return.
       * Check the corridor between the actors as well as the landing box. */
      for(int cx=px;cx!=x && clear;cx+=x>px?1:-1)
        if(MmxWeaponsTerrainSolid(r,cx,py-8,true,NULL)) clear=false;
      for (int y=floor-height;y<floor && clear;++y) for (int dx=-10;dx<=10;dx+=5) {
        unsigned t=MmxWeaponsTerrainClass(r,x+dx,y);
        if ((t>=0x33 && t!=0x39 && t!=0x3a) ||
            MmxWeaponsTerrainSolid(r,x+dx,y,true,NULL)) {clear=false;break;}
      }
      /* Source enemies and ride armor retain their native space; don't
       * drop a newly joined player directly onto an existing actor. */
      for (unsigned d=0xe18;d<0x1228 && clear;d+=(d==0xe18?0x50:64)) if (r[d]) {
        int dx=(int)word(r+d+5)-x,dy=(int)word(r+d+8)-(floor-16);
        if (dx>-28 && dx<28 && dy>-40 && dy<32) clear=false;
      }
      if (clear) {*out_x=(uint16_t)x;*out_y=(uint16_t)(floor-16);return true;}
    }
  }
  return false;
}
/* Speedrun-assist fallback: only save the CPU companion from a real void
 * pit, never from enemy damage. Uses the co-op's tested collision-aware
 * landing search near the controlled character; retains HP and inventory.
 * Activated before native bottom-screen fatal contact, with throttling.
 * If no safe landing exists, don't fabricate a coordinate or suppress death. */
static void cpu_companion_rescue(uint8_t *r) {
  if (!cpu_traversal_active() || !state.initialized || !r ||
      state.menu_owner || state.scene_owner || state.stage_pending ||
      r[0xd1]!=2 || r[0xd2]!=4 || r[0xd3]!=4 ||
      r[0x1f0c] || r[0x1f23] || r[0x1f48]) return;
  if (cpu_rescue_cooldown) {--cpu_rescue_cooldown;return;}
  const unsigned target=cpu_human_seat^1;
  MmxCoopPlayer *f=&state.players[target], *h=&state.players[cpu_human_seat];
  if (f->status!=MMX_COOP_ALIVE || h->status!=MMX_COOP_ALIVE ||
      !(f->body[0x27]&127) || !(h->body[0x27]&127) ||
      f->body[2]==12 || h->body[2]==12) return;
  const int bottom=(int)word(r+0x1e5c)+224;
  /* Native shared-screen pit death begins around bottom+32. Rescue in the
   * final few pixels only; the old bottom-64 threshold teleported Zero
   * nearly 100 pixels too early, before he could complete wall-jumps. */
  if ((int)word(f->body+8)<bottom+24 || floor_below(r,f->body)) return;
  unsigned previous=state.current;
  if (!MmxCoopSelect(r,cpu_human_seat)) return;
  uint16_t x=0,y=0;
  bool safe=MmxCoopFindLanding(r,&x,&y);
  if (safe) {
    place_other(r,x,y,true);
    cpu_rescue_cooldown=90;
    cpu_companion_reset_motion();
  }
  MmxCoopSelect(r,previous);
}
static void clear_player_combat(uint8_t *r,unsigned seat) {
  MmxCoopSelect(r,seat);MmxWeaponsCancelShots(r);MmxZeroCancel(r);
  if(r[0xc2f]&64) sound(r,0x17);
  memset(r+0xbff,0,5);memset(r+0xc98,0,0x180);r[0xc2f]&=(uint8_t)~64;
  MmxCoopSelect(r,state.anchor);
}
/* Both scene transport and voluntary join use the source teleport timing. */
static bool teleport_tick(uint8_t *r,MmxCoopPlayer *p) {
  MmxZeroState *z=&p->zero;
  switch(z->swap_phase) {
    case 1: if(++z->swap_tick==7) {z->swap_phase=2;z->swap_tick=0;} break;
    case 2: {
      int fixed=z->swap_y*256+z->swap_fraction-0x0aa6;
      z->swap_y=(int16_t)((fixed-255)/256);z->swap_fraction=(uint8_t)(fixed-z->swap_y*256);
      if((int)word(p->body+8)-word(r+0x1e50)+z->swap_y < -40) {
        z->swap_phase=z->swap_tick=z->swap_fraction=0;z->swap_y=0;
      }
      break;
    }
    case 4: z->swap_y+=8;if(z->swap_y>=0) {z->swap_y=0;z->swap_phase=5;z->swap_tick=0;} break;
    case 5: if(++z->swap_tick==7) z->swap_phase=0;break;
    default: z->swap_phase=0;break;
  }
  return !z->swap_phase;
}
/* $0C16 names the action a script forces on the world actor ($84:9FEB..A07D):
 * door walks ($18), boss intros ($3A/$1E), and so on; $84:A003 clears it.
 * The script then belongs to that body until it releases it. */
static bool world_scripted(const uint8_t *r) {
  const uint8_t *world=state.anchor==state.current ? r+0xba8 : state.players[state.anchor].body;
  return world[0x6e]!=0;
}
/* A boss or miniboss fight: $1F0E holds the boss whose health meter the HUD
 * draws ($80:DA0E; each boss stores its own slot there), and the encounter
 * classes cover intros before the meter appears and minibosses, which have
 * none. Velguarder ($26) and Vile ($67/$69) are the boss bodies themselves. */
static bool boss_fight(const uint8_t *r) {
  if(word(r+0x1f0e)) return true;
  for(unsigned d=0xe68;d<0x1228;d+=64) {
    unsigned c=r[d+10];
    if(r[d] && (MmxWidePolicy_IsBossEncounter((uint8_t)c) || c==0x26 || c==0x67 ||
        c==0x69 || c==0x01 /* Sting Chameleon's miniboss, $83:AE81 */)) return true;
  }
  return false;
}
/* Solid terrain somewhere between the body and the level's lowest camera
 * position: falling off the bottom of the screen there is not a pit. */
static bool floor_below(const uint8_t *r,const uint8_t *b) {
  int x=(int)word(b+5),bottom=(int)word(r+0x1e5c)+224+32;
  for(int y=(int)word(b+8);y<=bottom;y+=8)
    if(MmxWeaponsTerrainSolid(r,x,y,true,NULL)) return true;
  return false;
}
static bool capsule_slot(unsigned d) {
  return d>=0xe68 && d<0x1228 && !((d-0xe68)%64) && g_ram[d] && g_ram[d+10]==0x4d;
}
/* Sigma 1's armored Vile owns the playable body from the stun/grab through
 * NPC Zero's rescue. Armored Vile's .1=$04 starts his destruction; the
 * unarmored $69 intro and body lock then keep the existing scene active. */
static bool vile_capture(const uint8_t *r) {
  if(r[0x1f7a]!=9) return false;
  for(unsigned d=0xe68;d<0x1228;d+=64)
    if(r[d] && r[d+10]==0x67 && r[d+1]==2 && r[d+2]==0x16) return true;
  return false;
}
static bool vile_script_object(unsigned d) {
  if(!g_ram[d]) return false;
  if(g_ram[0x1f7a]==0) {
    if(d>=0xe68 && d<0x1228 && !((d-0xe68)%64)) {
      unsigned c=g_ram[d+10];
      /* The Highway ship loads Vile's music before spawning him. Replaying
       * this controller for P2 restores WRAM's old audio acknowledgement,
       * but the SPC has already accepted the upload: the real pass then
       * waits forever for an acknowledgement that can no longer arrive.
       * Vile and story Zero likewise run their scripts only once. */
      return c==0x1a || c==0x32 || c==0x33;
    }
    return false;
  }
  if(g_ram[0x1f7a]!=9) return false;
  if(d>=0xe68 && d<0x1228 && !((d-0xe68)%64)) {
    unsigned c=g_ram[d+10];
    return c==0x66 || c==0x64 || (c==0x67 && g_ram[d+1]<4) ||
        (c==0x69 && g_ram[d+1]<4);
  }
  if(d>=0x1428 && d<0x1628 && !((d-0x1428)%64) && g_ram[d+10]==0x16) {
    for(unsigned v=0xe68;v<0x1228;v+=64)
      if(g_ram[v] && g_ram[v+10]==0x67 && g_ram[v+1]<4) return true;
  }
  return false;
}
static void begin_scene(uint8_t *r) {
  unsigned other=state.anchor^1;
  if(state.scene_owner || state.players[other].status!=MMX_COOP_ALIVE ||
      !(state.players[other].body[0x27]&127)) return;
  MmxCoopSelect(r,other);MmxZeroCancel(r);MmxWeaponsCancelShots(r);
  memset(r+0xc98,0,0x180);MmxCoopSelect(r,state.anchor);
  MmxZeroState *z=&state.players[other].zero;
  z->swap_phase=1;z->swap_tick=z->swap_fraction=0;z->swap_y=0;
  state.scene_owner=(uint8_t)(state.anchor+1);state.scene_phase=1;
  TRACE(SCENE,0,1,state.scene_owner,NULL);
  sound(r,0x0f);
}
static bool scene_tick(uint8_t *r) {
  /* Capsule acquisition deliberately clears the body lock ($87:CD20).
   * $1F48 stays set through the subsequent recorded-input demonstration,
   * until $87:CE06. $1F3B clears earlier, before that demonstration starts. */
  if(!state.scene_owner && state.players[state.anchor^1].status==MMX_COOP_ALIVE &&
      (r[0xbcf]&127) && r[0xbaa]!=12 &&
      (vile_capture(r) || r[0x1f0c] || r[0x1f23] || r[0x1f48] || (r[0xc16] && (r[0x1f31] || r[0x1f3b])))) begin_scene(r);
  /* Unified view only: a player the shared camera leaves below the screen
   * is beamed out, and returns beside the other once there is a landing.
   * Over a real pit (no floor down to the level's lowest camera position)
   * he keeps falling into the native death zone. The partner is also
   * beamed as soon as an unentered Dr. Light capsule ($4D) exists while he
   * is away: its camera lock is about to leave him behind. */
  if(!state.scene_owner && !cpu_traversal_active() && !MmxCoopViewsOnline() &&
      state.players[0].status==MMX_COOP_ALIVE &&
      state.players[1].status==MMX_COOP_ALIVE && (state.players[0].body[0x27]&127) &&
      (state.players[1].body[0x27]&127)) {
    /* MmxCoopFrameTick has just captured the current body: both stored
     * copies are this frame's. */
    bool low[2];
    for(unsigned seat=0;seat<2;++seat) {
      const uint8_t *b=state.players[seat].body;
      /* A scripted door scroll moves the lower bound up past a partner
       * who is still climbing (Sigma 4's gate shaft): not a pit either. */
      low[seat]=(int)word(b+8)-32>=(int)word(r+0x1e50)+224 &&
          (floor_below(r,b) || (seat!=state.anchor && world_scripted(r)));
    }
    const uint8_t *b=state.players[state.anchor^1].body,*a=state.players[state.anchor].body;
    int dx=(int)word(b+5)-(int)word(a+5),dy=(int)word(b+8)-(int)word(a+8);
    bool away=dx<-96 || dx>96 || dy<-64 || dy>64,capsule=false;
    /* Only before it is entered: states 01 00 00 .. 01 02 02. */
    for(unsigned d=0xe68;d<0x1228 && away;d+=64)
      if(capsule_slot(d) && r[d+1]<=2 && r[d+2]<=2) capsule=true;
    /* A capsule belongs to X, not to whoever drives the world: X near an
     * unentered capsule takes over the world, so its native script runs on
     * him; the capsule rule below then beams Zero over. */
    unsigned xs=state.players[0].character==MMX_COOP_X ? 0 : 1;
    if(state.players[xs].character==MMX_COOP_X && xs!=state.anchor) {
      const uint8_t *xb=state.players[xs].body;
      for(unsigned d=0xe68;d<0x1228;d+=64)
        if(capsule_slot(d) && r[d+1]<=2 && r[d+2]<=2) {
          int cx=(int)word(r+d+5)-(int)word(xb+5),cy=(int)word(r+d+8)-(int)word(xb+8);
          if(cx>-96 && cx<96 && cy>-96 && cy<96) {
            state.anchor=(uint8_t)xs;MmxCoopSelect(r,xs);
            b=state.players[state.anchor^1].body;a=state.players[state.anchor].body;
            dx=(int)word(b+5)-(int)word(a+5);dy=(int)word(b+8)-(int)word(a+8);
            away=dx<-96 || dx>96 || dy<-64 || dy>64;capsule=away;
            break;
          }
        }
    }
    if(low[state.anchor] && !low[state.anchor^1]) {
      /* The other player takes over the world before the low one leaves. */
      state.anchor^=1;MmxCoopSelect(r,state.anchor);begin_scene(r);
    } else if(low[state.anchor^1] || capsule) begin_scene(r);
  }
  if(!state.scene_owner) return false;
  MmxCoopPlayer *p=&state.players[state.anchor^1];
  if(p->status!=MMX_COOP_ALIVE) {state.scene_owner=state.scene_phase=0;return false;}
  if(state.scene_phase==1 || state.scene_phase==3) {
    if(teleport_tick(r,p)) {
      if(state.scene_phase==1) state.scene_phase=2;
      else {state.scene_phase=state.scene_owner=0;
        state.select_armed=state.select_hold=state.p1_select_armed=state.p1_select_hold=0;}
    }
    r[0xb9d]=r[0xba0]=0;return true;
  }
  if(!vile_capture(r) && !r[0x1f0c] && !r[0xc16] && !r[0x1f23] && !r[0x1f13] && !r[0x1f48] && r[0x1f10]<6 && r[0xd3]==4) {
    uint16_t x,y;
    if(!MmxCoopFindLanding(r,&x,&y)) return false;
    place_other(r,x,y,true);p=&state.players[state.anchor^1];
    p->zero.swap_phase=4;p->zero.swap_y=(int16_t)(word(r+0x1e50)-(int)y-40);
    state.scene_phase=3;sound(r,0x0e);r[0xb9d]=r[0xba0]=0;return true;
  }
  return false;
}
/* The generated door states enter the interpreter (where door_hook runs) only
 * when the hook can open its pass: same conditions as below. Interpreting
 * $81:EC98 otherwise gains nothing, and during Storm Eagle's lift ride its
 * interpreted run dispatches to garbage ($50:D2ED) while the generated one,
 * as in single player, does not. */
static bool door_route(bool ec98) {
  return enabled && state.initialized && !state.menu_owner && !state.scene_owner &&
      !state.door_pass && !(ec98 && g_ram[0x1f41]) &&
      state.players[state.anchor^1].status==MMX_COOP_ALIVE;
}
bool MmxCoopDoorRouteE70D(const CpuState *cpu) { (void)cpu; return door_route(false); }
bool MmxCoopDoorRouteEC98(const CpuState *cpu) { (void)cpu; return door_route(true); }
static void door_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.menu_owner) return;
  unsigned at=pc&65535;
  if(at==0xe70d || at==0xec98) {
    if(!state.door_pass && !state.scene_owner && !(at==0xec98 && g_ram[0x1f41]) &&
        state.players[state.anchor^1].status==MMX_COOP_ALIVE) {
      state.door_pass=1;state.door_s=cpu->S;state.door_d=cpu->D;state.door_entry=(uint16_t)at;
      TRACE(DOOR,pc,1,0,cpu);
    }
    return;
  }
  if(!state.door_pass || cpu->S!=state.door_s || cpu->D!=state.door_d) return;
  TRACE(DOOR,pc,state.door_pass,0,cpu);
  if(at==0xe725 || at==0xecc7) {
    state.anchor=state.current;state.door_pass=0;begin_scene(g_ram);return;
  }
  if(state.door_pass==1) {
    MmxCoopSelect(g_ram,state.anchor^1);state.door_pass=2;
    interp_bridge_pre_opcode_redirect((pc&0xff0000)|state.door_entry);
  } else {MmxCoopSelect(g_ram,state.anchor);state.door_pass=0;}
}
/* Storm Eagle's E-tank elevator (enemy $59) clears .2C and asks $82:D7D7,
 * once a frame, whether the player stands on it: the query resolves contact
 * for the projected body ($0BA8) and sets .2C bit 0 for a rider. Only the
 * current seat was ever asked, so the other player fell through it and passed
 * through its sides. Ask again for the other living seat, exactly as
 * platform_hook retries $84:AB81: same entry .2C and registers, the partner
 * projected, and the two answers OR-ed into .2C so either rider lifts it.
 * The pass never spans a frame, so its state is host-only. D7D7's RTL is
 * found at run time: every $6B byte after the entry is hooked, and only the
 * one executed with the entry's stack pointer completes the pass. */
static struct {
  uint8_t pass,first,entry_2c,first_2c,p,db;
  uint16_t d,s,a,x,y,ra,rx,ry; uint8_t rp,rdb;
  uint32_t ret; /* JSL return address on the stack at entry */
  uint8_t carry; /* partner seat + 1 to carry at frame end, 0 for none */
  uint16_t carry_d; /* the riding top's slot */
  uint16_t carry_x,carry_y; /* elevator position when the partner boarded */
} lift;
/* The cart moves once, then applies $88:9821..9867 to each rider. Native
 * .2C bit 0 remains "any rider" for its AI; bits 1/2 remember seats for
 * the next frame's .38 comparison and are serialized in ordinary WRAM. */
static struct {
  bool ready;uint8_t pass,first,flags,previous,p,db;
  uint16_t d,s,a,x,y,ra,rx,ry;uint8_t rp,rdb;
} cart;
static struct {
  uint8_t pass,first,original,p,db,rp,rdb;uint16_t d,s,a,x,y,ra,rx,ry;
} elevator_move;
static uint32_t lift_stack_ret(const CpuState *cpu) {
  return cpu->S<0x1ffd ? (uint32_t)(g_ram[cpu->S+1]|g_ram[cpu->S+2]<<8|g_ram[cpu->S+3]<<16) : 0;
}
static void lift_rtl_hook(CpuState *cpu,uint32_t pc);
#ifndef MMX_VARIANT_JP
#define MMX_VARIANT_JP 0
#endif
static void lift_find_rtl(void) {
  static bool scanned;
  /* USA addresses: the JP build keeps the single-seat query. */
  if(MMX_VARIANT_JP || scanned || !g_snes || !g_snes->cart || !g_snes->cart->rom) return;
  scanned=true;
  const uint32_t base=(0x02u<<15)|(0xd7d7&0x7fff); /* LoROM $82:D7D7 */
  for(uint32_t i=1;i<0x400 && base+i<g_snes->cart->romSize;++i)
    if(g_snes->cart->rom[base+i]==0x6b)
      interp_bridge_add_pre_opcode_hook(0x820000|((0xd7d7+i)&0xffff),lift_rtl_hook);
}
/* Host side, between frames: hook registration never runs inside a hook. */
void MmxCoopHostFrame(void) { if(enabled) lift_find_rtl(); }
static bool lift_elevator(unsigned d) {
  /* Solid enemies that query $82:D7D7 for the current seat only:
   * Storm Eagle's E-tank elevator top ($59) and its column ($5A, 83 px
   * below), Flame Mammoth's scrap blocks dropped onto the conveyor
   * ($2A, from $87:9C7B/9D89), Armored Armadillo's minecart ($2B), and
   * Kuwanger's red moving platforms ($3F), and D-Rex's lower body ($62). */
  if(d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d]) return false;
  unsigned c=g_ram[d+10];
  return c==0x59 || c==0x5a || c==0x2a || c==0x2b || c==0x3f || c==0x62;
}
static void laser_reset(void);
static void lift_reset(void) {
  lift.pass=0;lift.carry=0;cart.ready=false;cart.pass=0;elevator_move.pass=0;laser_reset();
}
static void lift_close(void) {
  if(lift.pass==2) MmxCoopSelect(g_ram,lift.first);
  lift.pass=0;
}
static void lift_contact_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  /* The next part to query (the $5A column) runs after the top has moved:
   * carry the partner now, before the camera and sprites read him. */
  if(lift.carry && cpu->D!=lift.carry_d) MmxCoopLiftCarry(g_ram);
  diagnostic_event(g_ram,cpu,pc,"lift-contact");
  if(lift.pass || state.menu_owner || state.scene_owner || !lift_elevator(cpu->D) ||
      state.players[state.current^1].status!=MMX_COOP_ALIVE ||
      !(state.players[state.current^1].body[0x27]&127)) return;
  cpu_mirrors_to_p(cpu);
  lift.pass=1;lift.first=state.current;lift.d=cpu->D;lift.s=cpu->S;
  lift.a=cpu->A;lift.x=cpu->X;lift.y=cpu->Y;lift.p=cpu->P;lift.db=cpu->DB;
  lift.entry_2c=g_ram[cpu->D+0x2c];lift.ret=lift_stack_ret(cpu);
}
/* Kuwanger's main elevator uses its own fixed-width top check rather than
 * $82:D7D7. Replay only that check; its movement and camera script run once. */
static void kuwanger_lift_hook(CpuState *cpu,uint32_t pc) {
  unsigned d=cpu->D;
  if(!enabled || !state.initialized || state.menu_owner || state.scene_owner ||
      d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d] || g_ram[d+10]!=0x3d) return;
  if((pc&65535)==0xaf10) {
    if(lift.pass || state.players[state.current^1].status!=MMX_COOP_ALIVE ||
        !(state.players[state.current^1].body[0x27]&127)) return;
    cpu_mirrors_to_p(cpu);
    lift.pass=1;lift.first=state.current;lift.d=(uint16_t)d;lift.s=cpu->S;
    lift.a=cpu->A;lift.x=cpu->X;lift.y=cpu->Y;lift.p=cpu->P;lift.db=cpu->DB;
    lift.entry_2c=g_ram[d+0x2c];g_ram[d+0x3f]=0;return;
  }
  if(!lift.pass || cpu->S!=lift.s || d!=lift.d) return;
  if(lift.pass==1) {
    if(g_ram[d+0x2c]&4) g_ram[d+0x3f]|=1u<<state.current;
    lift.first_2c=g_ram[d+0x2c];cpu_mirrors_to_p(cpu);
    lift.ra=cpu->A;lift.rx=cpu->X;lift.ry=cpu->Y;lift.rp=cpu->P;lift.rdb=cpu->DB;
    MmxCoopSelect(g_ram,lift.first^1);lift.pass=2;g_ram[d+0x2c]=lift.entry_2c;
    cpu->A=lift.a;cpu->X=lift.x;cpu->Y=lift.y;cpu->P=lift.p;cpu->DB=lift.db;cpu_p_to_mirrors(cpu);
    interp_bridge_pre_opcode_redirect(0x87af10);return;
  }
  if(g_ram[d+0x2c]&4) g_ram[d+0x3f]|=1u<<state.current;
  g_ram[d+0x2c]|=lift.first_2c;MmxCoopSelect(g_ram,lift.first);
  cpu->A=lift.ra;cpu->X=lift.rx;cpu->Y=lift.ry;cpu->P=lift.rp;cpu->DB=lift.rdb;cpu_p_to_mirrors(cpu);
  lift.pass=0;
}
/* .3F records the elevator/red platform's two answers. Apply the native delta only
 * to those riders, so a partner boarding alone cannot drag the other body. */
static void kuwanger_carry_hook(CpuState *cpu,uint32_t pc) {
  unsigned d=cpu->D;
  if(!enabled || !state.initialized || d<0xe68 || d>=0x1228 || (d-0xe68)%64 ||
      !g_ram[d] || (g_ram[d+10]!=0x3d && g_ram[d+10]!=0x3f && g_ram[d+10]!=0x62)) return;
  if((pc&65535)==0xc715) {
    if(elevator_move.pass) return;
    unsigned riders=g_ram[d+0x3f]&3;
    for(unsigned seat=0;seat<2;++seat)
      if(state.players[seat].status!=MMX_COOP_ALIVE || !(state.players[seat].body[0x27]&127)) riders&=~(1u<<seat);
    if(!riders) riders=1u<<state.current; /* Original single-rider save/pass. */
    elevator_move.pass=1;elevator_move.original=state.current;
    elevator_move.first=(riders&(1u<<state.current)) ? state.current : state.current^1;
    elevator_move.d=(uint16_t)d;elevator_move.s=cpu->S;
    cpu_mirrors_to_p(cpu);elevator_move.a=cpu->A;elevator_move.x=cpu->X;elevator_move.y=cpu->Y;
    elevator_move.p=cpu->P;elevator_move.db=cpu->DB;
    MmxCoopSelect(g_ram,elevator_move.first);return;
  }
  if(!elevator_move.pass || elevator_move.d!=d || elevator_move.s!=cpu->S) return;
  if(elevator_move.pass==1 && (g_ram[d+0x3f]&(1u<<(elevator_move.first^1))) &&
      state.players[elevator_move.first^1].status==MMX_COOP_ALIVE &&
      (state.players[elevator_move.first^1].body[0x27]&127)) {
    cpu_mirrors_to_p(cpu);elevator_move.ra=cpu->A;elevator_move.rx=cpu->X;elevator_move.ry=cpu->Y;
    elevator_move.rp=cpu->P;elevator_move.rdb=cpu->DB;
    MmxCoopSelect(g_ram,elevator_move.first^1);elevator_move.pass=2;
    cpu->A=elevator_move.a;cpu->X=elevator_move.x;cpu->Y=elevator_move.y;
    cpu->P=elevator_move.p;cpu->DB=elevator_move.db;cpu_p_to_mirrors(cpu);
    interp_bridge_pre_opcode_redirect(0x82c715);return;
  }
  MmxCoopSelect(g_ram,elevator_move.original);
  if(elevator_move.pass==2) {
    cpu->A=elevator_move.ra;cpu->X=elevator_move.rx;cpu->Y=elevator_move.ry;
    cpu->P=elevator_move.rp;cpu->DB=elevator_move.rdb;cpu_p_to_mirrors(cpu);
  }
  elevator_move.pass=0;
}

/* Laser sensors ($43) test a body directly through $84:9C0E. A successful
 * contact arms all $44 turrets, whose delayed aim must keep that seat rather
 * than use whichever body happens to drive the world twenty frames later.
 * Native $43/$44 code leaves .3F unused; ownership stays in snapshot WRAM. */
static struct {uint8_t pass,first,p,db,aim_return,sensor_return;uint16_t d,s,a,x,y;} laser;
static void laser_reset(void) {memset(&laser,0,sizeof(laser));}
static void laser_contact_hook(CpuState *cpu,uint32_t pc) {
  unsigned d=cpu->D,at=pc&65535;
  if(!enabled || !state.initialized || state.menu_owner || state.scene_owner ||
      d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d] || g_ram[d+10]!=0x43) return;
  if(at==0x9c0e) {
    if(laser.pass || cpu->X!=0xba8) return;
    cpu_mirrors_to_p(cpu);laser.pass=1;laser.first=state.current;laser.d=(uint16_t)d;laser.s=cpu->S;
    laser.a=cpu->A;laser.x=cpu->X;laser.y=cpu->Y;laser.p=cpu->P;laser.db=cpu->DB;return;
  }
  if(!laser.pass || laser.s!=cpu->S || laser.d!=d) return;
  if(cpu->_flag_C) g_ram[d+0x3f]=(uint8_t)(state.current+1);
  else if(laser.pass==1 && state.players[laser.first^1].status==MMX_COOP_ALIVE &&
      (state.players[laser.first^1].body[0x27]&127)) {
    MmxCoopSelect(g_ram,laser.first^1);laser.pass=2;
    cpu->A=laser.a;cpu->X=laser.x;cpu->Y=laser.y;cpu->P=laser.p;cpu->DB=laser.db;cpu_p_to_mirrors(cpu);
    interp_bridge_pre_opcode_redirect(0x849c0e);return;
  }
  if(laser.pass==2) {
    /* Directional sensors also aim an immediate shot after this query.
     * Keep its tripper projected through that handler, then restore the
     * original world actor even if the one-shot sensor deletes itself. */
    if(cpu->_flag_C) laser.sensor_return=(uint8_t)(laser.first+1);
    else MmxCoopSelect(g_ram,laser.first);
  }
  laser.pass=0;
}
static void laser_target_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  unsigned d=cpu->D,at=pc&65535;
  if(at==0xb92f && laser.sensor_return && d==laser.d) {
    MmxCoopSelect(g_ram,laser.sensor_return-1);laser.sensor_return=0;return;
  }
  if(d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d]) return;
  if(at==0xb91c || at==0xba72) {
    if(!g_ram[d+1]) g_ram[d+0x3f]=0;
    return;
  }
  if(g_ram[d+10]!=0x44) return;
  if(at==0xba5c) {
    unsigned sensor=word(g_ram+2);
    if(sensor>=0xe68 && sensor<0x1228 && !((sensor-0xe68)%64) && g_ram[sensor+10]==0x43)
      g_ram[d+0x3f]=g_ram[sensor+0x3f];
  } else if(at==0xbb09) {
    unsigned owner=g_ram[d+0x3f];
    if(owner && owner<=2 && state.players[owner-1].status==MMX_COOP_ALIVE &&
        (state.players[owner-1].body[0x27]&127)) {
      laser.aim_return=(uint8_t)(state.current+1);MmxCoopSelect(g_ram,owner-1);
    }
  } else if(at==0xbb0d && laser.aim_return) {
    MmxCoopSelect(g_ram,laser.aim_return-1);laser.aim_return=0;
  }
}
static void lift_rtl_hook(CpuState *cpu,uint32_t pc) {
  /* Same stack depth, slot and return address: D7D7's own RTL, not another
   * bank $82 routine returning to a different caller. */
  if(!lift.pass || cpu->S!=lift.s || cpu->D!=lift.d || lift_stack_ret(cpu)!=lift.ret) return;
  diagnostic_event(g_ram,cpu,pc,"lift-return");
  unsigned d=lift.d;
  if(lift.pass==1) {
    lift.first_2c=g_ram[d+0x2c];
    cpu_mirrors_to_p(cpu);
    lift.ra=cpu->A;lift.rx=cpu->X;lift.ry=cpu->Y;lift.rp=cpu->P;lift.rdb=cpu->DB;
    MmxCoopSelect(g_ram,lift.first^1);lift.pass=2;
    g_ram[d+0x2c]=lift.entry_2c;
    cpu->A=lift.a;cpu->X=lift.x;cpu->Y=lift.y;cpu->DB=lift.db;cpu->P=lift.p;cpu_p_to_mirrors(cpu);
    interp_bridge_pre_opcode_redirect(0x82d7d7);
    return;
  }
  bool second_rides=g_ram[d+0x2c]&1;
  uint8_t combined=(uint8_t)(lift.first_2c|g_ram[d+0x2c]);
  if(g_ram[d+10]==0x3f || g_ram[d+10]==0x62)
    g_ram[d+0x3f]=(uint8_t)(((lift.first_2c&1)?1u<<lift.first:0)|
        (second_rides?1u<<(lift.first^1):0));
  if(g_ram[d+10]==0x2b) {
    cart.ready=true;cart.pass=0;cart.first=lift.first;cart.d=(uint16_t)d;
    cart.flags=(combined&~6u)|((lift.first_2c&1)<<(lift.first+1))|
        ((second_rides?1u:0u)<<((lift.first^1)+1));
    cart.previous=g_ram[d+0x38];
    /* Older single-rider saves have no seat bits: the old anchor was the
     * only rider the native cart had ever queried. */
    if(!(cart.previous&6) && (cart.previous&1)) cart.previous|=1u<<(lift.first+1);
    combined=cart.flags;
  }
  g_ram[d+0x2c]=combined;
  MmxCoopSelect(g_ram,lift.first);
  /* The handler moves the elevator after the query and carries only the
   * projected body, so it always gets the first seat's answer; a riding
   * partner is moved by the same amount in MmxCoopLiftCarry. */
  cpu->A=lift.ra;cpu->X=lift.rx;cpu->Y=lift.ry;cpu->DB=lift.rdb;cpu->P=lift.rp;cpu_p_to_mirrors(cpu);
  if(second_rides && g_ram[d+10]==0x59) {
    lift.carry_d=(uint16_t)d;
    lift.carry=(uint8_t)((lift.first^1)+1);
    lift.carry_x=(uint16_t)(g_ram[d+5]|g_ram[d+6]<<8);
    lift.carry_y=(uint16_t)(g_ram[d+8]|g_ram[d+9]<<8);
  }
  lift.pass=0;
}
static void cart_project(unsigned seat) {
  unsigned d=cart.d;
  g_ram[d+0x2c]=(cart.flags&~7u)|((cart.flags>>(seat+1))&1);
  g_ram[d+0x38]=(cart.previous&~7u)|((cart.previous>>(seat+1))&1);
}
static void cart_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || !cart.ready || cpu->D!=cart.d) return;
  if((pc&65535)==0x9821) {
    if(cart.pass) return;
    cart.pass=1;cart.s=cpu->S;
    cpu_mirrors_to_p(cpu);cart.a=cpu->A;cart.x=cpu->X;cart.y=cpu->Y;cart.p=cpu->P;cart.db=cpu->DB;
    cart_project(cart.first);return;
  }
  if(!cart.pass || cpu->S!=cart.s) return;
  if(cart.pass==1) {
    cpu_mirrors_to_p(cpu);cart.ra=cpu->A;cart.rx=cpu->X;cart.ry=cpu->Y;cart.rp=cpu->P;cart.rdb=cpu->DB;
    MmxCoopSelect(g_ram,cart.first^1);cart.pass=2;cart_project(cart.first^1);
    cpu->A=cart.a;cpu->X=cart.x;cpu->Y=cart.y;cpu->P=cart.p;cpu->DB=cart.db;cpu_p_to_mirrors(cpu);
    interp_bridge_pre_opcode_redirect(0x889821);return;
  }
  MmxCoopSelect(g_ram,cart.first);
  g_ram[cart.d+0x2c]=cart.flags;g_ram[cart.d+0x38]=cart.previous;
  cpu->A=cart.ra;cpu->X=cart.rx;cpu->Y=cart.ry;cpu->P=cart.rp;cpu->DB=cart.rdb;cpu_p_to_mirrors(cpu);
  cart.pass=0;cart.ready=false;
}
/* Shift the partner who answered the elevator's query by however far the
 * elevator moved after it, so both riders track it in the same frame. Runs at
 * the column's query (before the shared camera averages the two bodies and
 * the sprites are drawn), at the camera, and at frame end as a fallback. */
void MmxCoopLiftCarry(uint8_t *r) {
  if(!lift.carry) return;
  unsigned seat=lift.carry-1u,d=lift.carry_d;
  lift.carry=0;
  if(!enabled || !state.initialized || !lift_elevator(d) ||
      state.players[seat].status!=MMX_COOP_ALIVE) return;
  uint8_t *body=seat==state.current ? r+0xba8 : state.players[seat].body;
  int dx=(int16_t)((r[d+5]|r[d+6]<<8)-lift.carry_x);
  int dy=(int16_t)((r[d+8]|r[d+9]<<8)-lift.carry_y);
  if(!dx && !dy) return;
  if(dx<-16 || dx>16 || dy<-16 || dy>16) return; /* warped, not carried */
  uint16_t x=(uint16_t)((body[5]|body[6]<<8)+dx),y=(uint16_t)((body[8]|body[9]<<8)+dy);
  body[5]=(uint8_t)x;body[6]=(uint8_t)(x>>8);body[8]=(uint8_t)y;body[9]=(uint8_t)(y>>8);
}
/* Stage sections set the player's OBJ priority once, on the world actor:
 * entering Storm Eagle's ship writes .11 bit 4 (priority 3, in front of the
 * hull's high-priority foreground) to $0BB9 and clears it at the boss lift.
 * The partner kept priority 2, so the PPU's first-sprite rule let Zero's
 * shape punch the foreground through X wherever they overlapped. Both seats
 * otherwise always share these bits; follow the world actor's. */
void MmxCoopSyncPriority(uint8_t *r) {
  if(!enabled || !state.initialized || state.menu_owner || state.scene_owner) return;
  unsigned seat=state.anchor^1;
  if(state.players[seat].status!=MMX_COOP_ALIVE) return;
  const uint8_t *world=state.anchor==state.current ? r+0xba8 : state.players[state.anchor].body;
  uint8_t *body=seat==state.current ? r+0xba8 : state.players[seat].body;
  body[0x11]=(uint8_t)((body[0x11]&~0x30)|(world[0x11]&0x30));
  /* Ground shock ($36, Flame Mammoth's stomp) tests only the projected world
   * actor. Its first frame stuns a grounded partner too; the native state
   * then runs the stun for him as usual. */
  unsigned action=body[2];
  if(world[2]==0x36 && world[3]==0 && (body[0x2b]&4) && (body[0x27]&127) &&
      action!=0x36 && action!=0x0c && action!=0x0e && action!=0x18 && action<0x1e) {
    /* .85 is the stun's countdown, set by the stomp alongside the action.
     * Left 0, X's wrapped to 255 and kept him stunned ~288 frames. */
    body[2]=0x36;body[3]=0;body[0x85]=world[0x85];
  }
  /* .27 bit 7 marks a body not yet (or no longer) in play: spawns store
   * HP|$80 and death stores $80. The world actor's arrival clears it, but a
   * placed partner kept it for good. Contact damage ignores it; Launch
   * Octopus's current generator ($28) does not, so it never lifted him. */
  if((body[0x27]&128) && (body[0x27]&127) && !(world[0x27]&128) && action!=0x0c &&
      !MmxCoopTransitionActive())
    body[0x27]&=127;
}
static void eagle_lift_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.menu_owner || state.scene_owner) return;
  unsigned d=cpu->D;
  if(g_ram[d+10]!=0x48 || g_ram[d+1]!=2 || g_ram[d+2]) return;
  if((pc&65535)==0xc0ae) {
    if(!state.door_pass && state.players[state.anchor^1].status==MMX_COOP_ALIVE) {
      state.door_pass=1;state.door_entry=0xc0ae;state.door_s=cpu->S;state.door_d=cpu->D;
    }
    return;
  }
  if(!state.door_pass || state.door_entry!=0xc0ae || state.door_s!=cpu->S || state.door_d!=d) return;
  /* $82:D7D7 sets .2C only for a rider. Run just that contact query for the
   * second seat, then let the original lift script own its chosen actor. */
  if(g_ram[d+0x2c]) {
    state.anchor=state.current;state.door_pass=0;begin_scene(g_ram);
  } else if(state.door_pass==1) {
    MmxCoopSelect(g_ram,state.anchor^1);state.door_pass=2;
    interp_bridge_pre_opcode_redirect(0x87c0ae);
  } else {MmxCoopSelect(g_ram,state.anchor);state.door_pass=0;}
}
static bool living_on_screen(const uint8_t *r,unsigned seat) {
  const MmxCoopPlayer *p=&state.players[seat];
  const uint8_t *b=seat==state.current ? r+0xba8 : p->body;
  MmxCoopView camera=MmxCoopViewForPlayer(r,&state,seat);
  int x=(int)word(b+5)-camera.x,y=(int)word(b+8)-camera.y;
  int height=p->character==MMX_COOP_ZERO ? 44 : 36;
  return p->status==MMX_COOP_ALIVE && (b[0x27]&127) && b[2]!=12 &&
      !p->zero.swap_phase && x+12>0 && x-12<256 && y+16>0 && y+16-height<224;
}
/* A fallen player's Select asks to return beside the partner for one of the
 * team's spare lives ($1F80: $80:9B43 spends one per checkpoint restart, the
 * 1-up adds one at $81:E4B3). Never during a boss or miniboss fight. The
 * request waits for both that and a spare life, so a 1-up collected while
 * none were left brings him straight back. */
static bool respawn_tick(uint8_t *r,unsigned seat) {
  MmxCoopPlayer *p=&state.players[seat];
  unsigned bit=1u<<seat;
  bool ready=living_on_screen(r,seat^1) && r[0x1f80] && !boss_fight(r);
  if(p->pressed&4) {
    state.respawn_pending|=(uint8_t)bit;
    if(!ready) sound(r,0x74); /* $00:F1E4 password rejection: queued, not yet. */
  }
  if(!(state.respawn_pending&bit) || !ready) return false;
  MmxCoopSelect(r,seat^1);
  uint16_t x,y;
  if(!MmxCoopFindLanding(r,&x,&y)) return false;
  place_other(r,x,y,false); /* full HP and the buster; inventory is kept */
  --r[0x1f80];state.respawn_pending&=(uint8_t)~bit;
  MmxZeroState *z=&p->zero;z->swap_phase=4;z->swap_y=(int16_t)(word(r+0x1e50)-(int)y-40);
  z->swap_tick=z->swap_fraction=0;
  state.select_armed=state.select_hold=state.p1_select_armed=state.p1_select_hold=0;
  sound(r,0x0e);r[0xb9d]=r[0xba0]=0;return true;
}
static bool join_tick(uint8_t *r) {
  for(unsigned seat=0;seat<2;++seat) {
    MmxCoopPlayer *p=&state.players[seat];
    if (!p->zero.swap_phase) continue;
    unsigned phase=p->zero.swap_phase;
    if (teleport_tick(r,p)) {
      if(phase==2) p->status=MMX_COOP_ABSENT;
      /* The native armor reads its pilot's global body/input. A returning
       * P1 must not take that projection away from an occupied P2 armor. */
      else if(!seat && !(r[0xe18] && (r[0xe22]&0x40))) {
        state.anchor=0;MmxCoopSelect(r,0);
      }
      state.select_armed=state.select_hold=state.p1_select_armed=state.p1_select_hold=0;
    }
    r[0xb9d]=r[0xba0]=0;return true;
  }
  /* No voluntary join or withdrawal while a script holds the world actor.
   * P1 returning during Sigma 4's intro took the world from the locked Zero. */
  bool gameplay=r[0xd1]==2 && r[0xd2]==4 && r[0xd3]==4 && r[0xba9]==2 && !world_scripted(r) &&
      (r[0xbcf]&127) && r[0xbaa]!=12 &&
      !r[0x1f0c] && r[0x1f10]<6 && !r[0x1f23] && !r[0x1f48] && !r[0x1f19];
  if (!gameplay) {state.select_hold=state.p1_select_hold=0;return false;}
  for(unsigned seat=0;seat<2;++seat) {
    MmxCoopPlayer *p=&state.players[seat];
    uint8_t *hold=seat ? &state.select_hold : &state.p1_select_hold;
    uint8_t *armed=seat ? &state.select_armed : &state.p1_select_armed;
    if (!(p->input&4)) {*armed=1;*hold=0;}
    if (p->status==MMX_COOP_FALLEN) {*hold=0;if(respawn_tick(r,seat)) return true;continue;}
    if (!living_on_screen(r,seat^1)) {*hold=0;continue;}
    if (p->status==MMX_COOP_ALIVE) {
      /* An occupied armor still updates through its pilot's projected body.
       * Do not hide that body or hand control away during a voluntary exit. */
      if (!(p->body[0x27]&127) || p->body[2]==12 || p->body[2]==0x2c ||
          (seat==state.anchor && r[0xe18] && (r[0xe22]&0x40))) {*hold=0;continue;}
      if ((p->input&4) && *armed && ++*hold>=90) {
        /* The other seat now drives world scripts and native input. The
         * withdrawn body/inventory remains stored for a voluntary return. */
        state.anchor=(uint8_t)(seat^1);MmxCoopSelect(r,state.anchor);
        clear_player_combat(r,seat);
        MmxZeroState *z=&p->zero;z->swap_phase=1;z->swap_tick=z->swap_fraction=0;z->swap_y=0;
        *hold=0;sound(r,0x0f);r[0xb9d]=r[0xba0]=0;return true;
      }
      continue;
    }
    bool automatic=seat==1 && state.stage_pending==2;
    if (!(p->pressed&4) && !automatic) continue;
    MmxCoopSelect(r,seat^1);
    uint16_t x,y;
    if (!MmxCoopFindLanding(r,&x,&y)) {
      if(!automatic) sound(r,0x74); /* $00:F1E4 password rejection. */
      continue;
    }
    if (!MmxCoopPlacePartner(r,x,y)) continue;
    state.stage_pending=0;
    MmxZeroState *z=&p->zero;z->swap_phase=4;z->swap_y=(int16_t)(word(r+0x1e50)-(int)y-40);
    z->swap_tick=z->swap_fraction=0;
    state.select_armed=state.select_hold=state.p1_select_armed=state.p1_select_hold=0;
    sound(r,0x0e);r[0xb9d]=r[0xba0]=0;return true;
  }
  return false;
}

static unsigned slime_bit(unsigned d) {
  return d>=0x1428 && d<0x1628 && (d-0x1428)%64==0 && g_ram[d+10]==0x19 ?
      1u<<((d-0x1428)/64) : 0;
}
static void slime_owner(unsigned bit,unsigned player) {
  if(player) state.slime_p2|=bit;else state.slime_p2&=~bit;
}
static void slime_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  unsigned d=cpu->D,bit=slime_bit(d);
  if(!bit) return;
  if((pc&65535)==0xa939) {
    if(state.current!=state.anchor) TRACE(SLIME,pc,2,0,cpu);
    MmxCoopSelect(g_ram,state.anchor);return;
  }
  if(g_ram[d+2]==12 || g_ram[d+2]==14) {
    unsigned owner=(state.slime_p2&bit)!=0;
    if(state.players[owner].status==MMX_COOP_ALIVE && (state.players[owner].body[0x27]&127)) {
      TRACE(SLIME,pc,1,owner,cpu);MmxCoopSelect(g_ram,owner);
    }
    else {g_ram[d+2]=16;g_ram[d+3]=0;} /* Native release/pop animation. */
  }
}
/* Trace the enemy contact calls while a Storm Eagle elevator part lives. */
static bool contact_traced(unsigned d,unsigned entry) {
  if(!diagnostic_enabled) return false;
  if(capsule_slot(d)) return true;
  for(unsigned i=0;i<15;++i) {
    const uint8_t *e=g_ram+0xe68+i*64;
    if(e[0] && e[10]>=0x58 && e[10]<=0x5a) return true;
  }
  /* Shot contact ($84:9B43) for an enemy near the screen while the current
   * seat has a live shot: shows whether each seat's shots reach a target. */
  if(entry!=0x9b43 || d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d]) return false;
  int dx=(int)word(g_ram+d+5)-(int)word(g_ram+0x1e4d);
  if(dx<-64 || dx>320+64) return false;
  for(unsigned i=0;i<12;++i) if(g_ram[0xc98+i*32]) return true;
  return false;
}
static void contact_hook(CpuState *cpu,uint32_t pc) {
  if (!enabled || !state.initialized) return;
  unsigned at = pc & 65535;
  /* A lone survivor still uses the native contact path without a second pass. */
  if(at==0x9b03 && !state.contact_pass) {
    unsigned bit=slime_bit(cpu->D);
    if(bit && g_ram[cpu->D+2]!=12 && g_ram[cpu->D+2]!=14) slime_owner(bit,state.current);
  }
  if ((at == 0x9b03 || at == 0x9b43) && contact_traced(cpu->D,at))
    diagnostic_event(g_ram,cpu,pc,"contact-enter");
  if(state.menu_owner || state.scene_owner ||
      (!state.contact_pass && state.players[state.current^1].status!=MMX_COOP_ALIVE)) return;
  if (at == 0x9b03 || at == 0x9b43) {
    /* Dr. Light's capsule ($4D) is driven by the world actor alone. Retrying
     * its body contact for the partner re-entered the capsule mid-dialogue:
     * Zero walking into it moved it from Light's dialogue (state 6) to the
     * upgrade (state 8), so the armor sequence played under the text. */
    if (!state.contact_pass && (capsule_slot(cpu->D) ||
        (at==0x9b03 && vile_script_object(cpu->D)))) return;
    if (!state.contact_pass) {
      MmxCoopViewsContactPlayer(state.current);
      state.contact_pass = 1; state.contact_entry = (uint16_t)at;
      state.contact_s = cpu->S; state.contact_d = cpu->D;
      TRACE(CONTACT,pc,0,0,cpu);
    }
    return;
  }
  /* A helper may return through a shared RTL; only the owning guest call's
   * balanced return boundary can complete or restart this pass. */
  if (!state.contact_pass || cpu->S != state.contact_s || cpu->D != state.contact_d) return;
  if (contact_traced(cpu->D,state.contact_entry)) diagnostic_event(g_ram,cpu,pc,"contact-return");
  unsigned slime=state.contact_entry==0x9b03 ? slime_bit(cpu->D) : 0;
  if (state.contact_pass == 1) {
    /* Slimer's puddle can capture one actor. Keep the actual contact seat
     * for its later pin/escape states, instead of reusing the world anchor. */
    if(slime && (cpu->A&255)) {
      slime_owner(slime,state.current);state.contact_pass=0;TRACE(CONTACT,pc,3,cpu->A,cpu);return;
    }
    /* Retail stops the projectile scan after its first contact, including
     * immune/reflecting hits. Extend the scan to P2 only after a real miss. */
    if (state.contact_entry == 0x9b43 && at != 0x9b7d) { state.contact_pass = 0; TRACE(CONTACT,pc,3,cpu->A,cpu); return; }
    state.contact_a = cpu->A; state.contact_x = cpu->X; state.contact_y = cpu->Y;
    state.contact_db = cpu->DB; cpu_mirrors_to_p(cpu); state.contact_p = cpu->P;
    TRACE(CONTACT,pc,1,cpu->A,cpu);
    MmxCoopSelect(g_ram,MmxCoopViewsGetWorldState().contact_player^1); state.contact_pass = 2;
    interp_bridge_pre_opcode_redirect(0x840000 | state.contact_entry);
  } else {
    if(slime && (cpu->A&255)) slime_owner(slime,state.current);
    TRACE(CONTACT,pc,2,cpu->A,cpu);
    bool first_hit = (state.contact_a & 255) != 0;
    bool no_second_hit = !(cpu->A & 255);
    MmxCoopSelect(g_ram,MmxCoopViewsGetWorldState().contact_player);
    if (first_hit || no_second_hit) {
      cpu->A = state.contact_a; cpu->X = state.contact_x; cpu->Y = state.contact_y;
      cpu->DB = state.contact_db; cpu->P = state.contact_p; cpu_p_to_mirrors(cpu);
    }
    state.contact_pass = 0;
    /* A fatal contact can happen immediately before a boss tests player HP
     * and permanently disables its own damage receiver. Finish both contact
     * passes, then expose the living world actor to that native continuation. */
    select_world_survivor(g_ram);
  }
}
static bool pickup_slot(unsigned d) {
  if(d<0x1628 || d>=0x1928 || (d-0x1628)%48) return false;
  unsigned kind=g_ram[d+10];
  return kind==1 || kind==2 || kind==4 || kind==5 || kind==11;
}
static void pickup_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.menu_owner) return;
  unsigned at=pc&65535,d=cpu->D;
  if ((pickup_slot(d) && g_ram[d]) || (state.pickup_pass && d==state.pickup_d))
    diagnostic_event(g_ram,cpu,pc,"pickup");
  if(at==0xd2ed || at==0xd31b) {
    if(state.current!=state.anchor) TRACE(PICKUP,pc,6,0,cpu);
    MmxCoopSelect(g_ram,state.anchor);return;
  }
  if(at==0xd2e6 || at==0xd308) {
    if(d<0x1628 || d>=0x1928 || (d-0x1628)%48) return;
    unsigned slot=(d-0x1628)/48;
    if(!g_ram[d] || !g_ram[d+1] || !pickup_slot(d)) state.pickup_owner[slot]=0;
    if(state.pickup_owner[slot]) {
      if(state.pickup_owner[slot]-1u!=state.current) TRACE(PICKUP,pc,5,slot,cpu);
      MmxCoopSelect(g_ram,state.pickup_owner[slot]-1);
    }
    return;
  }
  if(at==0x9c0e) {
    if(!state.pickup_pass && !state.scene_owner && pickup_slot(d) && cpu->X==0xba8 &&
        !state.pickup_owner[(d-0x1628)/48] && state.players[state.anchor^1].status==MMX_COOP_ALIVE) {
      state.pickup_s=cpu->S;state.pickup_d=(uint16_t)d;state.pickup_pass=1;
      TRACE(PICKUP,pc,0,(d-0x1628)/48,cpu);
    }
    return;
  }
  if(!state.pickup_pass || cpu->S!=state.pickup_s || d!=state.pickup_d) return;
  if(cpu->_flag_C) {
    TRACE(PICKUP,pc,3,state.current,cpu);
    state.pickup_owner[(d-0x1628)/48]=(uint8_t)(state.current+1);state.pickup_pass=0;
  } else if(state.pickup_pass==1 && state.current==state.anchor) {
    /* Retry only native contact, never item movement or its refill task.
     * Keep a successful collector projected through the rest of this item. */
    TRACE(PICKUP,pc,1,0,cpu);
    MmxCoopSelect(g_ram,state.anchor^1);state.pickup_pass=2;
    interp_bridge_pre_opcode_redirect(0x849c0e);
  } else {TRACE(PICKUP,pc,2,0,cpu);MmxCoopSelect(g_ram,state.anchor);state.pickup_pass=0;}
}
/* Ghost replays. Native code that acts on the body it finds in $0BA8 only
 * ever sees the seat that is projected while it runs:
 * - player projectiles run once per seat ($00:D3DD..D3F9) with that seat's
 *   shots and body, so charged Shotgun Ice's sled only carried its owner;
 * - enemies and enemy projectiles run once ($00:D4EA, $00:D48D) with the
 *   world actor projected, so Launch Octopus's upward currents only lifted
 *   the world actor.
 * Before such a run, replay it once with the other seat's body projected,
 * keep only that body's motion result, and restore everything else (WRAM
 * except the SPC mirror bytes, weapon combat, Zero and co-op state, renderer
 * pieces), so every object
 * still advances once. Couch co-op only for objects: online views already
 * project the nearest player for AI. */
enum { GHOST_SHOTS=1, GHOST_OBJECT, GHOST_CURRENT, GHOST_EAGLE_WIND, GHOST_DREX_CONTACT };
static struct {
  uint8_t pass,kind,p,db;uint16_t a,x,y,s,d;uint32_t resume;
  uint8_t body[0x90];
  MmxWeaponCombatState combat;MmxZeroState zero;MmxRendererPieceMark pieces;
  MmxCoopViewWorldState world;
} shot_ghost;
static MmxCoopState shot_ghost_state;
static uint8_t shot_ghost_lift[sizeof(lift)],shot_ghost_cart[sizeof(cart)];
static uint8_t shot_ghost_ram[0x20000];
static void shot_ghost_reset(void) { shot_ghost.pass=0; }
static bool ghost_active(void) { return shot_ghost.pass==1; }
/* Shot replays keep position (with subpixels), velocity and the ground and
 * contact flags: a seat's shot pass also maintains that seat's own shot
 * count and firing state, which the other body must not inherit.
 * Object replays keep every byte the update wrote to the body. Launch
 * Octopus's current generator ($28) only flags a body it finds in one of its
 * four $82:D7D7 boxes (.3C, action $08); the player's own movement lifts it.
 * Contact damage taken during the replay is kept too, and the real update's
 * contact retry then finds the partner already hit and invulnerable.
 * Shot riders also need BD4 (.2C), not just last frame's BD3 (.2B): the
 * native landing controller consumes that flag on the following frame. */
static bool shot_ghost_field(unsigned i) {
  return shot_ghost.kind!=GHOST_SHOTS || (i>=4 && i<=9) || (i>=0x1a && i<=0x1d) || i==0x2b || i==0x2c;
}
static bool ghost_partner_ready(void) {
  const MmxCoopPlayer *o=&state.players[state.current^1];
  return o->status==MMX_COOP_ALIVE && (o->body[0x27]&127) && o->body[2]!=12 && !o->zero.swap_phase;
}
static bool ghost_near(unsigned d,int range) {
  const MmxCoopPlayer *o=&state.players[state.current^1];
  int dx=(int)word(g_ram+d+5)-(int)word(o->body+5),dy=(int)word(g_ram+d+8)-(int)word(o->body+8);
  return dx>=-range && dx<=range && dy>=-range && dy<=range;
}
static bool shot_ghost_wanted(void) {
  if(!ghost_partner_ready()) return false;
  /* Native shots only: imported X2/X3 shots carry the port's +3E tag. */
  for(unsigned d=0x1228;d<0x1428;d+=64)
    if(g_ram[d] && word(g_ram+d+0x3e)!=0x5758 && ghost_near(d,64)) return true;
  return false;
}
static bool lift_elevator(unsigned d);
static bool capsule_slot(unsigned d);
static bool platform_item(unsigned d);
static bool object_ghost_wanted(unsigned d) {
  if(MmxCoopViewsOnline() || state.menu_owner || state.scene_owner || state.contact_pass ||
      state.object_pass || g_ram[0xd3]!=4 || state.current!=state.anchor || !ghost_partner_ready()) return false;
  bool enemy=d>=0xe68 && d<0x1228 && !((d-0xe68)%64);
  bool projectile=d>=0x1428 && d<0x1628 && !((d-0x1428)%64);
  if((!enemy && !projectile) || !g_ram[d]) return false;
  unsigned c=g_ram[d+10];
  /* Objects with their own seat handling: D7D7 solids and minecarts,
   * AB81 lifts, Kuwanger's custom elevator and owned laser sensors/turrets,
   * bounce-pad riders, Dr. Light's capsule, Gulpfer's chase, Slimer's puddle.
   * Their hooks project seats, which a ghost replay deliberately forbids. */
  if(enemy && (lift_elevator(d) || platform_item(d) || c==0x3d ||
      c==0x40 || c==0x43 || c==0x44 || capsule_slot(d) || c==0x1d)) return false;
  /* Bosses script the player (intros, victory pose); they stay single-seat. */
  if(vile_script_object(d) || (enemy &&
      (c==0x67 || c==0x69 || MmxWidePolicy_IsBossEncounter((uint8_t)c)))) return false;
  if(projectile && c==0x19) return false;
  /* The current generator's boxes reach about 200 px from its origin. */
  return ghost_near(d,256);
}
static void shot_ghost_begin(CpuState *cpu,unsigned kind,uint32_t resume) {
  cpu_mirrors_to_p(cpu);
  shot_ghost.a=cpu->A;shot_ghost.x=cpu->X;shot_ghost.y=cpu->Y;shot_ghost.s=cpu->S;
  shot_ghost.d=cpu->D;shot_ghost.p=cpu->P;shot_ghost.db=cpu->DB;
  shot_ghost.kind=(uint8_t)kind;shot_ghost.resume=resume;
  memcpy(shot_ghost_ram,g_ram,sizeof(shot_ghost_ram));
  shot_ghost.combat=MmxWeaponsGetCombatState();shot_ghost.zero=MmxZeroGetState();
  shot_ghost.pieces=MmxRendererMarkPieces();shot_ghost.world=MmxCoopViewsGetWorldState();
  shot_ghost_state=state;
  memcpy(shot_ghost_lift,&lift,sizeof(lift));memcpy(shot_ghost_cart,&cart,sizeof(cart));
  memcpy(shot_ghost.body,state.players[state.current^1].body,sizeof(shot_ghost.body));
  memcpy(g_ram+0xba8,shot_ghost.body,sizeof(shot_ghost.body));
  shot_ghost.pass=1;
}
/* $7EFFFC..FFFF mirror the sound CPU: the driver's ready flag, current track
 * and upload handshake counter. A replay that changes music ($80:87A2, e.g.
 * Velguarder at full HP) really uploads to the SPC, which no WRAM rollback
 * undoes. Restoring the old counter left the real update spinning forever at
 * $80:8701 (LDA $7EFFFE / CMP $2142), so these bytes keep the replay's values. */
enum { GHOST_SPC_MIRROR=0x1fffc };
static void shot_ghost_end(CpuState *cpu,uint32_t pc) {
  uint8_t result[0x90];memcpy(result,g_ram+0xba8,sizeof(result));
  uint8_t spc[4];memcpy(spc,g_ram+GHOST_SPC_MIRROR,sizeof(spc));
  memcpy(g_ram,shot_ghost_ram,sizeof(shot_ghost_ram));
  memcpy(g_ram+GHOST_SPC_MIRROR,spc,sizeof(spc));
  MmxWeaponsSetCombatState(shot_ghost.combat);MmxZeroSetState(shot_ghost.zero);
  MmxRendererRewindPieces(shot_ghost.pieces);MmxCoopViewsSetWorldState(&shot_ghost.world);
  state=shot_ghost_state;
  memcpy(&lift,shot_ghost_lift,sizeof(lift));memcpy(&cart,shot_ghost_cart,sizeof(cart));
  uint8_t *body=state.players[state.current^1].body;
  bool moved=false;
  for(unsigned i=0;i<sizeof(result);++i)
    if(shot_ghost_field(i) && result[i]!=shot_ghost.body[i]) {body[i]=result[i];moved=true;}
  cpu->A=shot_ghost.a;cpu->X=shot_ghost.x;cpu->Y=shot_ghost.y;cpu->S=shot_ghost.s;
  cpu->D=shot_ghost.d;cpu->DB=shot_ghost.db;cpu->P=shot_ghost.p;cpu_p_to_mirrors(cpu);
  if(moved) diagnostic_event(g_ram,cpu,pc,shot_ghost.kind==GHOST_SHOTS ? "shot-rider" : "ghost-moved");
  shot_ghost.pass=2;
  interp_bridge_pre_opcode_redirect(shot_ghost.resume);
}
/* Per-object ghost and attribution at the enemy ($00:D4F6 -> D4F9) and enemy
 * projectile ($00:D499 -> D49C) update calls. A "body-moved" trace row names
 * any object whose update moved the projected body. */
static struct { uint16_t d,x,y; bool armed; } object_watch;
static void object_ghost_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  unsigned at=pc&65535;
  bool call=at==0xd4f6 || at==0xd499;
  if(call) {
    if(shot_ghost.pass==2 && shot_ghost.kind==GHOST_OBJECT) shot_ghost.pass=0;
    else if(!shot_ghost.pass && object_ghost_wanted(cpu->D)) {
      shot_ghost_begin(cpu,GHOST_OBJECT,pc&0xffffff);return;
    }
    if(!shot_ghost.pass) {
      object_watch.armed=true;object_watch.d=(uint16_t)cpu->D;
      object_watch.x=(uint16_t)word(g_ram+0xbad);object_watch.y=(uint16_t)word(g_ram+0xbb0);
    }
    return;
  }
  if(shot_ghost.pass==1 && shot_ghost.kind==GHOST_OBJECT) { shot_ghost_end(cpu,pc);return; }
  if(object_watch.armed && cpu->D==object_watch.d &&
      (word(g_ram+0xbad)!=object_watch.x || word(g_ram+0xbb0)!=object_watch.y))
    diagnostic_event(g_ram,cpu,pc,"body-moved");
  object_watch.armed=false;
}
/* Eagle's wind state directly offsets BA8 by two pixels. Replay only its
 * height/direction/position check ($87:DAC4..DB0E), before the attack timer
 * and animation advance. The boss itself must remain a single world actor.
 * This also works when independent views project a different world seat. */
static void eagle_wind_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  if((pc&65535)==0xdb0e) {
    if(shot_ghost.pass==1 && shot_ghost.kind==GHOST_EAGLE_WIND) shot_ghost_end(cpu,pc);
    return;
  }
  if(shot_ghost.pass==2 && shot_ghost.kind==GHOST_EAGLE_WIND) {shot_ghost.pass=0;return;}
  unsigned d=cpu->D;
  if(shot_ghost.pass || state.menu_owner || state.scene_owner || state.stage_pending ||
      g_ram[0xd3]!=4 || !ghost_partner_ready() || d<0xe68 || d>=0x1228 ||
      (d-0xe68)%64 || !g_ram[d] || g_ram[d+10]!=0x52) return;
  shot_ghost_begin(cpu,GHOST_EAGLE_WIND,pc&0xffffff);
}
/* D-Rex's extra body-contact flags follow its ordinary solid query. Keep
 * those flags private to each body too, without replaying its boss AI. */
static void drex_contact_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  if((pc&65535)==0xc35f) {
    if(shot_ghost.pass==1 && shot_ghost.kind==GHOST_DREX_CONTACT) shot_ghost_end(cpu,pc);
    return;
  }
  if(shot_ghost.pass==2 && shot_ghost.kind==GHOST_DREX_CONTACT) {shot_ghost.pass=0;return;}
  unsigned d=cpu->D;
  if(shot_ghost.pass || state.menu_owner || state.scene_owner || state.stage_pending ||
      g_ram[0xd3]!=4 || !ghost_partner_ready() || d<0xe68 || d>=0x1228 ||
      (d-0xe68)%64 || !g_ram[d] || g_ram[d+10]!=0x62) return;
  shot_ghost_begin(cpu,GHOST_DREX_CONTACT,pc&0xffffff);
}
/* Launch Octopus's vortex is effect $16, not the enemy current generator.
 * Its native update ($81:F493) clears/sets BE4 and moves only BA8. Replay
 * the effect for the other living body, retaining its contact and motion,
 * while the vortex animation, timer and bubbles advance only once. */
static void current_ghost_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  if((pc&65535)==0xd35c) {
    if(shot_ghost.pass==1 && shot_ghost.kind==GHOST_CURRENT) shot_ghost_end(cpu,pc);
    return;
  }
  if(shot_ghost.pass==2 && shot_ghost.kind==GHOST_CURRENT) {shot_ghost.pass=0;return;}
  if(shot_ghost.pass || state.menu_owner || state.scene_owner || state.stage_pending ||
      state.current!=state.anchor || g_ram[0xd3]!=4 || !ghost_partner_ready() ||
      cpu->D<0x1928 || cpu->D>=0x1d08 || (cpu->D-0x1928)%32 ||
      !g_ram[cpu->D] || g_ram[cpu->D+10]!=0x16) return;
  shot_ghost_begin(cpu,GHOST_CURRENT,pc&0xffffff);
}
static void object_hook(CpuState *cpu, uint32_t pc) {
  if ((pc&65535)==0xd3f9 && shot_ghost.pass==1 && shot_ghost.kind==GHOST_SHOTS) { shot_ghost_end(cpu,pc); return; }
  if (!enabled || !state.initialized || state.menu_owner || state.scene_owner ||
      g_ram[0xd1]!=2 || g_ram[0xd2]!=4 || g_ram[0xd3]>=10 ||
      state.players[state.anchor^1].status != MMX_COOP_ALIVE) return;
  unsigned at = pc & 65535;
  bool entry = at == 0xd2bd || at == 0xd3dd || at == 0xd3fa || at == 0xd43a || at == 0xd457 || at == 0x9d67;
  if (entry) {
    if (!state.object_pass && state.current==state.anchor) {
      state.object_pass = 1; state.object_entry = (uint16_t)at;
      TRACE(OBJECT,pc,0,at,cpu);
      if (at==0x9d67) TRACE_MARK(state.current,TERRAIN);
    } else if (!state.object_pass) {
      /* The partner is projected, so this pass runs once for the wrong actor. */
      TRACE(OBJECT,pc,4,at,cpu);
      if (at==0x9d67) {TRACE_MARK(state.current,TERRAIN);TRACE_MARK(state.current,NON_ANCHOR);}
    }
    if (at==0xd3dd && !shot_ghost.pass && state.object_entry==0xd3dd &&
        ((state.object_pass==1 && state.current==state.anchor) ||
         (state.object_pass==2 && state.current!=state.anchor)) && shot_ghost_wanted())
      shot_ghost_begin(cpu,GHOST_SHOTS,(pc&0xff0000)|0xd3dd);
    return;
  }
  if (at==0xd3f9 && shot_ghost.pass==2 && shot_ghost.kind==GHOST_SHOTS) shot_ghost.pass=0;
  if (state.object_pass == 1) {
    state.object_a = cpu->A; state.object_x = cpu->X; state.object_y = cpu->Y;
    state.object_s = cpu->S; state.object_d = cpu->D; state.object_db = cpu->DB;
    cpu_mirrors_to_p(cpu); state.object_p = cpu->P;
    TRACE(OBJECT,pc,1,state.object_entry,cpu);
    if (state.object_entry==0x9d67) TRACE_MARK(state.anchor^1,TERRAIN);
    MmxCoopSelect(g_ram,state.anchor^1); state.object_pass = 2;
    interp_bridge_pre_opcode_redirect((pc & 0xff0000) | state.object_entry);
  } else if (state.object_pass == 2) {
    TRACE(OBJECT,pc,2,state.object_entry,cpu);
    MmxCoopSelect(g_ram,state.anchor);
    cpu->A = state.object_a; cpu->X = state.object_x; cpu->Y = state.object_y;
    cpu->D = state.object_d; cpu->DB = state.object_db; cpu->P = state.object_p;
    cpu_p_to_mirrors(cpu); state.object_pass = 0;
  }
}
/* The second seat re-enters $84:AB81/AB56 with the registers the item routine
 * passed in (platform_entry), not with the first seat's return values. A rider
 * is carried from the item's own fields, but a new landing reads the caller's
 * inputs. Host-only: a state load invalidates it, keeping the previous
 * behavior for a pass already in progress. */
/* Items whose rider contact goes through $84:AB81/AB56: $0E/$0F/$10/$13/$14
 * use .2C as a boolean rider latch. (Storm Eagle's E-tank elevator, enemy
 * $59, does not call these helpers; see storm-eagle-collision-handoff.md.) */
static bool platform_item(unsigned d) {
  /* Kuwanger's little lift uses the same helpers from an enemy slot. */
  if(d>=0xe68 && d<0x1228 && !((d-0xe68)%64)) return g_ram[d] && g_ram[d+10]==0x16;
  if(d<0x1628 || d>=0x1928 || (d-0x1628)%48) return false;
  unsigned c=g_ram[d+10];
  return (c>=0x0e && c<=0x10) || c==0x13 || c==0x14;
}
/* Co-op keeps one rider bit per seat in .2C bits 0/1 and projects the current
 * seat's as bit 0 while the native helper runs. */
static void platform_project(unsigned d) {
  g_ram[d+0x2c]=(uint8_t)((g_ram[d+0x2c]&~3u)|((state.platform_riders>>state.current)&1));
}
static void platform_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.menu_owner || state.scene_owner) return;
  unsigned at=pc&65535,d=cpu->D;
  if((d>=0x1628 && d<0x1928 && !((d-0x1628)%48)) || (d>=0xe68 && d<0x1228 && !((d-0xe68)%64)))
    diagnostic_event(g_ram,cpu,pc,at==0xab81 || at==0xab56 ? "platform-enter" : "platform-return");
  /* Slot initialization still clears .2C, and snapshots retain both riders. */
  if(!platform_item(d)) return;
  if(at==0xab81 || at==0xab56) {
    if(state.contact_pass) {
      /* Re-entry for the partner's own retry is expected; anything else
       * means a platform helper ran without its second-seat pass. */
      if(state.contact_pass!=2 || state.contact_d!=d) {
        TRACE(PLATFORM,pc,4,g_ram[d+0x2c],cpu);TRACE_MARK(state.current,PLATFORM_SKIPPED);
      }
      return;
    }
    TRACE(PLATFORM,pc,0,g_ram[d+0x2c],cpu);
    if(at==0xab81) TRACE_MARK(state.current,PLATFORM);
    state.contact_entry=(uint16_t)at;state.contact_pass=1;
    state.contact_d=(uint16_t)d;state.contact_s=cpu->S;
    state.platform_riders=g_ram[d+0x2c]&3;
    cpu_mirrors_to_p(cpu);
    platform_entry.valid=true;platform_entry.d=(uint16_t)d;platform_entry.s=cpu->S;
    platform_entry.a=cpu->A;platform_entry.x=cpu->X;platform_entry.y=cpu->Y;
    platform_entry.p=cpu->P;platform_entry.db=cpu->DB;
    if(at==0xab81) platform_project(d);
    return;
  }
  if(!state.contact_pass || state.contact_d!=d || state.contact_s!=cpu->S ||
      (state.contact_entry!=0xab81 && state.contact_entry!=0xab56)) return;
  if(state.contact_entry==0xab81) {
    unsigned bit=1u<<state.current;
    state.platform_riders=(state.platform_riders&~bit)|(g_ram[d+0x2c]?bit:0);
  }
  if(state.contact_pass==1 && state.players[state.anchor^1].status==MMX_COOP_ALIVE &&
      (state.players[state.anchor^1].body[0x27]&127)) {
    state.contact_a=cpu->A;state.contact_x=cpu->X;state.contact_y=cpu->Y;
    state.contact_db=cpu->DB;cpu_mirrors_to_p(cpu);state.contact_p=cpu->P;
    TRACE(PLATFORM,pc,1,state.platform_riders,cpu);
    if(state.contact_entry==0xab81) TRACE_MARK(state.anchor^1,PLATFORM);
    MmxCoopSelect(g_ram,state.anchor^1);state.contact_pass=2;
    if(state.contact_entry==0xab81) platform_project(d);
    if(platform_entry.valid && platform_entry.d==d && platform_entry.s==cpu->S) {
      cpu->A=platform_entry.a;cpu->X=platform_entry.x;cpu->Y=platform_entry.y;
      cpu->DB=platform_entry.db;cpu->P=platform_entry.p;cpu_p_to_mirrors(cpu);
    }
    interp_bridge_pre_opcode_redirect(0x840000|state.contact_entry);
    return;
  }
  g_ram[d+0x2c]=state.platform_riders;
  TRACE(PLATFORM,pc,2,state.platform_riders,cpu);
  if(state.contact_pass==2) {
    MmxCoopSelect(g_ram,state.anchor);
    cpu->A=state.contact_a;cpu->X=state.contact_x;cpu->Y=state.contact_y;
    cpu->DB=state.contact_db;cpu->P=state.contact_p;cpu_p_to_mirrors(cpu);
  }
  state.contact_pass=state.platform_riders=0;platform_entry.valid=false;
}
static bool shared_screen(void) {
  return enabled && state.initialized && !state.menu_owner && !state.scene_owner && state.players[0].status==MMX_COOP_ALIVE &&
      state.players[1].status==MMX_COOP_ALIVE && !MmxCoopTransitionActive() &&
      g_ram[0xd1]==2 && g_ram[0xd2]==4 && g_ram[0xd3]==4 &&
      !g_ram[0x1f0c] && !g_ram[0x1f23] && !g_ram[0x1f48];
}
static void constrain_player(uint8_t *r) {
  bool independent=MmxCoopViewsOnline();
  if(independent || shared_screen()) {
    if(state.scene_owner || state.menu_owner || r[0xd3]!=4) return;
    /* Native camera clamping only sees the world actor. Clamp each
     * controller against the same authored room bounds before averaging.
     * Treat left-edge underflow as negative, not as a jump to $FFFF. */
    int x=(int16_t)word(r+0xbad),lo=word(r+0x1e56)+8,hi=word(r+0x1e58)+248;
    if(hi<lo) return;
    int limited=x<lo?lo:x>hi?hi:x;
    if(x!=limited) {putword(r+0xbad,(unsigned)limited);r[0xbac]=0;putword(r+0xbc2,0);}
    /* A partner can reach the authored pit edge while the native camera is
     * still far above it, so E12D's canonical-camera branch may never run. */
    if(independent && (r[0xbcf]&127) && r[0xbaa]!=12 &&
        (int16_t)(word(r+0xbb0)-32-word(r+0x1e5c)-224)>=0) {
      ++r[0xbd8];r[0xbd7]=8;r[0xbce]=127;r[0xbaa]=r[0xc12]=12;r[0xbab]=0;r[0xbcf]=128;
    }
    if(independent) return;
  }
  if (!shared_screen()) return;
  const MmxCoopPlayer *other=&state.players[state.current^1];
  int x=word(r+0xbad),ox=word(other->body+5);
  int limited=x<ox-224 ? ox-224 : x>ox+224 ? ox+224 : x;
  if (limited!=x) {putword(r+0xbad,(unsigned)limited);r[0xbac]=0;putword(r+0xbc2,0);}
}
static void camera_hook(CpuState *cpu,uint32_t pc) {
  MmxCoopLiftCarry(g_ram);
  if ((pc&65535)==0xe12d) {
    /* The native bottom-camera clamp checks only $0BB0. Apply that same
     * signed feet threshold to the other actor before the original check.
     * $84:9F2F..9F79's fatal-hit state keeps the ordinary death controller,
     * sound and orb objects; the shared camera still runs only once. */
    if (!enabled || !state.initialized || state.menu_owner || state.scene_owner || MmxCoopTransitionActive()) return;
    MmxCoopPlayer *p=&state.players[state.anchor^1];
    /* Below the level's lowest camera position, as online views already
     * use: the screen bottom only moved because the shared camera was pulled
     * up (a capsule's camera lock), and scene_tick beams him over. */
    /* While a script holds the world actor, scene_tick beams him instead. */
    if (p->status==MMX_COOP_ALIVE && (p->body[0x27]&127) && !world_scripted(g_ram) &&
        (int16_t)(word(p->body+8)-32-(word(g_ram+0x1e5c)+224))>=0) {
      TRACE(PIT,pc,state.anchor^1,0,cpu);
      ++p->body[0x30];p->body[0x2f]=8;p->body[0x26]=127;
      p->body[2]=p->body[0x6a]=12;p->body[3]=0;p->body[0x27]=128;
    }
    /* Unified view: the shared camera can leave the world actor below the
     * screen too. Its native check then uses the same level bound, so only
     * a real pit kills him; scene_tick beams him out otherwise. */
    unsigned bound=word(g_ram+0x1e5c)+224;
    if (!MmxCoopViewsOnline() && state.current==state.anchor && p->status==MMX_COOP_ALIVE &&
        (p->body[0x27]&127) && word(g_ram)<bound) putword(g_ram,bound);
    return;
  }
  if (!shared_screen() || MmxCoopViewsOnline()) return;
  unsigned axis=((pc&65535)==0xdebf || (pc&65535)==0xdeca) ? 8 : 5;
  /* Round the midpoint toward the world actor. Plain (a+b)/2 floors, so
   * two bodies falling at the same speed one subpixel phase apart (1 px gap
   * flickering to 0) moved the camera with whichever was higher, and the
   * other's screen position jumped a pixel every few frames. */
  int a=(int)word(state.players[state.anchor].body+axis);
  int b=(int)word(state.players[state.anchor^1].body+axis);
  cpu->A=(uint16_t)(a+(b-a)/2);
  cpu->_flag_N=(cpu->A&0x8000)!=0;cpu->_flag_Z=cpu->A==0;
  cpu->P=(cpu->P&~0x82)|(cpu->_flag_N?0x80:0)|(cpu->_flag_Z?2:0);
}
static void menu_hook(CpuState *cpu,uint32_t pc) {
  if (!enabled || !state.initialized || state.scene_owner || MmxCoopTransitionActive()) return;
  switch (pc&65535) {
    case 0xe57f:
      if (state.current==1) {
        MmxCoopApplyInput(g_ram);cpu->A=(uint16_t)word(g_ram+0xbe2);
        cpu->_flag_N=(cpu->A&0x8000)!=0;cpu->_flag_Z=cpu->A==0;
        cpu->P=(cpu->P&~0x82)|(cpu->_flag_N?0x80:0)|(cpu->_flag_Z?2:0);
      }
      break;
    case 0x9e68: {
      if (state.menu_owner) return;
      unsigned seat=(state.players[state.anchor].pressed&8) ? state.anchor : state.anchor^1;
      if (state.players[seat].status!=MMX_COOP_ALIVE || !(state.players[seat].pressed&8)) return;
      state.menu_owner=(uint8_t)(seat+1);state.menu_last=(uint8_t)seat;
      TRACE(MENU,pc,1,seat,cpu);
      MmxCoopSelect(g_ram,seat);
      MmxCoopApplyInput(g_ram);g_ram[0xbe3]|=0x10;
      break;
    }
    case 0x9eac: /* Native pause guards rejected the request. */
    case 0xc579: /* Equipment and subtank changes are now committed. */
      if(state.menu_owner) {TRACE(MENU,pc,0,0,cpu);MmxCoopSelect(g_ram,state.anchor);state.menu_owner=0;}
      break;
  }
}
static void death_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized || state.menu_owner) return;
  unsigned seat=state.current,other=seat^1;
  if((pc&65535)==0x9d9e) {
    TRACE(DEATH,pc,pc,seat,cpu);
    /* Native reset has already cleared the actor pools. Adopt that new body
     * for P1 instead of projecting a stored corpse back over the cleared RAM. */
    state.current=state.anchor=0;state.stage_pending=1;
    state.scene_owner=state.scene_phase=state.door_pass=0;
    state.players[1].status=MMX_COOP_ABSENT;
    MmxZeroState z={0};z.active_x=state.players[0].character==MMX_COOP_X;
    MmxZeroSetState(z);MmxWeaponsSetState(state.players[0].weapons);
    MmxWeaponsCancelShots(g_ram);MmxWeaponsPartnerCombat(&state.players[1].combat);
    if(g_snes && g_snes->cart) MmxZeroSetCollisionRom(g_snes->cart->rom,g_snes->cart->romSize);
    return;
  }
  if((pc&65535)==0x9ac7) {
    /* Main stage mode 6 assumes the whole team is dead and stops input/pause
     * polling. Keep mode 4 when another player still has HP. */
    if(state.players[other].status==MMX_COOP_ALIVE && (state.players[other].body[0x27]&127)) {
      TRACE(DEATH,pc,pc,seat,cpu);
      interp_bridge_pre_opcode_redirect((pc&0xff0000)|0x9ad9);
    }
    return;
  }
  if(cpu->D!=0xba8) return;
  TRACE(DEATH,pc,pc,seat,cpu);
  switch(pc&65535) {
    case 0x8a5c:
      state.solo_death[seat]=state.players[other].status==MMX_COOP_ALIVE &&
          ((state.players[other].body[0x27]&127)!=0 || seat!=state.anchor);
      if(state.solo_death[seat]) memcpy(state.death_flags[seat],g_ram+0x1f13,7);
      break;
    case 0x8ab7:
      if(state.solo_death[seat]) memcpy(state.death_flags[seat],g_ram+0x1f13,7);
      break;
    case 0x8a78:
    case 0x8acc:
      /* Keep the native death pose, countdown, sound and expanding orbs,
       * but do not freeze the partner or erase another task's freeze flags. */
      if(state.solo_death[seat]) memcpy(g_ram+0x1f13,state.death_flags[seat],7);
      break;
    case 0x8b0b:
      if(state.solo_death[seat]) {
        state.solo_death[seat]=0;state.players[seat].status=MMX_COOP_FALLEN;
        MmxZeroCancel(g_ram);MmxWeaponsCancelShots(g_ram);
        memset(g_ram+0xc38,0,0x1e0);g_ram[0xbb6]=0;g_ram[0xbcf]=128;
        MmxCoopCapture(g_ram);
      }
      break;
  }
}
static void dash_effect_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  unsigned at=pc&0x7fffff,d=cpu->D;
  if(at==0x019c86) {
    /* The allocator has written class $0B; its unused parent-pointer bytes
     * keep ownership in WRAM, including save states and rollback snapshots. */
    unsigned effect=cpu->X;
    if(d==0xba8 && effect>=0x1928 && effect<0x1d08 && (effect&31)==8 &&
        g_ram[effect] && g_ram[effect+10]==0x0b) {
      g_ram[effect+12]=(uint8_t)(state.current+1);g_ram[effect+13]=0xd5;
    }
  } else if(at==0x00f478) {
    if(d<0x1928 || d>=0x1d08 || (d&31)!=8 || g_ram[d+10]!=0x0b ||
        g_ram[d+13]!=0xd5 || !g_ram[d+12] || g_ram[d+12]>2) return;
    unsigned owner=g_ram[d+12]-1;
    state.effect_return=(uint8_t)(state.current+1);
    MmxCoopSelect(g_ram,owner);
    /* The native updater reads the projected body's position/action. Dead
     * or withdrawn players cannot leave a flame following the survivor. */
    if(state.players[owner].status!=MMX_COOP_ALIVE || !(g_ram[0xbcf]&127) ||
        state.players[owner].zero.swap_phase) {
      memset(g_ram+d,0,4);g_ram[d+14]=g_ram[d+15]=0;
      /* This is the outer JSR frame, before F478's JSL. Return via its RTS;
       * jumping to the inner updater's RTL would consume the wrong stack. */
      interp_bridge_pre_opcode_redirect(0x80f47c);
    }
  } else if(at==0x00f47c && state.effect_return) {
    unsigned previous=state.effect_return-1;state.effect_return=0;
    MmxCoopSelect(g_ram,previous);
  }
}
static void controller_hook(CpuState *cpu, uint32_t pc) {
  if (!enabled) return;
  if(g_ram[0xd1]!=2 || g_ram[0xd2]!=4 || g_ram[0xd3]>=10) return;
  if ((pc & 0x7fffff) == 0x048fcb) {
    /* There is exactly one X. Preserve his native CHR allocation; Zero's
     * complete body comes from the original X3 asset compositor. PHP has
     * already run, so the native PLP/RTL remains balanced. */
    if (MmxZeroActive() && cpu->D >= 0xba8 && cpu->D < 0xc98) {
      g_ram[cpu->D + 0x17] &= 127;
      interp_bridge_pre_opcode_redirect(0x848fc8);
    }
    return;
  }
  if ((pc & 0x7fffff) == 0x0280df) {
    /* Keep native visibility/culling, but do not enqueue a second pointer to
     * the projected P1 address. P2 is drawn from its immutable context. */
    if (state.initialized && state.current != state.anchor &&
        ((cpu->D >= 0xba8 && cpu->D < 0xe18) || (cpu->D >= 0x1228 && cpu->D < 0x1428)))
      interp_bridge_pre_opcode_redirect(0x82810a);
    return;
  }
  if ((pc & 0xffff) == 0x8136) {
    MmxCoopInitialize(g_ram);
    diagnostic_event(g_ram,cpu,pc,"controller-enter");
    if (state.initialized && !state.controller_pass) {
      if(state.current==1 || cpu_companion) MmxCoopApplyInput(g_ram);
      state.controller_pass=1;
      TRACE(CONTROLLER,pc,0,0,cpu);
      TRACE_MARK(state.current,CONTROLLER);
      if(state.current!=state.anchor) TRACE_MARK(state.current,NON_ANCHOR);
    }
    if(state.initialized && (refill_paused(g_ram) || partner_parked(g_ram)))
      interp_bridge_pre_opcode_redirect(0x81819c);
    return;
  }
  if (!state.initialized || !state.controller_pass) return;
  diagnostic_event(g_ram,cpu,pc,"controller-return");
  constrain_player(g_ram);
  unsigned other=state.current^1;
  if (state.controller_pass == 1 && !state.menu_owner && !state.scene_owner && state.players[other].status == MMX_COOP_ALIVE &&
      g_ram[0xd1] == 2 && g_ram[0xd2] == 4 && !g_ram[0x1f0c]) {
    state.return_a = cpu->A; state.return_x = cpu->X; state.return_y = cpu->Y;
    state.return_s = cpu->S; state.return_db = cpu->DB;
    cpu_mirrors_to_p(cpu); state.return_p = cpu->P;
    if(!(g_ram[0xbcf]&127) && (state.players[other].body[0x27]&127)) state.anchor=(uint8_t)other;
    MmxCoopSelect(g_ram,other); MmxCoopApplyInput(g_ram);
    /* $00:D1F3..D206 prepares these outside the player routine. P2 needs
     * its own previous position and per-frame fire-command reset too. */
    if (!g_ram[0x1f19]) {
      memcpy(g_ram+0xbca,g_ram+0xbad,2); memcpy(g_ram+0xbcc,g_ram+0xbb0,2);
    }
    g_ram[0x1f0d] = 0;
    state.controller_pass = 2;
    TRACE(CONTROLLER,pc,1,0,cpu);
    TRACE_MARK(state.current,CONTROLLER);
    cpu->P |= 0x30; cpu_p_to_mirrors(cpu); cpu->X &= 255; cpu->Y &= 255;
    /* Re-enter AFTER PHP/PHD/PLD. Both passes share exactly one prologue and
     * epilogue, so no synthetic JSL or extra stack frame is needed. */
    interp_bridge_pre_opcode_redirect(0x818136);
    return;
  }
  if (state.controller_pass == 2) {
    g_ram[0xbd4] = 0; /* P2's counterpart of $00:D21A. */
    MmxCoopSelect(g_ram,state.anchor);
    cpu->A = state.return_a; cpu->X = state.return_x; cpu->Y = state.return_y;
    cpu->DB = state.return_db; cpu->P = state.return_p; cpu_p_to_mirrors(cpu);
  }
  TRACE(CONTROLLER,pc,state.controller_pass==2 ? 2 : 3,0,cpu);
  MmxCoopCapture(g_ram);
  state.controller_pass = 0;
}
void MmxCoopTraceFrame(const uint8_t *r) {
  if (g_mmx_coop_trace && enabled && state.initialized) MmxCoopTraceFrameEnd(&state,r);
}
static void view_world_hook(CpuState *cpu,uint32_t pc) {
  if(!MmxCoopViewsOnline() || !enabled || !state.initialized || state.stage_pending ||
      state.menu_owner || state.scene_owner || g_ram[0xd3]!=4) {
    MmxWsCullHook(cpu,pc);return;
  }
  unsigned at=pc&65535;
  if(at==0xdcd7) {
    extern void MmxCoopViewsSpawnWorld(CpuState *);
    MmxCoopCapture(g_ram);MmxCoopViewsSpawnWorld(cpu);return;
  }
  if(at==0xdd2d) {
    extern bool MmxCoopViewsSpawnAllowed(uint16_t);
    if(!MmxCoopViewsSpawnAllowed(cpu->D)) interp_bridge_pre_opcode_redirect(0x00dd97);
    return;
  }
  unsigned d=cpu->D;
  if(d>0x1fe0) return;
  int x=word(g_ram+d+5),y=word(g_ram+d+8),margin=g_mmx_custom_view.extra;
  int left=64,right=320,top=64,bottom=288;
  if(at==0x809e) {left=96;right=352;top=80;bottom=304;}
  if(at==0x80c3) {left=32;right=288;top=16;bottom=240;}
  if(at==0x8957) {left=128;right=384;top=128;bottom=352;}
  bool outside=!MmxCoopViewContains(g_ram,&state,x,y,left,right,top,bottom,margin);
  cpu->_flag_C=outside;cpu->P=(cpu->P&~1)|(outside?1:0);
  /* Both coordinates must belong to the SAME player region. Skip the second
   * native camera comparison only after replacing the complete rectangle. */
  if(at==0x807d) interp_bridge_pre_opcode_redirect(0x82808c);
  if(at==0x809e) interp_bridge_pre_opcode_redirect(outside?0x8280af:0x8280d9);
  if(at==0x80c3) interp_bridge_pre_opcode_redirect(outside?0x8280d4:0x8280d9);
  if(at==0x8957) interp_bridge_pre_opcode_redirect(0x838966);
}
/* Couch co-op runs enemy AI against the projected world actor. Enemies that
 * act on the body they chase also need the nearest player there: Launch
 * Octopus's Gulpfer ($1D) homes in on and swallows $0BA8, so it never went
 * for P2. Sigma's bounce pad ($40) likewise activates and carries that body;
 * a discarded ghost activation never advances its bounce for the partner.
 * The enemy loops ($00:D4EA/D507) are always interpreted. */
static bool couch_nearest_target(unsigned d) {
  if(d<0xe68 || d>=0x1228 || (d-0xe68)%64 || !g_ram[d]) return false;
  return g_ram[d+10]==0x1d || g_ram[d+10]==0x40;
}
static bool vile_farewell(unsigned d) {
  return g_ram[0x1f7a]==9 && d>=0xe68 && d<0x1228 && !((d-0xe68)%64) &&
      g_ram[d] && g_ram[d+10]==0x66 && g_ram[d+1]==2 && g_ram[d+2]==14;
}
static void view_actor_hook(CpuState *cpu,uint32_t pc) {
  if(!enabled || !state.initialized) return;
  unsigned at=pc&65535;
  MmxCoopViewWorldState world=MmxCoopViewsGetWorldState();
  bool returning=at==0xd4f9 || at==0xd522 || at==0xd49c || at==0xd4c5;
  /* Kneeling story Zero checks $0BAD >= $0BB8 for his farewell. Evaluate
   * that trigger against playable X even when Zero drives the world. Once
   * it fires, X owns the dialogue and the partner's usual scene transport.
   * A dead or beaming X must return before the conversation can begin. */
  if(at==0xd4f6 && vile_farewell(cpu->D) && g_ram[cpu->D+3]<=2 &&
      !state.stage_pending && !state.menu_owner && !state.scene_owner && !world.actor_return) {
    unsigned xs=state.players[0].character==MMX_COOP_X ? 0 : 1;
    const MmxCoopPlayer *x=&state.players[xs];
    if(x->character!=MMX_COOP_X || x->status!=MMX_COOP_ALIVE ||
        !(x->body[0x27]&127) || x->zero.swap_phase) {
      interp_bridge_pre_opcode_redirect(0x00d4f9);return;
    }
    MmxCoopViewsActorReturn(state.current+1);MmxCoopSelect(g_ram,xs);return;
  }
  if(!MmxCoopViewsOnline() && !(returning ? world.actor_return :
      (at==0xd4f6 || at==0xd515) && couch_nearest_target(cpu->D))) return;
  if(returning) {
    if(world.actor_return) {
      if(at==0xd4f9 && vile_farewell(cpu->D) && g_ram[cpu->D+3]>=4 &&
          state.players[state.current].character==MMX_COOP_X) {
        state.anchor=state.current;MmxCoopViewsActorReturn(0);begin_scene(g_ram);return;
      }
      if(couch_nearest_target(cpu->D) &&
          g_ram[cpu->D+10]==0x1d && (g_ram[cpu->D+0x3b] || g_ram[cpu->D+0x3d]))
        g_ram[cpu->D+0x3f]=(uint8_t)(state.current+1);
      if(couch_nearest_target(cpu->D) && g_ram[cpu->D+10]==0x40)
        g_ram[cpu->D+0x3f]=g_ram[cpu->D+1]==4 ? (uint8_t)(state.current+1) : 0;
      MmxCoopSelect(g_ram,world.actor_return-1);MmxCoopViewsActorReturn(0);
      select_world_survivor(g_ram);
    }
    return;
  }
  if(state.stage_pending || state.menu_owner || state.scene_owner || g_ram[0xd3]!=4 || world.actor_return) return;
  if(cpu->D<0xe68 || (cpu->D>=0x1228 && cpu->D<0x1428)) return;
  /* Vile, his restraint projectile and story Zero always use the world
   * actor, including independent views. Choosing a nearby partner here
   * would hand the capture back and forth between bodies. */
  if(vile_script_object(cpu->D)) return;
  MmxCoopCapture(g_ram);
  unsigned nearest=state.anchor;uint64_t best=UINT64_MAX;
  bool fish=couch_nearest_target(cpu->D) && g_ram[cpu->D+10]==0x1d;
  bool pad=couch_nearest_target(cpu->D) && g_ram[cpu->D+10]==0x40;
  unsigned owner=fish || pad ? g_ram[cpu->D+0x3f] : 0;
  bool captured=(fish && (g_ram[cpu->D+0x3b] || g_ram[cpu->D+0x3d])) ||
      (pad && g_ram[cpu->D+1]==4);
  /* Gulpfer keeps the body it swallowed; a rising bounce pad keeps its
   * rider even when the partner moves closer. Both native routines leave
   * .3F unused, so ownership travels with snapshots and rollback. */
  if(captured && owner>=1 && owner<=2) {
    const MmxCoopPlayer *p=&state.players[owner-1];
    if(!pad || (p->status==MMX_COOP_ALIVE && (p->body[0x27]&127) && !p->zero.swap_phase)) {
      MmxCoopViewsActorReturn(state.current+1);MmxCoopSelect(g_ram,owner-1);return;
    }
    /* A retired rider cannot keep dragging the projected survivor. The
     * pad's idle animation re-arms its ordinary contact query. */
    g_ram[cpu->D+1]=2;g_ram[cpu->D+0x3f]=0;captured=false;
  }
  if(fish && !captured) g_ram[cpu->D+0x3f]=0;
  int ex=word(g_ram+cpu->D+5),ey=word(g_ram+cpu->D+8);
  for(unsigned seat=0;seat<2;++seat) {
    const MmxCoopPlayer *p=&state.players[seat];
    if(p->status!=MMX_COOP_ALIVE || !(p->body[0x27]&127) || p->body[2]==12 || p->zero.swap_phase) continue;
    /* A free fish must not chase a body hidden/parked inside another fish.
     * The native eligibility test would then refuse every swallow attempt. */
    if(fish && !captured && (!p->body[14] || p->body[0x30])) continue;
    /* Older snapshots have no .3F owner yet. Recover it from the body
     * the native capture parked, rather than a nearby uncaptured partner. */
    if(captured && !owner && p->body[14] && !p->body[0x30]) continue;
    int64_t dx=(int)word(p->body+5)-ex,dy=(int)word(p->body+8)-ey;
    uint64_t distance=(uint64_t)(dx*dx+dy*dy);
    if(distance<best) {best=distance;nearest=seat;}
  }
  MmxCoopViewsActorReturn(state.current+1);MmxCoopSelect(g_ram,nearest);
}
void MmxCoopRegisterHooks(void) {
  const unsigned actors[]={0xd4f6,0xd4f9,0xd515,0xd522,0xd499,0xd49c,0xd4b8,0xd4c5};
  for(unsigned i=0;i<sizeof(actors)/sizeof(actors[0]);++i)
    interp_bridge_set_pre_opcode_hook(actors[i],view_actor_hook);
  const unsigned ghosts[]={0xd4f6,0xd4f9,0xd499,0xd49c};
  for(unsigned i=0;i<sizeof(ghosts)/sizeof(ghosts[0]);++i)
    interp_bridge_add_pre_opcode_hook(ghosts[i],object_ghost_hook);
  interp_bridge_add_pre_opcode_hook(0x00d359,current_ghost_hook);
  interp_bridge_add_pre_opcode_hook(0x00d35c,current_ghost_hook);
  interp_bridge_set_pre_opcode_hook(0x87dac4,eagle_wind_hook);
  interp_bridge_set_pre_opcode_hook(0x87db0e,eagle_wind_hook);
  interp_bridge_set_pre_opcode_hook(0x88c333,drex_contact_hook);
  interp_bridge_set_pre_opcode_hook(0x88c35f,drex_contact_hook);
  const unsigned views[]={0x00dcd7,0x00dd2d,0x82807d,0x82809e,0x8280c3,0x838957};
  for(unsigned i=0;i<sizeof(views)/sizeof(views[0]);++i)
    interp_bridge_set_pre_opcode_hook(views[i],view_world_hook);
  interp_bridge_set_pre_opcode_hook(0x819c86,dash_effect_hook);
  interp_bridge_set_pre_opcode_hook(0x80f478,dash_effect_hook);
  interp_bridge_set_pre_opcode_hook(0x80f47c,dash_effect_hook);
  interp_bridge_set_pre_opcode_hook(0x87c0ae,eagle_lift_hook);
  interp_bridge_set_pre_opcode_hook(0x87c0b4,eagle_lift_hook);
  interp_bridge_set_pre_opcode_hook(0x82d7d7,lift_contact_hook);
  interp_bridge_set_pre_opcode_hook(0x87af10,kuwanger_lift_hook);
  interp_bridge_set_pre_opcode_hook(0x87af5c,kuwanger_lift_hook);
  interp_bridge_set_pre_opcode_hook(0x82c715,kuwanger_carry_hook);
  interp_bridge_set_pre_opcode_hook(0x82c733,kuwanger_carry_hook);
  const unsigned turrets[]={0x87b91c,0x87b92f,0x87ba72,0x87ba5c,0x87bb09,0x87bb0d};
  for(unsigned i=0;i<sizeof(turrets)/sizeof(turrets[0]);++i)
    interp_bridge_set_pre_opcode_hook(turrets[i],laser_target_hook);
  interp_bridge_set_pre_opcode_hook(0x889821,cart_hook);
  interp_bridge_set_pre_opcode_hook(0x889867,cart_hook);
  const unsigned platforms[]={0x84ab81,0x84ac34,0x84ab56,0x84ab80};
  for(unsigned i=0;i<sizeof(platforms)/sizeof(platforms[0]);++i)
    interp_bridge_set_pre_opcode_hook(platforms[i],platform_hook);
  interp_bridge_set_pre_opcode_hook(0x818136, controller_hook);
  interp_bridge_set_pre_opcode_hook(0x81819c, controller_hook);
  interp_bridge_set_pre_opcode_hook(0x848fcb, controller_hook);
  interp_bridge_set_pre_opcode_hook(0x8280df, controller_hook);
  const unsigned objects[] = {0xd2bd,0xd2dd,0xd3dd,0xd3f9,0xd3fa,0xd422,
      0xd43a,0xd456,0xd457,0xd47f,0x819d67,0x819d79};
  for (unsigned i=0;i<sizeof(objects)/sizeof(objects[0]);++i)
    interp_bridge_set_pre_opcode_hook(objects[i],object_hook);
  interp_bridge_set_pre_opcode_hook(0x83a934,slime_hook);
  interp_bridge_set_pre_opcode_hook(0x83a939,slime_hook);
  const unsigned contacts[] = {0x849b03,0x849b43,0x849b42,0x849b7d,0x849d82,
      0x849dc9,0x849dcc,0x849ee9};
  for (unsigned i=0;i<sizeof(contacts)/sizeof(contacts[0]);++i)
    interp_bridge_set_pre_opcode_hook(contacts[i],contact_hook);
  const unsigned cameras[]={0xdea0,0xdeab,0xdebf,0xdeca,0xe12d};
  for(unsigned i=0;i<sizeof(cameras)/sizeof(cameras[0]);++i)
    interp_bridge_set_pre_opcode_hook(cameras[i],camera_hook);
  interp_bridge_set_pre_opcode_hook(0x9e68,menu_hook);
  interp_bridge_set_pre_opcode_hook(0x9eac,menu_hook);
  interp_bridge_set_pre_opcode_hook(0xc579,menu_hook);
  interp_bridge_set_pre_opcode_hook(0xe57f,menu_hook);
  const unsigned pickups[]={0xd2e6,0xd2ed,0xd308,0xd31b,0x849c0e,0x849c15,0x849c1d,0x849d06};
  for(unsigned i=0;i<sizeof(pickups)/sizeof(pickups[0]);++i)
    interp_bridge_set_pre_opcode_hook(pickups[i],pickup_hook);
  const unsigned sensors[]={0x849c0e,0x849c15,0x849c1d,0x849d06};
  for(unsigned i=0;i<sizeof(sensors)/sizeof(sensors[0]);++i)
    interp_bridge_add_pre_opcode_hook(sensors[i],laser_contact_hook);
  const unsigned doors[]={0x81e70d,0x81e724,0x81e725,0x81ec98,0x81ecc6,0x81ecc7};
  for(unsigned i=0;i<sizeof(doors)/sizeof(doors[0]);++i)
    interp_bridge_set_pre_opcode_hook(doors[i],door_hook);
  const unsigned deaths[]={0x9d9e,0x9ac7,0x818a5c,0x818a78,0x818ab7,0x818acc,0x818b0b};
  for(unsigned i=0;i<sizeof(deaths)/sizeof(deaths[0]);++i)
    interp_bridge_set_pre_opcode_hook(deaths[i],death_hook);
}
