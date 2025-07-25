/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// server.h

#pragma once

#include "shared/shared.h"
#include "shared/list.h"
#include "shared/game.h"
#include "shared/gameext.h"

#include "common/bsp.h"
#include "common/cmd.h"
#include "common/cmodel.h"
#include "common/common.h"
#include "common/cvar.h"
#include "common/error.h"
#include "common/files.h"
#include "common/intreadwrite.h"
#include "common/msg.h"
#include "common/net/chan.h"
#include "common/net/net.h"
#include "common/pmove.h"
#include "common/prompt.h"
#include "common/protocol.h"
#include "common/zone.h"

#include "client/client.h"
#include "server/server.h"
#include "system/system.h"

#if USE_MVD_CLIENT
#include "server/mvd/client.h"
#endif

#if USE_ZLIB
#include <zlib.h>
#endif

//=============================================================================

#define SV_Malloc(size)         Z_TagMalloc(size, TAG_SERVER)
#define SV_Mallocz(size)        Z_TagMallocz(size, TAG_SERVER)
#define SV_CopyString(s)        Z_TagCopyString(s, TAG_SERVER)

#if USE_DEBUG
#define SV_DPrintf(level,...) \
    do { if (sv_debug && sv_debug->integer >= level) \
        Com_LPrintf(PRINT_DEVELOPER, __VA_ARGS__); } while (0)
#else
#define SV_DPrintf(...)
#endif

#define SV_BASELINES_SHIFT      6
#define SV_BASELINES_PER_CHUNK  (1 << SV_BASELINES_SHIFT)
#define SV_BASELINES_MASK       (SV_BASELINES_PER_CHUNK - 1)
#define SV_BASELINES_CHUNKS     (MAX_EDICTS >> SV_BASELINES_SHIFT)

#define SV_InfoSet(var, val) \
    Cvar_FullSet(var, val, CVAR_SERVERINFO|CVAR_ROM, FROM_CODE)

#if USE_CLIENT
#define SV_PAUSED (sv_paused->integer != 0)
#else
#define SV_PAUSED 0
#endif

#if USE_FPS
#define SV_GMF_VARIABLE_FPS GMF_VARIABLE_FPS
#else
#define SV_GMF_VARIABLE_FPS 0
#endif

// game features this server supports
#define SV_FEATURES (GMF_CLIENTNUM | GMF_PROPERINUSE | GMF_MVDSPEC | \
                     GMF_WANT_ALL_DISCONNECTS | GMF_ENHANCED_SAVEGAMES | \
                     SV_GMF_VARIABLE_FPS | GMF_EXTRA_USERINFO | \
                     GMF_IPV6_ADDRESS_AWARE | GMF_ALLOW_INDEX_OVERFLOW | \
                     GMF_PROTOCOL_EXTENSIONS)

// flag indicating if game uses new versions of gclient_t and pmove_t.
// doesn't enable protocol extensions by itself.
#define IS_NEW_GAME_API    (ge->apiversion == GAME_API_VERSION_NEW)

// ugly hack for SV_Shutdown
#define MVD_SPAWN_DISABLED  0
#define MVD_SPAWN_ENABLED   BIT(30)
#define MVD_SPAWN_INTERNAL  BIT(31)
#define MVD_SPAWN_MASK      (MVD_SPAWN_ENABLED | MVD_SPAWN_INTERNAL)

typedef struct {
    int         number;
    int         num_entities;
    unsigned    first_entity;
    player_packed_t ps;
    int         clientNum;
    int         areabytes;
    byte        areabits[MAX_MAP_AREA_BYTES];  // portalarea visibility bits
    unsigned    sentTime;                   // for ping calculations
    int         latency;
} client_frame_t;

typedef struct {
    int         solid32;

#if USE_FPS

// must be > MAX_FRAMEDIV
#define ENT_HISTORY_SIZE    8
#define ENT_HISTORY_MASK    (ENT_HISTORY_SIZE - 1)

    struct {
        vec3_t  origin;
        int     framenum;
    } history[ENT_HISTORY_SIZE];

    vec3_t      create_origin;
    int         create_framenum;
#endif
} server_entity_t;

// variable server FPS
#if USE_FPS
#define SV_FRAMERATE        sv.framerate
#define SV_FRAMETIME        sv.frametime.time
#define SV_FRAMEDIV         sv.frametime.div
#define SV_FRAMESYNC        !(sv.framenum % sv.frametime.div)
#define SV_CLIENTSYNC(cl)   !(sv.framenum % (cl)->framediv)
#else
#define SV_FRAMERATE        BASE_FRAMERATE
#define SV_FRAMETIME        BASE_FRAMETIME
#define SV_FRAMEDIV         1
#define SV_FRAMESYNC        1
#define SV_CLIENTSYNC(cl)   1
#endif

typedef struct {
    server_state_t  state;      // precache commands are only valid during load
    int             spawncount; // random number generated each server spawn
    bool            nextserver_pending;

#if USE_FPS
    int         framerate;
    frametime_t frametime;
#endif

    int         framenum;
    unsigned    frameresidual;

    char        mapcmd[MAX_QPATH];          // ie: *intro.cin+base

    char        name[MAX_QPATH];            // map name, or cinematic name
    cm_t        cm;

    configstring_t  configstrings[MAX_CONFIGSTRINGS];

    server_entity_t entities[MAX_EDICTS];
} server_t;

#define EDICT_NUM2(ge, n) ((edict_t *)((byte *)(ge)->edicts + (ge)->edict_size*(n)))
#define EDICT_NUM(n) EDICT_NUM2(ge, n)
#define NUM_FOR_EDICT(e) ((int)(((byte *)(e) - (byte *)ge->edicts) / ge->edict_size))

#define MAX_TOTAL_ENT_LEAFS        128

#define ENT_EXTENSION(csr, ent)  ((csr)->extended ? &(ent)->x : NULL)

typedef enum {
    cs_free,        // can be reused for a new connection
    cs_zombie,      // client has been disconnected, but don't reuse
                    // connection for a couple seconds
    cs_assigned,    // client_t assigned, but no data received from client yet
    cs_connected,   // netchan fully established, but not in game yet
    cs_primed,      // sent serverdata, client is precaching
    cs_spawned      // client is fully in game
} clstate_t;

#if USE_AC_SERVER

typedef enum {
    AC_NORMAL,
    AC_REQUIRED,
    AC_EXEMPT
} ac_required_t;

typedef enum {
    AC_QUERY_UNSENT,
    AC_QUERY_SENT,
    AC_QUERY_DONE
} ac_query_t;

#endif // USE_AC_SERVER

#define MSG_POOLSIZE        1024
#define MSG_TRESHOLD        (62 - sizeof(list_t))   // keep message_packet_t 64 bytes aligned

#define MSG_RELIABLE        BIT(0)
#define MSG_CLEAR           BIT(1)
#define MSG_COMPRESS        BIT(2)
#define MSG_COMPRESS_AUTO   BIT(3)

#define ZPACKET_HEADER      5

#define MAX_SOUND_PACKET    15
#define SOUND_PACKET        0       // special value for cursize

typedef struct {
    list_t              entry;
    uint16_t            cursize;    // zero means sound packet
    union {
        uint8_t         data[MSG_TRESHOLD];
        struct {
            uint16_t    index;
            uint16_t    sendchan;
            uint8_t     flags;
            uint8_t     volume;
            uint8_t     attenuation;
            uint8_t     timeofs;
            int32_t     pos[3];     // saved in case entity is freed
        };
    };
} message_packet_t;

#define RATE_MESSAGES   10

#define FOR_EACH_CLIENT(client) \
    LIST_FOR_EACH(client_t, client, &sv_clientlist, entry)

#define CLIENT_ACTIVE(cl) \
    ((cl)->state == cs_spawned && !(cl)->download && !(cl)->nodata)

#define PL_S2C(cl) (cl->frames_sent ? \
    (1.0f - (float)cl->frames_acked / cl->frames_sent) * 100.0f : 0.0f)
#define PL_C2S(cl) (cl->netchan.total_received ? \
    ((float)cl->netchan.total_dropped / cl->netchan.total_received) * 100.0f : 0.0f)
#define AVG_PING(cl) (cl->avg_ping_count ? \
    cl->avg_ping_time / cl->avg_ping_count : cl->ping)

typedef struct {
    unsigned    time;
    unsigned    credit;
    unsigned    credit_cap;
    unsigned    cost;
} ratelimit_t;

typedef struct client_s {
    list_t          entry;

    // core info
    clstate_t       state;
    edict_t         *edict;     // EDICT_NUM(clientnum+1)
    int             number;     // client slot number

    // client flags
    bool            reconnected: 1;
    bool            nodata: 1;
    bool            has_zlib: 1;
    bool            drop_hack: 1;
#if USE_ICMP
    bool            unreachable: 1;
#endif
    bool            http_download: 1;

    // userinfo
    char            userinfo[MAX_INFO_STRING];  // name, etc
    char            name[MAX_CLIENT_NAME];      // extracted from userinfo, high bits masked
    int             messagelevel;               // for filtering printed messages
    unsigned        rate;
    ratelimit_t     ratelimit_namechange;       // for suppressing "foo changed name" flood

    // console var probes
    char            *version_string;
    char            reconnect_var[16];
    char            reconnect_val[16];
    int             console_queries;

    // usercmd stuff
    unsigned        lastmessage;    // svs.realtime when packet was last received
    unsigned        lastactivity;   // svs.realtime when user activity was last seen
    int             lastframe;      // for delta compression
    usercmd_t       lastcmd;        // for filling in big drops
    int             command_msec;   // every seconds this is reset, if user
                                    // commands exhaust it, assume time cheating
    int             num_moves;      // reset every 10 seconds
    int             moves_per_sec;  // average movement FPS
    int             cmd_msec_used;
    float           timescale;

    int             ping, min_ping, max_ping;
    int             avg_ping_time, avg_ping_count;

    // frame encoding
    client_frame_t  frames[UPDATE_BACKUP];    // updates can be delta'd from here
    unsigned        frames_sent, frames_acked, frames_nodelta;
    int             framenum;
#if USE_FPS
    int             framediv;
#endif
    unsigned        frameflags;

    // rate dropping
    unsigned        message_size[RATE_MESSAGES];    // used to rate drop normal packets
    int             suppress_count;                 // number of messages rate suppressed
    unsigned        send_time, send_delta;          // used to rate drop async packets

    // current download
    byte            *download;      // file being downloaded
    int             downloadsize;   // total bytes (can't use EOF because of paks)
    int             downloadcount;  // bytes sent
    char            *downloadname;  // name of the file
    int             downloadcmd;    // svc_(z)download
    bool            downloadpending;

    // protocol stuff
    int             challenge;  // challenge of this user, randomly generated
    int             protocol;   // major version
    int             version;    // minor version
    int             settings[CLS_MAX];

    pmoveParams_t   pmp;        // spectator speed, etc
    msgEsFlags_t    esFlags;    // entity protocol flags
    msgPsFlags_t    psFlags;

    // packetized messages
    list_t              msg_free_list;
    list_t              msg_unreliable_list;
    list_t              msg_reliable_list;
    message_packet_t    *msg_pool;
    unsigned            msg_unreliable_bytes;   // total size of unreliable datagram
    unsigned            msg_dynamic_bytes;      // total size of dynamic memory allocated

    // per-client baseline chunks
    entity_packed_t     *baselines[SV_BASELINES_CHUNKS];

    // per-client packet entities
    unsigned            num_entities;   // UPDATE_BACKUP*MAX_PACKET_ENTITIES(_OLD)
    unsigned            next_entity;    // next state to use
    entity_packed_t     *entities;      // [num_entities]

    // server state pointers (hack for MVD channels implementation)
    const configstring_t    *configstrings;
    const cs_remap_t        *csr;
    const char              *gamedir, *mapname;
    const game_export_t     *ge;
    const cm_t              *cm;
    int                     infonum;    // slot number visible to client
    int                     spawncount;
    int                     maxclients;

    // netchan type dependent method
    void            (*AddMessage)(struct client_s *, const byte *, size_t, bool);

    // netchan
    netchan_t       netchan;
    int             numpackets; // for that nasty packetdup hack

    // misc
    time_t          connect_time; // time of initial connect

#if USE_AC_SERVER
    bool            ac_valid;
    ac_query_t      ac_query_sent;
    ac_required_t   ac_required;
    int             ac_file_failures;
    unsigned        ac_query_time;
    int             ac_client_type;
    string_entry_t  *ac_bad_files;
    char            *ac_token;
#endif
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quitting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define    MAX_CHALLENGES    1024

typedef struct {
    netadr_t    adr;
    unsigned    challenge;
    unsigned    time;
} challenge_t;

typedef struct {
    list_t      entry;
    netadr_t    addr;
    netadr_t    mask;
    unsigned    hits;
    time_t      time;   // time of the last hit
    char        comment[1];
} addrmatch_t;

typedef struct {
    list_t  entry;
    char    string[1];
} stuffcmd_t;

typedef enum {
    FA_IGNORE,
    FA_LOG,
    FA_PRINT,
    FA_STUFF,
    FA_KICK,

    FA_MAX
} filteraction_t;

typedef struct {
    list_t          entry;
    filteraction_t  action;
    char            *comment;
    char            string[1];
} filtercmd_t;

typedef struct {
    list_t          entry;
    filteraction_t  action;
    char            *var;
    char            *match;
    char            *comment;
} cvarban_t;

#define MAX_MASTERS         8       // max recipients for heartbeat packets
#define HEARTBEAT_SECONDS   300

typedef struct {
    netadr_t        adr;
    unsigned        last_ack;
    time_t          last_resolved;
    char            *name;
} master_t;

typedef struct {
    char            buffer[MAX_QPATH];  // original mapcmd
    char            server[MAX_QPATH];  // parsed map name
    char            *spawnpoint;
    server_state_t  state;
    int             loadgame;
    bool            endofunit;
    cm_t            cm;
} mapcmd_t;

typedef struct {
    bool        initialized;        // sv_init has completed
    unsigned    realtime;           // always increasing, no clamping, etc

    int         maxclients_soft;    // minus reserved slots
    int         maxclients;
    client_t    *client_pool;       // [maxclients]

#if USE_ZLIB
    z_stream        z;  // for compressing messages at once
    byte            *z_buffer;
    unsigned        z_buffer_size;
#endif

#if USE_SAVEGAMES
    int             gamedetecthack;
#endif

    cs_remap_t      csr;
    pmoveParams_t   pmp;

    unsigned        last_heartbeat;
    unsigned        last_timescale_check;

    unsigned        heartbeat_index;

    ratelimit_t     ratelimit_status;
    ratelimit_t     ratelimit_auth;
    ratelimit_t     ratelimit_rcon;

    challenge_t     challenges[MAX_CHALLENGES]; // to prevent invalid IPs from connecting
} server_static_t;

//=============================================================================

extern master_t     sv_masters[MAX_MASTERS];    // address of the master server

extern list_t       sv_banlist;
extern list_t       sv_blacklist;
extern list_t       sv_cmdlist_connect;
extern list_t       sv_cmdlist_begin;
extern list_t       sv_lrconlist;
extern list_t       sv_filterlist;
extern list_t       sv_cvarbanlist;
extern list_t       sv_infobanlist;
extern list_t       sv_clientlist;  // linked list of non-free clients

extern server_static_t      svs;        // persistent server info
extern server_t             sv;         // local server

extern cvar_t       *sv_hostname;
extern cvar_t       *sv_maxclients;
extern cvar_t       *sv_password;
extern cvar_t       *sv_reserved_slots;
extern cvar_t       *sv_airaccelerate;        // development tool
extern cvar_t       *sv_qwmod;                // atu QW Physics modificator
extern cvar_t       *sv_enforcetime;
#if USE_FPS
extern cvar_t       *sv_fps;
#endif
extern cvar_t       *sv_force_reconnect;
extern cvar_t       *sv_iplimit;

#if USE_DEBUG
extern cvar_t       *sv_debug;
extern cvar_t       *sv_pad_packets;
#endif
extern cvar_t       *sv_novis;
extern cvar_t       *sv_lan_force_rate;
extern cvar_t       *sv_calcpings_method;
extern cvar_t       *sv_changemapcmd;
extern cvar_t       *sv_max_download_size;
extern cvar_t       *sv_max_packet_entities;
extern cvar_t       *sv_trunc_packet_entities;
extern cvar_t       *sv_prioritize_entities;

extern cvar_t       *sv_strafejump_hack;
#if USE_PACKETDUP
extern cvar_t       *sv_packetdup_hack;
#endif
extern cvar_t       *sv_allow_map;
extern cvar_t       *sv_cinematics;
#if USE_SERVER
extern cvar_t       *sv_recycle;
#endif
extern cvar_t       *sv_enhanced_setplayer;

extern cvar_t       *sv_status_limit;
extern cvar_t       *sv_status_show;
extern cvar_t       *sv_auth_limit;
extern cvar_t       *sv_rcon_limit;
extern cvar_t       *sv_uptime;

extern cvar_t       *sv_allow_unconnected_cmds;

extern cvar_t       *g_features;

extern cvar_t       *sv_timeout;
extern cvar_t       *sv_zombietime;
extern cvar_t       *sv_ghostime;

extern client_t     *sv_client;
extern edict_t      *sv_player;


//===========================================================

//
// sv_main.c
//
void SV_DropClient(client_t *drop, const char *reason);
void SV_RemoveClient(client_t *client);
void SV_CleanClient(client_t *client);

void SV_InitOperatorCommands(void);

void SV_UserinfoChanged(client_t *cl);

bool SV_RateLimited(ratelimit_t *r);
void SV_RateRecharge(ratelimit_t *r);
void SV_RateInit(ratelimit_t *r, const char *s);

addrmatch_t *SV_MatchAddress(const list_t *list, const netadr_t *address);

int SV_CountClients(void);

#if USE_ZLIB
voidpf SV_zalloc(voidpf opaque, uInt items, uInt size);
void SV_zfree(voidpf opaque, voidpf address);
#endif

void sv_sec_timeout_changed(cvar_t *self);
void sv_min_timeout_changed(cvar_t *self);

//
// sv_init.c
//

void SV_ClientReset(client_t *client);
void SV_SetState(server_state_t state);
void SV_SpawnServer(const mapcmd_t *cmd);
bool SV_ParseMapCmd(mapcmd_t *cmd);
void SV_InitGame(unsigned mvd_spawn);

//
// sv_send.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET} redirect_t;
#define SV_OUTPUTBUF_LENGTH     (MAX_PACKETLEN_DEFAULT - 16)

#define SV_ClientRedirect() \
    Com_BeginRedirect(RD_CLIENT, sv_outputbuf, MAX_STRING_CHARS - 1, SV_FlushRedirect)

#define SV_PacketRedirect() \
    Com_BeginRedirect(RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect)

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect(int redirected, const char *outputbuf, size_t len);

void SV_SendClientMessages(void);
void SV_SendAsyncPackets(void);

void SV_Multicast(const vec3_t origin, multicast_t to);
void SV_ClientPrintf(client_t *cl, int level, const char *fmt, ...) q_printf(3, 4);
void SV_BroadcastPrintf(int level, const char *fmt, ...) q_printf(2, 3);
void SV_ClientCommand(client_t *cl, const char *fmt, ...) q_printf(2, 3);
void SV_BroadcastCommand(const char *fmt, ...) q_printf(1, 2);
void SV_ClientAddMessage(client_t *client, int flags);
void SV_ShutdownClientSend(client_t *client);
void SV_InitClientSend(client_t *newcl);

//
// sv_mvd.c
//
#if USE_MVD_SERVER
void SV_MvdRegister(void);
void SV_MvdPreInit(void);
void SV_MvdPostInit(void);
void SV_MvdShutdown(error_type_t type);
void SV_MvdBeginFrame(void);
void SV_MvdEndFrame(void);
void SV_MvdRunClients(void);
void SV_MvdStatus_f(void);
void SV_MvdMapChanged(void);
void SV_MvdClientDropped(client_t *client);

void SV_MvdUnicast(const edict_t *ent, int clientNum, bool reliable);
void SV_MvdMulticast(const mleaf_t *leaf, multicast_t to, bool reliable);
void SV_MvdConfigstring(int index, const char *string, size_t len);
void SV_MvdBroadcastPrint(int level, const char *string);
void SV_MvdStartSound(int entnum, int channel, int flags,
                      int soundindex, int volume,
                      int attenuation, int timeofs);

void SV_MvdRecord_f(void);
void SV_MvdStop_f(void);
#else
#define SV_MvdRegister()            (void)0
#define SV_MvdPreInit()             (void)0
#define SV_MvdPostInit()            (void)0
#define SV_MvdShutdown(type)        (void)0
#define SV_MvdBeginFrame()          (void)0
#define SV_MvdEndFrame()            (void)0
#define SV_MvdRunClients()          (void)0
#define SV_MvdStatus_f()            (void)0
#define SV_MvdMapChanged()          (void)0
#define SV_MvdClientDropped(client) (void)0

#define SV_MvdUnicast(ent, clientNum, reliable)     (void)0
#define SV_MvdMulticast(leafnum, to)                (void)0
#define SV_MvdConfigstring(index, string, len)      (void)0
#define SV_MvdBroadcastPrint(level, string)         (void)0
#define SV_MvdStartSound(entnum, channel, flags, \
                         soundindex, volume, \
                         attenuation, timeofs)      (void)0

#define SV_MvdRecord_f()    (void)0
#define SV_MvdStop_f()      (void)0
#endif

//
// sv_ac.c
//
#if USE_AC_SERVER
const char *AC_ClientConnect(client_t *cl);
void AC_ClientDisconnect(client_t *cl);
bool AC_ClientBegin(client_t *cl);
void AC_ClientAnnounce(client_t *cl);
void AC_ClientToken(client_t *cl, const char *token);

void AC_Register(void);
void AC_Disconnect(void);
void AC_Connect(unsigned mvd_spawn);
void AC_Run(void);

void AC_List_f(void);
void AC_Info_f(void);
#else
#define AC_ClientConnect(cl)        ""
#define AC_ClientDisconnect(cl)     (void)0
#define AC_ClientBegin(cl)          true
#define AC_ClientAnnounce(cl)       (void)0
#define AC_ClientToken(cl, token)   (void)0

#define AC_Register()               (void)0
#define AC_Disconnect()             (void)0
#define AC_Connect(mvd_spawn)       (void)0
#define AC_Run()                    (void)0

#define AC_List_f() \
    Com_Printf("This server does not support anticheat.\n")
#define AC_Info_f() \
    Com_Printf("This server does not support anticheat.\n")
#endif

//
// sv_user.c
//
void SV_New_f(void);
void SV_Begin_f(void);
void SV_ExecuteClientMessage(client_t *cl);
void SV_CloseDownload(client_t *client);
#if USE_FPS
void SV_AlignKeyFrames(client_t *client);
#else
#define SV_AlignKeyFrames(client) (void)0
#endif
cvarban_t *SV_CheckInfoBans(const char *info, bool match_only);

//
// sv_ccmds.c
//
#if USE_MVD_CLIENT || USE_MVD_SERVER
extern const cmd_option_t o_record[];
#endif

void SV_AddMatch_f(list_t *list);
void SV_DelMatch_f(list_t *list);
void SV_ListMatches_f(list_t *list);
client_t *SV_GetPlayer(const char *s, bool partial);
void SV_PrintMiscInfo(void);

//
// sv_ents.c
//

#define HAS_EFFECTS(ent) \
    ((ent)->s.modelindex || (ent)->s.effects || (ent)->s.sound || (ent)->s.event)

static inline void SV_CheckEntityNumber(edict_t *ent, int e, const char *func)
{
    if (q_unlikely(ent->s.number != e)) {
        Com_DWPrintf("%s: fixing ent->s.number: %d to %d\n", func, ent->s.number, e);
        ent->s.number = e;
    }
}

#define SV_CheckEntityNumber(ent, e) SV_CheckEntityNumber(ent, e, __func__)

void SV_BuildClientFrame(client_t *client);
bool SV_WriteFrameToClient_Default(client_t *client, unsigned maxsize);
bool SV_WriteFrameToClient_Enhanced(client_t *client, unsigned maxsize);

//
// sv_game.c
//
extern const game_export_t      *ge;
extern const game_export_ex_t   *gex;

void SV_InitGameProgs(void);
void SV_ShutdownGameProgs(void);

void PF_Pmove(void *pm);

//
// sv_save.c
//
#if USE_SAVEGAMES
void SV_AutoSaveBegin(const mapcmd_t *cmd);
void SV_AutoSaveEnd(void);
void SV_CheckForSavegame(const mapcmd_t *cmd);
void SV_CheckForEnhancedSavegames(void);
void SV_RegisterSavegames(void);
#else
#define SV_AutoSaveBegin(cmd)           (void)0
#define SV_AutoSaveEnd()                (void)0
#define SV_CheckForSavegame(cmd)        (void)0
#define SV_CheckForEnhancedSavegames()  (void)0
#define SV_RegisterSavegames()          (void)0
#endif

//
// ugly gclient_(old|new)_t accessors
//

static inline void SV_GetClient_ViewOrg(const client_t *client, vec3_t org)
{
    if (IS_NEW_GAME_API) {
        const gclient_new_t *cl = client->edict->client;
        VectorMA(cl->ps.viewoffset, 0.125f, cl->ps.pmove.origin, org);
    } else {
        const gclient_old_t *cl = client->edict->client;
        VectorMA(cl->ps.viewoffset, 0.125f, cl->ps.pmove.origin, org);
    }
}

static inline int SV_GetClient_ClientNum(const client_t *client)
{
    if (IS_NEW_GAME_API)
        return ((const gclient_new_t *)client->edict->client)->clientNum;
    else
        return ((const gclient_old_t *)client->edict->client)->clientNum;
}

static inline int SV_GetClient_Stat(const client_t *client, int stat)
{
    if (IS_NEW_GAME_API)
        return ((const gclient_new_t *)client->edict->client)->ps.stats[stat];
    else
        return ((const gclient_old_t *)client->edict->client)->ps.stats[stat];
}

static inline void SV_SetClient_Ping(const client_t *client, int ping)
{
    if (IS_NEW_GAME_API)
        ((gclient_new_t *)client->edict->client)->ping = ping;
    else
        ((gclient_old_t *)client->edict->client)->ping = ping;
}

//============================================================

//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld(void);
// called after the world model has been loaded, before linking any entities

void PF_UnlinkEdict(edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict(const cm_t *cm, edict_t *ent);
void PF_LinkEdict(edict_t *ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int SV_AreaEdicts(const vec3_t mins, const vec3_t maxs, edict_t **list, int maxcount, int areatype);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?

//===================================================================

//
// functions that interact with everything appropriate
//
int SV_PointContents(const vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids

trace_t q_gameabi SV_Trace(const vec3_t start, const vec3_t mins,
                           const vec3_t maxs, const vec3_t end,
                           edict_t *passedict, int contentmask);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)

trace_t q_gameabi SV_Clip(const vec3_t start, const vec3_t mins,
                          const vec3_t maxs, const vec3_t end,
                          edict_t *clip, int contentmask);
