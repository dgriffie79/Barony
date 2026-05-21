/*-------------------------------------------------------------------------------

	BARONY
	File: item_tool.c
	Desc: C functions for the tool category items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include "items.h"

bool itemIsThrowableTinkerTool(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( (item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)
		|| item->type == TOOL_DECOY
		|| item->type == TOOL_SENTRYBOT 
		|| item->type == TOOL_SPELLBOT 
		|| item->type == TOOL_GYROBOT
		|| item->type == TOOL_DUMMYBOT )
	{
		return true;
	}
	return false;
}
