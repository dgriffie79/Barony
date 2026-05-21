/*-------------------------------------------------------------------------------

	BARONY
	File: defines.cpp
	Desc: defines extern'd sound variables and stuff. This should really all be part of a class.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../defs.h"

Uint32 numsounds = 0;

#ifdef USE_FMOD
FMOD_SYSTEM* fmod_system = NULL;

FMOD_RESULT fmod_result;

int fmod_maxchannels = 256;
int fmod_flags;

FMOD_SOUND** sounds = NULL;
FMOD_SOUND** minesmusic = NULL;
FMOD_SOUND** swampmusic = NULL;
FMOD_SOUND** labyrinthmusic = NULL;
FMOD_SOUND** ruinsmusic = NULL;
FMOD_SOUND** underworldmusic = NULL;
FMOD_SOUND** hellmusic = NULL;
FMOD_SOUND** intromusic = NULL;
FMOD_SOUND** fortressmusic = NULL;
FMOD_SOUND* intermissionmusic = NULL;
FMOD_SOUND* minetownmusic = NULL;
FMOD_SOUND* splashmusic = NULL;
FMOD_SOUND* librarymusic = NULL;
FMOD_SOUND* shopmusic = NULL;
FMOD_SOUND* storymusic = NULL;
FMOD_SOUND** minotaurmusic = NULL;
FMOD_SOUND* herxmusic = NULL;
FMOD_SOUND* templemusic = NULL;
FMOD_SOUND* endgamemusic = NULL;
FMOD_SOUND* devilmusic = NULL;
FMOD_SOUND* escapemusic = NULL;
FMOD_SOUND* sanctummusic = NULL;
FMOD_SOUND* introductionmusic = NULL;
FMOD_SOUND** cavesmusic = NULL;
FMOD_SOUND** citadelmusic = NULL;
FMOD_SOUND* gnomishminesmusic = NULL;
FMOD_SOUND* greatcastlemusic = NULL;
FMOD_SOUND* sokobanmusic = NULL;
FMOD_SOUND* caveslairmusic = NULL;
FMOD_SOUND* bramscastlemusic = NULL;
FMOD_SOUND* hamletmusic = NULL;
FMOD_SOUND* tutorialmusic = NULL;
FMOD_SOUND* gameovermusic = NULL;
FMOD_SOUND* introstorymusic = NULL;
bool levelmusicplaying = false;

FMOD_CHANNEL* music_channel = NULL;
FMOD_CHANNEL* music_channel2 = NULL;
FMOD_CHANNEL* music_resume = NULL;

FMOD_CHANNELGROUP* sound_group = NULL;
FMOD_CHANNELGROUP* soundAmbient_group = NULL;
FMOD_CHANNELGROUP* soundEnvironment_group = NULL;
FMOD_CHANNELGROUP* soundNotification_group = NULL;
FMOD_CHANNELGROUP* music_group = NULL;
FMOD_CHANNELGROUP* music_notification_group = NULL;

FMOD_CHANNELGROUP* music_ensemble_global_send_group = NULL;
FMOD_CHANNELGROUP* music_ensemble_global_recv_group = NULL;
FMOD_CHANNELGROUP* music_ensemble_local_recv_player[MAXPLAYERS] = { NULL };
FMOD_CHANNELGROUP* music_ensemble_local_recv_group = NULL;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;
float dynamicAmbientVolume = 1.f;
float dynamicEnvironmentVolume = 1.f;
bool sfxUseDynamicAmbientVolume = true;
bool sfxUseDynamicEnvironmentVolume = true;

void* fmod_extraDriverData = NULL;
#endif //USE_FMOD

