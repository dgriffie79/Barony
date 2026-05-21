/*-------------------------------------------------------------------------------

	BARONY
	File: mechanism.c
	Desc: C-compatible mechanism utility functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include "entity.h"

void getPowerablesOnTile(int x, int y, list_t** list)
{
	list_t* entities = NULL;
	entities = checkTileForEntity(x, y);

	if (!entities)
	{
		return;
	}

	node_t* node = NULL;
	node_t* node2 = NULL;
	for (node = entities->first; node != NULL; node = node->next)
	{
		if (node->element)
		{
			Entity* entity = (Entity*)node->element;
			if (entity && entity->skill[28])
			{
				if (!(*list))
				{
					*list = (list_t*)malloc(sizeof(list_t));
					(*list)->first = NULL;
					(*list)->last = NULL;
				}

				node2 = list_AddNodeLast(*list);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
		}
	}
}
