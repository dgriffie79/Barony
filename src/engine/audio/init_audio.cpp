/*-------------------------------------------------------------------------------

	BARONY
	File: init_audio.cpp
	Desc: init, load, exit audio engine stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.h"
#include "../../files.h"
#include "../../ui/LoadingScreen.hpp"
#include "sound.hpp"

#include "../../ui/MainMenu.hpp"

#ifdef USE_FMOD

FMOD_SPEAKERMODE fmod_speakermode = FMOD_SPEAKERMODE_DEFAULT;
const char* fmod_speakermode_strings[FMOD_SPEAKERMODE_MAX] = {
    "FMOD_SPEAKERMODE_DEFAULT",
    "FMOD_SPEAKERMODE_RAW",
    "FMOD_SPEAKERMODE_MONO",
    "FMOD_SPEAKERMODE_STEREO",
    "FMOD_SPEAKERMODE_QUAD",
    "FMOD_SPEAKERMODE_SURROUND",
    "FMOD_SPEAKERMODE_5POINT1",
    "FMOD_SPEAKERMODE_7POINT1",
    "FMOD_SPEAKERMODE_7POINT1POINT4",
};

#endif

bool initSoundEngine()
{
#ifdef USE_FMOD
	printlog("[FMOD]: initializing FMOD...\n");
	fmod_result = FMOD_System_Create(&fmod_system);
	if (FMODErrorCheck())
	{
		printlog("[FMOD]: Failed to create FMOD. DISABLING AUDIO.\n");
		no_sound = true;
		return false;
	}
 
    int numRawSpeakers{};
    int sampleRate{};
    FMOD_SPEAKERMODE speakerMode{};
    FMOD_System_GetSoftwareFormat(fmod_system, &sampleRate, &speakerMode, &numRawSpeakers);
    FMOD_System_SetSoftwareFormat(fmod_system, sampleRate, fmod_speakermode, numRawSpeakers);
	FMOD_ADVANCEDSETTINGS settings{};
	settings.cbSize = sizeof(FMOD_ADVANCEDSETTINGS);
	settings.vol0virtualvol = 0.001;
	FMOD_System_SetAdvancedSettings(fmod_system, &settings);

	// default 64
	FMOD_System_SetSoftwareChannels(fmod_system, 32);

	if (!no_sound)
	{
		FMOD_INITFLAGS flags = FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_VOL0_BECOMES_VIRTUAL | FMOD_INIT_STREAM_FROM_UPDATE | FMOD_INIT_THREAD_UNSAFE;
#ifndef NDEBUG
		flags |= FMOD_INIT_PROFILE_ENABLE | FMOD_INIT_PROFILE_METER_ALL;
#endif
		fmod_result = FMOD_System_Init(fmod_system, fmod_maxchannels, flags, fmod_extraDriverData);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to initialize FMOD. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}

#ifndef NDEBUG
		//FMOD::Debug_Initialize(FMOD_DEBUG_LEVEL_WARNING | FMOD_DEBUG_TYPE_MEMORY);
#endif

		int selected_driver = 0;
		int numDrivers = 0;
		FMOD_System_GetNumDrivers(fmod_system, &numDrivers);
		for ( int i = 0; i < numDrivers; ++i )
		{
            constexpr int driverNameLen = 64;
            char driverName[driverNameLen] = "";
            FMOD_GUID guid;
            int rate{}, channels{};
            FMOD_SPEAKERMODE mode{};
            fmod_result = FMOD_System_GetDriverInfo(fmod_system, i, driverName, driverNameLen, &guid, &rate, &mode, &channels);
			if ( FMODErrorCheck() )
			{
				printlog("[FMOD]: Failed to read audio device index: %d", i);
			}
        
            mode = (FMOD_SPEAKERMODE)std::clamp((int)mode, (int)0, (int)FMOD_SPEAKERMODE_MAX - 1);
            printlog("[FMOD] Audio device found: %d %s | %08x %04x %04x | rate: %d | mode: %s | channels: %d",
                i, driverName, guid.Data1, guid.Data2, guid.Data3, rate, fmod_speakermode_strings[mode], channels);

			uint32_t _1; memcpy(&_1, &guid.Data1, sizeof(_1));
			uint64_t _2; memcpy(&_2, &guid.Data4, sizeof(_2));
			char guid_string[25];
			snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
			if (!selected_driver && MainMenu::current_audio_device == guid_string)
			{
				selected_driver = i;
			}
		}

		FMOD_System_SetDriver(fmod_system, selected_driver);
		FMOD_System_GetDriver(fmod_system, &selected_driver);
		printlog("[FMOD]: Current audio device: %d", selected_driver);

		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &sound_group);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to create sound channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &soundAmbient_group);
		if ( FMODErrorCheck() )
		{
			printlog("F[FMOD]: ailed to create sound ambient channel group.\n");
			no_sound = true;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &soundEnvironment_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create sound environment channel group.\n");
			no_sound = true;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_notification_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create notification channel group.\n");
			no_sound = true;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &soundNotification_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create sound notification channel group.\n");
			no_sound = true;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_group);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_ensemble_global_send_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		FMOD_ChannelGroup_SetVolumeRamp(music_ensemble_global_send_group, true);

		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_ensemble_global_recv_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}

		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_ensemble_local_recv_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_ensemble_local_recv_player[i]);
			if ( FMODErrorCheck() )
			{
				printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
				no_sound = true;
				return false;
			}
			FMOD_ChannelGroup_AddGroup(music_ensemble_local_recv_group, music_ensemble_local_recv_player[i]);
			FMOD_ChannelGroup_SetMode(music_ensemble_local_recv_player[i], FMOD_3D | FMOD_3D_WORLDRELATIVE);
		}
		{
			// add dsp

			//FMOD_DSP* dspeq = 0;
			//fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_MULTIBAND_EQ, &dspeq);
			//FMODErrorCheck();
			//
			//fmod_result = FMOD_Channel_AddDSP(music_ensemble_global_group, 0, dspeq);
			//FMODErrorCheck();
			//
			//FMOD_DSP_SetParameterInt(dspeq, FMOD_DSP_MULTIBAND_EQ_A_FILTER, FMOD_DSP_MULTIBAND_EQ_FILTER_NOTCH);
			//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_FREQUENCY, 2000);
			//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_GAIN, -6);
			//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_Q, 1.f);

			// global sends
			{
				// send group does not output to master bus
				FMOD_DSP* fader = NULL;
				fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_FADER, &fader);
				FMOD_DSP_SetParameterFloat(fader, FMOD_DSP_FADER_GAIN, -80.f); // inaudible
				FMOD_ChannelGroup_AddDSP(music_ensemble_global_send_group, 0, fader);
			}

			// global recv transceivers
			{
				for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
				{
					FMOD_DSP* transceiver = NULL;
					fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_TRANSCEIVER, &transceiver);
					FMOD_ChannelGroup_AddDSP(music_ensemble_global_recv_group, 1, transceiver);
					FMOD_DSP_SetParameterInt(transceiver, FMOD_DSP_TRANSCEIVER_CHANNEL, i + 1); // receive on channel x
					FMOD_DSP_SetParameterFloat(transceiver, FMOD_DSP_TRANSCEIVER_GAIN, -80.f); // inaudible
					FMOD_DSP_SetChannelFormat(transceiver, 0, 2, FMOD_SPEAKERMODE_STEREO); // force stereo on empty channel, otherwise defaults to mono
				}
			}

			// player recv transceivers
			{
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
					{
						FMOD_DSP* transceiver = NULL;
						fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_TRANSCEIVER, &transceiver);
						FMOD_ChannelGroup_AddDSP(music_ensemble_local_recv_player[c], 1, transceiver);
						FMOD_DSP_SetParameterInt(transceiver, FMOD_DSP_TRANSCEIVER_CHANNEL, i + 1); // receive on channel x
						FMOD_DSP_SetParameterFloat(transceiver, FMOD_DSP_TRANSCEIVER_GAIN, -80.f); // inaudible
						FMOD_DSP_SetChannelFormat(transceiver, 0, 2, FMOD_SPEAKERMODE_STEREO); // force stereo on empty channel, otherwise defaults to mono
					}
				}
			}

			// global recv reverb
			{
				FMOD_DSP* dspreverb = 0;
				fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_SFXREVERB, &dspreverb);
				FMODErrorCheck();

				fmod_result = FMOD_ChannelGroup_AddDSP(music_ensemble_global_recv_group, 0, dspreverb);
				FMOD_DSP_SetBypass(dspreverb, true);
				FMODErrorCheck();

				FMOD_REVERB_PROPERTIES props = FMOD_PRESET_OFF;
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DECAYTIME, props.DecayTime);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_EARLYDELAY, props.EarlyDelay);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LATEDELAY, props.LateDelay);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HFREFERENCE, props.HFReference);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HFDECAYRATIO, props.HFDecayRatio);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DIFFUSION, props.Diffusion);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DENSITY, props.Density);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, props.LowShelfFrequency);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LOWSHELFGAIN, props.LowShelfGain);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HIGHCUT, props.HighCut);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_EARLYLATEMIX, props.EarlyLateMix);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_WETLEVEL, props.WetLevel);
				FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f);
			}
		}
		//{
		//	// add dsp

		//	//FMOD_DSP* dspeq = 0;
		//	//fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_MULTIBAND_EQ, &dspeq);
		//	//FMODErrorCheck();
		//	//
		//	//fmod_result = FMOD_Channel_AddDSP(music_ensemble_local_group, 0, dspeq);
		//	//FMODErrorCheck();
		//	//
		//	//FMOD_DSP_SetParameterInt(dspeq, FMOD_DSP_MULTIBAND_EQ_A_FILTER, FMOD_DSP_MULTIBAND_EQ_FILTER_NOTCH);
		//	//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_FREQUENCY, 2000);
		//	//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_GAIN, -6);
		//	//FMOD_DSP_SetParameterFloat(dspeq, FMOD_DSP_MULTIBAND_EQ_A_Q, 1.f);

		//	FMOD_DSP* dspreverb = 0;
		//	fmod_result = FMOD_System_CreateDSPByType(fmod_system, FMOD_DSP_TYPE_SFXREVERB, &dspreverb);
		//	FMODErrorCheck();

		//	fmod_result = FMOD_Channel_AddDSP(music_ensemble_local_group, 0, dspreverb);
		//	FMODErrorCheck();

		//	FMOD_REVERB_PROPERTIES props = FMOD_PRESET_OFF;
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DECAYTIME, props.DecayTime);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_EARLYDELAY, props.EarlyDelay);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LATEDELAY, props.LateDelay);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HFREFERENCE, props.HFReference);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HFDECAYRATIO, props.HFDecayRatio);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DIFFUSION, props.Diffusion);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DENSITY, props.Density);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, props.LowShelfFrequency);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_LOWSHELFGAIN, props.LowShelfGain);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_HIGHCUT, props.HighCut);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_EARLYLATEMIX, props.EarlyLateMix);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_WETLEVEL, props.WetLevel);
		//	FMOD_DSP_SetParameterFloat(dspreverb, FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f);
		//}

		int selected_recording_driver = 0;
		int numRecordingDrivers = 0;
		FMOD_System_GetRecordNumDrivers(fmod_system, &numRecordingDrivers, NULL);
		for ( int i = 0; i < numRecordingDrivers; ++i )
		{
			constexpr int driverNameLen = 64;
			char driverName[driverNameLen] = "";
			FMOD_GUID guid;
			int rate{}, channels{};
			FMOD_SPEAKERMODE mode{};
			FMOD_DRIVER_STATE state{};
			fmod_result = FMOD_System_GetRecordDriverInfo(fmod_system, i, driverName, driverNameLen, &guid, &rate, &mode, &channels, &state);
			if ( FMODErrorCheck() )
			{
				printlog("[FMOD]: Failed to read recording device index: %d", i);
			}
			if ( strstr(driverName, "[loopback]") )
			{
				continue;
			}

			mode = (FMOD_SPEAKERMODE)std::clamp((int)mode, (int)0, (int)FMOD_SPEAKERMODE_MAX - 1);
			printlog("[FMOD] Recording device found: %d %s | %08x %04x %04x | rate: %d | mode: %s | channels: %d",
				i, driverName, guid.Data1, guid.Data2, guid.Data3, rate, fmod_speakermode_strings[mode], channels);

			uint32_t _1; memcpy(&_1, &guid.Data1, sizeof(_1));
			uint64_t _2; memcpy(&_2, &guid.Data4, sizeof(_2));
			char guid_string[25];
			snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
			if ( !selected_recording_driver && MainMenu::current_recording_audio_device == ""
				&& (state & FMOD_DRIVER_STATE_DEFAULT))
			{
				selected_recording_driver = i;
			}
			else if ( !selected_recording_driver && MainMenu::current_recording_audio_device == guid_string )
			{
				selected_recording_driver = i;
			}
		}

		VoiceChat.setRecordingDevice(selected_recording_driver);
	}
#endif

	// saves your ears getting blasted if the game starts without window focus.
	setGlobalVolume(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

	return !no_sound; //No double negatives pls
}

int loadSoundResources(real_t base_load_percent, real_t top_load_percent)
{
	File* fp;
	Uint32 c;
	char name[128];

	if ( !PHYSFS_getRealDir("sound/sounds.txt") )
	{
		printlog("error: could not find file: %s", "sound/sounds.txt");
		return 10;
	}

	// load sound effects
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	printlog("loading sounds...\n");
	fp = openDataFile(soundsDirectory.c_str(), "rb");
	for ( numsounds = 0; !fp->eof(); ++numsounds )
	{
		while ( fp->getc() != '\n' )
		{
			if ( fp->eof() )
			{
				break;
			}
		}
	}
	FileIO::close(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
#ifdef USE_FMOD
	sounds = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*numsounds);
	fp = openDataFile(soundsDirectory.c_str(), "rb");
	char full_path[PATH_MAX];
	for ( c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, 128);
		completePath(full_path, name);
		FMOD_MODE flags = FMOD_DEFAULT | FMOD_3D | FMOD_LOWMEM;
		if ( c == 133 || c == 672 || c == 135 || c == 155 || c == 149 || c == 710 )
		{
			flags |= FMOD_LOOP_NORMAL;
		}
		fmod_result = FMOD_System_CreateSound(fmod_system, full_path, flags, NULL, &sounds[c]);
		if (FMODErrorCheck())
		{
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", full_path, c + 1);
		}
		updateLoadingScreen(base_load_percent + (top_load_percent * c) / numsounds);
	}
	FileIO::close(fp);
	FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0);
#endif

	return 0;
}

void freeSoundResources()
{
	uint32_t c;
	// free sounds
#ifdef USE_FMOD
	printlog("freeing sounds...\n");
	if ( sounds != NULL )
	{
		for ( c = 0; c < numsounds && !no_sound; c++ )
		{
			if (sounds[c] != NULL)
			{
				if (sounds[c] != NULL)
				{
					FMOD_Sound_Release(sounds[c]); //Free the sound's FMOD sound.
				}
			}
		}
		free(sounds); //Then free the sound array.
	}
#endif
}

void exitSoundEngine()
{
#ifdef USE_FMOD
	if ( fmod_system )
	{
// no idea why this causes the game to hang for me.
// someone else investigate? -skrathbun
#ifndef LINUX
		FMOD_System_Close(fmod_system);
		FMOD_System_Release(fmod_system);
#endif
		fmod_system = NULL;
	}
#endif
}

