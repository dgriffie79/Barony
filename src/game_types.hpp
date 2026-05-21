#pragma once
#include "defs.h"

#include <cstdint>
#include <cstddef>
#include <algorithm>

#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include <mutex>
#include <queue>
#include <tuple>

using std::string;

static const std::vector<std::string> impulseStrings =
{
	"IN_FORWARD",
	"IN_LEFT",
	"IN_BACK",
	"IN_RIGHT",
	"IN_TURNL",
	"IN_TURNR",
	"IN_UP",
	"IN_DOWN",
	"IN_CHAT",
	"IN_COMMAND",
	"IN_STATUS",
	"IN_SPELL_LIST",
	"IN_CAST_SPELL",
	"IN_DEFEND",
	"IN_ATTACK",
	"IN_USE",
	"IN_AUTOSORT",
	"IN_MINIMAPSCALE",
	"IN_TOGGLECHATLOG",
	"IN_FOLLOWERMENU",
	"IN_FOLLOWERMENU_LASTCMD",
	"IN_FOLLOWERMENU_CYCLENEXT",
	"IN_HOTBAR_SCROLL_LEFT",
	"IN_HOTBAR_SCROLL_RIGHT",
	"IN_HOTBAR_SCROLL_SELECT"
};

// C++-specific globals
extern std::unordered_map<SDL_Keycode, bool> keystatus;
extern std::vector<vec4_t> lightmaps[MAXPLAYERS + 1];
extern std::vector<vec4_t> lightmapsSmoothed[MAXPLAYERS + 1];
extern std::unordered_map<int, AnimatedTile> tileAnimations;
extern bool& enableDebugKeys;
extern string lastname;

const int NUM_JOY_STATUS = SDL_CONTROLLER_BUTTON_MAX;
const int NUM_JOY_AXIS_STATUS = SDL_CONTROLLER_AXIS_MAX;

template <typename T>
void list_RemoveNodeWithElement(list_t &list, T element)
{
	for ( node_t *node = list.first; node != nullptr; node = node->next )
	{
		if ( *static_cast<T*>(node->element) == element )
		{
			list_RemoveNode(node);
			return;
		}
	}
}

struct Language
{
	static const char* get(const int line);
	static std::map<int, std::string> entries;
	static std::map<int, std::string> tmpEntries;
	static void reset();
	static int loadLanguage(char const* const lang, bool forceLoadBaseDirectory);
	static int reloadLanguage();
	static std::string languageCode;
};

struct map_t
{
	char name[32];
	char author[32];
	unsigned int width, height, skybox;
	Sint32 flags[16];
	Sint32* tiles = nullptr;
	std::unordered_map<Sint32, node_t*> entities_map;
	list_t* entities = nullptr;
	list_t* creatures = nullptr;
	list_t* worldUI = nullptr;
	bool* trapexcludelocations = nullptr;
	bool* monsterexcludelocations = nullptr;
	bool* lootexcludelocations = nullptr;
	std::set<int> liquidSfxPlayedTiles;
	std::map<int, Uint32> tileAttributes;
	static const Uint32 TILE_ATTRIBUTE_NODIG = 1 << 0;
	static const Uint32 TILE_ATTRIBUTE_SLIPPERY = 1 << 1;
	static const Uint32 TILE_ATTRIBUTE_SLOW = 1 << 2;
	static const Uint32 TILE_ATTRIBUTE_GREASE = 1 << 3;
	static const Uint32 TILE_ATTRIBUTE_TREASURE_ROOM = 1 << 4;
	char filename[256];

	bool tileHasAttribute(int x, int y, int layer, Uint32 attribute);
	void setMapHDRSettings();

	~map_t()
	{
		if ( trapexcludelocations )
		{
			free(trapexcludelocations);
			trapexcludelocations = nullptr;
		}
		if ( monsterexcludelocations )
		{
			free(monsterexcludelocations);
			monsterexcludelocations = nullptr;
		}
		if ( lootexcludelocations )
		{
			free(lootexcludelocations);
			lootexcludelocations = nullptr;
		}
	}
};

extern map_t map;

std::string stackTrace();

inline int generateDungeon(char* levelset, Uint32 seed, std::tuple<int, int, int, int> mapParameters)
{
	return generateDungeon(levelset, seed,
		std::get<0>(mapParameters),
		std::get<1>(mapParameters),
		std::get<2>(mapParameters),
		std::get<3>(mapParameters));
}
