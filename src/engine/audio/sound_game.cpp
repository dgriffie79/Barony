/*-------------------------------------------------------------------------------

	BARONY
	File: sound_game.cpp
	Desc: Contains all the code that will cause the editor to crash and burn.
	Quick workaround because I don't want to separate the editor and game
	into two separate projects just because of sound.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.h"
#include "../../game.hpp"
#include "../../stat.hpp"
#include "sound.hpp"
#include "../../entity.hpp"
#include "../../net.hpp"
#include "../../player.hpp"
#include "../../ui/GameUI.hpp"
#include "../../ui/MainMenu.hpp"
#include "../../mod_tools.hpp"
#include "../../ui/Button.hpp"

/*-------------------------------------------------------------------------------

	playSoundPlayer

	has the given player play the specified global sound effect. Mostly
	used by the server to instruct clients to play a certain sound.

-------------------------------------------------------------------------------*/
#ifdef USE_FMOD

FMOD_CHANNELGROUP* getChannelGroupForSoundIndex(Uint32 snd)
{
	if ( snd == 155 || snd == 135 ) // water/lava
	{
		return soundEnvironment_group;
	}
	if ( snd == 149 || snd == 133 )
	{
		return soundAmbient_group;
	}
	if ( SkillUpAnimation_t::soundIndexUsedForNotification(snd) )
	{
		return soundNotification_group;
	}
	return sound_group;
}

FMOD_CHANNEL* playSoundPlayer(int player, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSound(snd, vol);
	}
	else if ( multiplayer == SERVER && vol > 0 )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDG", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}

FMOD_CHANNEL* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return NULL;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSoundNotification(snd, vol);
	}
	else if ( multiplayer == SERVER && vol > 0 )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDN", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}

/*-------------------------------------------------------------------------------

	playSoundPos

	plays a sound effect with the given volume at the given
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	auto result = playSoundPosLocal(x, y, snd, vol);

	if (multiplayer == SERVER && vol > 0)
	{
		for (int c = 1; c < MAXPLAYERS; c++)
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			memcpy(net_packet->data, "SNDP", 4);
			SDLNet_Write32(x, &net_packet->data[4]);
			SDLNet_Write32(y, &net_packet->data[8]);
			SDLNet_Write16(snd, &net_packet->data[12]);
			net_packet->data[14] = vol;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return result;
}

FMOD_CHANNEL* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

#ifndef SOUND
	return NULL;
#endif

	FMOD_CHANNEL* channel;

	if (intro)
	{
		return NULL;
	}
	if (snd < 0 || snd >= numsounds)
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}

	if (!fmod_system)   //For the client.
	{
		return NULL;
	}

	FMOD_VECTOR position;
	position.x = (float)(x / (real_t)16.0);
	position.y = (float)(0.0);
	position.z = (float)(y / (real_t)16.0);

	if ( soundAmbient_group && getChannelGroupForSoundIndex(snd) == soundAmbient_group )
	{
		int numChannels = 0;
		FMOD_ChannelGroup_GetNumChannels(soundAmbient_group, &numChannels);
		for ( int i = 0; i < numChannels; ++i )
		{
			FMOD_CHANNEL* c;
			if ( FMOD_ChannelGroup_GetChannel(soundAmbient_group, i, &c) == FMOD_OK )
			{
				float audibility = 0.f;
				FMOD_Channel_GetAudibility(c, &audibility);
				float volume = 0.f;
				FMOD_Channel_GetVolume(c, &volume);
				FMOD_VECTOR playingPosition;
				FMOD_Channel_Get3DAttributes(c, &playingPosition, NULL);
				//printlog("Channel index: %d, audibility: %f, vol: %f, pos x: %.2f | y: %.2f", i, audibility, volume, playingPosition.z, playingPosition.x);
				if ( abs(volume - (vol / 255.f)) < 0.05 )
				{
					if ( (pow(playingPosition.x - position.x, 2) + pow(playingPosition.z - position.z, 2)) <= 2.25 )
					{
						//printlog("Culling sound due to proximity, pos x: %.2f | y: %.2f", position.z, position.x);
						return NULL;
					}
				}
			}
		}
	}

	if ( soundEnvironment_group && getChannelGroupForSoundIndex(snd) == soundEnvironment_group )
	{
		int numChannels = 0;
		FMOD_ChannelGroup_GetNumChannels(soundEnvironment_group, &numChannels);
		for ( int i = 0; i < numChannels; ++i )
		{
			FMOD_CHANNEL* c;
			if ( FMOD_ChannelGroup_GetChannel(soundEnvironment_group, i, &c) == FMOD_OK )
			{
				float audibility = 0.f;
				FMOD_Channel_GetAudibility(c, &audibility);
				float volume = 0.f;
				FMOD_Channel_GetVolume(c, &volume);
				FMOD_VECTOR playingPosition;
				FMOD_Channel_Get3DAttributes(c, &playingPosition, NULL);
				//printlog("Channel index: %d, audibility: %f, vol: %f, pos x: %.2f | y: %.2f", i, audibility, volume, playingPosition.z, playingPosition.x);
				if ( abs(volume - (vol / 255.f)) < 0.05 )
				{
					if ( (pow(playingPosition.x - position.x, 2) + pow(playingPosition.z - position.z, 2)) <= 4.5 )
					{
						//printlog("Culling sound due to proximity, pos x: %.2f | y: %.2f", position.z, position.x);
						return NULL;
					}
				}
			}
		}
	}

	/*FMOD_OPENSTATE openState;
	unsigned int percentBuffered = 0;
	bool starving = false;
	bool diskbusy = false;
	FMOD_Sound_GetOpenState(sounds[snd], &openState, &percentBuffered, &starving, &diskbusy);
	printlog("Sound: %d state: %d pc: %d starving: %d diskbusy: %d", snd, openState, percentBuffered, starving, diskbusy);*/
	fmod_result = FMOD_System_PlaySound(fmod_system, sounds[snd], getChannelGroupForSoundIndex(snd), true, &channel);
	if (FMODErrorCheck())
	{
		return NULL;
	}

	FMOD_Channel_SetVolume(channel, vol / 255.f);
	FMOD_Channel_Set3DAttributes(channel, &position, NULL);
    FMOD_Channel_SetMode(channel, FMOD_3D_WORLDRELATIVE);
	FMOD_Channel_SetPaused(channel, false);

	return channel;
}

/*-------------------------------------------------------------------------------

	playSoundEntity

	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPos(entity->x, entity->y, snd, vol);
}

FMOD_CHANNEL* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol)
{
	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

/*-------------------------------------------------------------------------------

	playSound

	plays a sound effect with the given volume and returns the channel that
	the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSound(Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}
#ifndef SOUND
	return NULL;
#endif
	if (!fmod_system || snd < 0 || snd >= numsounds || !getChannelGroupForSoundIndex(snd) )
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}
	FMOD_CHANNEL* channel = NULL;
	fmod_result = FMOD_System_PlaySound(fmod_system, sounds[snd], getChannelGroupForSoundIndex(snd), true, &channel);
    if (fmod_result == FMOD_OK && channel) {
        //Faux 3D. Set to 0 and then set the channel's mode to be relative  to the player's head to achieve global sound.
        FMOD_VECTOR position;
        position.x = 0.f;
        position.y = 0.f;
        position.z = 0.f;
        
        FMOD_Channel_SetVolume(channel, vol / 255.f);
        FMOD_Channel_Set3DAttributes(channel, &position, NULL);
        FMOD_Channel_SetMode(channel, FMOD_3D_HEADRELATIVE);
        
        if (FMODErrorCheck())
        {
            return NULL;
        }
        FMOD_Channel_SetPaused(channel, false);
    }
	return channel;
}

FMOD_CHANNEL* playSoundNotification(Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return NULL;
	}
#ifndef SOUND
	return NULL;
#endif
	if ( !fmod_system || snd < 0 || snd >= numsounds || !getChannelGroupForSoundIndex(snd) )
	{
		return NULL;
	}
	if ( sounds[snd] == NULL || vol == 0 )
	{
		return NULL;
	}
	FMOD_CHANNEL* channel;
	fmod_result = FMOD_System_PlaySound(fmod_system, sounds[snd], music_notification_group, true, &channel);
	//Faux 3D. Set to 0 and then set the channel's mode to be relative  to the player's head to achieve global sound.
	FMOD_VECTOR position;
	position.x = 0.f;
	position.y = 0.f;
	position.z = 0.f;

	FMOD_Channel_SetVolume(channel, vol / 255.f);
	FMOD_Channel_Set3DAttributes(channel, &position, NULL);
	FMOD_Channel_SetMode(channel, FMOD_3D_HEADRELATIVE);

	if ( FMODErrorCheck() )
	{
		return NULL;
	}
	FMOD_Channel_SetPaused(channel, false);
	return channel;
}


#else

/*-------------------------------------------------------------------------------

	playSoundEntity

	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/
void* playSound(Uint16 snd, Uint8 vol)
{
	return NULL;
}

void* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	int c;

	if (intro || vol == 0)
	{
		return NULL;
	}

	if (multiplayer == SERVER)
	{
		for (c = 1; c < MAXPLAYERS; c++)
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			memcpy(net_packet->data, "SNDP", 4);
			SDLNet_Write32(x, &net_packet->data[4]);
			SDLNet_Write32(y, &net_packet->data[8]);
			SDLNet_Write16(snd, &net_packet->data[12]);
			net_packet->data[14] = vol;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return NULL;
}

void* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	return NULL;
}

void* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol)
{
	if (entity == NULL)
	{
		return NULL;
	}
	return playSoundPos(entity->x, entity->y, snd, vol);
}

void* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol)
{
	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

void* playSoundPlayer(int player, Uint16 snd, Uint8 vol)
{
	int c;

	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSound(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDG", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}

void* playSoundNotification(Uint16 snd, Uint8 vol)
{
	return NULL;
}

void* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return NULL;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSoundNotification(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDN", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}
#endif

#ifdef USE_FMOD
VoiceChat_t VoiceChat;
VoiceChat_t::RingBuffer VoiceChat_t::ringBufferRecord(2048 * 20);
static ConsoleCommand ccmd_voice_init("/voice_init", "", 
	[](int argc, const char* argv[]) {
		VoiceChat.init();
});
static ConsoleCommand ccmd_voice_deinit("/voice_deinit", "",
	[](int argc, const char* argv[]) {
		VoiceChat.deinit();
	});

FMOD_RESULT F_CALLBACK pcmreadcallback(FMOD_SOUND* sound, void* data, unsigned int datalen)
{
	char* bitbuffer = (char*)data;
	if ( !sound ) { return FMOD_OK; }
	void* userData = NULL;
	fmod_result = FMOD_Sound_GetUserData((FMOD_SOUND*)sound, &userData);
	if ( FMODErrorCheck() ) { return FMOD_OK; }
	int player = reinterpret_cast<intptr_t>(userData);
	
	if ( !(player >= 0 && player < MAXPLAYERS) )
	{
		return FMOD_OK;
	}

	VoiceChat.PlayerChannels[player].audio_queue_mutex.lock();
    
	auto& audioQueue = VoiceChat.PlayerChannels[player].audioQueue;
	unsigned int bytesRead = std::min(datalen, (unsigned int)audioQueue.size());

	memcpy(bitbuffer, audioQueue.data(), bytesRead);
	audioQueue.erase(audioQueue.begin(), audioQueue.begin() + bytesRead);
	VoiceChat.PlayerChannels[player].totalSamplesRead += bytesRead;
	VoiceChat.PlayerChannels[player].updateLatency();

	VoiceChat.PlayerChannels[player].audio_queue_mutex.unlock();

	for ( unsigned int count = bytesRead; count < datalen; ++count )
	{
		bitbuffer[count] = 0;
	}
	return FMOD_OK;
}

void VoiceChat_t::PlayerChannels_t::updateLatency()
{
	int latency = (int)totalSamplesWritten - (int)totalSamplesRead;
	actualLatency = (int)((0.93f * actualLatency) + (0.03f * latency));

	if ( outputChannel )
	{
		int playbackRate = native_rate;
		if ( actualLatency < (int)(adjustedLatency - driftThreshold) )
		{
			playbackRate = native_rate - (int)(native_rate * (driftCorrectionPercentage / 100.0f));
		}
		else if ( actualLatency > (int)(adjustedLatency + driftThreshold) )
		{
			playbackRate = native_rate + (int)(native_rate * (driftCorrectionPercentage / 100.0f));
		}
		FMOD_Channel_SetFrequency(outputChannel, playbackRate);
	}
}

VoiceChat_t::VoicePlayerBarState VoiceChat_t::getVoiceState(const int player)
{
	int voice_no_send = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_SEND);
	int voice_pushtotalk = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_PTT);
	auto result = VOICE_STATE_NONE;
	if ( !(voice_no_send & (1 << player)) )
	{
		result = VOICE_STATE_INERT; //(voice_pushtotalk & (1 << player)) ? VOICE_STATE_INERT : VOICE_STATE_MUTE;
		if ( VoiceChat.PlayerChannels[player].talkingTicks > 0 
			|| VoiceChat.PlayerChannels[player].monitor_output_volume >= 0.05
			|| VoiceChat.PlayerChannels[player].lastAudibleTick > 0 )
		{
			if ( VoiceChat.PlayerChannels[player].monitor_output_volume < 0.05 )
			{
				if ( (voice_pushtotalk & (1 << player)) )
				{
					result = VOICE_STATE_INACTIVE_PTT;
				}
				else
				{
					if ( VoiceChat.PlayerChannels[player].lastAudibleTick > 0 )
					{
						result = VOICE_STATE_INACTIVE_PTT;
					}
					else
					{
						result = VOICE_STATE_INACTIVE;
					}
				}
			}
			else
			{
				result = (ticks % 50) > 25 ? VOICE_STATE_ACTIVE1 : VOICE_STATE_ACTIVE2;
			}
		}
	}
	return result;
}

VoiceChat_t::VoiceChat_t()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].player = i;
		PlayerChannels[i].audioQueue.reserve(PlayerChannels_t::audioQueueSizeLimit);
	}
}



void VoiceChat_t::init()
{
	if ( !useSystem ) { return; }
	if ( bInit ) { return; }

	nativeChannels = 1;
	nativeRate = BITRATE;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].native_rate = nativeRate;
		PlayerChannels[i].driftThreshold = (nativeRate * PlayerChannels[i].drift_ms) / 1000;       /* The point where we start compensating for drift */
		PlayerChannels[i].desiredLatency = (nativeRate * PlayerChannels[i].playback_latency_ms) / 1000;     /* User specified latency */
		PlayerChannels[i].adjustedLatency = PlayerChannels[i].desiredLatency;	 /* User specified latency adjusted for driver update granularity */
		PlayerChannels[i].actualLatency = PlayerChannels[i].desiredLatency;
	}

	recordingDesiredLatency = (nativeRate * recording_latency_ms) / 1000;
	recordingAdjustedLatency = (nativeRate * recording_latency_ms) / 1000;


	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].deinit();
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].setupPlayback();
	}

	if ( !loopbackPacket )
	{
		loopbackPacket = SDLNet_AllocPacket(NET_PACKET_SIZE);
	}



	bInit = true;
	logInfo("VoiceChat_t::init()");
}

void VoiceChat_t::setRecordingDevice(int device_index)
{
	deinitRecording(intro);
	recordDeviceIndex = device_index;
}

void VoiceChat_t::deinitRecording(bool resetPushTalkToggle)
{
	lastRecordTick = 0;
	recordingLastPos = 0;
	bIsRecording = false;
	bRecordingInit = false;
	fmod_result = FMOD_System_RecordStop(fmod_system, recordDeviceIndex);
	if ( recordingChannel )
	{
		fmod_result = FMOD_Channel_Stop(recordingChannel);
		recordingChannel = NULL;
	}
	if ( recordingSound )
	{
		FMOD_Sound_Release(recordingSound);
		recordingSound = NULL;
	}
	recordingSamples = 0;
	if ( resetPushTalkToggle )
	{
		voiceToggleTalk = false;
	}
	logInfo("VoiceChat_t::deinitRecording()");
}

void VoiceChat_t::initRecording()
{
	if ( initialized && !loading )
	{
		if ( recordingChannel )
		{
			fmod_result = FMOD_Channel_Stop(recordingChannel);
			recordingChannel = NULL;
		}
		if ( recordingSound )
		{
			FMOD_Sound_Release(recordingSound);
			recordingSound = NULL;
		}

		FMOD_CREATESOUNDEXINFO exinfo = { 0 };
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.numchannels = nativeChannels;
		exinfo.format = FMOD_SOUND_FORMAT_PCM16;
		exinfo.defaultfrequency = nativeRate;
		exinfo.length = nativeRate * sizeof(short) * nativeChannels; // 1 second buffer
		fmod_result = FMOD_System_CreateSound(fmod_system, 0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &recordingSound);
		if ( FMODErrorCheck() ) { return; }
		fmod_result = FMOD_Sound_GetLength(recordingSound, &recordingSoundLength, FMOD_TIMEUNIT_PCM);
		if ( FMODErrorCheck() ) { return; }

		int numDrivers = 0;
		fmod_result = FMOD_System_GetRecordNumDrivers(fmod_system, NULL, &numDrivers);
		if ( FMODErrorCheck() || numDrivers == 0 ) { return; }
		//fmod_result = FMOD_System_GetRecordDriverInfo(fmod_system, recordDeviceIndex, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
		//if ( FMODErrorCheck() ) { return; }
		fmod_result = FMOD_System_RecordStart(fmod_system, recordDeviceIndex, recordingSound, true);
		if ( FMODErrorCheck() ) { return; }
		bRecordingInit = true;
		logInfo("VoiceChat_t::initRecording()");
	}
}

void VoiceChat_t::PlayerChannels_t::deinit()
{
	audio_queue_mutex.lock();
	if ( outputChannel )
	{
		FMOD_Channel_Stop(outputChannel);
		outputChannel = NULL;
	}
	if ( outputSound )
	{
		FMOD_Sound_Release(outputSound);
		outputSound = NULL;
	}

	if ( VoiceChat.outChannelGroup )
	{
		FMOD_Sound_Release(VoiceChat.outChannelGroup);
		VoiceChat.outChannelGroup = NULL;
	}

	minimumSamplesWritten = -1;

	audio_queue_mutex.unlock();
	logInfo("VoiceChat_t::PlayerChannels_t::deinit()");
}

FMOD_REVERB_PROPERTIES reverbProperties;
void setReverbType(int type)
{
	switch ( type )
	{
	case 0:
		reverbProperties = FMOD_PRESET_OFF;
		break;
	case 1:
		reverbProperties = FMOD_PRESET_GENERIC;
		break;
	case 2:
		reverbProperties = FMOD_PRESET_PADDEDCELL;
		break;
	case 3:
		reverbProperties = FMOD_PRESET_ROOM;
		break;
	case 4:
		reverbProperties = FMOD_PRESET_BATHROOM;
		break;
	case 5:
		reverbProperties = FMOD_PRESET_LIVINGROOM;
		break;
	case 6:
		reverbProperties = FMOD_PRESET_STONEROOM;
		break;
	case 7:
		reverbProperties = FMOD_PRESET_AUDITORIUM;
		break;
	case 8:
		reverbProperties = FMOD_PRESET_CONCERTHALL;
		break;
	case 9:
		reverbProperties = FMOD_PRESET_CAVE;
		break;
	case 10:
		reverbProperties = FMOD_PRESET_ARENA;
		break;
	case 11:
		reverbProperties = FMOD_PRESET_HANGAR;
		break;
	case 12:
		reverbProperties = FMOD_PRESET_CARPETTEDHALLWAY;
		break;
	case 13:
		reverbProperties = FMOD_PRESET_HALLWAY;
		break;
	case 14:
		reverbProperties = FMOD_PRESET_STONECORRIDOR;
		break;
	case 15:
		reverbProperties = FMOD_PRESET_ALLEY;
		break;
	case 16:
		reverbProperties = FMOD_PRESET_FOREST;
		break;
	case 17:
		reverbProperties = FMOD_PRESET_CITY;
		break;
	case 18:
		reverbProperties = FMOD_PRESET_MOUNTAINS;
		break;
	case 19:
		reverbProperties = FMOD_PRESET_QUARRY;
		break;
	case 20:
		reverbProperties = FMOD_PRESET_PLAIN;
		break;
	case 21:
		reverbProperties = FMOD_PRESET_PARKINGLOT;
		break;
	case 22:
		reverbProperties = FMOD_PRESET_SEWERPIPE;
		break;
	case 23:
		reverbProperties = FMOD_PRESET_UNDERWATER;
		break;
	default:
		reverbProperties = FMOD_PRESET_OFF;
		break;
	}
}

void setReverbSettings(FMOD_DSP* dsp_reverb, int type, float wetLevel, float dryLevel)
{
	if ( !dsp_reverb ) { return; }
	setReverbType(type);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_DECAYTIME, reverbProperties.DecayTime);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_EARLYDELAY, reverbProperties.EarlyDelay);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_LATEDELAY, reverbProperties.LateDelay);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_HFREFERENCE, reverbProperties.HFReference);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_HFDECAYRATIO, reverbProperties.HFDecayRatio);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_DIFFUSION, reverbProperties.Diffusion);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_DENSITY, reverbProperties.Density);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, reverbProperties.LowShelfFrequency);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_LOWSHELFGAIN, reverbProperties.LowShelfGain);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_HIGHCUT, reverbProperties.HighCut);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_EARLYLATEMIX, reverbProperties.EarlyLateMix);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_WETLEVEL, reverbProperties.WetLevel + wetLevel);
	FMOD_DSP_SetParameterFloat(dsp_reverb, FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f + dryLevel);
	if ( type == 0 )
	{
		FMOD_DSP_SetBypass(dsp_reverb, true);
	}
	else
	{
		FMOD_DSP_SetBypass(dsp_reverb, false);
	}
}

void setEQSettings(FMOD_DSP* dsp_eq, float low, float mid, float high)
{
	if ( !dsp_eq ) { return; }
	FMOD_DSP_SetParameterFloat(dsp_eq, FMOD_DSP_THREE_EQ_LOWGAIN, low);
	FMOD_DSP_SetParameterFloat(dsp_eq, FMOD_DSP_THREE_EQ_MIDGAIN, mid);
	FMOD_DSP_SetParameterFloat(dsp_eq, FMOD_DSP_THREE_EQ_HIGHGAIN, high);
}

enum VoiceChat_t::DSPOrder : int
{
	DSPORDER_LIMITER = 1,
	DSPORDER_REVERB,
	DSPORDER_EQ,
	DSPORDER_FADER_LOCALGAIN,
	DSPORDER_FADER_CHANNELGAIN,
	DSPORDER_NORMALIZE,
	DSPORDER_END
};

static ConsoleCommand ccmd_voice_reverb("/voice_reverb", "",
	[](int argc, const char* argv[]) {
		if ( argc > 3 )
		{
			static int reverbType = -1;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD_DSP* dsp_reverb = NULL;
					fmod_result = FMOD_Channel_GetDSP(VoiceChat.PlayerChannels[i].outputChannel, VoiceChat_t::DSPORDER_REVERB, &dsp_reverb);
					if ( dsp_reverb )
					{
						FMOD_DSP_TYPE type;
						fmod_result = FMOD_DSP_GetType(dsp_reverb, &type);
						if ( type == FMOD_DSP_TYPE_SFXREVERB )
						{
							if ( atoi(argv[1]) < 0 )
							{
								++reverbType;
								if ( reverbType >= 24 )
								{
									reverbType = 0;
								}
								setReverbSettings(dsp_reverb, reverbType, atoi(argv[2]), atoi(argv[3]));
							}
							else
							{
								setReverbSettings(dsp_reverb, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
							}
						}
					}
				}
			}
		}
	});

static ConsoleCommand ccmd_voice_eq("/voice_eq", "",
	[](int argc, const char* argv[]) {
		if ( argc > 3 )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD_DSP* dsp_eq = NULL;
					fmod_result = FMOD_Channel_GetDSP(VoiceChat.PlayerChannels[i].outputChannel, VoiceChat_t::DSPORDER_EQ, &dsp_eq);
					if ( dsp_eq )
					{
						FMOD_DSP_TYPE type;
						fmod_result = FMOD_DSP_GetType(dsp_eq, &type);
						if ( type == FMOD_DSP_TYPE_THREE_EQ )
						{
							setEQSettings(dsp_eq, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
						}
					}
				}
			}
		}
	});

static ConsoleCommand ccmd_voice_limiter_release("/voice_limiter_release", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD_DSP* dsp_limiter = NULL;
					fmod_result = FMOD_Channel_GetDSP(VoiceChat.PlayerChannels[i].outputChannel, VoiceChat_t::DSPORDER_LIMITER, &dsp_limiter);
					if ( dsp_limiter )
					{
						FMOD_DSP_TYPE type;
						fmod_result = FMOD_DSP_GetType(dsp_limiter, &type);
						if ( type == FMOD_DSP_TYPE_LIMITER )
						{
							fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_RELEASETIME, atoi(argv[1]));
						}
					}
				}
			}
		}
	});

//void setPitchShiftSettings(FMOD_DSP* dsp, float pitch)
//{
//	if ( !dsp ) { return; }
//	FMOD_DSP_SetBypass(dsp, false);
//	FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_PITCHSHIFT_PITCH, pitch);
//}
//static ConsoleCommand ccmd_voice_pitchshift("/voice_pitchshift", "",
//	[](int argc, const char* argv[]) {
//		if ( argc > 1 )
//		{
//			for ( int i = 0; i < MAXPLAYERS; ++i )
//			{
//				if ( VoiceChat.PlayerChannels[i].outputChannel )
//				{
//					FMOD_DSP* dsp = NULL;
//					fmod_result = FMOD_Channel_GetDSP(VoiceChat.PlayerChannels[i].outputChannel, 4, &dsp);
//					if ( dsp )
//					{
//						FMOD_DSP_TYPE type;
//						fmod_result = FMOD_DSP_GetType(dsp, &type);
//						if ( type == FMOD_DSP_TYPE_PITCHSHIFT )
//						{
//							setPitchShiftSettings(dsp, atoi(argv[1]));
//						}
//					}
//				}
//			}
//		}
//	});
//void setChorusSettings(FMOD_DSP* dsp, float mix, float rate, float depth)
//{
//	if ( !dsp ) { return; }
//	FMOD_DSP_SetBypass(dsp, false);
//	FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_CHORUS_MIX, mix);
//	FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_CHORUS_RATE, rate);
//	FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_CHORUS_DEPTH, depth);
//}
//static ConsoleCommand ccmd_voice_chorus("/voice_chorus", "",
//	[](int argc, const char* argv[]) {
//		if ( argc > 3 )
//		{
//			for ( int i = 0; i < MAXPLAYERS; ++i )
//			{
//				if ( VoiceChat.PlayerChannels[i].outputChannel )
//				{
//					FMOD_DSP* dsp = NULL;
//					fmod_result = FMOD_Channel_GetDSP(VoiceChat.PlayerChannels[i].outputChannel, 5, &dsp);
//					if ( dsp )
//					{
//						FMOD_DSP_TYPE type;
//						fmod_result = FMOD_DSP_GetType(dsp, &type);
//						if ( type == FMOD_DSP_TYPE_CHORUS )
//						{
//							setChorusSettings(dsp, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
//						}
//					}
//				}
//			}
//		}
//	});

static ConsoleVariable<Vector4> cvar_voice_reverb_player("/voice_reverb_player", Vector4{ 0.f, 0.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_reverb_ghost("/voice_reverb_ghost", Vector4{ 9.f, -3.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_eq_player("/voice_eq_player", Vector4{ -3.f, 0.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_eq_ghost("/voice_eq_ghost", Vector4{ -6.f, -3.f, 0.f, 0.f });
static FMOD_VECTOR fmod_rolloff_points[6] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{6.f, 0.8f, 0.f},
	{14.f, 0.6f, 0.f},
	{30.f, 0.4f, 0.f},
	{64.f, 0.2f, 0.f}
};
static FMOD_VECTOR fmod_rolloff_points_arena[6] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{6.f, 0.9f, 0.f},
	{14.f, 0.8f, 0.f},
	{30.f, 0.7f, 0.f},
	{64.f, 0.5f, 0.f}
};

static FMOD_VECTOR fmod_rolloff_null_points[4] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{16.f, 0.1f, 0.f},
	{20.f, 0.0f, 0.f}
};

void VoiceChat_t::updateOnMapChange3DRolloff()
{
	bool bossMap = false;
	if ( !strcmp(map.name, "Hell Boss")
		|| !strcmp(map.name, "Sanctum")
		|| !strcmp(map.name, "Boss") )
	{
		bossMap = true;
	}


	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( VoiceChat.PlayerChannels[i].outputChannel )
		{
			if ( VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
			{
				fmod_result = FMOD_Channel_Set3DCustomRolloff(VoiceChat.PlayerChannels[i].outputChannel, bossMap ? fmod_rolloff_points_arena : fmod_rolloff_points, 6);
			}
			else
			{
				fmod_result = FMOD_Channel_Set3DCustomRolloff(VoiceChat.PlayerChannels[i].outputChannel, bossMap ? fmod_rolloff_points_arena : fmod_rolloff_null_points, 4);
			}
		}
	}
}

void VoiceChat_t::PlayerChannels_t::setupPlayback()
{
	if ( no_sound )
	{
		return;
	}
	const int windowSize = desiredLatency * sizeof(short) * VoiceChat.nativeChannels;
	FMOD_CREATESOUNDEXINFO exinfoBuffer = { 0 };
	exinfoBuffer.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfoBuffer.numchannels = VoiceChat.nativeChannels;
	exinfoBuffer.format = FMOD_SOUND_FORMAT_PCM16;
	exinfoBuffer.defaultfrequency = VoiceChat.nativeRate;
	exinfoBuffer.length = windowSize/* * sizeof(short) * nativeChannels*/;
	exinfoBuffer.pcmreadcallback = pcmreadcallback;
	exinfoBuffer.userdata = (void*)(intptr_t)(player);
	fmod_result = FMOD_System_CreateSound(fmod_system, NULL, FMOD_LOOP_NORMAL 
		| FMOD_OPENUSER 
		| FMOD_CREATESTREAM 
		| FMOD_3D
		| FMOD_3D_CUSTOMROLLOFF, &exinfoBuffer, &outputSound);

	if ( !VoiceChat.outChannelGroup )
	{
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &VoiceChat.outChannelGroup);
		if ( VoiceChat.outChannelGroup )
		{
			FMOD_ChannelGroup_SetVolume(VoiceChat.outChannelGroup, std::max(0.0f, MainMenu::master_volume));
		}
	}

	fmod_result = FMOD_System_PlaySound(fmod_system, outputSound, VoiceChat.outChannelGroup, false, &outputChannel);

	FMOD_Channel_SetPriority(outputChannel, 64); // default 128

	static ConsoleCommand ccmd_voice_rolloff("/voice_rolloff", "",
		[](int argc, const char* argv[]) {
			if ( argc > 3 )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( VoiceChat.PlayerChannels[i].outputChannel )
					{
						int index = atoi(argv[1]);
						if ( index >= 0 && index < 6 )
						{
							fmod_rolloff_points[index].x = atoi(argv[2]) / 100.f;
							fmod_rolloff_points[index].y = atoi(argv[3]) / 100.f;
						}
						FMOD_Channel_Set3DCustomRolloff(VoiceChat.PlayerChannels[i].outputChannel, fmod_rolloff_points, 6);
					}
				}
			}
			messagePlayer(clientnum, MESSAGE_HINT, "Rolloff values:\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}",
				fmod_rolloff_points[0].x, fmod_rolloff_points[0].y, fmod_rolloff_points[1].x, fmod_rolloff_points[1].y,
				fmod_rolloff_points[2].x, fmod_rolloff_points[2].y, fmod_rolloff_points[3].x, fmod_rolloff_points[3].y,
				fmod_rolloff_points[4].x, fmod_rolloff_points[4].y, fmod_rolloff_points[5].x, fmod_rolloff_points[5].y);
		}
	);
	if ( !VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
	{
		fmod_result = FMOD_Channel_Set3DCustomRolloff(outputChannel, fmod_rolloff_null_points, 4);
	}
	else
	{
		fmod_result = FMOD_Channel_Set3DCustomRolloff(outputChannel, fmod_rolloff_points, 6);
	}

	for ( int i = 1; i < DSPORDER_END; ++i )
	{
		if ( i == DSPORDER_REVERB )
		{
			FMOD_DSP* dsp_reverb = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_SFXREVERB, &dsp_reverb);
			if ( dsp_reverb )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_reverb);
				setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_reverb, true, true);
				}
			}
		}
		else if ( i == DSPORDER_EQ )
		{
			FMOD_DSP* dsp_eq = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_THREE_EQ, &dsp_eq);
			if ( dsp_eq )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_eq);
				setEQSettings(dsp_eq, cvar_voice_eq_player->x, cvar_voice_eq_player->y, cvar_voice_eq_player->z);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_eq, true, true);
				}
			}
		}
		else if ( i == DSPORDER_NORMALIZE )
		{
			FMOD_DSP* dsp_normalize = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_NORMALIZE, &dsp_normalize);
			if ( dsp_normalize )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_normalize);
				FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_FADETIME, kNormalizeFadeTime);
				FMOD_DSP_SetMeteringEnabled(dsp_normalize, true, true);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_normalize, true, true);
				}
			}
		}
		else if ( i == DSPORDER_LIMITER )
		{
			FMOD_DSP* dsp_limiter = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_LIMITER, &dsp_limiter);
			if ( dsp_limiter )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_limiter);
				fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_MAXIMIZERGAIN, 0.f);
				fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_CEILING, 0.f);
				fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_RELEASETIME, 1000.f);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_limiter, true, true);
				}
			}
		}
		else if ( i == DSPORDER_FADER_LOCALGAIN )
		{
			FMOD_DSP* dsp_local_fader = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_FADER, &dsp_local_fader);
			if ( dsp_local_fader )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_local_fader);
				fmod_result = FMOD_DSP_SetParameterFloat(dsp_local_fader, FMOD_DSP_FADER_GAIN, std::min(kMaxGain, (std::max(-80.f, (localChannelGain - 100.f)))));
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_local_fader, true, true);
				}
			}
		}
		else if ( i == DSPORDER_FADER_CHANNELGAIN )
		{
			FMOD_DSP* dsp_fader = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_FADER, &dsp_fader);
			if ( dsp_fader )
			{
				fmod_result = FMOD_Channel_AddDSP(outputChannel, i, dsp_fader);
				fmod_result = FMOD_DSP_SetParameterFloat(dsp_fader, FMOD_DSP_FADER_GAIN, (std::min(kMaxGain, std::max(-80.f, (channelGain - 100.f)))));
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					FMOD_DSP_SetMeteringEnabled(dsp_fader, true, true);
				}
			}
		}
	}


	/*FMOD_DSP* dsp_misc = NULL;
	fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_PITCHSHIFT, &dsp_misc);
	if ( dsp_misc )
	{
		FMOD_Channel_AddDSP(outputChannel, 4, dsp_misc);
		FMOD_DSP_SetBypass(dsp_misc, true);
	}

	dsp_misc = NULL;
	fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_CHORUS, &dsp_misc);
	if ( dsp_misc )
	{
		FMOD_Channel_AddDSP(outputChannel, 5, dsp_misc);
		FMOD_DSP_SetBypass(dsp_misc, true);
	}*/

	logInfo("VoiceChat_t::PlayerChannels_t::setupPlayback()");
}

static ConsoleCommand ccmd_voice_encoding("/voice_encoding", "",
	[](int argc, const char* argv[]) {
		VoiceChat.using_encoding = !VoiceChat.using_encoding;
		messagePlayer(clientnum, MESSAGE_HINT, "Voice encoding set to %d", VoiceChat.using_encoding);
	});

void VoiceChat_t::pushAvailableDatagrams()
{
	if ( using_encoding )
	{

	}
	else
	{
		while ( ringBufferRecord.GetReadAvail() )
		{
			recordingDatagrams.push_back(std::vector<char>(std::min(FRAME_SIZE, ringBufferRecord.GetReadAvail())));
			auto& back = recordingDatagrams.back();
			ringBufferRecord.Read(back.data(), back.size());
		}
	}
}

bool VoiceChat_t::mainMenuAudioTabOpen()
{
	if ( MainMenu::main_menu_frame )
	{
		if ( auto frame = MainMenu::main_menu_frame->findFrame("settings") )
		{
			if ( auto settings_subwindow = frame->findFrame("settings_subwindow") )
			{
				if ( auto slider = settings_subwindow->findSlider("setting_master_volume_slider") )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool VoiceChat_t::getAudioSettingBool(VoiceChat_t::AudioSettingBool option)
{
	AudioSettings_t* setting = &activeSettings;
	if ( MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("settings") )
	{
		setting = &mainmenuSettings;
	}

	if ( !setting ) { return false; }

	switch ( option )
	{
	case AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_INPUT:
		return setting->enable_voice_input;
		break;
	case AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_RECEIVE:
		return setting->enable_voice_receive;
		break;
	case AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD:
		return setting->loopback_local_record;
		break;
	case AudioSettingBool::VOICE_SETTING_PUSHTOTALK:
		return setting->pushToTalk;
		break;
	case AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF:
		return setting->use_custom_rolloff;
		break;
	default:
		break;
	}
	return false;
}
float VoiceChat_t::getAudioSettingFloat(VoiceChat_t::AudioSettingFloat option)
{
	AudioSettings_t* setting = &activeSettings;
	if ( MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("settings") )
	{
		setting = &mainmenuSettings;
	}

	if ( !setting ) { return 0.f; }

	switch ( option )
	{
	case AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN:
		return setting->recordingGain;
		break;
	case AudioSettingFloat::VOICE_SETTING_VOICE_GLOBAL_VOLUME:
		return setting->voice_global_volume;
		break;
	case AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD:
		return setting->recordingNormalizeThreshold;
		break;
	case AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP:
		return setting->recordingNormalizeAmp;
		break;
	default:
		break;
	}
	return 0.f;
}

const char* VoiceChat_t::getVoiceChatBindingName(int player)
{
	return "Voice Chat";
	//if ( player < 0 || player >= MAXPLAYERS ) { return ""; }

	////bool pauseMenuSlidersAvailable = false;
	////if ( !intro && gamePaused && MainMenu::main_menu_frame )
	////{
	////	auto selectedWidget = MainMenu::main_menu_frame->findSelectedWidget(MainMenu::getMenuOwner());
	////	if ( selectedWidget )
	////	{
	////		if ( !strcmp(selectedWidget->getName(), "pause player status audio") )
	////		{
	////			pauseMenuSlidersAvailable = true;
	////		}
	////		else if ( !strcmp(selectedWidget->getName(), "audio slider") )
	////		{
	////			pauseMenuSlidersAvailable = true;
	////		}
	////		else
	////		{
	////			auto& actions = selectedWidget->getWidgetActions();
	////			auto find = actions.find("MenuPageLeft");
	////			if ( find != actions.end() && find->second == "pause player status audio" )
	////			{
	////				if ( auto pauseAudioSliders = MainMenu::main_menu_frame->findFrame("pause player status audio") )
	////				{
	////					pauseMenuSlidersAvailable = true;
	////				}
	////			}
	////		}
	////	}
	////}

	//const char* voiceBindingName = inputs.bPlayerUsingKeyboardControl(player) && !inputs.hasController(player) ? "Voice Chat" : "Voice Chat Gamepad";
	///*if ( inputs.hasController(clientnum) && (intro || pauseMenuSlidersAvailable) )
	//{
	//	voiceBindingName = "Voice Chat (Pause Menu)";
	//}*/
	//return voiceBindingName;
}

ConsoleVariable<bool> cvar_voice_debug("/voice_debug", false);
void VoiceChat_t::updateRecording()
{
	allowInputs = false;
	pushAvailableDatagrams();

	auto& input = Input::inputs[clientnum];
	const char* voiceBindingName = getVoiceChatBindingName(clientnum);

	if ( (multiplayer == SINGLE && !*cvar_voice_debug)
		|| !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_INPUT)
		|| (intro && (!net_packet 
			|| (MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("lobby") && !MainMenu::isPlayerSignedIn(clientnum))))
		|| input.input(voiceBindingName).type == Input::binding_t::INVALID )
	{
		// record on the main menu settings tab to view levels
		if ( !(mainMenuAudioTabOpen()) )
		{
			if ( bRecordingInit )
			{
				deinitRecording(intro);
			}
			return;
		}
	}

	if ( initialized && !loading && !bRecordingInit )
	{
		initRecording();
	}

	fmod_result = FMOD_System_IsRecording(fmod_system, recordDeviceIndex, &bIsRecording);
	if ( (fmod_result != FMOD_ERR_RECORD_DISCONNECTED && fmod_result != FMOD_OK)
		|| !bIsRecording )
	{
		FMODErrorCheck();
		return;
	}

	bool pauseMenuSlidersAvailable = false;
	if ( !intro && gamePaused && MainMenu::main_menu_frame )
	{
		auto selectedWidget = MainMenu::main_menu_frame->findSelectedWidget(MainMenu::getMenuOwner());
		if ( selectedWidget )
		{
			if ( !strcmp(selectedWidget->getName(), "pause player status audio") )
			{
				pauseMenuSlidersAvailable = true;
			}
			else if ( !strcmp(selectedWidget->getName(), "audio slider") )
			{
				pauseMenuSlidersAvailable = true;
			}
			else
			{
				auto& actions = selectedWidget->getWidgetActions();
				auto find = actions.find("MenuPageLeft");
				if ( find != actions.end() && find->second == "pause player status audio" )
				{
					if ( auto pauseAudioSliders = MainMenu::main_menu_frame->findFrame("pause player status audio") )
					{
						pauseMenuSlidersAvailable = true;
					}
				}
			}
		}
	}

	int voice_no_send = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_SEND);
	if ( (players[clientnum]->bControlEnabled || intro)
		&& !players[clientnum]->usingCommand()
		&& !inputstr
		&& !(voice_no_send & (1 << clientnum))
		&& (!gamePaused || (gamePaused && (MainMenu::isCutsceneActive() || movie || pauseMenuSlidersAvailable))) )
	{
		allowInputs = true;
	}

	bool doRecord = false;

	if ( mainMenuAudioTabOpen() )
	{
		if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) )
		{
			doRecord = true;
		}
	}

	if ( lastRecordTick > 0 )
	{
		--lastRecordTick;
	}

	if ( allowInputs )
	{
		if ( lastRecordTick > 0 )
		{
			doRecord = true; // give x ms of letting go of record to continue
		}
		if ( !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.consumeBinaryToggle(voiceBindingName) )
			{
				voiceToggleTalk = !voiceToggleTalk;
				if ( voiceToggleTalk )
				{
					//messagePlayer(clientnum, MESSAGE_INTERACTION, Language::get(6449));
					Player::soundActivate();
					//playSound(710, 64);
					doRecord = true;
				}
				else
				{
					//messagePlayer(clientnum, MESSAGE_INTERACTION, Language::get(6450));
					Player::soundCancel();
					//playSound(711, 64);
				}
			}
		}
		else if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.binaryToggle(voiceBindingName) )
			{
				doRecord = true;
				if ( lastRecordTick == 0 )
				{
					//playSound(710, 64);
					//Player::soundActivate();
				}
				lastRecordTick = TICKS_PER_SECOND / 4;
			}
			else
			{
				if ( lastRecordTick == 1 )
				{
					//playSound(711, 64);
				}
			}
		}
	}
	else
	{
		if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.binaryToggle(voiceBindingName) )
			{
				input.consumeBinaryToggle(voiceBindingName);
			}
		}
	}

	if ( !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
	{
		if ( voiceToggleTalk )
		{
			doRecord = true;
			lastRecordTick = TICKS_PER_SECOND / 4;
		}
	}

	if ( doRecord )
	{
		PlayerChannels[clientnum].talkingTicks = TICKS_PER_SECOND / 4;
	}

	if ( true )
	{
		unsigned int recordPos = 0;
		fmod_result = FMOD_System_GetRecordPosition(fmod_system, recordDeviceIndex, &recordPos);
		if ( fmod_result != FMOD_ERR_RECORD_DISCONNECTED )
		{
			if ( FMODErrorCheck() ) { return; }
		}

		unsigned int recordDelta = (recordPos >= recordingLastPos) ? (recordPos - recordingLastPos) : (recordPos + recordingSoundLength - recordingLastPos);
		recordingSamples += recordDelta;

		static unsigned int minRecordDelta = (unsigned int)-1;
		if ( recordDelta && (recordDelta < minRecordDelta) )
		{
			minRecordDelta = recordDelta; /* Smallest driver granularity seen so far */
			recordingAdjustedLatency = (recordDelta <= recordingDesiredLatency) ? recordingDesiredLatency : recordDelta; /* Adjust our latency if driver granularity is high */
		}

		/*
			Delay playback until our desired latency is reached.
		*/
		if ( recordingSamples >= recordingAdjustedLatency && !loading )
		{
			if ( !recordingChannel )
			{
				fmod_result = FMOD_System_PlaySound(fmod_system, recordingSound, 0, false, &recordingChannel);
				if ( FMODErrorCheck() ) { return; }
				//FMOD_Channel_SetVolume(recordingChannel, 0.f);
				if ( FMODErrorCheck() ) { return; }

				FMOD_DSP* dsp_fader = NULL;
				fmod_result = FMOD_Channel_GetDSP(recordingChannel, 0, &dsp_fader);
				if ( dsp_fader )
				{
					FMOD_DSP_SetParameterFloat(dsp_fader, FMOD_DSP_FADER_GAIN, -79.9);
				}

				FMOD_DSP* dsp_limiter = NULL;
				fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_LIMITER, &dsp_limiter);
				if ( dsp_limiter )
				{
					FMOD_Channel_AddDSP(recordingChannel, 1, dsp_limiter);
					//fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_MAXIMIZERGAIN, (std::max(0.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f))));
					//FMOD_DSP_SetMeteringEnabled(dsp_limiter, true, true);
					fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_MAXIMIZERGAIN, 0.f);
					fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_CEILING, 0.f);
					fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter, FMOD_DSP_LIMITER_RELEASETIME, 1000.f);
				}

				fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_FADER, &dsp_fader);
				if ( dsp_fader )
				{
					FMOD_Channel_AddDSP(recordingChannel, 2, dsp_fader);
					FMOD_DSP_SetChannelFormat(dsp_fader, 0, nativeChannels, FMOD_SPEAKERMODE_MONO);
					fmod_result = FMOD_DSP_SetParameterFloat(dsp_fader, FMOD_DSP_FADER_GAIN, std::min(kMaxGain, 
						getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f));
					FMOD_DSP_SetMeteringEnabled(dsp_fader, true, true);
				}

				FMOD_DSP* dsp_normalize = NULL;
				fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_NORMALIZE, &dsp_normalize);
				if ( dsp_normalize )
				{
					FMOD_Channel_AddDSP(recordingChannel, 3, dsp_normalize);
					FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_FADETIME, kNormalizeFadeTime);
					if ( getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP) < 1.f )
					{
						FMOD_DSP_SetBypass(dsp_normalize, true);
					}
					else
					{
						FMOD_DSP_SetBypass(dsp_normalize, false);
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_MAXAMP, 
							std::min(100.f, std::max(1.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP))));
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_THRESHOLD,
							std::min(1.f, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD))));
					}
					FMOD_DSP_SetMeteringEnabled(dsp_normalize, true, true);
				}
			}
		}

		if ( recordPos != recordingLastPos )
		{
			if ( recordingChannel )
			{
				int num_dsps = 0;
				fmod_result = FMOD_Channel_GetNumDSPs(recordingChannel, &num_dsps);
				if ( fmod_result != FMOD_ERR_INVALID_HANDLE )
				{
					for ( int i = 0; i < num_dsps; ++i )
					{
						FMOD_DSP* dsp = NULL;
						fmod_result = FMOD_Channel_GetDSP(recordingChannel, i, &dsp);
						FMOD_DSP_TYPE type;
						FMOD_DSP_GetType(dsp, &type);

						bool in, out;
						FMOD_DSP_GetMeteringEnabled(dsp, &in, &out);
						if ( type == FMOD_DSP_TYPE_FADER )
						{
							if ( i == 0 )
							{
								FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_FADER_GAIN, -79.9);
							}
						}
						if ( type == FMOD_DSP_TYPE_NORMALIZE )
						{
							if ( getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP) < 1.f )
							{
								FMOD_DSP_SetBypass(dsp, true);
							}
							else
							{
								FMOD_DSP_SetBypass(dsp, false);
								fmod_result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_NORMALIZE_MAXAMP,
									std::min(kMaxNormalizeAmp, std::max(1.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP))));
								fmod_result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_NORMALIZE_THRESHOLD,
									std::min(kMaxNormalizeThreshold, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD))));
							}
							if ( in && out )
							{
								FMOD_DSP_METERING_INFO input_metering{};
								FMOD_DSP_METERING_INFO output_metering{};
								fmod_result = FMOD_DSP_GetMeteringInfo(dsp, &input_metering, &output_metering);
								loopback_input_volume = std::max(input_metering.peaklevel[0], loopback_input_volume - 0.02f);
							}
						}
						/*if ( type == FMOD_DSP_TYPE_LIMITER )
						{
							fmod_result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_LIMITER_MAXIMIZERGAIN, (std::max(0.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f))));
						}*/
						if ( type == FMOD_DSP_TYPE_FADER && in && out )
						{
							fmod_result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_FADER_GAIN, std::min(kMaxGain, 
								std::max(-80.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f)));
							FMOD_DSP_METERING_INFO input_metering{};
							FMOD_DSP_METERING_INFO output_metering{};
							FMOD_DSP_GetMeteringInfo(dsp, &input_metering, &output_metering);
							loopback_output_volume = std::max(output_metering.peaklevel[0], loopback_output_volume - 0.02f);
						}
					}
				}
			}
		}

		if ( recordPos != recordingLastPos )
		{
			int blocklength = (int)recordPos - (int)recordingLastPos;
			if ( blocklength < 0 )
			{
				blocklength += (int)recordingSoundLength;
			}

			void* ptr1 = NULL;
			unsigned len1 = 0;
			void* ptr2 = NULL;
			unsigned len2 = 0;

			if ( doRecord )
			{
				FMOD_Sound_Lock(recordingSound, recordingLastPos * nativeChannels * sizeof(short),
					blocklength * sizeof(short) * nativeChannels, &ptr1, &ptr2, &len1, &len2);

				if ( len1 > 0 )
				{
					VoiceChat_t::ringBufferRecord.Write((char*)ptr1, len1);
				}
				if ( len2 > 0 )
				{
					VoiceChat_t::ringBufferRecord.Write((char*)ptr2, len2);
				}
				FMOD_Sound_Unlock(recordingSound, ptr1, ptr2, len1, len2);
			}
		}
		recordingLastPos = recordPos;
	}

	pushAvailableDatagrams();
}

static ConsoleVariable<bool> cvar_voice_loopback_ingame("/voice_loopback_ingame", false);
void VoiceChat_t::update()
{
	if ( !bInit )
	{
		return;
	}

	updateRecording();

	sendPackets();

	FMOD_VECTOR listener_pos;
	fmod_result = FMOD_System_Get3DListenerAttributes(fmod_system, 0, &listener_pos, NULL, NULL, NULL);
	bool hasListenerPos = fmod_result == FMOD_OK;

	static bool prevCustomRolloff = true;
	if ( prevCustomRolloff != getAudioSettingBool(VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
	{
		updateOnMapChange3DRolloff();
	}
	prevCustomRolloff = getAudioSettingBool(VOICE_SETTING_USE_CUSTOM_ROLLOFF);

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( PlayerChannels[i].talkingTicks > 0 )
		{
			--PlayerChannels[i].talkingTicks;
		}
		if ( PlayerChannels[i].lastAudibleTick > 0 )
		{
			--PlayerChannels[i].lastAudibleTick;
		}
		if ( PlayerChannels[i].outputChannel )
		{
			bool isVirtual = false;
			fmod_result = FMOD_Channel_IsVirtual(PlayerChannels[i].outputChannel, &isVirtual);
			if ( isVirtual )
			{
				PlayerChannels[i].audio_queue_mutex.lock();
				PlayerChannels[i].audioQueue.clear();
				PlayerChannels[i].audio_queue_mutex.unlock();
			}

			float volume = std::min(std::max(0.0f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_VOICE_GLOBAL_VOLUME)), 1.f);
			FMOD_Channel_SetVolume(PlayerChannels[i].outputChannel, volume);

			if ( (multiplayer == SINGLE && !*cvar_voice_debug) )
			{
				if ( !mainMenuAudioTabOpen() )
				{
					FMOD_Channel_SetVolume(PlayerChannels[i].outputChannel, 0.f);
				}
			}
			if ( client_disconnected[i] )
			{
				FMOD_Channel_SetVolume(PlayerChannels[i].outputChannel, 0.f);
			}

			FMOD_DSP* dsp_reverb = NULL;
			FMOD_DSP* dsp_eq = NULL;
			{
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_REVERB, &dsp_reverb);
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_EQ, &dsp_eq);
				if ( dsp_eq )
				{
					FMOD_DSP_SetBypass(dsp_eq, client_disconnected[i] || (i != clientnum && multiplayer == SINGLE));
				}
			}

			{
				FMOD_DSP* dsp_normalize = NULL;
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_NORMALIZE, &dsp_normalize);
				if ( dsp_normalize )
				{
					if ( PlayerChannels[i].normalize_amp < 1.f )
					{
						FMOD_DSP_SetBypass(dsp_normalize, true);
					}
					else
					{
						FMOD_DSP_SetBypass(dsp_normalize, client_disconnected[i] || (i != clientnum && multiplayer == SINGLE));
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_MAXAMP, PlayerChannels[i].normalize_amp);
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_normalize, FMOD_DSP_NORMALIZE_THRESHOLD, PlayerChannels[i].normalize_threshold);
					}
				}
			}

			{
				FMOD_DSP* dsp_fader_local = NULL;
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_FADER_LOCALGAIN, &dsp_fader_local);
				if ( dsp_fader_local )
				{
					if ( i == clientnum )
					{
						PlayerChannels[i].localChannelGain = 100.f;
					}
					if ( intro )
					{
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_fader_local, FMOD_DSP_FADER_GAIN, 0.f);
					}
					else
					{
						fmod_result = FMOD_DSP_SetParameterFloat(dsp_fader_local, FMOD_DSP_FADER_GAIN,
							std::min(kMaxGain, (std::max(-80.f, (PlayerChannels[i].localChannelGain - 100.f)))));
					}
				}
			}

			{
				FMOD_DSP* dsp_limiter_channelGain = NULL;
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_LIMITER, &dsp_limiter_channelGain);
				if ( dsp_limiter_channelGain )
				{
					/*fmod_result = FMOD_DSP_SetParameterFloat(dsp_limiter_channelGain, FMOD_DSP_LIMITER_MAXIMIZERGAIN,
						std::max(0.f, PlayerChannels[i].channelGain - 100.f));*/
				}
			}
			{
				FMOD_DSP* dsp_fader = NULL;
				fmod_result = FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_FADER_CHANNELGAIN, &dsp_fader);
				if ( dsp_fader )
				{
					fmod_result = FMOD_DSP_SetParameterFloat(dsp_fader, FMOD_DSP_FADER_GAIN, 
						std::min(kMaxGain, (std::max(-80.f, PlayerChannels[i].channelGain - 100.f))));
				}
			}

			if ( i == clientnum 
				&& !((mainMenuAudioTabOpen() && getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD))
					|| (getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) && *cvar_voice_loopback_ingame) ) )
			{
				// if clientnum, use local volume when not testing loopback audio
				if ( PlayerChannels[i].talkingTicks > 0 )
				{
					PlayerChannels[i].monitor_input_volume = loopback_input_volume;
					PlayerChannels[i].monitor_output_volume = loopback_output_volume;
				}
				else
				{
					PlayerChannels[i].monitor_input_volume = 0.f;
					PlayerChannels[i].monitor_output_volume = 0.f;
				}
			}
			else
			{
				FMOD_DSP_METERING_INFO input_metering{};
				FMOD_DSP_METERING_INFO output_metering{};
				FMOD_DSP* dsp_end = NULL;
				FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, DSPORDER_END - 1, &dsp_end);
				FMOD_DSP* dsp_start = NULL;
				FMOD_Channel_GetDSP(PlayerChannels[i].outputChannel, 1, &dsp_start);

				if ( dsp_end )
				{
					fmod_result = FMOD_DSP_GetMeteringInfo(dsp_end, &input_metering, &output_metering);
					PlayerChannels[i].monitor_input_volume = std::max(input_metering.peaklevel[0], PlayerChannels[i].monitor_input_volume - 0.02f);
				}

				if ( dsp_start )
				{
					fmod_result = FMOD_DSP_GetMeteringInfo(dsp_start, &input_metering, &output_metering);
					PlayerChannels[i].monitor_output_volume = std::max(output_metering.peaklevel[0], PlayerChannels[i].monitor_output_volume - 0.02f);
					if ( PlayerChannels[i].monitor_output_volume > 0.05 )
					{
						PlayerChannels[i].lastAudibleTick = TICKS_PER_SECOND / 4;
					}
				}
			}
			//messagePlayer(i, MESSAGE_DEBUG, "In: %.2f Out: %.2f", PlayerChannels[i].monitor_input_volume, PlayerChannels[i].monitor_output_volume);

			FMOD_MODE mode;
			fmod_result = FMOD_Channel_GetMode(PlayerChannels[i].outputChannel, &mode);

			static ConsoleVariable<bool> cvar_voice_audibility("/voice_audibility", false);
			if ( *cvar_voice_audibility && i == clientnum )
			{
				float audibility = 0.f;
				FMOD_Channel_GetAudibility(PlayerChannels[i].outputChannel, &audibility);
				if ( audibility > 0.0001 )
				{
					messagePlayer(i, MESSAGE_HINT, "Audibility: %.2f", audibility);
				}
			}

			if ( intro || MainMenu::isCutsceneActive() || movie )
			{
				mode &= ~(FMOD_3D);
				mode |= FMOD_2D;
				fmod_result = FMOD_Channel_SetMode(PlayerChannels[i].outputChannel, mode);

				setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
				setEQSettings(dsp_eq, cvar_voice_eq_player->x, cvar_voice_eq_player->y, cvar_voice_eq_player->z);
			}
			else
			{
				mode &= ~(FMOD_2D);
				mode |= FMOD_3D;
				fmod_result = FMOD_Channel_SetMode(PlayerChannels[i].outputChannel, mode);

				constexpr real_t lowpassAmount = 12.0;
				constexpr real_t lowpassMinDist = 2.0;
				if ( Entity* entity = Player::getPlayerInteractEntity(i) )
				{
					FMOD_VECTOR position;
					position.x = (float)(entity->x / (real_t)16.0);
					position.y = (float)(0.0);
					position.z = (float)(entity->y / (real_t)16.0);
					fmod_result = FMOD_Channel_Set3DAttributes(PlayerChannels[i].outputChannel, &position, NULL);

					real_t listenerMeters = 0.0;
					if ( hasListenerPos )
					{
						real_t dx = listener_pos.x - position.x;
						real_t dy = listener_pos.z - position.z;
						listenerMeters = sqrt(dx * dx + dy * dy);
					}

					real_t lowpassAttenuation = 0.0;
					if ( listenerMeters >= lowpassMinDist )
					{
						lowpassAttenuation = std::min(1.0, (listenerMeters - lowpassMinDist) / 32.0);
					}

					if ( entity->behavior == &actDeathGhost )
					{
						setReverbSettings(dsp_reverb, cvar_voice_reverb_ghost->x, cvar_voice_reverb_ghost->y, cvar_voice_reverb_ghost->z);
						setEQSettings(dsp_eq, 
							cvar_voice_eq_ghost->x - lowpassAmount * lowpassAttenuation, 
							cvar_voice_eq_ghost->y, 
							cvar_voice_eq_ghost->z);
					}
					else
					{
						setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
						setEQSettings(dsp_eq, 
							cvar_voice_eq_player->x - lowpassAmount * lowpassAttenuation, 
							cvar_voice_eq_player->y, 
							cvar_voice_eq_player->z);
					}
				}
				else
				{
					real_t listenerMeters = 0.0;
					if ( hasListenerPos )
					{
						FMOD_VECTOR position;
						fmod_result = FMOD_Channel_Get3DAttributes(PlayerChannels[i].outputChannel, &position, NULL);
						if ( fmod_result == FMOD_OK )
						{
							real_t dx = listener_pos.x - position.x;
							real_t dy = listener_pos.z - position.z;
							listenerMeters = sqrt(dx * dx + dy * dy);
						}
					}

					real_t lowpassAttenuation = 0.0;
					if ( listenerMeters >= lowpassMinDist )
					{
						lowpassAttenuation = std::min(1.0, (listenerMeters - lowpassMinDist) / 32.0);
					}

					setReverbSettings(dsp_reverb, cvar_voice_reverb_ghost->x, cvar_voice_reverb_ghost->y, cvar_voice_reverb_ghost->z);
					setEQSettings(dsp_eq, 
						cvar_voice_eq_ghost->x - lowpassAmount * lowpassAttenuation, 
						cvar_voice_eq_ghost->y, 
						cvar_voice_eq_ghost->z);
				}
			}

			if ( dsp_reverb )
			{
				if ( client_disconnected[i] || (i != clientnum && multiplayer == SINGLE) )
				{
					FMOD_DSP_SetBypass(dsp_reverb, true);
				}
			}
		}
	}
}
void VoiceChat_t::deinit()
{
	if ( loopbackPacket )
	{
		SDLNet_FreePacket(loopbackPacket);
		loopbackPacket = NULL;
	}

	deinitRecording();
	FMODErrorCheck();

	if ( recordingChannel )
	{
		FMOD_Channel_Stop(recordingChannel);
		recordingChannel = NULL;
	}

	if ( recordingSound )
	{
		FMOD_Sound_Release(recordingSound);
		recordingSound = NULL;
	}
	recordingLastPos = 0;
	recordingSamples = 0;
	bIsRecording = false;
	bRecordingInit = false;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].deinit();
	}

	bInit = false;
	logInfo("VoiceChat_t::deinit()");
}

int VoiceChat_t::packetVoiceDataIdx = 15;
void VoiceChat_t::sendPackets()
{
	UDPpacket* packet = (net_packet) ? net_packet : (getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) ? loopbackPacket : NULL);
	if ( !packet )
	{
		recordingDatagrams.clear();
		return;
	}

	bool loopback = false;
	bool sendVoice = true;
	if ( mainMenuAudioTabOpen()
		&& getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) )
	{
		loopback = true; // we're testing in the settings menu
		sendVoice = false;
	}
	if ( *cvar_voice_loopback_ingame && getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) ) // debug send to all
	{
		loopback = true;
		sendVoice = true;
	}

	static ConsoleVariable<int> cvar_voice_packet_limit("/voice_packet_limit", 10);
	static ConsoleVariable<int> cvar_voice_all_loopback("/voice_all_loopback", false);
	int voice_no_recv = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_RECV);
	for ( int i = 0; i < *cvar_voice_packet_limit; ++i )
	{
		if ( recordingDatagrams.size() )
		{
			memset(packet->data, 0, NET_PACKET_SIZE);
			strcpy((char*)packet->data, "VOIP");
			SDLNet_Write32(datagramSequence, &packet->data[5]);
			SDLNet_Write16(recordingDatagrams.front().size(), &packet->data[9]);
			Uint8 gain = (std::max(-kMaxGain * 10.f, std::min(kMaxGain * 10.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f) * 10.f))) + 120;
			packet->data[11] = gain;
			Uint8 normalizeAmp = std::min(kMaxNormalizeAmp, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP)));
			packet->data[12] = normalizeAmp;
			Uint16 normalizeThreshold = 400.f * std::min(kMaxNormalizeThreshold, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD)));
			SDLNet_Write16(normalizeThreshold, &packet->data[13]);
			packet->data[4] = 0;
			packet->data[4] |= getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) ? (1 << 7) : 0;
			packet->data[4] |= using_encoding ? (1 << 6) : 0;
			memcpy(packet->data + packetVoiceDataIdx, (char*)recordingDatagrams.front().data(), recordingDatagrams.front().size());
			packet->len = packetVoiceDataIdx + recordingDatagrams.front().size();
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( loopback && i == clientnum )
				{
					packet->data[4] &= ~(0xF);
					packet->data[4] |= (Uint8)clientnum;
					receivePacket(packet);
				}
				else if ( loopback && *cvar_voice_all_loopback )
				{
					packet->data[4] &= ~(0xF);
					packet->data[4] |= (Uint8)i;
					receivePacket(packet);
				}
				
				if ( sendVoice )
				{
					if ( i == 0 )
					{
						if ( multiplayer == CLIENT )
						{
							packet->data[4] &= ~(0xF);
							packet->data[4] |= (Uint8)clientnum;
							packet->address.host = net_server.host;
							packet->address.port = net_server.port;
							sendPacket(net_sock, -1, packet, 0);
						}
					}
					else if ( i > 0 )
					{
						if ( !client_disconnected[i] && multiplayer == SERVER )
						{
							if ( !(voice_no_recv & i) )
							{
								packet->data[4] &= ~(0xF);
								packet->data[4] |= (Uint8)clientnum;
								packet->address.host = net_clients[i - 1].host;
								packet->address.port = net_clients[i - 1].port;
								sendPacket(net_sock, -1, packet, i - 1);
							}
						}
					}
				}
			}
			recordingDatagrams.erase(recordingDatagrams.begin());
			++datagramSequence;
		}
		else
		{
			break;
		}
	}
}



void VoiceChat_t::receivePacket(UDPpacket* packet)
{
	if ( !packet )
	{
		return;
	}
	if ( !bInit )
	{
		return;
	}

	int player = (packet->data[4]) & 0xF;
	bool encodedFrame = (packet->data[4] & (1 << 6));
	int voice_no_recv = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_RECV);
	bool localNoRecv = (voice_no_recv & (1 << clientnum)) && (player != clientnum);

	if ( player >= 0 && player < MAXPLAYERS
		&& !localNoRecv
		&& packet->len > packetVoiceDataIdx
		&& (packet->len - packetVoiceDataIdx == SDLNet_Read16(&packet->data[9])) )
	{
		Uint32 sequence = SDLNet_Read32(&packet->data[5]);
		PlayerChannels[player].channelGain = (std::min(kMaxGain, std::max(-kMaxGain, ((float)(packet->data[11]) - 120.f) / 10.f))) + 100.f;
		PlayerChannels[player].normalize_amp = std::max(0.f, std::min(kMaxNormalizeAmp, (float)packet->data[12]));
		PlayerChannels[player].normalize_threshold = std::max(0.f, std::min(kMaxNormalizeThreshold, (float)SDLNet_Read16(&packet->data[13]) / 400.f));
		PlayerChannels[player].talkingTicks = TICKS_PER_SECOND / 4;
		unsigned int readBytes = (packet->len - packetVoiceDataIdx);
		if ( (readBytes % 2 == 0 && !encodedFrame) || encodedFrame ) // should be even numbered unencoded
		{
			if ( encodedFrame )
			{
				logError("received encoded frame without Opus encoder configured");
				return;
			}
			else
			{
				PlayerChannels[player].audio_queue_mutex.lock();
				int i = 0;
				for ( i = 0; i < readBytes && PlayerChannels[player].audioQueue.size() < PlayerChannels_t::audioQueueSizeLimit; ++i )
				{
					PlayerChannels[player].audioQueue.push_back(packet->data[packetVoiceDataIdx + i]);
				}
				PlayerChannels[player].audio_queue_mutex.unlock();
				unsigned int samplesWritten = i;
				PlayerChannels[player].totalSamplesWritten += samplesWritten;
				if ( samplesWritten != 0 && (samplesWritten < PlayerChannels[player].minimumSamplesWritten) )
				{
					PlayerChannels[player].minimumSamplesWritten = samplesWritten;
					PlayerChannels[player].adjustedLatency = std::max(samplesWritten, PlayerChannels[player].desiredLatency);
				}
				if ( PlayerChannels[player].audioQueue.size() >= PlayerChannels_t::audioQueueSizeLimit )
				{
					logInfo("warning: VoiceChat_t::receivePacket() audio queue full");
				}
			}
		}
	}

	if ( multiplayer == SERVER )
	{
		if ( player != clientnum ) // don't forward the server's loopback
		{
			for ( int i = 1; i < MAXPLAYERS; ++i )
			{
				if ( i != player && !client_disconnected[i] && !(voice_no_recv & (1 << i)) )
				{
					// forward this on to other clients
					packet->address.host = net_clients[i - 1].host;
					packet->address.port = net_clients[i - 1].port;
					sendPacket(net_sock, -1, packet, i - 1);
				}
			}
		}
	}
}

VoiceChat_t::RingBuffer::RingBuffer(int sizeBytes)
{
	_data = new char[sizeBytes];
	memset(_data, 0, sizeBytes);
	_size = sizeBytes;
	_readPtr = 0;
	_writePtr = 0;
	_writeBytesAvail = sizeBytes;
}

VoiceChat_t::RingBuffer::~RingBuffer()
{
	delete[] _data;
}

bool VoiceChat_t::RingBuffer::Empty()
{
	memset(_data, 0, _size);
	_readPtr = 0;
	_writePtr = 0;
	_writeBytesAvail = _size;
	return true;
}

int VoiceChat_t::RingBuffer::Read(char* dataPtr, int numBytes)
{
	if ( dataPtr == 0 || numBytes <= 0 || _writeBytesAvail == _size )
	{
		return 0;
	}

	int readBytesAvail = _size - _writeBytesAvail;

	if ( numBytes > readBytesAvail )
	{
		numBytes = readBytesAvail;
	}

	if ( numBytes > _size - _readPtr )
	{
		int len = _size - _readPtr;
		memcpy(dataPtr, _data + _readPtr, len);
		memcpy(dataPtr + len, _data, numBytes - len);
	}
	else
	{
		memcpy(dataPtr, _data + _readPtr, numBytes);
	}

	_readPtr = (_readPtr + numBytes) % _size;
	_writeBytesAvail += numBytes;

	return numBytes;
}

int VoiceChat_t::RingBuffer::Write(char* dataPtr, int numBytes)
{
	if ( dataPtr == 0 || numBytes <= 0 || _writeBytesAvail == 0 )
	{
		return 0;
	}

	if ( numBytes > _writeBytesAvail )
	{
		numBytes = _writeBytesAvail;
	}

	if ( numBytes > _size - _writePtr )
	{
		int len = _size - _writePtr;
		memcpy(_data + _writePtr, dataPtr, len);
		memcpy(_data, dataPtr + len, numBytes - len);
	}
	else
	{
		memcpy(_data + _writePtr, dataPtr, numBytes);
	}

	_writePtr = (_writePtr + numBytes) % _size;
	_writeBytesAvail -= numBytes;

	return numBytes;
}

int VoiceChat_t::RingBuffer::GetSize()
{
	return _size;
}

int VoiceChat_t::RingBuffer::GetWriteAvail()
{
	return _writeBytesAvail;
}

int VoiceChat_t::RingBuffer::GetReadAvail()
{
	return _size - _writeBytesAvail;
}

EnsembleSounds_t ensembleSounds;

FMOD_RESULT F_CALLBACK ensembleExplorationCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2);

FMOD_RESULT F_CALLBACK ensembleCombatCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2);

FMOD_VECTOR ensemble_global_position {0.f, 0.f, 0.f};

void EnsembleSounds_t::playSong()
{
	songTransitionState = TRANSITION_EXPLORE;
	unsigned int dsp_block_len = 0;
	fmod_result = FMOD_System_GetDSPBufferSize(fmod_system, &dsp_block_len, 0);
	int outputrate = 0;
	fmod_result = FMOD_System_GetSoftwareFormat(fmod_system, &outputrate, 0, 0);
	FMODErrorCheck();
	unsigned long long clock_start = 0;

	float buffer_ms = (dsp_block_len * 1000.f / (float)outputrate);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; i++ )
	{
		fmod_result = FMOD_System_PlaySound(fmod_system, exploreSound[i], transceiver_group[i], true, &exploreChannel[i]);
		fmod_result = FMOD_Channel_SetCallback(exploreChannel[i], ensembleExplorationCallback);

		fmod_result = FMOD_System_PlaySound(fmod_system, combatSound[i], transceiver_group[i], true, &combatChannel[i]);
		fmod_result = FMOD_Channel_SetCallback(combatChannel[i], ensembleCombatCallback);

		unsigned int songPos = 0;
		FMOD_SYNCPOINT* syncPoint = NULL;
		fmod_result = FMOD_Sound_GetSyncPoint(exploreSound[i], exploreSongSeek, &syncPoint);
		fmod_result = FMOD_Sound_GetSyncPointInfo(exploreSound[i], syncPoint, NULL, 0, &songPos, FMOD_TIMEUNIT_PCM);
		fmod_result = FMOD_Channel_SetPosition(exploreChannel[i], songPos, FMOD_TIMEUNIT_PCM);

		unsigned long long clock_now = 0;
		fmod_result = FMOD_Channel_GetDSPClock(exploreChannel[i], 0, &clock_now);
		if ( !clock_start )
		{
			clock_start = clock_now;
			clock_start += (dsp_block_len * 5);
		}
		assert(clock_now < clock_start);

		/*if ( !clock_start )
		{
		}
		else
		{
			float freq;
			unsigned int slen = 0;
			fmod_result = FMOD_Sound_GetLength(sound[count], &slen, FMOD_TIMEUNIT_PCM);

			fmod_result = FMOD_Sound_GetDefaults(sound[count], &freq, 0);
			slen = (unsigned int)((float)slen / freq * outputrate);

			clock_start += slen;
		}*/

		FMOD_Channel_SetDelay(exploreChannel[i], clock_start, 0, false);

		unsigned long long fade_delay = (1000.f / buffer_ms) * dsp_block_len;
		fmod_result = FMOD_Channel_RemoveFadePoints(exploreChannel[i], 0, (unsigned long long)(-1));
		fmod_result = FMOD_Channel_AddFadePoint(exploreChannel[i], clock_start, 0.f);
		fmod_result = FMOD_Channel_AddFadePoint(exploreChannel[i], clock_start + fade_delay, 1.f);

		fmod_result = FMOD_Channel_SetPaused(exploreChannel[i], false);

		FMOD_ChannelGroup_SetVolume(transceiver_group[i], 0.f);

		fmod_result = FMOD_Channel_SetMode(exploreChannel[i], FMOD_3D_HEADRELATIVE);
		fmod_result = FMOD_Channel_Set3DAttributes(exploreChannel[i], &ensemble_global_position, NULL);
		fmod_result = FMOD_Channel_SetMode(combatChannel[i], FMOD_3D_HEADRELATIVE);
		fmod_result = FMOD_Channel_Set3DAttributes(combatChannel[i], &ensemble_global_position, NULL);
	}
}

void EnsembleSounds_t::stopPlaying(bool setCombatDelay)
{
	songTransitionState = TRANSITION_EXPLORE;
	if ( setCombatDelay )
	{
		combatDelay = TICKS_PER_SECOND * 5;
	}
	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? FMOD_Channel_Stop(exploreChannel[i]) : FMOD_OK;
		exploreChannel[i] = NULL;
		fmod_result = combatChannel[i] ? FMOD_Channel_Stop(combatChannel[i]) : FMOD_OK;
		combatChannel[i] = NULL;

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? FMOD_Channel_Stop(combatTransChannel[j][i]) : FMOD_OK;
			combatTransChannel[j][i] = NULL;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? FMOD_Channel_Stop(exploreTransChannel[j][i]) : FMOD_OK;
			exploreTransChannel[j][i] = NULL;
		}
	}
}

void EnsembleSounds_t::deinit()
{
	stopPlaying(true);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? FMOD_Channel_Stop(exploreChannel[i]) : FMOD_OK;
		exploreChannel[i] = NULL;
		fmod_result = combatChannel[i] ? FMOD_Channel_Stop(combatChannel[i]) : FMOD_OK;
		combatChannel[i] = NULL;
		fmod_result = exploreSound[i] ? FMOD_Sound_Release(exploreSound[i]) : FMOD_OK;
		exploreSound[i] = NULL;
		fmod_result = combatSound[i] ? FMOD_Sound_Release(combatSound[i]) : FMOD_OK;
		combatSound[i] = NULL;

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? FMOD_Channel_Stop(combatTransChannel[j][i]) : FMOD_OK;
			combatTransChannel[j][i] = NULL;
			fmod_result = combatTransSound[j][i] ? FMOD_Sound_Release(combatTransSound[j][i]) : FMOD_OK;
			combatTransSound[j][i] = NULL;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? FMOD_Channel_Stop(exploreTransChannel[j][i]) : FMOD_OK;
			exploreTransChannel[j][i] = NULL;
			fmod_result = exploreTransSound[j][i] ? FMOD_Sound_Release(exploreTransSound[j][i]) : FMOD_OK;
			exploreTransSound[j][i] = NULL;
		}
	}
}

static ConsoleCommand ccmd_ensemble_transition_mode("/ensemble_transition_mode", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			ensembleSounds.songTransitionMode = (EnsembleSounds_t::TransitionMode)atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_HINT, "Set transition mode to: %d", (int)ensembleSounds.songTransitionMode);
		}
});

static ConsoleCommand ccmd_ensemble_transition_state("/ensemble_transition_state", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			ensembleSounds.songTransitionState = (EnsembleSounds_t::SongTransitionState)atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_HINT, "Set transition state to: %d", (int)ensembleSounds.songTransitionState);
		}
	});

bool checkSoundReady(FMOD_SOUND* sound, const char* str, int i, int j)
{
	if ( !sound ) { 
		printlog("EnsembleSounds_t::setup() Warn: no sound pointer for %s: %d %d", str, i, j);
		return true; 
	}
	FMOD_OPENSTATE openState = FMOD_OPENSTATE_LOADING;
	unsigned int percentBuffered = 0;
	bool starving = false;
	bool diskBusy = false;
	fmod_result = FMOD_Sound_GetOpenState(sound, &openState, &percentBuffered, &starving, &diskBusy);

	if ( fmod_result == FMOD_OK )
	{
		if ( openState == FMOD_OPENSTATE_READY )
		{
			return true;
		}
	}

	return false;
}

void EnsembleSounds_t::setup()
{
	songTransitionState = TRANSITION_EXPLORE;

	if ( no_sound )
	{
		return;
	}

	Uint32 startTick = SDL_GetTicks();

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		FMOD_System_Update(fmod_system);
		bool result = checkSoundReady(exploreSound[i], "explore sound", i, 0);
		if ( !result )
		{
			i = 0;
			continue;
		}
		++i;
	}

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		FMOD_System_Update(fmod_system);
		bool result = checkSoundReady(combatSound[i], "combat sound", i, 0);
		if ( !result )
		{
			i = 0;
			continue;
		}
		++i;
	}

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		FMOD_System_Update(fmod_system);
		bool result = false;
		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			if ( j >= 2 )
			{
				result = true;
			}
			else
			{
				result = checkSoundReady(combatTransSound[j][i], "combat trans sound", i, j);
			}
			if ( !result )
			{
				break;
			}
		}

		if ( !result )
		{
			i = 0;
			continue;
		}

		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			if ( j != 3 )
			{
				result = true;
			}
			else
			{
				result = checkSoundReady(exploreTransSound[j][i], "explore trans sound", i, j);
			}
			if ( !result )
			{
				break;
			}
		}

		if ( !result )
		{
			i = 0;
			continue;
		}

		++i;
	}

	Uint32 endTick = SDL_GetTicks();

	printlog("EnsembleSounds_t::setup() in %lums", endTick - startTick);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? FMOD_Channel_Stop(exploreChannel[i]) : FMOD_OK;
		fmod_result = combatChannel[i] ? FMOD_Channel_Stop(combatChannel[i]) : FMOD_OK;
		//fmod_result = exploreSound[i] ? FMOD_Sound_Release(exploreSound[i]) : FMOD_OK;
		//fmod_result = combatSound[i] ? FMOD_Sound_Release(combatSound[i]) : FMOD_OK;
		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? FMOD_Channel_Stop(combatTransChannel[j][i]) : FMOD_OK;
			//fmod_result = combatTransSound[j][i] ? FMOD_Sound_Release(combatTransSound[j][i]) : FMOD_OK;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? FMOD_Channel_Stop(exploreTransChannel[j][i]) : FMOD_OK;
			//fmod_result = exploreTransSound[j][i] ? FMOD_Sound_Release(exploreTransSound[j][i]) : FMOD_OK;
		}

		if ( !transceiver_group[i] )
		{
			fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &transceiver_group[i]);

			FMOD_DSP* transceiver = NULL;
			fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_TRANSCEIVER, &transceiver);
			fmod_result = FMOD_ChannelGroup_AddDSP(transceiver_group[i], 0, transceiver);
			fmod_result = FMOD_DSP_SetParameterBool(transceiver, FMOD_DSP_TRANSCEIVER_TRANSMIT, true); // sending signal
			fmod_result = FMOD_DSP_SetParameterInt(transceiver, FMOD_DSP_TRANSCEIVER_CHANNEL, 1 + i); // sending on channel x

			FMOD_ChannelGroup_AddGroup(music_ensemble_global_send_group, transceiver_group[i]);
		}

		if ( exploreSound[i] )
		{
			int syncPoints = 0;
			FMOD_Sound_GetNumSyncPoints(exploreSound[i], &syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = NULL;
				fmod_result = FMOD_Sound_GetSyncPoint(exploreSound[i], point, &syncpoint);
				if ( syncpoint )
				{
					FMOD_Sound_DeleteSyncPoint(exploreSound[i], syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			FMOD_Sound_GetLength(exploreSound[i], &len, FMOD_TIMEUNIT_PCM);
			FMOD_Sound_GetFormat(exploreSound[i], &type, NULL, &chan, NULL);
			float freq = 0;
			FMOD_Sound_GetDefaults(exploreSound[i], &freq, NULL);
			int beatTime = freq * (60 / 120.f); // beats per sample, 120bpm

			int interval = 1;
			int numBeats = 2;
			exploreSoundSyncPointInterval = interval * (beatTime * numBeats);
			unsigned int beat = beatTime * 3 / 4; // starting point from beginning of song
			//FMOD_Sound_SetLoopPoints(exploreSound[i], 0, FMOD_TIMEUNIT_PCM, len - beat, FMOD_TIMEUNIT_PCM);
			int numSyncPoints = ((len - beat) / (interval * beatTime * numBeats)) + 1;
			int index = -1;
			if ( i == 0 )
			{
				exploreSyncPoints.clear();
				exploreSyncPointsToSeek.clear();
				exploreSyncPointsUnique.clear();
			}
			int seekBar = -1;
			int oldSeekBar = -1;
			while ( beat < len )
			{
				++index;
				if ( i == 0 )
				{
					if ( index == 0 )
					{
						seekBar = -1; // no action
					}
					else if ( index < 166 )
					{
						seekBar = (index / 8) * 8;
					}
					else if ( index < 310 )
					{
						seekBar = 166 + ((index - 166) / 8) * 8;
					}
					else if ( index == 310 || index == 311 )
					{
						seekBar = 302;
					}
					else if ( index < 360 )
					{
						seekBar = 312 + ((index - 312) / 8) * 8;
					}
					else if ( index == 360 || index == 361 )
					{
						seekBar = 352;
					}
					else if ( index >= 362 )
					{
						seekBar = 362;
					}
					if ( oldSeekBar != seekBar )
					{
						exploreSyncPointsUnique.push_back(seekBar);
					}
					oldSeekBar = seekBar;
				}
				fmod_result = FMOD_Sound_AddSyncPoint(exploreSound[i], beat, FMOD_TIMEUNIT_PCM, NULL, NULL);
				if ( i == 0 )
				{
					exploreSyncPoints.push_back(beat);
					exploreSyncPointsToSeek.push_back(seekBar);
				}
				FMODErrorCheck();
				beat += interval * (beatTime * numBeats);
			}

			fmod_result = FMOD_System_PlaySound(fmod_system, exploreSound[i], transceiver_group[i], true, &exploreChannel[i]);
			fmod_result = FMOD_Channel_SetMode(exploreChannel[i], FMOD_3D_HEADRELATIVE);
			fmod_result = FMOD_Channel_Set3DAttributes(exploreChannel[i], &ensemble_global_position, NULL);
			FMODErrorCheck();
			fmod_result = FMOD_Channel_SetCallback(exploreChannel[i], ensembleExplorationCallback);
		}

		if ( combatSound[i] )
		{
			int syncPoints = 0;
			FMOD_Sound_GetNumSyncPoints(combatSound[i], &syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = NULL;
				FMOD_Sound_GetSyncPoint(combatSound[i], point, &syncpoint);
				if ( syncpoint )
				{
					FMOD_Sound_DeleteSyncPoint(combatSound[i], syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			FMOD_Sound_GetLength(combatSound[i], &len, FMOD_TIMEUNIT_PCM);
			FMOD_Sound_GetFormat(combatSound[i], &type, NULL, &chan, NULL);
			float freq = 0;
			FMOD_Sound_GetDefaults(combatSound[i], &freq, NULL);
			int beatTime = freq * (60 / 90.f) / 8.f; // eighths, 90bpm
			combatBeat = beatTime;
			int interval = 2;
			int numBeats = 7;
			combatSoundSyncPointInterval = interval * (beatTime * numBeats);
			unsigned int beat = beatTime * 0; // starting point from beginning of song
			int numSyncPoints = ((len - beat) / (interval * beatTime * numBeats)) + 1;
			int index = -1;
			if ( i == 0 )
			{
				combatSyncPoints.clear();
			}
			while ( beat < len )
			{
				++index;
				fmod_result = FMOD_Sound_AddSyncPoint(combatSound[i], beat, FMOD_TIMEUNIT_PCM, NULL, NULL);
				if ( i == 0 )
				{
					combatSyncPoints.push_back(beat);
				}
				FMODErrorCheck();
				beat += interval * beatTime * numBeats;
			}

			fmod_result = FMOD_System_PlaySound(fmod_system, combatSound[i], transceiver_group[i], true, &combatChannel[i]);
			fmod_result = FMOD_Channel_SetMode(combatChannel[i], FMOD_3D_HEADRELATIVE);
			fmod_result = FMOD_Channel_Set3DAttributes(combatChannel[i], &ensemble_global_position, NULL);
			FMODErrorCheck();
			FMOD_Channel_SetCallback(combatChannel[i], ensembleCombatCallback);
		}

		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			if ( !exploreTransSound[j][i] )
			{
				continue;
			}

			int syncPoints = 0;
			FMOD_Sound_GetNumSyncPoints(exploreTransSound[j][i], &syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = NULL;
				fmod_result = FMOD_Sound_GetSyncPoint(exploreTransSound[j][i], point, &syncpoint);
				if ( syncpoint )
				{
					FMOD_Sound_DeleteSyncPoint(exploreTransSound[j][i], syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			FMOD_Sound_GetLength(exploreTransSound[j][i], &len, FMOD_TIMEUNIT_PCM);
			FMOD_Sound_GetFormat(exploreTransSound[j][i], &type, NULL, &chan, NULL);
			float freq = 0;
			FMOD_Sound_GetDefaults(exploreTransSound[j][i], &freq, NULL);
			int beatTime = freq * (60 / 120.f); // beats per sample, 120bpm

			int interval = 1;
			int numBeats = 4;
			unsigned int beat = interval * beatTime * numBeats;
			if ( j == 3 )
			{
				fmod_result = FMOD_Sound_AddSyncPoint(exploreTransSound[j][i], 0, FMOD_TIMEUNIT_PCM, NULL, NULL);
				fmod_result = FMOD_Sound_AddSyncPoint(exploreTransSound[j][i], 8 * beat / 16, FMOD_TIMEUNIT_PCM, NULL, NULL);
			}
			else
			{
				fmod_result = FMOD_Sound_AddSyncPoint(exploreTransSound[j][i], 4 * beat / 8, FMOD_TIMEUNIT_PCM, NULL, NULL);
				fmod_result = FMOD_Sound_AddSyncPoint(exploreTransSound[j][i], beat, FMOD_TIMEUNIT_PCM, NULL, NULL);
			}
			FMODErrorCheck();
			//result = FMOD_System_PlaySound(fmod_system, exploreTransSound[j][i], groupTx[i], true, &channelTrans[j][count]);
			//FMODErrorCheck();
		}

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			if ( !combatTransSound[j][i] )
			{
				continue;
			}

			int syncPoints = 0;
			FMOD_Sound_GetNumSyncPoints(combatTransSound[j][i], &syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = NULL;
				fmod_result = FMOD_Sound_GetSyncPoint(combatTransSound[j][i], point, &syncpoint);
				if ( syncpoint )
				{
					FMOD_Sound_DeleteSyncPoint(combatTransSound[j][i], syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			FMOD_Sound_GetLength(combatTransSound[j][i], &len, FMOD_TIMEUNIT_PCM);
			FMOD_Sound_GetFormat(combatTransSound[j][i], &type, NULL, &chan, NULL);
			float freq = 0;
			FMOD_Sound_GetDefaults(combatTransSound[j][i], &freq, NULL);
			int beatTime = freq * (60 / 90.f); // beats per sample, 120bpm

			int interval = 2;
			if ( j == 3 )
			{
				interval = 1;
			}
			int numBeats = 4;
			unsigned int beat = interval * beatTime * numBeats;
			fmod_result = FMOD_Sound_AddSyncPoint(combatTransSound[j][i], 3 * beat / 8, FMOD_TIMEUNIT_PCM, NULL, NULL);
			fmod_result = FMOD_Sound_AddSyncPoint(combatTransSound[j][i], beat, FMOD_TIMEUNIT_PCM, NULL, NULL);
			FMODErrorCheck();
			//result = FMOD_System_PlaySound(fmod_system, combatTransSound[j][i], groupTx[i], true, &channelCombatTrans[j][count]);
			//FMODErrorCheck();
		}
	}

	ensembleSounds.firstTimeSetup = false;
}

static ConsoleVariable<int> cvar_ensemble_combat_length("/ensemble_combat_length", 30);

FMOD_RESULT F_CALLBACK ensembleCombatCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2)
{
	FMOD_RESULT result;
	if ( callbacktype == FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT )
	{
		int currentSyncPoint = reinterpret_cast<intptr_t>(commanddata1);
		FMOD_CHANNEL* chan = reinterpret_cast<FMOD_CHANNEL*>(channelcontrol);
		FMOD_SOUND* sound = NULL;
		FMOD_Channel_GetCurrentSound(chan, &sound);
		if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_ENDING )
		{
			if ( sound == ::ensembleSounds.combatSound[0] )
			{
				unsigned int currentPos = 0;
				FMOD_Channel_GetPosition(chan, &currentPos, FMOD_TIMEUNIT_PCM);

				FMOD_SYNCPOINT* syncPoint = NULL;
				FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
				unsigned int currentOffset = 0;
				result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

				if ( currentPos >= currentOffset // check our callback is within expected sync range
					&& (currentPos < currentOffset + ensembleSounds.combatSoundSyncPointInterval) )
				{
					int syncInterval = 8 / 2;
					int syncpointNext = currentSyncPoint + syncInterval - currentSyncPoint % (syncInterval);
					/*if ( currentSyncPoint % (syncInterval * 2) >= syncInterval )
					{
						syncpointNext += syncInterval;
					}*/

					bool noTransition = false;
					if ( ensembleSounds.ticksCombatPlaying < *cvar_ensemble_combat_length * TICKS_PER_SECOND )
					{
						noTransition = true;
					}

					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF
						|| noTransition )
					{
						syncpointNext = currentSyncPoint + 1;
					}

					FMOD_SYNCPOINT* syncPoint2 = NULL;
					int numSyncPoints = 0;
					FMOD_Sound_GetNumSyncPoints(sound, &numSyncPoints);
					while ( syncpointNext >= numSyncPoints )
					{
						syncpointNext -= numSyncPoints;
					}

					FMOD_Sound_GetSyncPoint(sound, syncpointNext, &syncPoint2);

					unsigned int soundLength = 0;
					FMOD_Sound_GetLength(sound, &soundLength, FMOD_TIMEUNIT_PCM);

					unsigned int soundPos = 0;
					FMOD_Channel_GetPosition(chan, &soundPos, FMOD_TIMEUNIT_PCM);

					unsigned int offset2 = 0;
					result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint2, NULL, 0, &offset2, FMOD_TIMEUNIT_PCM);

					unsigned int delayLength = 0;
					if ( soundPos > offset2 )
					{
						// wrap ahead
						delayLength = (soundLength - soundPos) + offset2;
					}
					else
					{
						delayLength = offset2 - soundPos;
					}

					ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_COMBAT_ENDED;

					//FMOD_Channel_AddFadePoint(chan, clock_now + (unsigned long long)(rate * 2.0), 0.f);
					int combatTransition = 0;
					if ( syncpointNext >= 28 && syncpointNext <= 32 )
					{
						combatTransition = 1;
					}
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
					{
						combatTransition = 0;
					}

					float volume = 0.f;
					FMOD_Channel_GetVolume(chan, &volume);
					/*if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF && volume < 0.5f )
					{
						noTransition = true;
						delayLength = ensembleSounds.combatSoundSyncPointInterval;
					}*/
					auto mainChannelDelay = delayLength;

					unsigned int buffersize = 0;
					FMOD_System_GetDSPBufferSize(fmod_system, &buffersize, NULL);
					int sampleRate = 1;
					FMOD_System_GetSoftwareFormat(fmod_system, &sampleRate, NULL, NULL);
					float buffer_ms = (buffersize * 1000.f / (float)sampleRate);

					for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
					{
						unsigned long long clock_now = 0;
						result = FMOD_Channel_GetDSPClock(ensembleSounds.combatChannel[i], 0, &clock_now);
						result = FMOD_Channel_SetDelay(ensembleSounds.combatChannel[i], 0, clock_now + (unsigned long long)(mainChannelDelay), false);

						if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT && noTransition )
						{
							result = FMOD_Channel_AddFadePoint(ensembleSounds.combatChannel[i], clock_now, 1.f);
							result = FMOD_Channel_AddFadePoint(ensembleSounds.combatChannel[i], clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}
						else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT && !noTransition )
						{
							result = FMOD_Channel_AddFadePoint(ensembleSounds.combatChannel[i], clock_now + (unsigned long long)(mainChannelDelay) - 500.f * buffer_ms, 1.f);
							result = FMOD_Channel_AddFadePoint(ensembleSounds.combatChannel[i], clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}

						unsigned int transitionLength = 0;
						result = FMOD_Sound_GetLength(ensembleSounds.combatTransSound[combatTransition][i], &transitionLength, FMOD_TIMEUNIT_PCM);
						//result = FMOD_Channel_SetPaused(channelCombatTrans[combatTransition][i], true);

						FMOD_SYNCPOINT* syncPoint4 = NULL;
						unsigned int offset3 = 0;
						if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE && !noTransition )
						{
							result = FMOD_System_PlaySound(fmod_system, ensembleSounds.combatTransSound[combatTransition][i], 
								ensembleSounds.transceiver_group[i], true, &ensembleSounds.combatTransChannel[combatTransition][i]);
							result = FMOD_Channel_SetMode(ensembleSounds.combatTransChannel[combatTransition][i], FMOD_3D_HEADRELATIVE);
							result = FMOD_Channel_Set3DAttributes(ensembleSounds.combatTransChannel[combatTransition][i], &ensemble_global_position, NULL);

							FMOD_SYNCPOINT* syncPoint3 = NULL;
							result = FMOD_Sound_GetSyncPoint(ensembleSounds.combatTransSound[combatTransition][i], 1, &syncPoint3);
							result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.combatTransSound[combatTransition][i], syncPoint3, NULL, 
								0, &offset3, FMOD_TIMEUNIT_PCM);

							if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = FMOD_Channel_SetPosition(ensembleSounds.combatTransChannel[combatTransition][i], 0, FMOD_TIMEUNIT_PCM);
							}
							else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = FMOD_Sound_GetSyncPoint(ensembleSounds.combatTransSound[combatTransition][i], 0, &syncPoint4);
								unsigned int offset4 = 0;
								result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.combatTransSound[combatTransition][i], syncPoint4, NULL,
									0, &offset4, FMOD_TIMEUNIT_PCM);

								FMOD_Channel_SetPosition(ensembleSounds.combatTransChannel[combatTransition][i], offset4, FMOD_TIMEUNIT_PCM);

								unsigned int beat = ensembleSounds.combatBeat * 64;
								offset3 = offset3 - offset4;
								offset3 -= beat / 4;
								transitionLength -= offset4;
							}

							result = FMOD_Channel_SetDelay(ensembleSounds.combatTransChannel[combatTransition][i], clock_now + (unsigned long long)(delayLength),
								clock_now + (unsigned long long)(delayLength) + transitionLength, true);
							{
								unsigned long long delay = 0;
								result = FMOD_Channel_GetDelay(ensembleSounds.combatTransChannel[combatTransition][i], &delay, NULL);

								result = FMOD_Channel_RemoveFadePoints(ensembleSounds.combatTransChannel[combatTransition][i], 0, (unsigned long long)(-1));
								result = FMOD_Channel_AddFadePoint(ensembleSounds.combatTransChannel[combatTransition][i], delay, 0.f);
								unsigned long long fade_delay = (10.f / buffer_ms) * buffersize;
								result = FMOD_Channel_AddFadePoint(ensembleSounds.combatTransChannel[combatTransition][i], delay + fade_delay, 1.f);
							}

							result = FMOD_Channel_SetPaused(ensembleSounds.combatTransChannel[combatTransition][i], false);
						}

						result = FMOD_Channel_SetPaused(ensembleSounds.exploreChannel[i], true);
						result = FMOD_Channel_SetPosition(ensembleSounds.exploreChannel[i], 0, FMOD_TIMEUNIT_PCM);

						//FMOD_Channel_SetVolume(ensembleSounds.exploreChannel[i], 0.f);
						//FMOD_Channel_SetVolume(ensembleSounds.combatTransChannel[combatTransition][i], 0.f);

						// code to seek to position if needed, but have pickup notes at 0:00 currently
						{
							unsigned int songPos = 0;
							result = FMOD_Sound_GetSyncPoint(ensembleSounds.exploreSound[i], ensembleSounds.exploreSongSeek, &syncPoint);
							result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.exploreSound[i], syncPoint, NULL, 0, &songPos, FMOD_TIMEUNIT_PCM);
							result = FMOD_Channel_SetPosition(ensembleSounds.exploreChannel[i], songPos, FMOD_TIMEUNIT_PCM);
						}

						result = FMOD_Channel_SetDelay(ensembleSounds.exploreChannel[i], clock_now + (unsigned long long)(delayLength + offset3), 0, false);

						{
							unsigned long long delay = 0;
							result = FMOD_Channel_GetDelay(ensembleSounds.exploreChannel[i], &delay, NULL);

							unsigned long long fade_delay = (1000.f / buffer_ms) * buffersize;

							result = FMOD_Channel_RemoveFadePoints(ensembleSounds.exploreChannel[i], 0, (unsigned long long)(-1));
							result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreChannel[i], delay, 0.f);
							result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreChannel[i], delay + fade_delay, 1.f);
						}
						result = FMOD_Channel_SetPaused(ensembleSounds.exploreChannel[i], false);
					}
				}
			}
		}

		if ( false && currentSyncPoint % 8 == 0 )
		{
			unsigned int currentOffset = 0;
			FMOD_SYNCPOINT* syncPoint = NULL;
			result = FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
			result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

			currentSyncPoint = ensembleSounds.combatSongSeek * 8;

			unsigned int offset = 0;
			result = FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
			result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &offset, FMOD_TIMEUNIT_PCM);

			unsigned int currentPos = 0;
			result = FMOD_Channel_GetPosition(chan, &currentPos, FMOD_TIMEUNIT_PCM);

			// this goes crazy and calls back every missed sync point, twice for the same setpoint
			if ( currentPos >= currentOffset // check our callback is within expected sync range
				&& (currentPos < currentOffset + ensembleSounds.combatSoundSyncPointInterval) // check our callback is within expected sync range
				&& (abs((int)currentPos - (int)offset) > ((ensembleSounds.combatSoundSyncPointInterval) * 0.9)) ) // check our position is sufficiently far and not a double up
			{
				//currentPos = currentPos % soundCombatSyncPointInterval; // callback interval
				currentPos = currentPos - currentOffset;
				result = FMOD_Channel_SetPosition(chan, offset + currentPos, FMOD_TIMEUNIT_PCM);
			}
			//FMOD_Channel_GetPosition(chan, &pos, FMOD_TIMEUNIT_MS);
		}
	}

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ensembleExplorationCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2)
{
	FMOD_RESULT result = FMOD_OK;
	if ( callbacktype == FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT )
	{
		int currentSyncPoint = reinterpret_cast<intptr_t>(commanddata1);
		FMOD_CHANNEL* chan = reinterpret_cast<FMOD_CHANNEL*>(channelcontrol);
		FMOD_SOUND* sound = NULL;
		FMOD_Channel_GetCurrentSound(chan, &sound);

		if ( sound == ::ensembleSounds.exploreSound[0] ) // should this be [5] ??
		{
			if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_ENDED )
			{
				ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_EXPLORE;
			}
			if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_START && currentSyncPoint > 0 )
			{
				unsigned int currentPos = 0;
				FMOD_Channel_GetPosition(chan, &currentPos, FMOD_TIMEUNIT_PCM);

				FMOD_SYNCPOINT* syncPoint = NULL;
				FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
				unsigned int currentOffset = 0;
				result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

				if ( currentPos >= currentOffset // check our callback is within expected sync range
					&& (currentPos < currentOffset + ensembleSounds.exploreSoundSyncPointInterval) )
				{
					int syncInterval = 8 / 2;
					int syncpointNext = currentSyncPoint + syncInterval - currentSyncPoint % (syncInterval);
					if ( currentSyncPoint % (syncInterval * 2) >= syncInterval )
					{
						syncpointNext += syncInterval;
					}

					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						syncpointNext = currentSyncPoint + 1;
					}

					FMOD_SYNCPOINT* syncPoint2 = NULL;
					int numSyncPoints = 0;
					FMOD_Sound_GetNumSyncPoints(sound, &numSyncPoints);
					while ( syncpointNext >= numSyncPoints )
					{
						syncpointNext -= numSyncPoints;
					}

					FMOD_Sound_GetSyncPoint(sound, syncpointNext, &syncPoint2);

					unsigned int soundLength = 0;
					FMOD_Sound_GetLength(sound, &soundLength, FMOD_TIMEUNIT_PCM);

					unsigned int offset2 = 0;
					result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint2, NULL, 0, &offset2, FMOD_TIMEUNIT_PCM);

					unsigned int delayLength = 0;
					if ( currentPos > offset2 )
					{
						// wrap ahead
						delayLength = (soundLength - currentPos) + offset2;
					}
					else
					{
						delayLength = offset2 - currentPos;
					}

					ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_COMBAT;

					//FMOD_Channel_AddFadePoint(chan, clock_now + (unsigned long long)(rate * 2.0), 0.f);
					int explorationTransition = 2;
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						explorationTransition = 3;
					}
					else if ( syncpointNext == 20 )
					{
						explorationTransition = 1;
					}
					else if ( syncpointNext == 28 )
					{
						explorationTransition = 0;
					}

					float volume = 0.f;
					FMOD_Channel_GetVolume(chan, &volume);
					bool noTransition = false;
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF && volume < 0.5f )
					{
						//noTransition = true;
						delayLength = ensembleSounds.exploreSoundSyncPointInterval / 4;
					}
					auto mainChannelDelay = delayLength * 2;
					if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						mainChannelDelay = delayLength;
					}

					unsigned int buffersize = 0;
					FMOD_System_GetDSPBufferSize(fmod_system, &buffersize, NULL);
					int sampleRate = 1;
					FMOD_System_GetSoftwareFormat(fmod_system, &sampleRate, NULL, NULL);
					float buffer_ms = (buffersize * 1000.f / (float)sampleRate);

					for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
					{
						unsigned long long clock_now = 0;
						result = FMOD_Channel_GetDSPClock(ensembleSounds.exploreChannel[i], 0, &clock_now);
						result = FMOD_Channel_SetDelay(ensembleSounds.exploreChannel[i], 0, clock_now + (unsigned long long)(mainChannelDelay), false);

						{
							result = FMOD_Channel_RemoveFadePoints(ensembleSounds.exploreChannel[i], 0, (unsigned long long)(-1));
							result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreChannel[i], clock_now, 1.f);
							result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreChannel[i], clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}

						unsigned int transitionLength = 0;
						result = FMOD_Sound_GetLength(ensembleSounds.exploreTransSound[explorationTransition][i], &transitionLength, FMOD_TIMEUNIT_PCM);
						//result = FMOD_Channel_SetPaused(channelTrans[explorationTransition][i], true);

						FMOD_SYNCPOINT* syncPoint4 = NULL;
						unsigned int offset3 = 0;
						if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE && !noTransition )
						{
							if ( ensembleSounds.exploreTransChannel[explorationTransition][i] )
							{
								FMOD_Channel_Stop(ensembleSounds.exploreTransChannel[explorationTransition][i]);
							}
							result = FMOD_System_PlaySound(fmod_system, ensembleSounds.exploreTransSound[explorationTransition][i], 
								ensembleSounds.transceiver_group[i], true, &ensembleSounds.exploreTransChannel[explorationTransition][i]);
							result = FMOD_Channel_SetMode(ensembleSounds.exploreTransChannel[explorationTransition][i], FMOD_3D_HEADRELATIVE);
							result = FMOD_Channel_Set3DAttributes(ensembleSounds.exploreTransChannel[explorationTransition][i], &ensemble_global_position, NULL);

							FMOD_SYNCPOINT* syncPoint3 = NULL;
							result = FMOD_Sound_GetSyncPoint(ensembleSounds.exploreTransSound[explorationTransition][i], 1, &syncPoint3);
							result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.exploreTransSound[explorationTransition][i], syncPoint3, NULL, 
								0, &offset3, FMOD_TIMEUNIT_PCM);

							if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = FMOD_Channel_SetPosition(ensembleSounds.exploreTransChannel[explorationTransition][i], 0, FMOD_TIMEUNIT_PCM);
							}
							else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF 
								|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
							{
								result = FMOD_Sound_GetSyncPoint(ensembleSounds.exploreTransSound[explorationTransition][i], 0, &syncPoint4);
								unsigned int offset4 = 0;
								result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.exploreTransSound[explorationTransition][i], syncPoint4, NULL, 
									0, &offset4, FMOD_TIMEUNIT_PCM);

								FMOD_Channel_SetPosition(ensembleSounds.exploreTransChannel[explorationTransition][i], offset4, FMOD_TIMEUNIT_PCM);

								offset3 = offset3 - offset4;
								transitionLength -= offset4;
							}

							result = FMOD_Channel_SetDelay(ensembleSounds.exploreTransChannel[explorationTransition][i], clock_now + (unsigned long long)(delayLength),
								clock_now + (unsigned long long)(delayLength) + transitionLength, true);

							{
								unsigned long long delay = 0;
								result = FMOD_Channel_GetDelay(ensembleSounds.exploreTransChannel[explorationTransition][i], &delay, NULL);
								result = FMOD_Channel_RemoveFadePoints(ensembleSounds.exploreTransChannel[explorationTransition][i], 0, (unsigned long long)(-1));
								result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreTransChannel[explorationTransition][i], delay, 0.f);
								unsigned long long fade_delay = (500.f / buffer_ms) * buffersize;
								result = FMOD_Channel_AddFadePoint(ensembleSounds.exploreTransChannel[explorationTransition][i], delay + fade_delay, 1.f);
							}

							result = FMOD_Channel_SetPaused(ensembleSounds.exploreTransChannel[explorationTransition][i], false);
						}

						result = FMOD_Channel_SetPaused(ensembleSounds.combatChannel[i], true);

						unsigned int songPos = 0;
						result = FMOD_Sound_GetSyncPoint(ensembleSounds.combatSound[i], ensembleSounds.combatSongSeek * 8, &syncPoint);
						result = FMOD_Sound_GetSyncPointInfo(ensembleSounds.combatSound[i], syncPoint, NULL, 0, &songPos, FMOD_TIMEUNIT_PCM);
						result = FMOD_Channel_SetPosition(ensembleSounds.combatChannel[i], songPos, FMOD_TIMEUNIT_PCM);

						//result = FMOD_Channel_SetVolume(ensembleSounds.combatChannel[i], 0.f);
						//result = FMOD_Channel_SetVolume(ensembleSounds.exploreTransChannel[explorationTransition][i], 0.f);

						result = FMOD_Channel_RemoveFadePoints(ensembleSounds.combatChannel[i], 0, (unsigned long long)(-1));
						result = FMOD_Channel_SetDelay(ensembleSounds.combatChannel[i], clock_now + (unsigned long long)(delayLength + offset3), 0, false);
						result = FMOD_Channel_SetPaused(ensembleSounds.combatChannel[i], false);
					}
				}
			}
		}

		if ( currentSyncPoint == 0 /*currentSyncPoint % 8 == 0*/ )
		{
			unsigned int currentOffset = 0;
			FMOD_SYNCPOINT* syncPoint = NULL;
			result = FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
			result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

			currentSyncPoint = ensembleSounds.exploreSongSeek;

			unsigned int offset = 0;
			result = FMOD_Sound_GetSyncPoint(sound, currentSyncPoint, &syncPoint);
			result = FMOD_Sound_GetSyncPointInfo(sound, syncPoint, NULL, 0, &offset, FMOD_TIMEUNIT_PCM);

			unsigned int currentPos = 0;
			result = FMOD_Channel_GetPosition(chan, &currentPos, FMOD_TIMEUNIT_PCM);

			// this goes crazy and calls back every missed sync point, twice for the same setpoint
			if ( currentPos >= currentOffset // check our callback is within expected sync range
				&& (currentPos < currentOffset + ensembleSounds.exploreSoundSyncPointInterval) // check our callback is within expected sync range
				&& (abs((int)currentPos - (int)offset) > ((ensembleSounds.exploreSoundSyncPointInterval) * 0.9)) ) // check our position is sufficiently far and not a double up
			{
				//currentPos = currentPos % soundSyncPointInterval; // callback interval
				currentPos = currentPos - currentOffset;
				result = FMOD_Channel_SetPosition(chan, offset + currentPos, FMOD_TIMEUNIT_PCM);
			}
		}
	}

	return FMOD_OK;
}

ConsoleVariable<int> cvar_ensemble_explore_seek("/ensemble_explore_seek", -1);
ConsoleVariable<int> cvar_ensemble_combat_seek("/ensemble_combat_seek", -1);
void EnsembleSounds_t::updatePlayingChannelVolumes()
{
	bool fade_only_mode = (songTransitionMode == TRANSITION_MODE_FADE);
	static ConsoleVariable<float> cvar_ensemble_fade_in("/ensemble_fade_in", 0.02f);
	static ConsoleVariable<float> cvar_ensemble_fade_out("/ensemble_fade_out", 0.01f);
	float fadeoutspeed = fade_only_mode ? 2 * *cvar_ensemble_fade_out : *cvar_ensemble_fade_out;
	float fadeinspeed = *cvar_ensemble_fade_in;

	if ( songTransitionMode == TRANSITION_MODE_FULL )
	{
		fadeoutspeed = 0.f;
		fadeinspeed = 1.f;
	}

	if ( exploreChannel[5] )
	{
		bool playing = false;
		float audibility = 0.f;
		FMOD_Channel_GetAudibility(exploreChannel[5], &audibility);
		FMOD_Channel_IsPlaying(exploreChannel[5], &playing);

		if ( playing && audibility > 0.01 )
		{
			unsigned int pos = 0;
			FMOD_Channel_GetPosition(exploreChannel[5], &pos, FMOD_TIMEUNIT_PCM);
			{
				int lastPoint = -1;
				int index = -1;
				for ( auto& point : exploreSyncPoints )
				{
					++index;
					if ( index == 0 )
					{
						continue;
					}
					if ( point > pos )
					{
						break;
					}
					lastPoint = index;
				}
				if ( lastPoint >= 0 )
				{
					if ( *cvar_ensemble_explore_seek >= 0 )
					{
						lastPoint = *cvar_ensemble_explore_seek;
					}
					if ( lastPoint >= (exploreSyncPointsToSeek.size() - 4) )
					{
						exploreSongSeek = 0; // loop back to beginning
					}
					else
					{
						if ( lastPoint < exploreSyncPointsToSeek.size() )
						{
							exploreSongSeek = exploreSyncPointsToSeek[lastPoint];
						}
						else
						{
							exploreSongSeek = 0; // loop back to beginning, unknown error?
						}
					}
				}
			}
		}
	}

	if ( combatChannel[5] )
	{
		bool playing = false;
		float audibility = 0.f;
		FMOD_Channel_GetAudibility(combatChannel[5], &audibility);
		FMOD_Channel_IsPlaying(combatChannel[5], &playing);
		bool paused = true;
		FMOD_Channel_GetPaused(combatChannel[5], &paused);

		if ( playing && !paused && audibility > 0.0001 )
		{
			unsigned int pos = 0;
			FMOD_Channel_GetPosition(combatChannel[5], &pos, FMOD_TIMEUNIT_PCM);
			{
				int lastPoint = -1;
				int index = -1;
				for ( auto& point : combatSyncPoints )
				{
					++index;
					if ( index % 8 != 0 )
					{
						continue;
					}
					if ( point > pos )
					{
						break;
					}
					lastPoint = index;
				}
				if ( lastPoint >= 0 )
				{
					if ( *cvar_ensemble_combat_seek >= 0 )
					{
						lastPoint = *cvar_ensemble_combat_seek;
					}
					combatSongSeek = lastPoint / 8;
				}
			}
		}
		if ( playing && !paused && audibility > 0.0001 )
		{
			if ( lastUpdateTick != ticks )
			{
				++ticksCombatPlaying;
				lastTickCombatPlaying = ticks;
			}
		}
		else
		{
			ticksCombatPlaying = 0;
		}
	}
	else
	{
		ticksCombatPlaying = 0;
	}

	if ( lastUpdateTick != ticks )
	{
		if ( combat && songTransitionState == TRANSITION_COMBAT_START )
		{
			messagePlayer(clientnum, MESSAGE_DEBUG, "Combat start");
		}
		else if ( !combat && songTransitionState == TRANSITION_COMBAT_ENDING )
		{
			messagePlayer(clientnum, MESSAGE_DEBUG, "Combat end");
		}
		if ( combatDelay > 0 )
		{
			--combatDelay;
		}
	}
	lastUpdateTick = ticks;

	// non-callback volume control if needed
	for ( int i = 0; i < NUMENSEMBLEMUSIC && false; ++i )
	{
		if ( songTransitionState == TRANSITION_EXPLORE || songTransitionState == TRANSITION_COMBAT_ENDED )
		{
			bool playing = false;
			float audibility = 0.f;
			if ( exploreChannel[i] )
			{
				FMOD_Channel_GetAudibility(exploreChannel[i], &audibility);
				FMOD_Channel_IsPlaying(exploreChannel[i], &playing);
				if ( playing )
				{
					unsigned int ms = 0;
					FMOD_Channel_GetPosition(exploreChannel[i], &ms, FMOD_TIMEUNIT_MS);
					if ( ms > 250 /*|| fade_only_mode*/ )
					{
						float volume = 0.f;
						FMOD_Channel_GetVolume(exploreChannel[i], &volume);
						volume = std::min(1.f, volume + fadeinspeed);
						FMOD_Channel_SetVolume(exploreChannel[i], volume);
					}
				}
				else
				{
					FMOD_Channel_SetVolume(exploreChannel[i], 0.f);
				}
			}

			playing = false;
			audibility = 0.f;
			if ( combatChannel[i] )
			{
				FMOD_Channel_GetAudibility(combatChannel[i], &audibility);
				FMOD_Channel_IsPlaying(combatChannel[i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(combatChannel[i], &volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					FMOD_Channel_SetVolume(combatChannel[i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(combatChannel[i], 0.f);
				}
			}

			for ( int j = 0; j < 3; ++j )
			{
				if ( !exploreTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				FMOD_Channel_GetAudibility(exploreTransChannel[j][i], &audibility);
				FMOD_Channel_IsPlaying(exploreTransChannel[j][i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(exploreTransChannel[j][i], &volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					FMOD_Channel_SetVolume(exploreTransChannel[j][i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(exploreTransChannel[j][i], 0.f);
				}
			}

			for ( int j = 0; j < 4; ++j )
			{
				if ( !combatTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				FMOD_Channel_GetAudibility(combatTransChannel[j][i], &audibility);
				FMOD_Channel_IsPlaying(combatTransChannel[j][i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(combatTransChannel[j][i], &volume);
					volume = std::min(1.f, volume + fadeinspeed);
					FMOD_Channel_SetVolume(combatTransChannel[j][i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(combatTransChannel[j][i], 0.f);
				}
			}
		}
		else if ( songTransitionState == TRANSITION_COMBAT )
		{
			bool playing = false;
			float audibility = 0.f;
			if ( exploreChannel[i] )
			{
				FMOD_Channel_GetAudibility(exploreChannel[i], &audibility);
				FMOD_Channel_IsPlaying(exploreChannel[i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(exploreChannel[i], &volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					FMOD_Channel_SetVolume(exploreChannel[i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(exploreChannel[i], 0.f);
				}
			}

			playing = false;
			audibility = 0.f;
			if ( combatChannel[i] )
			{
				FMOD_Channel_GetAudibility(combatChannel[i], &audibility);
				FMOD_Channel_IsPlaying(combatChannel[i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(combatChannel[i], &volume);
					volume = std::min(1.f, volume + fadeinspeed);
					FMOD_Channel_SetVolume(combatChannel[i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(combatChannel[i], 0.f);
				}
			}

			for ( int j = 0; j < 3; ++j )
			{
				if ( !exploreTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				FMOD_Channel_GetAudibility(exploreTransChannel[j][i], &audibility);
				FMOD_Channel_IsPlaying(exploreTransChannel[j][i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(exploreTransChannel[j][i], &volume);
					volume = std::min(1.f, volume + fadeinspeed);
					FMOD_Channel_SetVolume(exploreTransChannel[j][i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(exploreTransChannel[j][i], 0.f);
				}
			}

			for ( int j = 0; j < 4; ++j )
			{
				if ( !combatTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				FMOD_Channel_GetAudibility(combatTransChannel[j][i], &audibility);
				FMOD_Channel_IsPlaying(combatTransChannel[j][i], &playing);
				if ( playing )
				{
					float volume = 0.f;
					FMOD_Channel_GetVolume(combatTransChannel[j][i], &volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					FMOD_Channel_SetVolume(combatTransChannel[j][i], volume);
				}
				else
				{
					FMOD_Channel_SetVolume(combatTransChannel[j][i], 0.f);
				}
			}
		}
	}
}
#endif
