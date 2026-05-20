/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_gamemode.cpp - Game mode management (tutorial, seeded runs, challenge runs)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"


void GameModeManager_t::Tutorial_t::startTutorial(std::string mapToSet)
{
	if ( mapToSet.compare("") == 0 )
	{
		launchHub();
	}
	else
	{
		if ( mapToSet.find(".lmp") == std::string::npos )
		{
			mapToSet.append(".lmp");
		}
		setTutorialMap(mapToSet);
	}
#ifndef EDITOR
	gameModeManager.setMode(gameModeManager.GameModes::GAME_MODE_TUTORIAL);
	GameplayPreferences_t::reset();
	gameplayPreferences[0].process();
#endif
	stats[0]->clearStats();
	strcpy(stats[0]->name, "Player");
	stats[0]->sex = static_cast<sex_t>(local_rng.rand() % 2);
	stats[0]->playerRace = RACE_HUMAN;
	stats[0]->stat_appearance = local_rng.rand() % NUMAPPEARANCES;
	client_classes[0] = CLASS_WARRIOR;
	initClass(0);
}

void GameModeManager_t::Tutorial_t::buttonReturnToTutorialHub(button_t* my)
{
	buttonStartSingleplayer(nullptr);
	gameModeManager.Tutorial.launchHub();
}

void GameModeManager_t::Tutorial_t::buttonRestartTrial(button_t* my)
{
	std::string mapname = map.name;
	if ( mapname.find("Tutorial Hub") == std::string::npos
		&& mapname.find("Tutorial ") != std::string::npos )
	{
		buttonStartSingleplayer(nullptr);
		std::string number = mapname.substr(mapname.find("Tutorial ") + strlen("Tutorial "), 2);
		std::string filename = "tutorial";
		filename.append(std::to_string(stoi(number)));
		filename.append(".lmp");
		gameModeManager.Tutorial.setTutorialMap(filename);
		gameModeManager.Tutorial.dungeonLevel = currentlevel;

		int tutorialNum = stoi(number);
		if ( tutorialNum > 0 && tutorialNum <= gameModeManager.Tutorial.kNumTutorialLevels )
		{
			gameModeManager.Tutorial.onMapRestart(tutorialNum);
		}
		return;
	}
	buttonReturnToTutorialHub(nullptr);
}

#ifdef EDITOR
void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	return;
}
void GameModeManager_t::Tutorial_t::createFirstTutorialCompletedPrompt()
{
	return;
}
#else
void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	MainMenu::openGameoverWindow(0, true);
}

void GameModeManager_t::Tutorial_t::createFirstTutorialCompletedPrompt()
{
	MainMenu::tutorialFirstTimeCompleted();
}
#endif

//TODO: NX PORT: Update for the Switch?
void GameModeManager_t::Tutorial_t::readFromFile()
{
	levels.clear();
	if ( PHYSFS_getRealDir("/data/tutorial_strings.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/tutorial_strings.json");
		inputPath.append("/data/tutorial_strings.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("levels") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}
		int version = d["version"].GetInt();

		for ( rapidjson::Value::ConstMemberIterator level_itr = d["levels"].MemberBegin(); level_itr != d["levels"].MemberEnd(); ++level_itr )
		{
			Tutorial_t::Level_t level;
			level.filename = level_itr->name.GetString();
			level.title = level_itr->value["title"].GetString();
			level.description = level_itr->value["desc"].GetString();
			levels.push_back(level);
		}
		Menu.windowTitle = d["window_title"].GetString();
		Menu.defaultHoverText = d["default_hover_text"].GetString();
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
	else
	{
		return;
	}

	if ( PHYSFS_getRealDir(tutorialScoresFilename.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(tutorialScoresFilename.c_str());
		inputPath.append(tutorialScoresFilename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("levels") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());

			// recreate this file, corrupted?
			printlog("[JSON]: File %s corrupt, recreating...", inputPath.c_str());
			d.Clear();
			d.SetObject();
			CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
			CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(FirstTimePrompt.showFirstTimePrompt));
			CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(firstTutorialCompleted));

			rapidjson::Value levelsObj(rapidjson::kObjectType);
			CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
			for ( auto it = levels.begin(); it != levels.end(); ++it )
			{
				rapidjson::Value level(rapidjson::kObjectType);
				level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
				CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
			}
			writeToFile((cJSON*)(void*)&d);
			return;
		}
		int version = d["version"].GetInt();

		this->FirstTimePrompt.showFirstTimePrompt = d["first_time_prompt"].GetBool();
		if ( d.HasMember("first_tutorial_complete") )
		{
			this->firstTutorialCompleted = d["first_tutorial_complete"].GetBool();
		}

		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			if ( d["levels"].HasMember(it->filename.c_str()) && d["levels"][it->filename.c_str()].HasMember("completion_time") )
			{
				it->completionTime = d["levels"][it->filename.c_str()]["completion_time"].GetUint();
			}
		}
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
	else
	{
		printlog("[JSON]: File %s does not exist, creating...", tutorialScoresFilename.c_str());

		rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(true));
		CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(false));

		this->FirstTimePrompt.showFirstTimePrompt = true;
		this->firstTutorialCompleted = false;

		rapidjson::Value levelsObj(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			rapidjson::Value level(rapidjson::kObjectType);
			level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
			CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
		}
		writeToFile((cJSON*)(void*)&d);
	}
}

void GameModeManager_t::Tutorial_t::writeToDocument()
{
	if ( levels.empty() )
	{
		printlog("[JSON]: Could not write tutorial scores to file due to empty levels vector.");
		return;
	}

	if ( !PHYSFS_getRealDir(tutorialScoresFilename.c_str()) )
	{
		printlog("[JSON]: Error file %s does not exist", tutorialScoresFilename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(tutorialScoresFilename.c_str());
	inputPath.append(tutorialScoresFilename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}
	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	if ( !d.HasMember("version") || !d.HasMember("levels") )
	{
		printlog("[JSON]: File %s corrupt, recreating...", inputPath.c_str());
		d.Clear();
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(FirstTimePrompt.showFirstTimePrompt));
		CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(firstTutorialCompleted));

		rapidjson::Value levelsObj(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			rapidjson::Value level(rapidjson::kObjectType);
			level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
			CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
		}
	}
	else
	{
		d["first_time_prompt"].SetBool(this->FirstTimePrompt.showFirstTimePrompt);
		if ( !d.HasMember("first_tutorial_complete") )
		{
			CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(false));
		}
		d["first_tutorial_complete"].SetBool(this->firstTutorialCompleted);

		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			d["levels"][it->filename.c_str()]["completion_time"].SetUint(it->completionTime);
		}
	}

	writeToFile((cJSON*)(void*)&d);
}

void GameModeManager_t::Tutorial_t::Menu_t::open()
{
	// deprecated
	assert(0);
	return;
}

void GameModeManager_t::Tutorial_t::Menu_t::onClickEntry()
{
	if ( this->selectedMenuItem == -1 )
	{
		return;
	}
	buttonStartSingleplayer(nullptr);
	gameModeManager.setMode(GameModeManager_t::GAME_MODE_TUTORIAL_INIT);
	if ( gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt )
	{
		gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
		gameModeManager.Tutorial.writeToDocument();
	}
	gameModeManager.Tutorial.startTutorial(gameModeManager.Tutorial.levels.at(this->selectedMenuItem).filename);
	steamStatisticUpdate(STEAM_STAT_TUTORIAL_ENTERED, ESteamStatTypes::STEAM_STAT_INT, 1);
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::createPrompt()
{
	return;
	//bWindowOpen = true;
	//showFirstTimePrompt = false;
	//if ( !title_bmp )
	//{
	//	return;
	//}

	//// create window
	//subwindow = 1;
	//subx1 = xres / 2 - ((0.75 * title_bmp->w / 2) + 52);
	//subx2 = xres / 2 + ((0.75 * title_bmp->w / 2) + 52);
	//suby1 = yres / 2 - ((0.75 * title_bmp->h / 2) + 88);
	//suby2 = yres / 2 + ((0.75 * title_bmp->h / 2) + 88);
	//strcpy(subtext, "");

	//Uint32 centerWindowX = subx1 + (subx2 - subx1) / 2;

	//button_t* button = newButton();
	//strcpy(button->label, Language::get(3965));
	//button->sizex = strlen(Language::get(3965)) * 10 + 8;
	//button->sizey = 20;
	//button->x = centerWindowX - button->sizex / 2;
	//button->y = suby2 - 28 - 24;
	//button->action = &buttonPromptEnterTutorialHub;
	//button->visible = 1;
	//button->focused = 1;

	//button = newButton();
	//strcpy(button->label, Language::get(3966));
	//button->sizex = strlen(Language::get(3966)) * 12 + 8;
	//button->sizey = 20;
	//button->x = centerWindowX - button->sizex / 2;
	//button->y = suby2 - 28;
	//button->action = &buttonSkipPrompt;
	//button->visible = 1;
	//button->focused = 1;
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::drawDialogue()
{
	return;
	/*if ( !bWindowOpen )
	{
		return;
	}
	Uint32 centerWindowX = subx1 + (subx2 - subx1) / 2;

	SDL_Rect pos;
	pos.x = centerWindowX - 0.75 * title_bmp->w / 2;
	pos.y = suby1 + 4;
	pos.w = 0.75 * title_bmp->w;
	pos.h = 0.75 * title_bmp->h;
	SDL_Rect scaled;
	scaled.x = 0;
	scaled.y = 0;
	scaled.w = title_bmp->w * 0.75;
	scaled.h = title_bmp->h * 0.75;
	drawImageScaled(title_bmp, nullptr, &pos);
	
	ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(Language::get(3936)) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 13, makeColorRGB(255, 255, 0), Language::get(3936));
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(Language::get(3967)) * TTF12_WIDTH) / 2, suby2 + 8 - TTF12_HEIGHT * 11, Language::get(3967));
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(Language::get(3967)) * TTF12_WIDTH) / 2 - TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 11, Language::get(3968));*/
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::buttonSkipPrompt(button_t* my)
{
	gameModeManager.Tutorial.FirstTimePrompt.doButtonSkipPrompt = true;
	gameModeManager.Tutorial.writeToDocument();
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::buttonPromptEnterTutorialHub(button_t* my)
{
	gameModeManager.Tutorial.Menu.selectedMenuItem = 0; // set the tutorial hub to enter.
	gameModeManager.Tutorial.Menu.onClickEntry();
	gameModeManager.Tutorial.writeToDocument();
}

void GameModeManager_t::CurrentSession_t::SeededRun_t::setup(std::string _seedString)
{
	if ( _seedString == "" )
	{
		seed = 0;
		seedString = "";
		return;
	}
	int num = atoi(_seedString.c_str());
	if ( num == 0 )
	{
		seed = djb2Hash(const_cast<char*>(_seedString.c_str()));
	}
	else
	{
		seed = num;
	}
	seedString = _seedString;
}

void GameModeManager_t::CurrentSession_t::SeededRun_t::reset()
{
	seed = 0;
	seedString = "";
}

bool GameModeManager_t::allowsSaves()
{
	if ( currentMode == GAME_MODE_DEFAULT )
	{
		return true;
	}
	else if ( currentMode == GAME_MODE_CUSTOM_RUN )
	{
		return true;
	}
	return false;
}

void GameModeManager_t::setMode(const GameModes mode)
{
	currentMode = mode;
}

bool GameModeManager_t::allowsStatisticsOrAchievements(const char* achName, int statIndex)
{
	if ( currentMode == GAME_MODE_CUSTOM_RUN && currentSession.challengeRun.isActive()
		&& currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		if ( achName )
		{
			if ( !strcmp(achName, "BARONY_ACH_BLOOM_PLANTED") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_DUNGEONSEED") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_GROWTH_MINDSET") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_REAP_SOW") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_SPROUTS") )
			{
				return true;
			}
		}
		else if ( statIndex >= 0 )
		{
			switch ( statIndex )
			{
				case SteamStatIndexes::STEAM_STAT_DUNGEONSEED:
					return true;
				default:
					break;
			}
		}
		return false;
	}
	if ( currentMode == GAME_MODE_DEFAULT || currentMode == GAME_MODE_TUTORIAL
		|| currentMode == GAME_MODE_CUSTOM_RUN 
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsHiscores()
{
	if ( currentMode == GAME_MODE_CUSTOM_RUN && currentSession.challengeRun.isActive()
		&& currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		return false;
	}
	if ( currentMode == GAME_MODE_DEFAULT || currentMode == GAME_MODE_CUSTOM_RUN
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::isFastDeathGrave()
{
	if ( currentMode == GAME_MODE_TUTORIAL || currentMode == GAME_MODE_TUTORIAL_INIT )
	{
		return true;
	}
	if ( currentMode == GAME_MODE_CUSTOM_RUN )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsGlobalHiscores()
{
	if ( currentMode == GAME_MODE_DEFAULT )
	{
		if ( currentSession.seededRun.seed == 0 )
		{
			return true;
		}
	}
	else if ( (currentMode == GAME_MODE_CUSTOM_RUN
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT) )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsBoulderBreak()
{
	if ( currentMode != GAME_MODE_TUTORIAL )
	{
		return true;
	}
	return false;
}

std::vector<std::string> GameModeManager_t::CurrentSession_t::SeededRun_t::prefixes;
std::vector<std::string> GameModeManager_t::CurrentSession_t::SeededRun_t::suffixes;

void GameModeManager_t::CurrentSession_t::SeededRun_t::readSeedNamesFromFile()
{
	const std::string filename = "data/seed_names.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[10000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("prefixes") || !d.HasMember("suffixes") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	prefixes.clear();
	suffixes.clear();
	for ( auto it = d["prefixes"].Begin(); it != d["prefixes"].End(); ++it )
	{
		prefixes.push_back(it->GetString());
	}
	for ( auto it = d["suffixes"].Begin(); it != d["suffixes"].End(); ++it )
	{
		suffixes.push_back(it->GetString());
	}
}

#ifndef EDITOR
void GameModeManager_t::CurrentSession_t::ChallengeRun_t::updateKillEvent(Entity* entity)
{
	if ( multiplayer == CLIENT || !isActive() || !entity )
	{
		return;
	}

	if ( gameModeManager.currentSession.challengeRun.numKills < 0 )
	{
		return;
	}
	if ( !(eventType == CHEVENT_KILLS_MONSTERS
		|| eventType == CHEVENT_KILLS_FURNITURE) )
	{
		return;
	}

	if ( entity->behavior == &actMonster && eventType != CHEVENT_KILLS_MONSTERS )
	{
		return;
	}
	if ( entity->behavior == &actFurniture && eventType != CHEVENT_KILLS_FURNITURE )
	{
		return;
	}

	auto& killTotal = gameStatistics[STATISTICS_TOTAL_KILLS];
	killTotal++;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		achievementObserver.playerAchievements[i].totalKillsTickUpdate = true;
	}

	if ( killTotal <= gameModeManager.currentSession.challengeRun.numKills )
	{
		if ( killTotal % 10 == 0 || killTotal == 1 || killTotal == gameModeManager.currentSession.challengeRun.numKills )
		{
			const char* challengeName = "CHALLENGE_MONSTER_KILLS";
			if ( eventType == CHEVENT_KILLS_FURNITURE )
			{
				challengeName = "CHALLENGE_FURNITURE_KILLS";
			}
			UIToastNotificationManager.createStatisticUpdateNotification(challengeName, killTotal, gameModeManager.currentSession.challengeRun.numKills);
			if ( multiplayer == SERVER )
			{
				for ( int i = 1; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && !players[i]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "CHCT");
						SDLNet_Write16((Sint16)killTotal, &net_packet->data[4]);
						SDLNet_Write16((Sint16)gameModeManager.currentSession.challengeRun.numKills, &net_packet->data[6]);
						net_packet->data[8] = eventType;
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 9;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
			}
		}
	}
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::applySettings()
{
	if ( !inUse ) { return; }

	svFlags = setFlags;
	svFlags |= SV_FLAG_HUNGER;
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::setup(std::string _scenario)
{
	reset();
	scenarioStr = _scenario;
	if ( scenarioStr == "" )
	{
		return;
	}

	inUse = loadScenario();
	if ( inUse )
	{
		printlog("[Challenge]: Loaded scenario");
	}
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::reset()
{
	if ( inUse )
	{
		printlog("[Challenge]: Resetting");
	}
	inUse = false;

	if ( !baseStats )
	{
		baseStats = new Stat(0);
	}
	if ( !addStats )
	{
		addStats = new Stat(0);
	}

	addStats->clearStats();
	addStats->HP = 0;
	addStats->MAXHP = 0;
	addStats->MP = 0;
	addStats->MAXMP = 0;

	baseStats->clearStats();
	baseStats->HP = 0;
	baseStats->MAXHP = 0;
	baseStats->MP = 0;
	baseStats->MAXMP = 0;

	seed = 0;
	lockedFlags = 0;
	setFlags = 0;
	classnum = -1;
	race = -1;
	customBaseStats = false;
	customAddStats = false;

	globalXPPercent = 100;
	globalGoldPercent = 100;
	playerWeightPercent = 100;
	playerSpeedMax = 12.5;

	eventType = -1;
	winLevel = -1;
	startLevel = -1;
	winCondition = -1;
	numKills = -1;

}

#ifndef NDEBUG
static ConsoleVariable<int> cvar_challengerace("/challengerace", -1);
static ConsoleVariable<int> cvar_challengeclass("/challengeclass", -1);
static ConsoleVariable<int> cvar_challengeevent("/challengeevent", -1);
#endif

bool GameModeManager_t::CurrentSession_t::ChallengeRun_t::loadScenario()
{
	rapidjson::Document d;
	const char* json = scenarioStr.c_str();
	d.Parse(json);

	if ( !d.HasMember("version") || !d.HasMember("seed") || !d.HasMember("lockedFlags") || !d.HasMember("setFlags") )
	{
		printlog("[JSON]: Error: Scenario has no 'version' value in json file, or JSON syntax incorrect!");
		reset();
		return false;
	}

	seed = d["seed"].GetUint();
	seed_word = d["seed_word"].GetString();
	lockedFlags = d["lockedFlags"].GetUint();
	setFlags = d["setFlags"].GetUint();
	lid = d["lid"].GetString();
	lid_version = d["lid_version"].GetInt();

	if ( d.HasMember("base_stats") )
	{
		const rapidjson::Value& stats = d["base_stats"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("enabled") == 0 )
			{
				customBaseStats = itr->value.GetInt() == 0 ? false : true;
			}
			else if ( name.compare("HP") == 0 )
			{
				baseStats->HP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXHP") == 0 )
			{
				baseStats->MAXHP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MP") == 0 )
			{
				baseStats->MP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXMP") == 0 )
			{
				baseStats->MAXMP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("STR") == 0 )
			{
				baseStats->STR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("DEX") == 0 )
			{
				baseStats->DEX = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CON") == 0 )
			{
				baseStats->CON = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("INT") == 0 )
			{
				baseStats->INT = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PER") == 0 )
			{
				baseStats->PER = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CHR") == 0 )
			{
				baseStats->CHR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("EXP") == 0 )
			{
				baseStats->EXP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("LVL") == 0 )
			{
				baseStats->LVL = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("GOLD") == 0 )
			{
				baseStats->GOLD = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PROFICIENCIES") == 0 )
			{
				int index = -1;
				for ( auto arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
				{
					++index;
					if ( index >= NUMPROFICIENCIES )
					{
						break;
					}
					baseStats->setProficiency(index, arr_itr->GetInt());
				}
			}
		}
	}

	if ( d.HasMember("add_stats") )
	{
		const rapidjson::Value& stats = d["add_stats"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("enabled") == 0 )
			{
				customAddStats = itr->value.GetInt() == 0 ? false : true;
			}
			else if ( name.compare("HP") == 0 )
			{
				addStats->HP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXHP") == 0 )
			{
				addStats->MAXHP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MP") == 0 )
			{
				addStats->MP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXMP") == 0 )
			{
				addStats->MAXMP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("STR") == 0 )
			{
				addStats->STR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("DEX") == 0 )
			{
				addStats->DEX = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CON") == 0 )
			{
				addStats->CON = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("INT") == 0 )
			{
				addStats->INT = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PER") == 0 )
			{
				addStats->PER = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CHR") == 0 )
			{
				addStats->CHR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("EXP") == 0 )
			{
				addStats->EXP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("LVL") == 0 )
			{
				addStats->LVL = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("GOLD") == 0 )
			{
				addStats->GOLD = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PROFICIENCIES") == 0 )
			{
				int index = -1;
				for ( auto arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
				{
					++index;
					if ( index >= NUMPROFICIENCIES )
					{
						break;
					}
					addStats->setProficiency(index, arr_itr->GetInt());
				}
			}
		}
	}

	if ( d.HasMember("character") )
	{
		const rapidjson::Value& stats = d["character"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("class") == 0 )
			{
				classnum = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("race") == 0 )
			{
				race = static_cast<Sint32>(itr->value.GetInt());
			}
		}
	}

	if ( d.HasMember("gameplay") )
	{
		const rapidjson::Value& stats = d["gameplay"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("winLevel") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					winLevel = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("event_type") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					eventType = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("numKills") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					numKills = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("startLevel") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					startLevel = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("winCondition") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					winCondition = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("globalXPPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					globalXPPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("globalGoldPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					globalGoldPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("playerWeightPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					playerWeightPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("playerSpeedMax") == 0 )
			{
				if ( itr->value.GetFloat() >= 0 )
				{
					playerSpeedMax = static_cast<Sint32>(itr->value.GetFloat());
				}
			}
		}
	}

#ifndef NDEBUG
	if ( *cvar_challengerace >= 0 )
	{
		race = *cvar_challengerace;
	}
	if ( *cvar_challengeclass >= 0 )
	{
		classnum = *cvar_challengeclass;
	}
	if ( *cvar_challengeevent >= 0 )
	{
		eventType = *cvar_challengeevent;
	}
#endif

	return true;
}
#endif

