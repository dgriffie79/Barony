#pragma once
#include "Config.hpp"

// System includes (MUST be outside extern "C" to avoid corrupting
// GLEW function pointer types, SDL headers, and Windows API)
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#define GL_GLEXT_PROTOTYPES
#ifdef PATH_MAX
#undef PATH_MAX
#endif
#define PATH_MAX 1024
#include <windows.h>
#pragma warning ( push )
#pragma warning( disable : 4091 )
#include <Dbghelp.h>
#pragma warning( pop )
#undef min
#undef max
#endif

#define GL_GLEXT_PROTOTYPES
#ifdef WINDOWS
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#ifndef WINDOWS
#include <GL/glext.h>
#endif
#include "SDL_opengl.h"

#include "SDL.h"
#ifdef WINDOWS
#include "SDL_syswm.h"
#endif
#include "SDL_image.h"
#include "SDL_net.h"
#include "SDL_ttf.h"
#include "savepng.h"

#ifdef WINDOWS
#include <io.h>
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

#if _MSC_VER != 1900
#define snprintf _snprintf
#endif
#define access _access
#endif

#ifdef __arm__
typedef float real_t;
#else
typedef double real_t;
#endif

#include "physfs.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Entity;
typedef struct Entity Entity;

// safe string functions
char* stringCopy(char* dest, const char* src, size_t dest_size, size_t src_size);
char* stringCopyUnsafe(char* dest, const char* src, size_t dest_size);
char* stringCat(char* dest, const char* src, size_t dest_size, size_t src_size);
int stringCmp(const char* str1, const char* str2, size_t str1_size, size_t str2_size);
size_t stringLen(const char* str, size_t size);
char* stringStr(char* str1, const char* str2, size_t str1_size, size_t str2_size);

#define PI 3.14159265358979323846

// impulses
#define IN_FORWARD 0
#define IN_LEFT 1
#define IN_BACK 2
#define IN_RIGHT 3
#define IN_TURNL 4
#define IN_TURNR 5
#define IN_UP 6
#define IN_DOWN 7
#define IN_CHAT 8
#define IN_COMMAND 9
#define IN_STATUS 10
#define IN_SPELL_LIST 11
#define IN_CAST_SPELL 12
#define IN_DEFEND 13
#define IN_ATTACK 14
#define IN_USE 15
#define IN_AUTOSORT 16
#define IN_MINIMAPSCALE 17
#define IN_TOGGLECHATLOG 18
#define IN_FOLLOWERMENU 19
#define IN_FOLLOWERMENU_LASTCMD 20
#define IN_FOLLOWERMENU_CYCLENEXT 21
#define IN_HOTBAR_SCROLL_LEFT 22
#define IN_HOTBAR_SCROLL_RIGHT 23
#define IN_HOTBAR_SCROLL_SELECT 24
#define NUMIMPULSES 25

static const unsigned INJOY_STATUS = 0;
static const unsigned INJOY_SPELL_LIST = 1;
static const unsigned INJOY_PAUSE_MENU = 2;
static const unsigned INJOY_DPAD_LEFT = 3;
static const unsigned INJOY_DPAD_RIGHT = 4;
static const unsigned INJOY_DPAD_UP = 5;
static const unsigned INJOY_DPAD_DOWN = 6;
static const unsigned INJOY_MENU_LEFT_CLICK = 7;
static const unsigned INJOY_MENU_NEXT = 8;
static const unsigned INJOY_MENU_CANCEL = 9;
static const unsigned INJOY_MENU_SETTINGS_NEXT = 10;
static const unsigned INJOY_MENU_SETTINGS_PREV = 11;
static const unsigned INJOY_MENU_REFRESH_LOBBY = 12;
static const unsigned INJOY_MENU_DONT_LOAD_SAVE = 13;
static const unsigned INJOY_MENU_RANDOM_NAME = 14;
static const unsigned INJOY_MENU_RANDOM_CHAR = 15;
static const unsigned INJOY_MENU_INVENTORY_TAB = 16;
static const unsigned INJOY_MENU_MAGIC_TAB = 17;
static const unsigned INJOY_MENU_USE = 18;
static const unsigned INJOY_MENU_HOTBAR_CLEAR = 19;
static const unsigned INJOY_MENU_DROP_ITEM = 20;
static const unsigned INJOY_MENU_CHEST_GRAB_ALL = 21;
static const unsigned INJOY_MENU_CYCLE_SHOP_LEFT = 22;
static const unsigned INJOY_MENU_CYCLE_SHOP_RIGHT = 23;
static const unsigned INJOY_MENU_BOOK_PREV = 24;
static const unsigned INJOY_MENU_BOOK_NEXT = 25;
static const unsigned INDEX_JOYBINDINGS_START_MENU = 7;
static const unsigned INJOY_GAME_USE = 26;
static const unsigned INJOY_GAME_DEFEND = 27;
static const unsigned INJOY_GAME_ATTACK = 28;
static const unsigned INJOY_GAME_CAST_SPELL = 29;
static const unsigned INJOY_GAME_HOTBAR_ACTIVATE = 30;
static const unsigned INJOY_GAME_HOTBAR_PREV = 31;
static const unsigned INJOY_GAME_HOTBAR_NEXT = 32;
static const unsigned INJOY_GAME_MINIMAPSCALE = 33;
static const unsigned INJOY_GAME_TOGGLECHATLOG = 34;
static const unsigned INJOY_GAME_FOLLOWERMENU = 35;
static const unsigned INJOY_GAME_FOLLOWERMENU_LASTCMD = 36;
static const unsigned INJOY_GAME_FOLLOWERMENU_CYCLE = 37;
static const unsigned INDEX_JOYBINDINGS_START_GAME = 26;
static const unsigned NUM_JOY_IMPULSES = 38;
static const unsigned UNBOUND_JOYBINDING = 399;
static const int NUM_HOTBAR_CATEGORIES = 12;
static const int NUM_AUTOSORT_CATEGORIES = 12;
static const int RIGHT_CLICK_IMPULSE = 285;

#define SDL_BUTTON_WHEELUP 4
#define SDL_BUTTON_WHEELDOWN 5

#define indev_displaytime 7000

enum LightModifierValues
{
	GLOBAL_LIGHT_MODIFIER_STOPPED,
	GLOBAL_LIGHT_MODIFIER_INUSE,
	GLOBAL_LIGHT_MODIFIER_DISSIPATING
};

typedef enum ESteamStatTypes
{
	STEAM_STAT_INT = 0,
	STEAM_STAT_FLOAT = 1,
	STEAM_STAT_AVGRATE = 2,
} ESteamStatTypes;

typedef struct SteamStat_t
{
	int m_ID;
	ESteamStatTypes m_eStatType;
	const char *m_pchStatName;
	int m_iValue;
	float m_flValue;
	float m_flAvgNumerator;
	float m_flAvgDenominator;
} SteamStat_t;

typedef struct SteamGlobalStat_t
{
	int m_ID;
	ESteamStatTypes m_eStatType;
	const char *m_pchStatName;
	long long m_iValue;
	float m_flValue;
	float m_flAvgNumerator;
	float m_flAvgDenominator;
} SteamGlobalStat_t;

typedef struct node_t
{
	struct node_t* next;
	struct node_t* prev;
	struct list_t* list;
	void* element;
	void (*deconstructor)(void* data);
	Uint32 size;
} node_t;

typedef struct list_t
{
	node_t* first;
	node_t* last;
} list_t;

typedef struct button_t
{
	char label[32];
	Sint32 x, y;
	Uint32 sizex, sizey;
	Uint8 visible;
	Uint8 focused;
	SDL_Keycode key;
	int joykey;
	bool pressed;
	bool needclick;
	bool outline;
	node_t* node;
	void (*action)(struct button_t* my);
} button_t;

typedef struct voxel_t
{
	Sint32 sizex, sizey, sizez;
	Uint8* data;
	Uint8 palette[256][3];
} voxel_t;

typedef struct vertex_t
{
	real_t x, y, z;
} vertex_t;

typedef struct polyquad_t
{
	vertex_t vertex[4];
	Uint8 r, g, b;
	int side;
} polyquad_t;

typedef struct polytriangle_t
{
	vertex_t vertex[3];
	vertex_t normal;
	Uint8 r, g, b;
} polytriangle_t;

typedef struct polymodel_t
{
	polytriangle_t* faces;
	uint64_t numfaces;
	GLuint vao;
	GLuint positions;
	GLuint colors;
	GLuint normals;
} polymodel_t;

typedef struct string_t
{
	Uint32 lines;
	char* data;
	node_t* node;
	Uint32 color;
	Uint32 time;
	int player;
} string_t;

#ifdef __cplusplus
typedef struct door_t
{
	enum DoorDir : Sint32 { DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_NORTH };
	enum DoorEdge : Sint32 { EDGE_EAST, EDGE_SOUTHEAST, EDGE_SOUTH, EDGE_SOUTHWEST, EDGE_WEST, EDGE_NORTHWEST, EDGE_NORTH, EDGE_NORTHEAST };
	Sint32 x, y;
	DoorDir dir;
	DoorEdge edge;
} door_t;
#else
typedef struct door_t
{
	Sint32 x, y;
	Sint32 dir;
	Sint32 edge;
} door_t;
#define DIR_EAST 0
#define DIR_SOUTH 1
#define DIR_WEST 2
#define DIR_NORTH 3
#define EDGE_EAST 0
#define EDGE_SOUTHEAST 1
#define EDGE_SOUTH 2
#define EDGE_SOUTHWEST 3
#define EDGE_WEST 4
#define EDGE_NORTHWEST 5
#define EDGE_NORTH 6
#define EDGE_NORTHEAST 7
#endif

typedef struct deleteent_t
{
	Uint32 uid;
	Uint32 tries;
} deleteent_t;
#define MAXTRIES 6
#define MAXDELETES 2

#define HORIZONTAL 1
#define VERTICAL 2
typedef struct hit_t
{
	real_t x, y;
	int mapx, mapy;
	struct Entity* entity;
	int side;
} hit_t;

typedef struct cameravars_t
{
	real_t shakex;
	real_t shakex2;
	int shakey;
	int shakey2;
} cameravars_t;

#ifdef __cplusplus
struct vec4 {
	vec4(float f) : x(f), y(f), z(f), w(f) {}
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	vec4() = default;
	float x, y, z, w;
};
typedef vec4 vec4_t;

struct mat4x4 {
	mat4x4(float f) : x(f,0.f,0.f,0.f), y(0.f,f,0.f,0.f), z(0.f,0.f,f,0.f), w(0.f,0.f,0.f,f) {}
	mat4x4(float xx, float xy, float xz, float xw,
	       float yx, float yy, float yz, float yw,
	       float zx, float zy, float zz, float zw,
	       float wx, float wy, float wz, float ww) : x(xx,xy,xz,xw), y(yx,yy,yz,yw), z(zx,zy,zz,zw), w(wx,wy,wz,ww) {}
	mat4x4() : mat4x4(1.f) {}
	vec4_t x, y, z, w;
};
typedef mat4x4 mat4x4_t;
#else
typedef struct vec4 { float x, y, z, w; } vec4_t;
typedef struct mat4x4 { vec4_t x, y, z, w; } mat4x4_t;
#endif

typedef struct AnimatedTile
{
	int indices[8];
} AnimatedTile;

struct Language;

#ifdef __cplusplus
class Item;
#else
typedef struct Item Item;
#endif

typedef struct map_t map_t;

#define MAPLAYERS 3
#define OBSTACLELAYER 1
#define MAPFLAGS 16
#define MAPFLAGTEXTS 20
static const int MAP_FLAG_CEILINGTILE = 0;
static const int MAP_FLAG_DISABLETRAPS = 1;
static const int MAP_FLAG_DISABLEMONSTERS = 2;
static const int MAP_FLAG_DISABLELOOT = 3;
static const int MAP_FLAG_GENBYTES1 = 4;
static const int MAP_FLAG_GENBYTES2 = 5;
static const int MAP_FLAG_GENBYTES3 = 6;
static const int MAP_FLAG_GENBYTES4 = 7;
static const int MAP_FLAG_GENBYTES5 = 8;
static const int MAP_FLAG_GENBYTES6 = 9;
static const int MAP_FLAG_GENTOTALMIN = 4;
static const int MAP_FLAG_GENTOTALMAX = 5;
static const int MAP_FLAG_GENMONSTERMIN = 6;
static const int MAP_FLAG_GENMONSTERMAX = 7;
static const int MAP_FLAG_GENLOOTMIN = 8;
static const int MAP_FLAG_GENLOOTMAX = 9;
static const int MAP_FLAG_GENDECORATIONMIN = 10;
static const int MAP_FLAG_GENDECORATIONMAX = 11;
static const int MAP_FLAG_DISABLEDIGGING = 12;
static const int MAP_FLAG_DISABLETELEPORT = 13;
static const int MAP_FLAG_DISABLELEVITATION = 14;
static const int MAP_FLAG_GENADJACENTROOMS = 15;
static const int MAP_FLAG_DISABLEOPENING = 16;
static const int MAP_FLAG_DISABLEMESSAGES = 17;
static const int MAP_FLAG_DISABLEHUNGER = 18;
static const int MAP_FLAG_PERIMETER_GAP = 19;

#define MFLAG_DISABLEDIGGING ((map.flags[MAP_FLAG_GENBYTES3] >> 24) & 0xFF)
#define MFLAG_DISABLETELEPORT ((map.flags[MAP_FLAG_GENBYTES3] >> 16) & 0xFF)
#define MFLAG_DISABLELEVITATION ((map.flags[MAP_FLAG_GENBYTES3] >> 8) & 0xFF)
#define MFLAG_GENADJACENTROOMS ((map.flags[MAP_FLAG_GENBYTES3] >> 0) & 0xFF)
#define MFLAG_DISABLEOPENING ((map.flags[MAP_FLAG_GENBYTES4] >> 24) & 0xFF)
#define MFLAG_DISABLEMESSAGES ((map.flags[MAP_FLAG_GENBYTES4] >> 16) & 0xFF)
#define MFLAG_DISABLEHUNGER ((map.flags[MAP_FLAG_GENBYTES4] >> 8) & 0xFF)
#define MFLAG_PERIMETER_GAP ((map.flags[MAP_FLAG_GENBYTES4] >> 0) & 0xFF)

#define CLIPNEAR 2
#define CLIPFAR 4000
#define TEXTURESIZE 32
#define TEXTUREPOWER 5
#define MAXPLAYERS 4

#define SINGLE 0
#define SERVER 1
#define CLIENT 2
#define DIRECTSERVER 3
#define DIRECTCLIENT 4
#define SERVERCROSSPLAY 5
#define SPLITSCREEN 6

#define MAXTEXTURES 10240
#define MAXBUFFERS 256

#define AVERAGEFRAMES 32
#define LOCAL_ACHIEVEMENTS
#define VERTEX_ARRAYS_ENABLED

static const unsigned NUM_MOUSE_STATUS = 6;
static const int MINIMAP_MAX_DIMENSION = 512;

extern Sint32 display_id;
extern Sint32 xres;
extern Sint32 yres;
extern int mainloop;
extern bool initialized;
extern bool loading;
extern int game;
extern Uint32 ticks;
extern SDL_Window* screen;
extern SDL_GLContext renderer;
extern SDL_Event event;
extern bool firstmouseevent;
extern char const * window_title;
extern Sint32 fullscreen;
extern bool borderless;
extern bool smoothlighting;
extern Sint32 mousex, mousey;
extern Sint32 omousex, omousey;
extern Sint32 mousexrel, mouseyrel;
extern char* inputstr;
extern int inputlen;
extern bool fingerdown;
extern int fingerx;
extern int fingery;
extern int ofingerx;
extern int ofingery;
extern Sint8 mousestatus[NUM_MOUSE_STATUS];
extern Uint32 cursorflash;
extern Sint32 camx, camy;
extern Sint32 newcamx, newcamy;
extern int subwindow;
extern int subx1, subx2, suby1, suby2;
extern char subtext[1024];
extern int rscale;
extern real_t vidgamma;
extern bool verticalSync;
extern bool showStatusEffectIcons;
extern bool minimapPingMute;
extern bool mute_audio_on_focus_lost;
extern bool mute_player_monster_sounds;
extern int minimapTransparencyForeground;
extern int minimapTransparencyBackground;
extern int minimapScale;
extern int minimapObjectZoom;
extern Uint32 fov;
extern Uint32 fpsLimit;
extern cameravars_t cameravars[MAXPLAYERS];
extern Uint32 lastkeypressed;
extern SDL_bool EnableMouseCapture;
extern bool stop;
extern bool movie;
extern bool genmap;
extern char classtoquickstart[256];
extern bool splitscreen;
extern FILE* logfile;
extern list_t messages;
extern list_t command_history;
extern node_t* chosen_command;
extern bool command;
extern char command_str[128];
extern Sint32 multiplayer;
extern bool directConnect;
extern bool client_disconnected[MAXPLAYERS];
extern int minotaurlevel;
extern list_t ttfTextHash[256];
extern TTF_Font* ttf8;
extern TTF_Font* ttf12;
extern TTF_Font* ttf16;
extern SDL_Surface* font8x8_bmp;
extern SDL_Surface* font12x12_bmp;
extern SDL_Surface* font16x16_bmp;
extern SDL_Surface** sprites;
extern SDL_Surface** tiles;
extern voxel_t** models;
extern polymodel_t* polymodels;
extern bool useModelCache;
extern Uint32 imgref, vboref;
extern GLuint* texid;
extern bool disablevbos;
extern SDL_Surface** allsurfaces;
extern Uint32 numsprites;
extern Uint32 numtiles;
extern Uint32 nummodels;
extern Sint32 audio_rate, audio_channels, audio_buffers;
extern Uint16 audio_format;
extern real_t musvolume;
extern real_t sfxvolume;
extern real_t sfxAmbientVolume;
extern real_t sfxEnvironmentVolume;
extern real_t sfxNotificationVolume;
extern bool instrument_bg_enabled;
extern bool instrument_fg_enabled;
extern bool musicPreload;
extern bool* animatedtiles;
extern bool* swimmingtiles;
extern bool* lavatiles;
extern char tempstr[1024];
extern Sint8 minimap[MINIMAP_MAX_DIMENSION][MINIMAP_MAX_DIMENSION];
extern Uint32 mapseed;
extern bool* shoparea;
extern list_t button_l;
extern list_t light_l;
extern IPaddress net_server;
extern IPaddress* net_clients;
extern UDPsocket net_sock;
extern UDPpacket* net_packet;
extern TCPsocket net_tcpsock;
extern TCPsocket* net_tcpclients;
extern SDLNet_SocketSet tcpset;
extern hit_t hit;
extern list_t entitiesdeleted;
extern bool no_sound;
extern bool spamming;
extern bool showfirst;
extern bool logCheckObstacle;
extern int logCheckObstacleCount;
extern bool logCheckMainLoopTimers;
extern bool autoLimbReload;
extern bool ENABLE_STACK_TRACES;
extern const Uint32 ttfTextCacheLimit;

#define NUM_STEAM_STATISTICS 73
extern SteamStat_t g_SteamStats[NUM_STEAM_STATISTICS];
extern SteamGlobalStat_t g_SteamAPIGlobalStats[1];

#define TTF8_WIDTH 7
#define TTF8_HEIGHT 12
#define TTF12_WIDTH 9
#define TTF12_HEIGHT 16
#define TTF16_WIDTH 12
#define TTF16_HEIGHT 22

#define getSizeOfText(A, B, C, D) TTF_SizeUTF8(A, B, C, D)
#define getHeightOfFont(A) TTF_FontHeight(A)

#include "hash.h"

void printlog(const char* str, ...);
const char* gl_error_string(GLenum err);
#define GL_CHECK_ERR(expression) ({ \
    expression;\
    GLenum err;\
    while((err = glGetError()) != GL_NO_ERROR) {\
        printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
            err, gl_error_string(err));\
    }\
})
#define GL_CHECK_ERR_RET(expression) ({ \
    __typeof__(expression) retval = expression;\
    GLenum err;\
    while((err = glGetError()) != GL_NO_ERROR) {\
        printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
            err, gl_error_string(err));\
    }\
    retval;\
})

int sgn(real_t x);
int numdigits_sint16(Sint16 x);
int longestline(char const * const str);
int concatedStringLength(char* str, ...);

void list_FreeAll(list_t* list);
void list_RemoveNode(node_t* node);
node_t* list_AddNodeFirst(list_t* list);
node_t* list_AddNodeLast(list_t* list);
node_t* list_AddNode(list_t* list, int index);
Uint32 list_Size(list_t* list);
list_t* list_Copy(list_t* destlist, list_t* srclist);
list_t* list_CopyNew(list_t* srclist);
Uint32 list_Index(node_t* node);
node_t* list_Node(list_t* list, int index);

#ifdef __cplusplus
extern "C" {
#endif
void defaultDeconstructor(void* data);
void stringDeconstructor(void* data);
void emptyDeconstructor(void* data);
void listDeconstructor(void* data);
button_t* newButton(void);
#ifdef __cplusplus
}
#endif

void entityDeconstructor(void* data);
void statDeconstructor(void* data);
void lightDeconstructor(void* data);
void mapDeconstructor(void* data);
struct Entity* newEntity(Sint32 sprite, Uint32 pos, list_t* entlist, list_t* creaturelist);
string_t* newString(list_t* list, Uint32 color, Uint32 time, int player, char const * const content, ...);

SDL_Cursor* newCursor(char const * const image[]);

int generateDungeon(char* levelset, Uint32 seed,
    int secretChance, int darknessChance, int minotaurChance, int disableNormalExit);
void assignActions(map_t* map);

extern char const *cursor_pencil[];
extern char const *cursor_point[];
extern char const *cursor_brush[];
extern char const *cursor_fill[];

GLuint create_shader(const char* filename, GLenum type);

void GO_SwapBuffers(SDL_Window* screen);

void stackTraceUnique(void);
void finishStackTraceUnique(void);

time_t getTime(void);
void getTimeAndDate(time_t t, int* year, int* month, int* day, int* hour, int* min, int* second);
char* getTimeFormatted(time_t t, char* buf, size_t size);
char* getTimeAndDateFormatted(time_t t, char* buf, size_t size);

#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif

#ifdef __cplusplus
} /* extern "C" */

/* C++-only overloads (outside extern "C" so they get mangled names) */
const char* stringStr(const char* str1, const char* str2, size_t str1_size, size_t str2_size);

#endif
