/*-------------------------------------------------------------------------------

	BARONY
	File: objects.cpp
	Desc: contains object constructors and deconstructors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include <new>
#include "entity.h"
#include "messages.hpp"

/*-------------------------------------------------------------------------------

	entityDeconstructor

	Frees the memory occupied by a node pointing to an entity

-------------------------------------------------------------------------------*/

void entityDeconstructor(void* data)
{
	Entity* entity;

	if ( data != nullptr )
	{
		entity = (Entity*)data;

		//TODO: If I am part of the creaturelist, remove my node from that list.)

		//free(data);
		delete entity;
	}
}

/*-------------------------------------------------------------------------------

statDeconstructor

Frees the memory occupied by a node pointing to stat

-------------------------------------------------------------------------------*/

void statDeconstructor(void* data)
{
	Stat* stat;

	if ( data != nullptr )
	{
		stat = (Stat*)data;
		//free(data);
		delete stat;
	}
}

/*-------------------------------------------------------------------------------

	lightDeconstructor

	Frees the memory occupied by a node pointing to a light

-------------------------------------------------------------------------------*/

void lightDeconstructor(void* data)
{
	if (data != nullptr) {
        light_t* light = (light_t*)data;
		if (light->tiles != nullptr) {
            const auto lightsize = (light->radius * 2 + 1) * (light->radius * 2 + 1);
            const auto mapsize = map.width * map.height;
			for (int y = 0; y < light->radius * 2; y++) {
				for (int x = 0; x < light->radius * 2; x++) {
                    const auto soff = y + x * (light->radius * 2 + 1);
                    if (soff < 0 || soff >= lightsize) {
                        continue;
                    }
                    const auto& s = light->tiles[soff];
                    const auto doff = (y + light->y - light->radius) + (x + light->x - light->radius) * map.height;
                    if (doff < 0 || doff >= mapsize) {
                        continue;
                    }
                    if (light->index) {
                        auto& d = lightmaps[light->index][doff];
                        d.x -= s.x;
                        d.y -= s.y;
                        d.z -= s.z;
                        d.w -= s.w;
                    } else {
                        for (int c = 0; c < MAXPLAYERS + 1; ++c) {
                            auto& d = lightmaps[c][doff];
                            d.x -= s.x;
                            d.y -= s.y;
                            d.z -= s.z;
                            d.w -= s.w;
                        }
                    }
				}
			}
			free(light->tiles);
		}
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	mapDeconstructor

	Frees the memory occupied by a node pointing to a map

-------------------------------------------------------------------------------*/

void mapDeconstructor(void* data)
{
	map_t* map;

	if ( data != nullptr )
	{
		map = (map_t*)data;
		if ( map->tiles != nullptr )
		{
			free(map->tiles);
		}
		if ( map->creatures )
		{
			list_FreeAll(map->creatures); //TODO: This needed?
			delete map->creatures;
		}
		if ( map->entities != nullptr )
		{
			list_FreeAll(map->entities);
			free(map->entities);
		}
		if ( map->worldUI )
		{
			list_FreeAll(map->worldUI);
			delete map->worldUI;
		}
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	newEntity

	Creates a new entity with empty settings and places it in the entity list

-------------------------------------------------------------------------------*/

Entity* newEntity(Sint32 sprite, Uint32 pos, list_t* entlist, list_t* creaturelist)
{
	Entity* entity = nullptr;

	// allocate memory for entity
	/*if( (entity = (Entity *) malloc(sizeof(Entity)))==NULL ) {
		printlog( "failed to allocate memory for new entity!\n" );
		exit(1);
	}*/
	bool failedToAllocate = false;
	try
	{
		entity = new Entity(sprite, pos, entlist, creaturelist);
	}
	catch (std::bad_alloc& ba)
	{
		failedToAllocate = true;
	}

	if ( failedToAllocate || !entity )
	{
		printlog("failed to allocate memory for new entity!\n");
		exit(1);
	}

	return entity;
}

/*-------------------------------------------------------------------------------

	newString

	Creates a new string and places it in a list

-------------------------------------------------------------------------------*/

string_t* newString(list_t* list, Uint32 color, Uint32 time, int player, char const * const content, ...)
{
	string_t* string;
	char str[1024] = { 0 };
	va_list argptr;
	int c, i;

	// allocate memory for string
	if ( (string = (string_t*) malloc(sizeof(string_t))) == NULL )
	{
		printlog( "failed to allocate memory for new string!\n" );
		exit(1);
	}

	if ( content )
	{
		if ( strlen(content) > 2048 )
		{
			printlog( "error creating new string: buffer overflow.\n" );
			exit(1);
		}
	}

    string->time = time;
	string->color = color;
	string->lines = 1;
	string->player = player;
	if ( content != NULL )
	{
		if ( list && list == &messages )
		{
			std::string sanitizedStr = messageSanitizePercentSign(content, nullptr).c_str();
			const char* strPtr = sanitizedStr.c_str();
			// format the content
			va_start( argptr, strPtr);
			i = vsnprintf(str, 1023, strPtr, argptr);
			va_end( argptr );
		}
		else
		{
			// format the content
			va_start(argptr, content);
			i = vsnprintf(str, 1023, content, argptr);
			va_end(argptr);
		}
		string->data = (char*) malloc(sizeof(char) * (i + 1));
		if ( !string->data )
		{
			printlog( "error creating new string: couldn't allocate string data.\n" );
			exit(1);
		}
		memset(string->data, 0, sizeof(char) * (i + 1));
		for ( c = 0; c < i; c++ )
		{
			if ( str[c] == 10 )   // line feed
			{
				string->lines++;
			}
		}
		strncpy(string->data, str, i);
	}
	else
	{
		string->data = NULL;
	}

	// add the string to the list
	if ( list != NULL )
	{
		string->node = list_AddNodeLast(list);
		string->node->element = string;
		string->node->deconstructor = &stringDeconstructor;
		string->node->size = sizeof(string_t);
	}
	else
	{
		string->node = NULL;
	}

	return string;
}
