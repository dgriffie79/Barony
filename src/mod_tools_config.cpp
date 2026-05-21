/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_config.cpp - Class hotbar config, local achievements, gameplay preferences (converted from rapidjson to cJSON)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

ClassHotbarConfig_t::ClassHotbar_t ClassHotbarConfig_t::ClassHotbarsDefault[NUMCLASSES];
ClassHotbarConfig_t::ClassHotbar_t ClassHotbarConfig_t::ClassHotbars[NUMCLASSES];

void ClassHotbarConfig_t::writeToFile(HotbarConfigType fileWriteType, HotbarConfigWriteMode writeMode)
{
	std::string outputDir = "/config/";
	if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		outputDir = "/data/";
	}

	if ( !PHYSFS_getRealDir(outputDir.c_str()) )
	{
		printlog("[JSON]: ClassHotbarConfig_t: %s directory not found", outputDir.c_str());
		return;
	}
	std::string outputPath = PHYSFS_getRealDir(outputDir.c_str());
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "config/class_hotbars.json";
	if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		fileName = "data/class_hotbars.json";
	}
	outputPath.append(fileName.c_str());

	cJSON* exportDocument = NULL;
	bool writeNewFile = true;
	if ( fileWriteType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		File* fp = FileIO::open(outputPath.c_str(), "rb");
		if ( !fp )
		{
			if ( writeMode == HOTBAR_CONFIG_DELETE )
			{
				printlog("[JSON]: Could not locate json file %s, skipping deletion...", outputPath.c_str());
				return;
			}
			else
			{
				printlog("[JSON]: Could not locate json file %s, creating new file...", outputPath.c_str());
				fp = FileIO::open(outputPath.c_str(), "wb");
				if ( !fp )
				{
					printlog("[JSON]: Error opening json file %s for write!", outputPath.c_str());
					return;
				}
				exportDocument = cJSON_CreateObject();
			}
		}
		else
		{
			char buf[80000];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
			buf[count] = '\0';
			FileIO::close(fp);

			exportDocument = cJSON_Parse(buf);
			if ( !exportDocument )
			{
				printlog("[JSON]: Could not parse existing file %s, creating new...", outputPath.c_str());
				exportDocument = cJSON_CreateObject();
			}
			else
			{
				printlog("[JSON]: Loaded existing file %s", outputPath.c_str());
				writeNewFile = false;
			}
		}
	}
	else
	{
		exportDocument = cJSON_CreateObject();
	}

	const int VERSION = 1;

	if ( fileWriteType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		std::string classname = playerClassInternalNames[client_classes[clientnum]];
		if ( writeNewFile )
		{
			cJSON_AddNumberToObject(exportDocument, "version", VERSION);
			cJSON_AddObjectToObject(exportDocument, "classes");
		}
		else
		{
			cJSON* verItem = cJSON_GetObjectItem(exportDocument, "version");
			if ( verItem ) verItem->valueint = VERSION;
		}

		cJSON* classesObj = cJSON_GetObjectItem(exportDocument, "classes");
		if ( !classesObj )
		{
			classesObj = cJSON_AddObjectToObject(exportDocument, "classes");
		}
		if ( !cJSON_HasObjectItem(classesObj, classname.c_str()) )
		{
			if ( writeMode == HOTBAR_CONFIG_DELETE )
			{
				printlog("[JSON]: Custom layout not found for class '%s', skipping deletion...", classname.c_str());
				cJSON_Delete(exportDocument);
				return;
			}
			cJSON_AddObjectToObject(classesObj, classname.c_str());
		}

		if ( writeMode == HOTBAR_CONFIG_DELETE )
		{
			printlog("[JSON]: Custom layout found for class '%s', removing...", classname.c_str());
			cJSON_DeleteItemFromObject(classesObj, classname.c_str());
		}
		else
		{
			auto& hotbar_t = players[clientnum]->hotbar;
			std::string layoutname = "classic";
			if ( hotbar_t.useHotbarFaceMenu )
			{
				layoutname = "modern";
			}
			cJSON* classObj = cJSON_GetObjectItem(classesObj, classname.c_str());
			if ( !classObj )
			{
				classObj = cJSON_AddObjectToObject(classesObj, classname.c_str());
			}
			if ( !cJSON_HasObjectItem(classObj, layoutname.c_str()) )
			{
				cJSON_AddObjectToObject(classObj, layoutname.c_str());
			}

			cJSON* layoutObj = cJSON_GetObjectItem(classObj, layoutname.c_str());
			for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
			{
				std::string slotnum = std::to_string(i);
				if ( !cJSON_HasObjectItem(layoutObj, slotnum.c_str()) )
				{
					cJSON_AddObjectToObject(layoutObj, slotnum.c_str());
				}

				cJSON* slot = cJSON_GetObjectItem(layoutObj, slotnum.c_str());
				if ( !cJSON_HasObjectItem(slot, "items") )
				{
					cJSON_AddArrayToObject(slot, "items");
				}
				else
				{
					// clear array to overwrite
					cJSON* itemsArr = cJSON_GetObjectItem(slot, "items");
					if ( itemsArr )
					{
						// remove all items from array
						cJSON* item = itemsArr->child;
						while ( item )
						{
							cJSON* next = item->next;
							cJSON_DeleteItemFromArray(itemsArr, 0);
							item = next;
						}
					}
				}
				if ( hotbar_t.slots()[i].item != 0 )
				{
					if ( Item* item = uidToItem(hotbar_t.slots()[i].item) )
					{
						if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
						{
							std::string itemstr = itemNameStrings[item->type + 2];
							if ( itemstr == "spell_item" )
							{
								if ( spell_t* spell = getSpellFromItem(clientnum, item, false) )
								{
									itemstr = ItemTooltips.spellItems[spell->ID].internalName;
								}
								else
								{
									continue;
								}
							}
							cJSON* itemsArr = cJSON_GetObjectItem(slot, "items");
							if ( itemsArr )
							{
								cJSON_AddItemToArray(itemsArr, cJSON_CreateString(itemstr.c_str()));
							}
						}
					}
				}
			}
		}
	}
	else if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		cJSON_AddNumberToObject(exportDocument, "version", VERSION);
		cJSON* allClassesObject = cJSON_AddObjectToObject(exportDocument, "classes");

		int classIndex = -1;
		for ( auto& classname : playerClassInternalNames )
		{
			++classIndex;
			cJSON* classObj = cJSON_CreateObject();
			cJSON_AddObjectToObject(classObj, "classic");
			cJSON_AddObjectToObject(classObj, "modern");

			auto& hotbar_t = players[clientnum]->hotbar;

			std::vector<std::string> layoutTypes = { "classic", "modern" };
			for ( auto& layout : layoutTypes )
			{
				if ( layout == "classic" )
				{
					hotbar_t.useHotbarFaceMenu = false;
				}
				else
				{
					hotbar_t.useHotbarFaceMenu = true;
				}
				stats[clientnum]->clearStats();
				client_classes[clientnum] = classIndex;
				initClass(clientnum);

				cJSON* layoutObj = cJSON_GetObjectItem(classObj, layout.c_str());
				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
				{
					std::string slotnum = std::to_string(i);
					cJSON* slotObj = cJSON_AddObjectToObject(layoutObj, slotnum.c_str());
					cJSON_AddArrayToObject(slotObj, "items");
					if ( hotbar_t.slots()[i].item != 0 )
					{
						if ( Item* item = uidToItem(hotbar_t.slots()[i].item) )
						{
							if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
							{
								std::string itemstr = itemNameStrings[item->type + 2];
								if ( itemstr == "spell_item" )
								{
									if ( spell_t* spell = getSpellFromItem(clientnum, item, false) )
									{
										itemstr = ItemTooltips.spellItems[spell->ID].internalName;
									}
									else
									{
										continue;
									}
								}
								cJSON* itemsArr = cJSON_GetObjectItem(slotObj, "items");
								if ( itemsArr )
								{
									cJSON_AddItemToArray(itemsArr, cJSON_CreateString(itemstr.c_str()));
								}
							}
						}
					}
				}
			}
			cJSON_AddItemToObject(allClassesObject, classname.c_str(), classObj);
		}

		stats[clientnum]->clearStats();
		client_classes[clientnum] = CLASS_BARBARIAN;
		initClass(clientnum);
	}

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", outputPath.c_str());
		if ( exportDocument ) cJSON_Delete(exportDocument);
		return;
	}
	char* json = cJSON_Print(exportDocument);
	if ( json )
	{
		fp->write(json, sizeof(char), strlen(json));
		cJSON_free(json);
	}
	FileIO::close(fp);
	cJSON_Delete(exportDocument);

	printlog("[JSON]: Successfully wrote json file %s", outputPath.c_str());
	return;
}

void ClassHotbarConfig_t::readFromFile(ClassHotbarConfig_t::HotbarConfigType fileReadType)
{
	std::string filename = "data/class_hotbars.json";
	if ( fileReadType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		filename = "config/class_hotbars.json";
	}
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		if ( fileReadType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
		{
			printlog("[JSON]: Notice: No custom class hotbar layout found '%s'", filename.c_str());
		}
		else
		{
			printlog("[JSON]: Error: Could not find json file %s", filename.c_str());
		}
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	static char buf[140000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d )
	{
		printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
		return;
	}
	if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "classes") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		cJSON_Delete(d);
		return;
	}

	cJSON* classesObj = cJSON_GetObjectItem(d, "classes");
	if ( classesObj )
	{
		for ( cJSON* classes = classesObj->child; classes; classes = classes->next )
		{
			int classIndex = -1;
			for ( auto& s : playerClassInternalNames )
			{
				++classIndex;
				if ( s == classes->string )
				{
					break;
				}
			}
			if ( !(classIndex >= CLASS_BARBARIAN && classIndex < NUMCLASSES) )
			{
				continue;
			}

			for ( cJSON* layout = classes->child; layout; layout = layout->next )
			{
				if ( strcmp(layout->string, "classic") && strcmp(layout->string, "modern") )
				{
					continue;
				}
				bool facebarLayout = false;
				if ( !strcmp(layout->string, "modern") )
				{
					facebarLayout = true;
				}

				auto& customOrDefaultHotbar = (fileReadType == HOTBAR_LAYOUT_DEFAULT_CONFIG) ? ClassHotbarsDefault[classIndex] : ClassHotbars[classIndex];
				auto& classHotbar = facebarLayout ? customOrDefaultHotbar.layoutModern : customOrDefaultHotbar.layoutClassic;
				classHotbar.hasData = true;
				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
				{
					std::string slotnum = std::to_string(i);
					cJSON* slot = cJSON_GetObjectItem(layout, slotnum.c_str());
					if ( !slot )
					{
						continue;
					}
					cJSON* itemsArr = cJSON_GetObjectItem(slot, "items");
					if ( itemsArr && cJSON_IsArray(itemsArr) )
					{
						cJSON* itemArr = NULL;
						cJSON_ArrayForEach(itemArr, itemsArr)
						{
							std::string itemString = itemArr->valuestring;
							int itemType = WOODEN_SHIELD;
							bool found = false;
							bool spell = false;
							if ( itemString.find("spell_") != std::string::npos )
							{
								spell = true;
								for ( int spellID = 0; spellID < NUM_SPELLS; ++spellID )
								{
									if ( ItemTooltips.spellItems[spellID].internalName == itemString )
									{
										itemType = spellID + 10000; // special id offset
										found = true;
										break;
									}
								}
							}
							else
							{
								for ( int c = 0; c < NUMITEMS; ++c )
								{
									if ( itemString.compare(itemNameStrings[c + 2]) == 0 )
									{
										itemType = c;
										found = true;
										break;
									}
								}
							}
							if ( found )
							{
								auto findVal = std::find(classHotbar.hotbar[i].itemTypes.begin(), classHotbar.hotbar[i].itemTypes.end(),
									itemType);
								if ( findVal == classHotbar.hotbar[i].itemTypes.end() )
								{
									classHotbar.hotbar[i].itemTypes.push_back(itemType);
								}
								else
								{
									*findVal = itemType;
								}
							}
						}
					}
				}
			}
		}
	}
	cJSON_Delete(d);
	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
}

void ClassHotbarConfig_t::ClassHotbar_t::ClassHotbarLayout_t::init()
{
	hasData = false;
	hotbar.clear();
	hotbar_alternates.clear();

	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		hotbar.push_back(HotbarEntry_t(i));
	}
	for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
	{
		std::vector<HotbarEntry_t> althotbar;
		for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
		{
			althotbar.push_back(HotbarEntry_t(i));
		}
		hotbar_alternates.push_back(althotbar);
	}
}

void ClassHotbarConfig_t::init()
{
	for ( int c = 0; c < NUMCLASSES; ++c )
	{
		auto& classHotbar = ClassHotbars[c];
		auto& classHotbarDefault = ClassHotbarsDefault[c];
		classHotbar.layoutClassic.init();
		classHotbar.layoutModern.init();
		classHotbarDefault.layoutClassic.init();
		classHotbarDefault.layoutModern.init();
	}

	readFromFile(HOTBAR_LAYOUT_DEFAULT_CONFIG);
	readFromFile(HOTBAR_LAYOUT_CUSTOM_CONFIG);
}

void ClassHotbarConfig_t::assignHotbarSlots(const int player)
{
	int classnum = client_classes[player];
	auto& layoutDefault = players[player]->hotbar.useHotbarFaceMenu ? ClassHotbarsDefault[classnum].layoutModern : ClassHotbarsDefault[classnum].layoutClassic;
	auto& layoutCustom = players[player]->hotbar.useHotbarFaceMenu ? ClassHotbars[classnum].layoutModern : ClassHotbars[classnum].layoutClassic;

	auto& hotbar_t = players[player]->hotbar;
	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		hotbar_t.slots()[i].item = 0;
		hotbar_t.slots()[i].resetLastItem();
	}

	std::vector<std::pair<int, HotbarEntry_t*>> itemsAndSlots;

	for ( auto& slot : layoutDefault.hotbar )
	{
		if ( !slot.itemTypes.empty() )
		{
			for ( auto itemType : slot.itemTypes )
			{
				auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
					[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
				if ( it == itemsAndSlots.end() )
				{
					itemsAndSlots.push_back(std::make_pair(itemType, &slot));
				}
				else
				{
					// update existing entry
					it->second = &slot;
				}
			}
		}
	}
	if ( layoutCustom.hasData )
	{
		printlog("[Class Hotbar]: Found custom layout for class '%s'", playerClassInternalNames[classnum].c_str());
		itemsAndSlots.clear();
		for ( auto& slot : layoutCustom.hotbar )
		{
			if ( !slot.itemTypes.empty() )
			{
				for ( auto itemType : slot.itemTypes )
				{
					auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
						[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
					if ( it == itemsAndSlots.end() )
					{
						itemsAndSlots.push_back(std::make_pair(itemType, &slot));
					}
					else
					{
						// update existing entry
						it->second = &slot;
					}
				}
			}
		}
	}

	struct MatchingItem_t
	{
		Item* item = nullptr;
		int slotnum = -1;
		MatchingItem_t(Item* _item, const int _slotnum) :
			item(_item),
			slotnum(_slotnum)
		{};
		MatchingItem_t() {};
	};
	std::map<int, MatchingItem_t> matchingItems;
	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			int itemType = item->type;
			if ( itemCategory(item) == SPELL_CAT )
			{
				if ( item->appearance >= 1000 )
				{
					continue; // shaman form spells
				}
				if ( spell_t* spell = getSpellFromItem(player, item, false) )
				{
					itemType = spell->ID + 10000;
				}
				else
				{
					continue;
				}
			}
			auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
				[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
			if ( it != itemsAndSlots.end() )
			{
				// store inventory items in a lookup table
				matchingItems[itemType] = MatchingItem_t(item, it->second->slotnum);
			}
		}
	}

	for ( auto& itemAndSlot : itemsAndSlots )
	{
		// go through each slot, and each item. if item found, place it in hotbar slot
		// if multiple items per slot, last item will override the slot
		if ( matchingItems.find(itemAndSlot.first) != matchingItems.end() )
		{
			if ( matchingItems[itemAndSlot.first].item )
			{
				hotbar_t.slots()[matchingItems[itemAndSlot.first].slotnum].item = matchingItems[itemAndSlot.first].item->uid;
			}
		}
	}
}

LocalAchievements_t LocalAchievements;

void LocalAchievements_t::readFromFile()
{
	LocalAchievements.init();
	Compendium_t::AchievementData_t::achievementsNeedFirstData = false;

	char path[PATH_MAX] = "";
	completePath(path, "savegames/achievements.json", outputdir);

	File* fp = FileIO::open(path, "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", path);
		return;
	}

	static char buf[65536 * 2];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d )
	{
		printlog("[JSON]: Error: Could not parse json file %s", path);
		return;
	}
	if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "achievements") || !cJSON_HasObjectItem(d, "statistics") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", path);
		cJSON_Delete(d);
		return;
	}

	cJSON* achievementsObj = cJSON_GetObjectItem(d, "achievements");
	if ( achievementsObj )
	{
		for ( cJSON* achievement = achievementsObj->child; achievement; achievement = achievement->next )
		{
			auto& ach = LocalAchievements.achievements[achievement->string];
			ach.name = achievement->string;
			cJSON* unlockedItem = cJSON_GetObjectItem(achievement, "unlocked");
			if ( unlockedItem ) ach.unlocked = cJSON_IsTrue(unlockedItem);
			cJSON* timeItem = cJSON_GetObjectItem(achievement, "unlock_time");
			if ( timeItem ) ach.unlockTime = (int64_t)timeItem->valuedouble;

			auto find = Compendium_t::achievements.find(achievement->string);
			if ( find != Compendium_t::achievements.end() )
			{
				auto& achData = find->second;
				achData.unlocked = ach.unlocked;
				achData.unlockTime = ach.unlockTime;
				if ( ach.unlocked )
				{
					Compendium_t::AchievementData_t::achievementUnlockedLookup.insert(ach.name);
				}
			}
		}
	}

	cJSON* statisticsObj = cJSON_GetObjectItem(d, "statistics");
	if ( statisticsObj )
	{
		for ( cJSON* statistic = statisticsObj->child; statistic; statistic = statistic->next )
		{
			std::string statStr = statistic->string;
			const int statNum = stoi(statStr);
			auto& stat = LocalAchievements.statistics[statNum];
			cJSON* progressItem = cJSON_GetObjectItem(statistic, "progress");
			if ( progressItem ) stat.value = progressItem->valueint;
		}
	}

	for ( int statNum = 0; statNum < NUM_STEAM_STATISTICS; ++statNum )
	{
		g_SteamStats[statNum].m_iValue = LocalAchievements.statistics[statNum].value;
	}
	sortAchievementsForDisplay();
	cJSON_Delete(d);
}

void LocalAchievements_t::writeToFile()
{
	char path[PATH_MAX] = "";
	completePath(path, "savegames/achievements.json", outputdir);

	cJSON* exportDocument = cJSON_CreateObject();

	const int VERSION = 1;

	cJSON_AddNumberToObject(exportDocument, "version", VERSION);
	cJSON* allAchObj = cJSON_AddObjectToObject(exportDocument, "achievements");
	for ( auto& ach : Compendium_t::achievements )
	{
		if ( LocalAchievements.achievements.find(ach.first) == LocalAchievements.achievements.end() )
		{
			continue;
		}
		auto& achData = LocalAchievements.achievements[ach.first];

		cJSON* obj = cJSON_CreateObject();
		cJSON_AddBoolToObject(obj, "unlocked", achData.unlocked);
		cJSON_AddNumberToObject(obj, "unlock_time", (double)achData.unlockTime);
		cJSON_AddItemToObject(allAchObj, ach.first.c_str(), obj);
	}

	cJSON* allStatObj = cJSON_AddObjectToObject(exportDocument, "statistics");
	for ( int i = 0; i < NUM_STEAM_STATISTICS; ++i )
	{
		if ( LocalAchievements.statistics.find(i) == LocalAchievements.statistics.end() )
		{
			continue;
		}
		auto& statData = LocalAchievements.statistics[i];

		std::string statNum = std::to_string(i);
		cJSON* obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(obj, "progress", statData.value);
		cJSON_AddItemToObject(allStatObj, statNum.c_str(), obj);
	}

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		cJSON_Delete(exportDocument);
		return;
	}
	char* json = cJSON_Print(exportDocument);
	if ( json )
	{
		fp->write(json, sizeof(char), strlen(json));
		cJSON_free(json);
	}
	FileIO::close(fp);
	cJSON_Delete(exportDocument);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

void LocalAchievements_t::init()
{
	LocalAchievements.achievements.clear();
	for ( auto& ach : Compendium_t::achievements )
	{
		LocalAchievements.achievements[ach.first].unlocked = false;
		LocalAchievements.achievements[ach.first].unlockTime = 0;
		LocalAchievements.achievements[ach.first].name = ach.first;
	}
	LocalAchievements.statistics.clear();
	for ( int i = 0; i < NUM_STEAM_STATISTICS; ++i )
	{
		LocalAchievements.statistics[i].value = 0;
	}
}

void LocalAchievements_t::updateAchievement(const char* name, const bool unlocked)
{
	if ( achievements.find(name) != achievements.end() )
	{
		auto& ach = achievements[name];
		bool oldUnlocked = ach.unlocked;
		ach.unlocked = unlocked;
		if ( ach.unlocked && !oldUnlocked )
		{
			auto t = getTime();
			ach.unlockTime = t;

			UIToastNotificationManager.createAchievementNotification(name);
		}
	}
}

void LocalAchievements_t::updateStatistic(const int stat_num, const int value)
{
	if ( statistics.find(stat_num) != statistics.end() )
	{
		auto& stat = statistics[stat_num];
		stat.value = value;
	}
}

GameplayPreferences_t gameplayPreferences[MAXPLAYERS];

void GameplayPreferences_t::GameplayPreference_t::set(const int _value)
{
	if ( value != _value )
	{
		needsUpdate = true;
	}
	value = _value;
}

void GameplayPreferences_t::requestUpdateFromClient()
{
	if ( player == 0 ) { return; }
	if ( client_disconnected[player] ) { return; }
	if ( !net_packet )
	{
		return;

	}
	strcpy((char*)net_packet->data, "GPPU");
	net_packet->data[4] = (Uint8)player;
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 5;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void GameplayPreferences_t::sendToClients(const int targetPlayer)
{
	if ( targetPlayer == 0 ) { return; }
	if ( client_disconnected[targetPlayer] ) { return; }
	if ( !net_packet )
	{
		return;
	}

	strcpy((char*)net_packet->data, "GPPR");
	net_packet->data[4] = (Uint8)player;
	net_packet->data[5] = (Uint8)GPREF_ENUM_END;
	int index = 0;
	for ( auto& pref : preferences )
	{
		Uint8 data = (pref.value & 0xFF);
		net_packet->data[6 + index] = data;
		++index;
	}
	net_packet->address.host = net_clients[targetPlayer - 1].host;
	net_packet->address.port = net_clients[targetPlayer - 1].port;
	net_packet->len = 6 + index;
	sendPacketSafe(net_sock, -1, net_packet, targetPlayer - 1);
}

void GameplayPreferences_t::receivePacket()
{
	if ( !net_packet )
	{
		return;
	}
	int player = (Uint8)net_packet->data[4];
	if ( player >= 0 && player < MAXPLAYERS )
	{
		auto& playerPrefs = gameplayPreferences[player];
		const int numPrefs = (Uint8)net_packet->data[5];
		for ( int i = 0; i < numPrefs && i < GPREF_ENUM_END; ++i )
		{
			int data = (net_packet->data[6 + i] & 0xFF);
			playerPrefs.preferences[i].value = data;
			playerPrefs.preferences[i].needsUpdate = false;
		}
		playerPrefs.lastUpdateTick = ticks;
	}
}

void GameplayPreferences_t::sendToServer()
{
	if ( multiplayer != CLIENT ) { return; }
	if ( !net_packet )
	{
		return;
	}

	strcpy((char*)net_packet->data, "GPPR");
	net_packet->data[4] = (Uint8)player;
	net_packet->data[5] = (Uint8)GPREF_ENUM_END;
	int index = 0;
	for ( auto& pref : preferences )
	{
		Uint8 data = (pref.value & 0xFF);
		net_packet->data[6 + index] = data;
		++index;
		pref.needsUpdate = false;
	}
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 6 + index;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void GameplayPreferences_t::process()
{
	if ( player < 0 )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( &gameplayPreferences[i] == this )
			{
				player = i;
				break;
			}
		}
	}

	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		int index = 0;
		for ( auto& pref : preferences )
		{
			switch ( index )
			{
				case GPREF_ARACHNOPHOBIA:
					pref.set(MainMenu::arachnophobia_filter ? 1 : 0);
					break;
				case GPREF_COLORBLIND:
					pref.set(colorblind ? 1 : 0);
					break;
				case GPREF_VOICE_NO_RECV:
					// we don't have voice receive volume set, drop packets
#ifdef USE_FMOD
					pref.set(((!VoiceChat.useSystem || !VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_RECEIVE)) ? 1 : 0) << player);
#else
					pref.set(1 << player);
#endif
					break;
				case GPREF_VOICE_NO_SEND:
					// we don't have voice input device or otherwise don't want to use it
#ifdef USE_FMOD
					pref.set(((!VoiceChat.useSystem || !VoiceChat.bRecordingInit
						|| (VoiceChat.mainMenuAudioTabOpen()
							&& VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD))
						|| !VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_INPUT) 
							|| !VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_RECEIVE)) ? 1 : 0) << player);
#else				
					pref.set(1 << player);
#endif
					break;
				case GPREF_VOICE_PTT:
#ifdef USE_FMOD
					pref.set((VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_PUSHTOTALK) ? 1 : 0) << player);
#else
					pref.set(1 << player);
#endif

					break;
				default:
					break;
			}
			++index;
		}
	}

	if ( multiplayer == CLIENT )
	{
		bool doUpdate = false;
		if ( players[player]->isLocalPlayer() )
		{
			for ( auto& pref : preferences )
			{
				if ( pref.needsUpdate )
				{
					doUpdate = true;
					pref.needsUpdate = false;
				}
			}
			if ( ticks - lastUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
			{
				doUpdate = true;
			}
			if ( doUpdate )
			{
				sendToServer();
			}
			isInit = true;
		}
		if ( doUpdate )
		{
			lastUpdateTick = ticks;
		}
	}
	else if ( multiplayer == SERVER )
	{
		bool doUpdate = false;
		if ( players[player]->isLocalPlayer() )
		{
			isInit = true;
		}
		else
		{
			if ( !client_disconnected[player] )
			{
				isInit = true;
				for ( auto& pref : preferences )
				{
					if ( pref.needsUpdate )
					{
						doUpdate = true;
						pref.needsUpdate = false;
					}
				}

				if ( ticks - lastUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
				{
					doUpdate = true;
				}

				if ( doUpdate )
				{
					requestUpdateFromClient();
				}
			}
			else
			{
				if ( isInit )
				{
					for ( auto& pref : preferences )
					{
						pref.reset();
					}
					isInit = false;
				}
			}
		}

		if ( doUpdate )
		{
			lastUpdateTick = ticks;
		}
	}

	if ( multiplayer != CLIENT && player == clientnum )
	{
		isInit = true;
		GameplayPreferences_t::serverProcessGameConfig();
	}
}

GameplayPreferences_t::GameplayPreference_t GameplayPreferences_t::gameConfig[GameplayPreferences_t::GOPT_ENUM_END];
Uint32 GameplayPreferences_t::lastGameConfigUpdateTick = 0;
void GameplayPreferences_t::reset()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		for ( auto& pref : gameplayPreferences[i].preferences )
		{
			pref.reset();
		}
		gameplayPreferences[i].lastUpdateTick = 0;
		gameplayPreferences[i].isInit = false;
	}
	for ( auto& conf : gameConfig )
	{
		conf.reset();
	}
	lastGameConfigUpdateTick = 0;
}
void GameplayPreferences_t::serverUpdateGameConfig()
{
	lastGameConfigUpdateTick = ticks;
	if ( !net_packet )
	{
		return;
	}
	for ( int i = 1; i < MAXPLAYERS; ++i )
	{
		if ( !players[i]->isLocalPlayer() && !client_disconnected[i] )
		{
			strcpy((char*)net_packet->data, "GOPT");
			net_packet->data[4] = (Uint8)GOPT_ENUM_END;
			int index = 0;
			for ( auto& conf : gameConfig )
			{
				Uint8 data = (conf.value & 0xFF);
				net_packet->data[5 + index] = data;
				++index;
			}
			net_packet->address.host = net_clients[i - 1].host;
			net_packet->address.port = net_clients[i - 1].port;
			net_packet->len = 5 + index;
			sendPacketSafe(net_sock, -1, net_packet, i - 1);
		}
	}
}

void GameplayPreferences_t::receiveGameConfig()
{
	if ( !net_packet ) { return; }
	auto& gameConfig = GameplayPreferences_t::gameConfig;
	const int numConfigs = (Uint8)net_packet->data[4];
	for ( int i = 0; i < numConfigs && i < GOPT_ENUM_END; ++i )
	{
		int data = (net_packet->data[5 + i] & 0xFF);
		gameConfig[i].value = data;
		gameConfig[i].needsUpdate = false;
	}
	lastGameConfigUpdateTick = ticks;
}

void GameplayPreferences_t::serverProcessGameConfig()
{
	bool doUpdate = false;
	for ( int pref = 0; pref < GOPT_ENUM_END; ++pref )
	{
		int value = 0;
		switch ( pref )
		{
			case GOPT_ARACHNOPHOBIA:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						if ( gameplayPreferences[i].preferences[GPREF_ARACHNOPHOBIA].value != 0 )
						{
							value = 1;
						}
					}
				}
				if ( value != oldValue )
				{
					if ( multiplayer == SERVER )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( !client_disconnected[i] )
							{
								if ( value != 0 )
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4333));
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4334));
								}
							}
						}
					}
				}
				break;
			}
			case GOPT_COLORBLIND:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						if ( gameplayPreferences[i].preferences[GPREF_COLORBLIND].value != 0 )
						{
							value = 1;
						}
					}
				}
				if ( value != oldValue )
				{
					if ( multiplayer == SERVER )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( !client_disconnected[i] )
							{
								if ( value != 0 )
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4342));
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4343));
								}
							}
						}
					}
				}
				break;
			}
			case GOPT_VOICE_NO_RECV:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));

				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						value |= gameplayPreferences[i].preferences[GPREF_VOICE_NO_RECV].value;
					}
				}
				break;
			}
			case GOPT_VOICE_NO_SEND:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));

				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						value |= gameplayPreferences[i].preferences[GPREF_VOICE_NO_SEND].value;
					}
				}
				break;
			}
			case GOPT_VOICE_PTT:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));

				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						value |= gameplayPreferences[i].preferences[GPREF_VOICE_PTT].value;
					}
				}
				break;
			}
			default:
				break;
		}
		gameConfig[pref].set(value);
		if ( gameConfig[pref].needsUpdate )
		{
			doUpdate = true;
		}
		gameConfig[pref].needsUpdate = false;
	}

	if ( ticks - lastGameConfigUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
	{
		doUpdate = true;
	}

	if ( doUpdate && multiplayer == SERVER )
	{
		serverUpdateGameConfig();
	}

	if ( doUpdate )
	{
		lastGameConfigUpdateTick = ticks;
	}
}
