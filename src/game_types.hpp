#pragma once
#include "defs.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <map>

using std::string;

// C++-specific globals
extern std::unordered_map<SDL_Keycode, bool> keystatus;
extern std::vector<vec4_t> lightmaps[MAXPLAYERS + 1];
extern std::vector<vec4_t> lightmapsSmoothed[MAXPLAYERS + 1];
extern std::unordered_map<int, AnimatedTile> tileAnimations;
extern bool& enableDebugKeys;
extern string lastname;

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
		if ( trapexcludelocations ) { free(trapexcludelocations); trapexcludelocations = nullptr; }
		if ( monsterexcludelocations ) { free(monsterexcludelocations); monsterexcludelocations = nullptr; }
		if ( lootexcludelocations ) { free(lootexcludelocations); lootexcludelocations = nullptr; }
	}
};

extern map_t map;

std::string stackTrace();
