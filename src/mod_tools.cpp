/*-------------------------------------------------------------------------------

BARONY
File: mod_tools.cpp
Desc: misc modding tools

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/
#define MOD_TOOLS_CPP
#include "items.h"
#include "mod_tools.hpp"
#include "menu.h"
#include "classdescriptions.h"
#include "draw.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "ui/Field.hpp"
#include "ui/Image.hpp"
#include "ui/MainMenu.hpp"
#include "shops.hpp"
#include "interface/ui.hpp"
#include "ui/GameUI.hpp"
#include "init.h"
#include "ui/LoadingScreen.hpp"
#include <thread>
#include <future>
#include <fstream>

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
BaronyRNG MonsterStatCustomManager::monster_stat_rng;
GameplayCustomManager gameplayCustomManager;
GameModeManager_t gameModeManager;
ItemTooltips_t ItemTooltips;
GlyphRenderer_t GlyphHelper;
ScriptTextParser_t ScriptTextParser;
StatueManager_t StatueManager;
DebugTimers_t DebugTimers;

const std::vector<std::string> MonsterStatCustomManager::itemStatusStrings =
{
	"broken",
	"decrepit",
	"worn",
	"serviceable",
	"excellent"
};

const std::vector<std::string> MonsterStatCustomManager::shopkeeperTypeStrings =
{
	"equipment",
	"hats",
	"jewelry",
	"books",
	"apothecary",
	"staffs",
	"food",
	"hardware",
	"hunting",
	"general"
};
