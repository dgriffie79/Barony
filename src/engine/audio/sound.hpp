/*-------------------------------------------------------------------------------

	BARONY
	File: sound.hpp
	Desc: Defines sound related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define FMOD_AUDIO_GUID_FMT "%.8x%.16llx"

#include <stdio.h>
#ifdef USE_FMOD
#include <fmod.hpp>
#endif
#include <mutex>
#include <queue>
#include "../../interface/consolecommand.hpp"

extern Uint32 numsounds;
bool initSoundEngine(); //If it fails to initialize the sound engine, it'll just disable audio.
void exitSoundEngine();
int loadSoundResources(real_t base_load_percent, real_t top_load_percent);
void freeSoundResources();
// all parameters should be in ranges of [0.0 - 1.0]
void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment, real_t notification);
void setAudioDevice(const std::string& device);
void setRecordDevice(const std::string& device);
bool loadMusic();

#ifdef USE_FMOD

#define SOUND
#define MUSIC

extern FMOD_SPEAKERMODE fmod_speakermode;

extern const char* fmod_speakermode_strings[FMOD_SPEAKERMODE_MAX]; 

extern FMOD::System* fmod_system;

extern FMOD_RESULT fmod_result;

extern int fmod_maxchannels;
extern int fmod_flags;
extern void* fmod_extraDriverData;
extern bool levelmusicplaying;

extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;

extern FMOD::Sound** sounds;
extern FMOD::Sound** minesmusic;
#define NUMMINESMUSIC 5
extern FMOD::Sound** swampmusic;
#define NUMSWAMPMUSIC 4
extern FMOD::Sound** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern FMOD::Sound** ruinsmusic;
#define NUMRUINSMUSIC 3
extern FMOD::Sound** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern FMOD::Sound** hellmusic;
#define NUMHELLMUSIC 3
extern FMOD::Sound** intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern FMOD::Sound** minotaurmusic, *herxmusic, *templemusic;
extern FMOD::Sound* endgamemusic, *escapemusic, *devilmusic, *sanctummusic, *tutorialmusic, *introstorymusic, *gameovermusic;
extern FMOD::Sound* introductionmusic;
#define NUMMINOTAURMUSIC 2
extern FMOD::Sound** cavesmusic;
extern FMOD::Sound** citadelmusic;
extern FMOD::Sound* gnomishminesmusic;
extern FMOD::Sound* greatcastlemusic;
extern FMOD::Sound* sokobanmusic;
extern FMOD::Sound* caveslairmusic;
extern FMOD::Sound* bramscastlemusic;
extern FMOD::Sound* hamletmusic;
extern FMOD::Sound** fortressmusic;
#define NUMCAVESMUSIC 3
#define NUMCITADELMUSIC 3
#define NUMINTROMUSIC 3
#define NUMFORTRESSMUSIC 2
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern FMOD::Channel* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.

extern FMOD::ChannelGroup* sound_group, *music_group;
extern FMOD::ChannelGroup* soundAmbient_group, *soundEnvironment_group, *music_notification_group, *soundNotification_group;

#define NUMENSEMBLEMUSIC 8
extern FMOD::ChannelGroup* music_ensemble_global_send_group;
extern FMOD::ChannelGroup* music_ensemble_global_recv_group;
extern FMOD::ChannelGroup* music_ensemble_local_recv_player[MAXPLAYERS];
extern FMOD::ChannelGroup* music_ensemble_local_recv_group;
extern ConsoleVariable<float> cvar_ensemble_vol_bg;
extern ConsoleVariable<int> cvar_ensemble_explore_seek;
extern ConsoleVariable<int> cvar_ensemble_combat_seek;
struct EnsembleSounds_t
{
    float ensemble_recv_global_volume = 0.f;
    float ensemble_recv_player_volume = 0.f;
    static const int NUM_EXPLORE_TRANS = 4;
    static const int NUM_COMBAT_TRANS = 4;
    FMOD::Sound* exploreSound[NUMENSEMBLEMUSIC] = { nullptr };
    FMOD::Channel* exploreChannel[NUMENSEMBLEMUSIC] = { nullptr };

    FMOD::Sound* combatSound[NUMENSEMBLEMUSIC] = { nullptr };
    FMOD::Channel* combatChannel[NUMENSEMBLEMUSIC] = { nullptr };

    FMOD::Sound* exploreTransSound[NUM_EXPLORE_TRANS][NUMENSEMBLEMUSIC] = { nullptr };
    FMOD::Channel* exploreTransChannel[NUM_EXPLORE_TRANS][NUMENSEMBLEMUSIC] = { nullptr };

    FMOD::Sound* combatTransSound[NUM_COMBAT_TRANS][NUMENSEMBLEMUSIC] = { nullptr };
    FMOD::Channel* combatTransChannel[NUM_COMBAT_TRANS][NUMENSEMBLEMUSIC] = { nullptr };

    FMOD::ChannelGroup* transceiver_group[NUMENSEMBLEMUSIC] = { nullptr };

    enum SongTransitionState
    {
        TRANSITION_EXPLORE,
        TRANSITION_COMBAT_START,
        TRANSITION_COMBAT,
        TRANSITION_COMBAT_ENDING,
        TRANSITION_COMBAT_ENDED
    };
    SongTransitionState songTransitionState = TRANSITION_EXPLORE;
    enum TransitionMode
    {
        TRANSITION_MODE_FULL,
        TRANSITION_MODE_FADE,
        TRANSITION_MODE_FADE_HALF,
        TRANSITION_MODE_DEFAULT
    };
    TransitionMode songTransitionMode = TRANSITION_MODE_DEFAULT;
    void setup();
    void playSong();
    void deinit();
    void stopPlaying(bool setCombatDelay);
    unsigned int exploreSoundSyncPointInterval = 0;
    unsigned int combatSoundSyncPointInterval = 0;
    int exploreSongSeek = 0;
    int combatSongSeek = 0;
    std::vector<unsigned int> exploreSyncPoints;
    std::vector<int> exploreSyncPointsToSeek;
    std::vector<int> exploreSyncPointsUnique;
    std::vector<unsigned int> combatSyncPoints;
    void updatePlayingChannelVolumes();
    int combatBeat = 0;
    Uint32 ticksCombatPlaying = 0;
    Uint32 lastTickCombatPlaying = 0;
    Uint32 combatDelay = 0;
    Uint32 lastUpdateTick = 0;
    bool firstTimeSetup = true;
};
extern EnsembleSounds_t ensembleSounds;


/*
 * Checks for FMOD errors. Store return value of all FMOD functions in fmod_result so that this funtion can access it and check for errors.
 * Returns true on error (and prints an error message), false if everything went fine.
 */
bool FMODErrorCheck();

void sound_update(int player, int index, int numplayers);

FMOD::Channel* playSoundPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSound(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotification(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundVelocity();

void stopMusic();
void playMusic(FMOD::Sound* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defaults every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment, dynamicAmbientVolume, dynamicEnvironmentVolume;
extern bool sfxUseDynamicAmbientVolume, sfxUseDynamicEnvironmentVolume;

class VoiceChat_t
{
    static int packetVoiceDataIdx; // index start of voice data in VOIP packet
	int recording_latency_ms = 50;
	int recordDeviceIndex = 0;
	bool bInit = false;
	int nativeRate = 0;
	int nativeChannels = 1;
	FMOD::Sound* recordingSound = nullptr;
	FMOD::Channel* recordingChannel = nullptr;
	unsigned int recordingSoundLength = 0;
	unsigned int recordingSamples = 0;
	unsigned int recordingAdjustedLatency = 0;
	unsigned int recordingDesiredLatency = 0;
	unsigned int recordingLastPos = 0;
	Uint32 lastRecordTick = 0;
	bool bIsRecording = false;
	std::vector<std::vector<char>> recordingDatagrams;
	Uint32 datagramSequence = 0;
	UDPpacket* loopbackPacket = nullptr;
public:
    enum DSPOrder : int;
    bool mainMenuAudioTabOpen();
    bool allowInputs = false;
    static constexpr float kMaxGain = 10.f;
    static constexpr float kNormalizeFadeTime = 100.f;
    static constexpr float kMaxNormalizeAmp = 100.f;
    static constexpr float kMaxNormalizeThreshold = 1.f;
	bool useSystem = false;
	bool bRecordingInit = false;
	float loopback_input_volume = 0.f;
	float loopback_output_volume = 0.f;
    struct AudioSettings_t
    {
        bool loopback_local_record = false;
        float voice_global_volume = 100.f;
        bool enable_voice_input = false;
        bool enable_voice_receive = true;
        float recordingGain = 100.f;
        bool pushToTalk = true;
        bool use_custom_rolloff = true;
        float recordingNormalizeAmp = 20.f;
        float recordingNormalizeThreshold = 1.0f;
    };
    AudioSettings_t mainmenuSettings;
    AudioSettings_t activeSettings;
    enum AudioSettingBool
    {
        VOICE_SETTING_LOOPBACK_LOCAL_RECORD,
        VOICE_SETTING_ENABLE_VOICE_INPUT,
        VOICE_SETTING_ENABLE_VOICE_RECEIVE,
        VOICE_SETTING_PUSHTOTALK,
        VOICE_SETTING_USE_CUSTOM_ROLLOFF
    };
    enum AudioSettingFloat
    {
        VOICE_SETTING_VOICE_GLOBAL_VOLUME,
        VOICE_SETTING_RECORDINGGAIN,
        VOICE_SETTING_NORMALIZE_AMP,
        VOICE_SETTING_NORMALIZE_THRESHOLD
    };
    bool getAudioSettingBool(AudioSettingBool option);
    float getAudioSettingFloat(AudioSettingFloat option);
    void updateOnMapChange3DRolloff();
	bool using_encoding = false;
	bool voiceToggleTalk = false;
	FMOD::ChannelGroup* outChannelGroup = nullptr;
	class PlayerChannels_t
	{
	public:
		std::mutex audio_queue_mutex;
		float channelGain = 100.f;
        float localChannelGain = 100.f;
        float normalize_amp = 20.f;
        float normalize_threshold = 0.1f;
		int talkingTicks = 0;
        int lastAudibleTick = 0;
		int player = -1;
		int drift_ms = 10;
		int native_rate = 0;
		int playback_latency_ms = 150;
		float monitor_input_volume = 0.f;
		float monitor_output_volume = 0.f;
		unsigned int minimumSamplesWritten = -1;
		unsigned int driftThreshold = 0;
		float driftCorrectionPercentage = 0.5f;
		static const size_t audioQueueSizeLimit = 48000;
		std::vector<char> audioQueue;
		int totalSamplesRead = 0;
		int totalSamplesWritten = 0;
		void updateLatency();
		FMOD::Sound* outputSound = nullptr;
		FMOD::Channel* outputChannel = nullptr;
		unsigned int desiredLatency = 0;
		unsigned int adjustedLatency = 0;
		int actualLatency = 0;
		void setupPlayback();
		void deinit();
		std::priority_queue<std::pair<int, std::vector<char>>> voiceDatagrams;
	};

	PlayerChannels_t PlayerChannels[MAXPLAYERS];

	VoiceChat_t();

	void setRecordingDevice(int device_index);
	void init();
	void deinitRecording(bool resetPushTalkToggle = true);
	void initRecording();
	void deinit();
	void updateRecording();
    const char* getVoiceChatBindingName(int player);
    void pushAvailableDatagrams();
	void update();
	void receivePacket(UDPpacket* packet);
	void sendPackets();
    enum VoicePlayerBarState
    {
        VOICE_STATE_NONE,
        VOICE_STATE_INERT,
        VOICE_STATE_MUTE,
        VOICE_STATE_INACTIVE,
        VOICE_STATE_INACTIVE_PTT,
        VOICE_STATE_ACTIVE1,
        VOICE_STATE_ACTIVE2
    };
    VoicePlayerBarState getVoiceState(const int player);

    static constexpr int FRAME_SIZE = 480;
    static constexpr int BITRATE = 24000;
    static void logError(const char* str, ...)
    {
        char newstr[1024] = { 0 };
        va_list argptr;

        // format the content
        va_start(argptr, str);
        vsnprintf(newstr, 1023, str, argptr);
        va_end(argptr);
        printlog("[FMOD Voice Error]: %s", newstr);
    }
    static void logInfo(const char* str, ...)
    {
        char newstr[1024] = { 0 };
        va_list argptr;

        // format the content
        va_start(argptr, str);
        vsnprintf(newstr, 1023, str, argptr);
        va_end(argptr);
        printlog("[FMOD Voice Info]: %s", newstr);
    }
	class RingBuffer
	{
	public:
		RingBuffer(int sizeBytes);
		~RingBuffer();
		int Read(char* dataPtr, int numBytes);
		int Write(char* dataPtr, int numBytes);
		bool Empty(void);
		int GetSize();
		int GetWriteAvail();
		int GetReadAvail();
	private:
		char* _data;
		int _size;
		int _readPtr;
		int _writePtr;
		int _writeBytesAvail;
	};
	static RingBuffer ringBufferRecord;
};
extern VoiceChat_t VoiceChat;


#else
void* playSound(Uint16, Uint8);
void* playSoundPos(real_t x, real_t y, Uint16, Uint8);
void* playSoundPosLocal(real_t, real_t, Uint16, Uint8);
void* playSoundEntity(Entity*, Uint16, Uint8);
void* playSoundEntityLocal(Entity*, Uint16, Uint8);
void* playSoundPlayer(int, Uint16, Uint8);
void* playSoundNotification(Uint16, Uint8);
void* playSoundNotificationPlayer(int, Uint16, Uint8);
#endif
