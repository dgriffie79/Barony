/*-------------------------------------------------------------------------------

	BARONY
	File: collision.c
	Desc: contains pure-C collision detection functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include "entity.h"

#include <math.h>

real_t entityDist(Entity* my, Entity* your)
{
	real_t dx, dy;
	dx = my->x - your->x;
	dy = my->y - your->y;
	return sqrt(dx * dx + dy * dy);
}

bool entityInsideEntity(Entity* entity1, Entity* entity2)
{
	if ( !entity1 || !entity2 ) { return false; }
	if ( entity1->x + entity1->sizex > entity2->x - entity2->sizex )
	{
		if ( entity1->x - entity1->sizex < entity2->x + entity2->sizex )
		{
			if ( entity1->y + entity1->sizey > entity2->y - entity2->sizey )
			{
				if ( entity1->y - entity1->sizey < entity2->y + entity2->sizey )
				{
					return true;
				}
			}
		}
	}
	return false;
}
