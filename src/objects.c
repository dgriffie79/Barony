/*-------------------------------------------------------------------------------

	BARONY
	File: objects.c
	Desc: C-compatible object constructors and deconstructors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include "light.h"

void defaultDeconstructor(void* data)
{
	if (data != NULL)
	{
		free(data);
	}
}

void stringDeconstructor(void* data)
{
	string_t* string;
	if (data != NULL)
	{
		string = (string_t*)data;
		if ( string->data != NULL )
		{
			free(string->data);
			string->data = NULL;
		}
		free(data);
	}
}

void emptyDeconstructor(void* data)
{
	return;
}

void listDeconstructor(void* data)
{
	list_t* list;

	if (data != NULL)
	{
		list = (list_t*)data;
		list_FreeAll(list);
		free(data);
	}
}

button_t* newButton(void)
{
	button_t* button;

	// allocate memory for button
	if ( (button = (button_t*) malloc(sizeof(button_t))) == NULL )
	{
		printlog( "failed to allocate memory for new button!\n" );
		exit(1);
	}

	// add the button to the button list
	button->node = list_AddNodeLast(&button_l);
	button->node->element = button;
	button->node->deconstructor = &defaultDeconstructor;
	button->node->size = sizeof(button_t);

	// now set all of my data elements to ZERO or NULL
	button->x = 0;
	button->y = 0;
	button->sizex = 0;
	button->sizey = 0;
	button->visible = 1;
	button->focused = 0;
	button->key = 0;
	button->joykey = -1;
	button->pressed = false;
	button->needclick = true;
	button->action = NULL;
	strcpy(button->label, "nodef");

	button->outline = false;

	return button;
}

light_t* newLight(int index, Sint32 x, Sint32 y, Sint32 radius)
{
	light_t* light;

	// allocate memory for light
	if ((light = (light_t*) malloc(sizeof(light_t))) == NULL) {
		printlog( "failed to allocate memory for new light!\n" );
		exit(1);
	}

	// add the light to the light list
	light->node = list_AddNodeLast(&light_l);
	light->node->element = light;
	light->node->deconstructor = &lightDeconstructor;
	light->node->size = sizeof(light_t);

	// now set all of my data elements to ZERO or NULL
	light->index = index;
	light->x = x;
	light->y = y;
	light->radius = radius;
	if (light->radius > 0) {
		const size_t size = sizeof(vec4_t) * (radius * 2 + 1) * (radius * 2 + 1);
		light->tiles = (vec4_t*)malloc(size);
		memset(light->tiles, 0, size);
	} else {
		light->tiles = NULL;
	}
	return light;
}
