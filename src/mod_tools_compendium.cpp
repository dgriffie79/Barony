/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_compendium.cpp - Compendium (bestiary, items, magic, codex, world, events, achievements)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

void jsonVecToVec(rapidjson::Value& val, std::vector<std::string>& vec )
{
	for ( auto itr = val.Begin(); itr != val.End(); ++itr )
	{
		if ( itr->IsString() )
		{
			vec.push_back(itr->GetString());
		}
	}
}

void jsonVecToVec(rapidjson::Value& val, std::vector<Sint32>& vec)
{
	for ( auto itr = val.Begin(); itr != val.End(); ++itr )
	{
		if ( itr->IsInt() )
		{
			vec.push_back(itr->GetInt());
		}
	}
}

#ifndef EDITOR
Compendium_t CompendiumEntries;
Item Compendium_t::compendiumItem;
Compendium_t::CompendiumEntityCurrent Compendium_t::compendiumEntityCurrent;
Entity Compendium_t::compendiumItemModel(-1, 0, nullptr, nullptr);
bool Compendium_t::tooltipNeedUpdate = false;
SDL_Rect Compendium_t::tooltipPos;

std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMonsters_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumMonsters_t::contentsMap;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMonsters_t::contents_unfiltered;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumMonsters_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumWorld_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumWorld_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumWorld_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumCodex_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumCodex_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumCodex_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumItems_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumItems_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumItems_t::unlocks;
std::map<int, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumItems_t::itemUnlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMagic_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumMagic_t::contentsMap;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::AchievementData_t::contents;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::AchievementData_t::unlocks;
std::map<std::string, std::string> Compendium_t::AchievementData_t::contentsMap;
int Compendium_t::CompendiumMonsters_t::completionPercent = 0;
int Compendium_t::CompendiumCodex_t::completionPercent = 0;
int Compendium_t::CompendiumItems_t::completionPercent = 0;
int Compendium_t::CompendiumMagic_t::completionPercent = 0;
int Compendium_t::CompendiumWorld_t::completionPercent = 0;
int Compendium_t::AchievementData_t::completionPercent = 0;
int Compendium_t::CompendiumMonsters_t::numUnread = 0;
int Compendium_t::CompendiumCodex_t::numUnread = 0;
int Compendium_t::CompendiumItems_t::numUnread = 0;
int Compendium_t::CompendiumMagic_t::numUnread = 0;
int Compendium_t::CompendiumWorld_t::numUnread = 0;
int Compendium_t::AchievementData_t::numUnread = 0;

std::map<int, std::string> Compendium_t::Events_t::monsterIDToString;
std::map<int, std::string> Compendium_t::Events_t::codexIDToString;
std::map<int, std::string> Compendium_t::Events_t::worldIDToString;
std::map<int, std::string> Compendium_t::Events_t::itemIDToString;

void Compendium_t::readContentsLang(std::string name, std::map<std::string, std::vector<std::pair<std::string, std::string>>>& contents,
	std::map<std::string, std::string>& contentsMap)
{
	contents.clear();
	contentsMap.clear();

	std::string filename = "lang/compendium_lang/";
	filename += name;
	filename += ".json";
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

	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("contents") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d["contents"].Begin(); itr != d["contents"].End(); ++itr )
	{
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			contents["default"].push_back(std::make_pair(itr2->value.GetString(), itr2->name.GetString()));
			if ( name == "contents_monsters"
				&& (!strcmp(itr2->value.GetString(), "crab") || !strcmp(itr2->value.GetString(), "bubbles")) )
			{
				// dont read into map
			}
			else
			{
				contentsMap[itr2->value.GetString()] = itr2->name.GetString();
			}
		}
	}

	if ( d.HasMember("contents_alphabetical") )
	{
		for ( auto itr = d["contents_alphabetical"].Begin(); itr != d["contents_alphabetical"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				contents["alphabetical"].push_back(std::make_pair(itr2->value.GetString(), itr2->name.GetString()));
				if ( name == "contents_monsters"
					&& (!strcmp(itr2->value.GetString(), "crab") || !strcmp(itr2->value.GetString(), "bubbles")) )
				{
					// dont read into map
				}
				else
				{
					contentsMap[itr2->value.GetString()] = itr2->name.GetString();
				}
			}
		}
	}
}

void Compendium_t::CompendiumMonsters_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_monsters", contents_unfiltered, contentsMap);
}

void Compendium_t::CompendiumWorld_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_world", contents, contentsMap);
}

void Compendium_t::CompendiumCodex_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_codex", contents, contentsMap);
}

void Compendium_t::CompendiumItems_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_items", contents, contentsMap);
}

void Compendium_t::CompendiumMagic_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_magic", contents, contentsMap);
}

void Compendium_t::AchievementData_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_achievements", contents, contentsMap);
}

void Compendium_t::updateTooltip()
{
	bool update = tooltipNeedUpdate;
	tooltipNeedUpdate = false;

	if ( MainMenu::main_menu_frame )
	{
		auto compendiumFrame = MainMenu::main_menu_frame->findFrame("compendium");
		if ( !compendiumFrame ) { return; }
		
		players[MainMenu::getMenuOwner()]->inventoryUI.updateInventoryItemTooltip(compendiumFrame);
		
		if ( update )
		{
			players[MainMenu::getMenuOwner()]->hud.updateFrameTooltip(&compendiumItem, tooltipPos.x, tooltipPos.y, Player::PANEL_JUSTIFY_RIGHT, compendiumFrame);
		}

		if ( Frame* tooltipContainerFrame = compendiumFrame->findFrame("player tooltip container 0") )
		{
			if ( auto prompt = tooltipContainerFrame->findFrame("item_widget") )
			{
				if ( auto tooltip = tooltipContainerFrame->findFrame("player tooltip 0") )
				{
					if ( tooltip->getSize().w == 0 )
					{
						prompt->setOpacity(0.0);
					}
					else
					{
						prompt->setOpacity(tooltip->getOpacity());
					}
					if ( Compendium_t::compendiumItem.type == SPELL_ITEM )
					{
						prompt->setOpacity(0.0);
					}
					SDL_Rect framePos = prompt->getSize();
					framePos.x = tooltip->getSize().x + tooltip->getSize().w - 6 - framePos.w;
					framePos.y = tooltip->getSize().y + tooltip->getSize().h - 10;
					//framePos.x = 802 + 378 / 2 - framePos.w / 2;
					//framePos.y = 110 - framePos.h + 14;
					prompt->setSize(framePos);
				}
			}
		}

	}
}

void Compendium_t::readItemsTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = items.find(key);
		if ( find != items.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
		}
	}
}

void Compendium_t::readItemsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/comp_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	items.clear();
	Compendium_t::Events_t::itemEventLookup.clear();
	Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::Events_t::itemIDToString.clear();
	Compendium_t::CompendiumItems_t::readContentsLang();

	auto& entries = d["items"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = items[name];

		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		for ( auto itr = w["items"].Begin(); itr != w["items"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				CompendiumItems_t::Codex_t::CodexItem_t item;
				item.name = itr2->name.GetString();
				item.rotation = 0;
				if ( itr2->value.HasMember("rotation") )
				{
					item.rotation = itr2->value["rotation"].GetInt();
				}
				obj.items_in_category.push_back(item);
			}
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}

		std::set<std::string> alwaysTrackedEvents = {
			"APPRAISED",
			"RUNS_COLLECTED"
		};

		for ( auto& item : obj.items_in_category )
		{
			const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
			if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
			{
				item.itemID = itemType;
				Compendium_t::Events_t::itemIDToString[itemType] = name;
				if ( ::items[itemType].item_slot != NO_EQUIP )
				{
					alwaysTrackedEvents.insert("BROKEN");
					alwaysTrackedEvents.insert("DEGRADED");
					alwaysTrackedEvents.insert("REPAIRS");
				}

				Compendium_t::Events_t::itemDisplayedEventsList.erase(itemType);
				Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(itemType);
			}
		}

		for ( auto& s : alwaysTrackedEvents )
		{
			auto find = Compendium_t::Events_t::eventIdLookup.find(s);
			if ( find != Compendium_t::Events_t::eventIdLookup.end() )
			{
				auto find2 = Compendium_t::Events_t::events.find(find->second);
				if ( find2 != Compendium_t::Events_t::events.end() )
				{
					for ( auto& item : obj.items_in_category )
					{
						const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
						if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
						{
							Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
						}
					}
				}
			}
		}

		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
							}
						}
					}
				}
			}
		}

		std::set<int> itemsInList;
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[itemType];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(itemType);
							}
						}
					}
				}
			}
		}

		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			for ( auto item : itemsInList )
			{
				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[item];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[item];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() )
						{
							vec2.push_back(customEvents[index]);
						}
						else
						{
							vec2.push_back("");
						}
					}
					else
					{
						vec2.push_back("");
					}
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : items )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_items.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

void Compendium_t::readMagicTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_magic.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMagicTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = magic.find(key);
		if ( find != magic.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
		}
	}
}

void Compendium_t::readMagicFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/comp_magic.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	magic.clear();
	//Compendium_t::Events_t::itemEventLookup.clear();
	//Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::CompendiumMagic_t::readContentsLang();

	auto& entries = d["items"];
	std::unordered_set<std::string> ignoredSpells;
	std::unordered_set<std::string> ignoredSpellbooks;

	if ( d.HasMember("exclude_spells") )
	{
		for ( auto itr = d["exclude_spells"].Begin(); itr != d["exclude_spells"].End(); ++itr )
		{
			ignoredSpells.insert(itr->GetString());
		}
	}
	if ( d.HasMember("exclude_spellbooks") )
	{
		for ( auto itr = d["exclude_spellbooks"].Begin(); itr != d["exclude_spellbooks"].End(); ++itr )
		{
			ignoredSpellbooks.insert(itr->GetString());
		}
	}

	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = magic[name];

		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		std::set<std::string> objSpellsLookup;
		for ( auto itr = w["items"].Begin(); itr != w["items"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				CompendiumItems_t::Codex_t::CodexItem_t item;
				item.name = itr2->name.GetString();
				item.rotation = 0;
				if ( itr2->value.HasMember("rotation") )
				{
					item.rotation = itr2->value["rotation"].GetInt();
				}
				if ( item.name.find("spell_") != std::string::npos )
				{
					for ( auto& spell : allGameSpells )
					{
						if ( item.name == spell.second->spell_internal_name )
						{
							item.spellID = spell.second->ID;
							objSpellsLookup.insert(item.name);
							break;
						}
					}
				}
				else if ( item.name.find("spellbook_") != std::string::npos )
				{
					for ( auto& spell : allGameSpells )
					{
						int book = getSpellbookFromSpellID(spell.second->ID);
						if ( book >= WOODEN_SHIELD && book < NUMITEMS && ::items[book].category == SPELLBOOK )
						{
							if ( item.name == itemNameStrings[book + 2] )
							{
								item.spellID = spell.second->ID;
								break;
							}
						}
					}
				}
				obj.items_in_category.push_back(item);
			}
		}

		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}

		//std::list<spell_t*> spellsToSort;
		//bool spells = name.find("spells") != std::string::npos;
		//bool spellbooks = name.find("spellbooks") != std::string::npos;
		//if ( spells || spellbooks )
		//{
		//	for ( auto spell : allGameSpells )
		//	{
		//		if ( spells && ignoredSpells.find(spell->spell_internal_name) != ignoredSpells.end() )
		//		{
		//			continue;
		//		}
		//		if ( spellbooks )
		//		{
		//			int book = getSpellbookFromSpellID(spell->ID);
		//			if ( book >= WOODEN_SHIELD && book < NUMITEMS && ::items[book].category == SPELLBOOK )
		//			{
		//				if ( ignoredSpellbooks.find(itemNameStrings[book + 2]) != ignoredSpellbooks.end() )
		//				{
		//					continue;
		//				}
		//			}
		//			else
		//			{
		//				continue;
		//			}
		//		}

		//		if ( name.find("damage") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_DAMAGE)
		//				== ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else if ( name.find("status") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
		//				== ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else if ( name.find("utility") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_DAMAGE)
		//				!= ItemTooltips.spellItems[spell->ID].spellTags.end()
		//				|| ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
		//				!= ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else
		//		{
		//			continue;
		//		}
		//		spellsToSort.push_back(spell);
		//	}

		//	if ( spellbooks )
		//	{
		//		spellsToSort.sort([](const spell_t* lhs, const spell_t* rhs) {
		//			const int bookLeft = getSpellbookFromSpellID(lhs->ID);
		//			const int bookRight = getSpellbookFromSpellID(rhs->ID);

		//			const int bookLevelLeft = ::items[bookLeft].level >= 0 ? ::items[bookLeft].level : 10000; // -1 level sorted to the end
		//			const int bookLevelRight = ::items[bookRight].level >= 0 ? ::items[bookRight].level : 10000;

		//			if ( bookLevelLeft < bookLevelRight ) { return true; }
		//			if ( bookLevelLeft > bookLevelRight ) { return false; }
		//			if ( rhs->difficulty > lhs->difficulty ) { return true; }
		//			if ( rhs->difficulty < lhs->difficulty ) { return false; }

		//			return rhs->ID < lhs->ID;
		//			});
		//	}
		//	else
		//	{
		//		spellsToSort.sort([](const spell_t* lhs, const spell_t* rhs) {
		//			if ( rhs->difficulty > lhs->difficulty ) { return true; }
		//			if ( rhs->difficulty < lhs->difficulty ) { return false; }
		//			
		//			return rhs->ID < lhs->ID;
		//			});
		//	}

		//	for ( auto spell : spellsToSort )
		//	{
		//		CompendiumItems_t::Codex_t::CodexItem_t item;
		//		item.rotation = 0;
		//		if ( spellbooks )
		//		{
		//			int book = getSpellbookFromSpellID(spell->ID);
		//			item.name = itemNameStrings[book + 2];
		//			item.rotation = 180;
		//		}
		//		else
		//		{
		//			item.name = spell->spell_internal_name;
		//		}
		//		item.spellID = spell->ID;
		//		obj.items_in_category.push_back(item);
		//	}
		//}

		std::set<std::string> alwaysTrackedEvents = {
			"APPRAISED",
			"RUNS_COLLECTED"
		};

		for ( auto& item : obj.items_in_category )
		{
			bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
			const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
			if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
			{
				item.itemID = itemType;
				if ( !isSpell )
				{
					Compendium_t::Events_t::itemIDToString[itemType] = name;
					Compendium_t::Events_t::itemDisplayedEventsList.erase(itemType);
					Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(itemType);
				}
				else
				{
					Compendium_t::Events_t::itemIDToString[Compendium_t::Events_t::kEventSpellOffset + item.spellID] = name;
					Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
					Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
				}
				if ( itemType != SPELL_ITEM && ::items[itemType].item_slot != NO_EQUIP )
				{
					alwaysTrackedEvents.insert("BROKEN");
					alwaysTrackedEvents.insert("DEGRADED");
					alwaysTrackedEvents.insert("REPAIRS");
				}
			}
		}

		for ( auto& s : alwaysTrackedEvents )
		{
			auto find = Compendium_t::Events_t::eventIdLookup.find(s);
			if ( find != Compendium_t::Events_t::eventIdLookup.end() )
			{
				auto find2 = Compendium_t::Events_t::events.find(find->second);
				if ( find2 != Compendium_t::Events_t::events.end() )
				{
					for ( auto& item : obj.items_in_category )
					{
						bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
						const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
						if ( itemType == SPELL_ITEM )
						{
							Compendium_t::Events_t::itemEventLookup[Compendium_t::Events_t::kEventSpellOffset + item.spellID].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
						}
						else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
						{
							Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
						}
					}
				}
			}
		}

		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
							const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType == SPELL_ITEM )
							{
								Compendium_t::Events_t::itemEventLookup[Compendium_t::Events_t::kEventSpellOffset + item.spellID].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
							}
							else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
							}
						}
					}
				}
			}
		}

		std::set<int> itemsInList;
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
							const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType == SPELL_ITEM )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventSpellOffset + item.spellID];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
							}
							else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[itemType];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(itemType);
							}
						}
					}
				}
			}
		}

		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			for ( auto item : itemsInList )
			{
				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[item];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[item];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() )
						{
							vec2.push_back(customEvents[index]);
						}
						else
						{
							vec2.push_back("");
						}
					}
					else
					{
						vec2.push_back("");
					}
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : magic )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_magic.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

void Compendium_t::readCodexTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_codex.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readCodexTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = codex.find(key);
		if ( find != codex.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.details.clear();
			if ( itr->value.HasMember("details") )
			{
				jsonVecToVec(itr->value["details"], find->second.details);
			}
			for ( auto& line : find->second.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' )
					{
						line[0] = '\x1E';
					}
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' )
							{
								line[c] = '\x1E';
								break;
							}
							else if ( line[c] != ' ' )
							{
								break;
							}
						}
					}
				}
			}

			find->second.linesToHighlight.clear();
			if ( itr->value.HasMember("details_line_highlights") )
			{
				for ( auto itr2 = itr->value["details_line_highlights"].Begin(); itr2 != itr->value["details_line_highlights"].End(); ++itr2 )
				{
					if ( itr2->HasMember("color") )
					{
						Uint8 r, g, b;
						if ( (*itr2)["color"].HasMember("r") )
						{
							r = (*itr2)["color"]["r"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("g") )
						{
							g = (*itr2)["color"]["g"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("b") )
						{
							b = (*itr2)["color"]["b"].GetInt();
						}

						find->second.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else
					{
						find->second.linesToHighlight.push_back(0);
					}
				}
			}
		}
	}
}

void Compendium_t::readCodexFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/codex.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readCodexFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("codex") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	codex.clear();

	Compendium_t::Events_t::eventCodexIDLookup.clear();
	Compendium_t::Events_t::eventCodexLookup.clear();
	Compendium_t::Events_t::codexIDToString.clear();
	Compendium_t::CompendiumCodex_t::readContentsLang();

	auto& entries = d["codex"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = codex[name];

		obj.id = w["event_lookup"].GetInt();
		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		if ( w.HasMember("details") )
		{
			jsonVecToVec(w["details"], obj.details);
		}
		obj.imagePath = w["img"].GetString();
		if ( w.HasMember("enable_tutorial") )
		{
			obj.enableTutorial = w["enable_tutorial"].GetBool();
		}
		if ( w.HasMember("rendered_imgs") )
		{
			jsonVecToVec(w["rendered_imgs"], obj.renderedImagePaths);
		}
		if ( w.HasMember("models") )
		{
			jsonVecToVec(w["models"], obj.models);
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}
		obj.linesToHighlight.clear();
		for ( auto& line : obj.details )
		{
			if ( line.size() > 0 )
			{
				if ( line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				else
				{
					for ( size_t c = 0; c < line.size(); ++c )
					{
						if ( line[c] == '-' )
						{
							line[c] = '\x1E';
							break;
						}
						else if ( line[c] != ' ' )
						{
							break;
						}
					}
				}
			}
		}
		if ( w.HasMember("feature_img") )
		{
			obj.featureImg = w["feature_img"].GetString();
		}
		if ( w.HasMember("details_line_highlights") )
		{
			for ( auto itr = w["details_line_highlights"].Begin(); itr != w["details_line_highlights"].End(); ++itr )
			{
				if ( itr->HasMember("color") )
				{
					Uint8 r, g, b;
					if ( (*itr)["color"].HasMember("r") )
					{
						r = (*itr)["color"]["r"].GetInt();
					}
					if ( (*itr)["color"].HasMember("g") )
					{
						g = (*itr)["color"]["g"].GetInt();
					}
					if ( (*itr)["color"].HasMember("b") )
					{
						b = (*itr)["color"]["b"].GetInt();
					}

					obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
				}
				else
				{
					obj.linesToHighlight.push_back(0);
				}
			}
		}

		Compendium_t::Events_t::eventCodexIDLookup[name] = obj.id;
		Compendium_t::Events_t::codexIDToString[obj.id + Compendium_t::Events_t::kEventCodexOffset] = name;
		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						Compendium_t::Events_t::eventCodexLookup[(Compendium_t::EventTags)find2->second.id].insert(name);
					}
				}
			}
		}

		Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventCodexOffset + obj.id);
		Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventCodexOffset + obj.id);
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
						if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
							== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
						{
							vec.push_back((Compendium_t::EventTags)find2->second.id);
						}
					}
				}
			}
		}
		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
			int index = -1;
			for ( auto& v : vec )
			{
				++index;
				auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
				if ( v == EventTags::CPDM_CUSTOM_TAG )
				{
					if ( index < customEvents.size() )
					{
						vec2.push_back(customEvents[index]);
					}
					else
					{
						vec2.push_back("");
					}
				}
				else
				{
					vec2.push_back("");
				}
			}
		}
	}
}

void Compendium_t::readWorldTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_world.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readWorldTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = worldObjects.find(key);
		if ( find != worldObjects.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.details.clear();
			if ( itr->value.HasMember("details") )
			{
				jsonVecToVec(itr->value["details"], find->second.details);
			}
			for ( auto& line : find->second.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' )
					{
						line[0] = '\x1E';
					}
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' )
							{
								line[c] = '\x1E';
								break;
							}
							else if ( line[c] != ' ' )
							{
								break;
							}
						}
					}
				}
			}

			find->second.linesToHighlight.clear();
			if ( itr->value.HasMember("details_line_highlights") )
			{
				for ( auto itr2 = itr->value["details_line_highlights"].Begin(); itr2 != itr->value["details_line_highlights"].End(); ++itr2 )
				{
					if ( itr2->HasMember("color") )
					{
						Uint8 r, g, b;
						if ( (*itr2)["color"].HasMember("r") )
						{
							r = (*itr2)["color"]["r"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("g") )
						{
							g = (*itr2)["color"]["g"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("b") )
						{
							b = (*itr2)["color"]["b"].GetInt();
						}

						find->second.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else
					{
						find->second.linesToHighlight.push_back(0);
					}
				}
			}
		}
	}
}

void Compendium_t::readWorldFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/world.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readWorldFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("world") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	worldObjects.clear();
	Compendium_t::Events_t::eventWorldIDLookup.clear();
	Compendium_t::Events_t::eventWorldLookup.clear();
	Compendium_t::Events_t::worldIDToString.clear();
	Compendium_t::CompendiumWorld_t::readContentsLang();

	auto& entries = d["world"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = worldObjects[name];

		obj.id = w["event_lookup"].GetInt();
		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		if ( w.HasMember("details") )
		{
			jsonVecToVec(w["details"], obj.details);
		}
		obj.imagePath = w["img"].GetString();
		if ( w.HasMember("models") )
		{
			jsonVecToVec(w["models"], obj.models);
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}
		if ( w.HasMember("feature_img") )
		{
			obj.featureImg = w["feature_img"].GetString();
		}
		obj.linesToHighlight.clear();
		for ( auto& line : obj.details )
		{
			if ( line.size() > 0 )
			{
				if ( line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				else
				{
					for ( size_t c = 0; c < line.size(); ++c )
					{
						if ( line[c] == '-' )
						{
							line[c] = '\x1E';
							break;
						}
						else if ( line[c] != ' ' )
						{
							break;
						}
					}
				}
			}
		}
		if ( w.HasMember("details_line_highlights") )
		{
			for ( auto itr = w["details_line_highlights"].Begin(); itr != w["details_line_highlights"].End(); ++itr )
			{
				if ( itr->HasMember("color") )
				{
					Uint8 r, g, b;
					if ( (*itr)["color"].HasMember("r") )
					{
						r = (*itr)["color"]["r"].GetInt();
					}
					if ( (*itr)["color"].HasMember("g") )
					{
						g = (*itr)["color"]["g"].GetInt();
					}
					if ( (*itr)["color"].HasMember("b") )
					{
						b = (*itr)["color"]["b"].GetInt();
					}

					obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
				}
				else
				{
					obj.linesToHighlight.push_back(0);
				}
			}
		}

		Compendium_t::Events_t::eventWorldIDLookup[name] = obj.id;
		Compendium_t::Events_t::worldIDToString[obj.id + Compendium_t::Events_t::kEventWorldOffset] = name;
		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						Compendium_t::Events_t::eventWorldLookup[(Compendium_t::EventTags)find2->second.id].insert(name);
					}
				}
			}
		}
		Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventWorldOffset + obj.id);
		Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventWorldOffset + obj.id);
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
						if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
							== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
						{
							vec.push_back((Compendium_t::EventTags)find2->second.id);
						}
					}
				}
			}
		}
		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
			int index = -1;
			for ( auto& v : vec )
			{
				++index;
				auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
				if ( v == EventTags::CPDM_CUSTOM_TAG )
				{
					if ( index < customEvents.size() )
					{
						vec2.push_back(customEvents[index]);
					}
					else
					{
						vec2.push_back("");
					}
				}
				else
				{
					vec2.push_back("");
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : worldObjects )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.details )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("details", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& color : obj.second.linesToHighlight )
				{
					rapidjson::Value val(rapidjson::kObjectType);
					if ( color == 0 )
					{
					}
					else
					{
						Uint8 r, g, b, a;
						getColor(color, &r, &g, &b, &a);

						rapidjson::Value colorVal(rapidjson::kObjectType);
						colorVal.AddMember("r", r, d.GetAllocator());
						colorVal.AddMember("g", g, d.GetAllocator());
						colorVal.AddMember("b", b, d.GetAllocator());
						colorVal.AddMember("a", a, d.GetAllocator());

						val.AddMember("color", colorVal, d.GetAllocator());
					}
					arr.PushBack(val, d.GetAllocator());
				}
				entry.AddMember("details_line_highlights", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_world.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

std::map<std::string, int> Compendium_t::Events_t::monsterUniqueIDLookup;
std::map<Compendium_t::EventTags, std::set<int>> Compendium_t::Events_t::eventMonsterLookup;
void Compendium_t::readMonstersTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_monsters.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMonstersTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = monsters.find(key);
		if ( find != monsters.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.abilities.clear();
			if ( itr->value.HasMember("abilities") )
			{
				jsonVecToVec(itr->value["abilities"], find->second.abilities);
			}
			find->second.inventory.clear();
			if ( itr->value.HasMember("inventory") )
			{
				jsonVecToVec(itr->value["inventory"], find->second.inventory);
			}
		}
	}
}

void Compendium_t::readMonstersFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/monsters.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMonstersFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("monsters") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	monsters.clear();

	Compendium_t::Events_t::monsterUniqueIDLookup.clear();
	Compendium_t::Events_t::eventMonsterLookup.clear();
	Compendium_t::Events_t::monsterIDToString.clear();
	
	Compendium_t::CompendiumMonsters_t::readContentsLang();

	if ( d.HasMember("unique_tags") )
	{
		for ( auto itr = d["unique_tags"].MemberBegin(); itr != d["unique_tags"].MemberEnd(); ++itr )
		{
			Compendium_t::Events_t::monsterUniqueIDLookup[itr->name.GetString()] = itr->value.GetInt();
		}
	}

	for ( int i = 0; i < NUMMONSTERS; ++i )
	{
		int type = i + Compendium_t::Events_t::kEventMonsterOffset;
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_SOLO].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_PARTY].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_MULTIPLAYER].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_BY].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_RECRUITED].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILL_XP].insert(type);
		if ( i == GYROBOT )
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GYROBOT_FLIPS].insert(type);
		}

		Compendium_t::Events_t::monsterIDToString[type] = monstertypename[i];
	}
	for ( auto& pair : Compendium_t::Events_t::monsterUniqueIDLookup )
	{
		int type = pair.second + Compendium_t::Events_t::kEventMonsterOffset;
		Compendium_t::Events_t::monsterIDToString[type] = pair.first;
		if ( pair.first == "ghost" )
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_SPAWNED].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_TELEPORTS].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_PINGS].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_PUSHES].insert(type);
		}
		else
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_SOLO].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_PARTY].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_MULTIPLAYER].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_BY].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_RECRUITED].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILL_XP].insert(type);

			if ( pair.first == "mysterious shop" )
			{
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_MERCHANT_ORBS].insert(type);
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_SHOP_BOUGHT].insert(type);
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_SHOP_SPENT].insert(type);
			}
		}
	}

	auto& entries = d["monsters"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		if ( itr->value.HasMember("type") )
		{
			name = itr->value["type"].GetString();
		}
		int monsterType = NOTHING;
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			if ( name == monstertypename[i] )
			{
				monsterType = i;
				break;
			}
		}

		if ( monsterType == NOTHING && name != "ghost" ) { continue; }

		auto& m = itr->value;
		auto& monster = monsters[itr->name.GetString()];
		monster.monsterType = monsterType;
		monster.unique_npc = m.HasMember("unique_npc") ? m["unique_npc"].GetString() : "";
		if ( m.HasMember("blurb") )
		{
			jsonVecToVec(m["blurb"], monster.blurb);
		}
		if ( m.HasMember("img") )
		{
			monster.imagePath = m["img"].GetString();
		}
		if ( m.HasMember("lore_points") )
		{
			monster.lorePoints = m["lore_points"].GetInt();
		}
		auto& stats = m["stats"];
		jsonVecToVec(stats["hp"], monster.hp);
		jsonVecToVec(stats["ac"], monster.ac);
		jsonVecToVec(stats["spd"], monster.spd);
		jsonVecToVec(stats["atk"], monster.atk);
		jsonVecToVec(stats["rangeatk"], monster.rangeatk);
		jsonVecToVec(stats["pwr"], monster.pwr);
		if ( stats.HasMember("str") )
		{
			jsonVecToVec(stats["str"], monster.str);
		}
		if ( stats.HasMember("con") )
		{
			jsonVecToVec(stats["con"], monster.con);
		}
		if ( stats.HasMember("dex") )
		{
			jsonVecToVec(stats["dex"], monster.dex);
		}
		if ( stats.HasMember("lvl") )
		{
			jsonVecToVec(stats["lvl"], monster.lvl);
		}
		if ( stats.HasMember("species") )
		{
			auto species = CompendiumMonsters_t::SPECIES_NONE;
			std::string str = stats["species"].GetString();
			if ( str == "humanoid" )
			{
				species = CompendiumMonsters_t::SPECIES_HUMANOID;
			}
			else if ( str == "beast" )
			{
				species = CompendiumMonsters_t::SPECIES_BEAST;
			}
			else if ( str == "beastfolk" )
			{
				species = CompendiumMonsters_t::SPECIES_BEASTFOLK;
			}
			else if ( str == "undead" )
			{
				species = CompendiumMonsters_t::SPECIES_UNDEAD;
			}
			else if ( str == "demonoid" )
			{
				species = CompendiumMonsters_t::SPECIES_DEMONOID;
			}
			else if ( str == "construct" )
			{
				species = CompendiumMonsters_t::SPECIES_CONSTRUCT;
			}
			else if ( str == "elemental" )
			{
				species = CompendiumMonsters_t::SPECIES_ELEMENTAL;
			}
			monster.species = species;
		}
		if ( m.HasMember("abilities") )
		{
			jsonVecToVec(m["abilities"], monster.abilities);
		}
		{
			int i = 0;
			for ( auto itr = m["resistances"].Begin(); itr != m["resistances"].End(); ++itr )
			{
				monster.resistances[i] = itr->GetInt();
				++i;
			}
		}
		if ( m.HasMember("inventory") )
		{
			jsonVecToVec(m["inventory"], monster.inventory);
		}
		if ( m.HasMember("models") )
		{
			jsonVecToVec(m["models"], monster.models);
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : monsters )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.abilities )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("abilities", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.inventory )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("inventory", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_monsters.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

Uint32 Compendium_t::lastTickUpdate = 0;
std::map<Compendium_t::EventTags, Compendium_t::Events_t::Event_t> Compendium_t::Events_t::events;
std::map<std::string, Compendium_t::EventTags> Compendium_t::Events_t::eventIdLookup;
std::map<int, std::set<Compendium_t::EventTags>> Compendium_t::Events_t::itemEventLookup;
std::map<Compendium_t::EventTags, std::set<int>> Compendium_t::Events_t::eventItemLookup;
std::map<Compendium_t::EventTags, std::set<std::string>> Compendium_t::Events_t::eventWorldLookup;
std::map<Compendium_t::EventTags, std::set<std::string>> Compendium_t::Events_t::eventCodexLookup;
std::map<std::string, int> Compendium_t::Events_t::eventWorldIDLookup;
std::map<std::string, int> Compendium_t::Events_t::eventCodexIDLookup;
std::map<Compendium_t::EventTags, std::map<int, int>> Compendium_t::Events_t::eventClassIds;
std::map<int, std::vector<Compendium_t::EventTags>> Compendium_t::Events_t::itemDisplayedEventsList;
std::map<int, std::vector<std::string>> Compendium_t::Events_t::itemDisplayedCustomEventsList;
std::map<std::string, std::string> Compendium_t::Events_t::customEventsValues;
std::map<Compendium_t::EventTags, std::map<int, Compendium_t::Events_t::EventVal_t>> Compendium_t::Events_t::playerEvents;
std::map<Compendium_t::EventTags, std::map<int, Compendium_t::Events_t::EventVal_t>> Compendium_t::Events_t::serverPlayerEvents[MAXPLAYERS];
std::map<Compendium_t::EventTags, std::map<std::string, std::string>> Compendium_t::Events_t::eventLangEntries;
std::map<std::string, std::map<std::string, std::string>> Compendium_t::Events_t::eventCustomLangEntries;

void Compendium_t::Events_t::readEventsTranslations()
{
	const std::string filename = "data/compendium/events_text.json";
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

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	eventLangEntries.clear();
	eventCustomLangEntries.clear();
	for ( auto itr = d["tags"].Begin(); itr != d["tags"].End(); ++itr )
	{
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			auto find = eventIdLookup.find(itr2->name.GetString());
			if ( find != eventIdLookup.end())
			{
				EventTags tag = eventIdLookup[find->first];
				auto& entry = eventLangEntries[tag];
				for ( auto itr3 = itr2->value.MemberBegin(); itr3 != itr2->value.MemberEnd(); ++itr3 )
				{
					entry[itr3->name.GetString()] = itr3->value.GetString();
				}
			}
		}
	}
	if ( d.HasMember("custom_tags") )
	{
		for ( auto itr = d["custom_tags"].MemberBegin(); itr != d["custom_tags"].MemberEnd(); ++itr )
		{
			for ( auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2 )
			{
				eventCustomLangEntries[itr->name.GetString()][itr2->name.GetString()] = itr2->value.GetString();
			}
		}
	}
}

std::string Compendium_t::Events_t::formatEventRecordText(Sint32 value, const char* formatType, int formatVal, std::map<std::string, std::string>& langMap)
{
	std::string resultsFormatting = "%d";
	if ( formatType && !strcmp(formatType, "cycle") )
	{
		std::string fmt = "format";
		fmt += std::to_string(formatVal);

		if ( langMap.find(fmt) != langMap.end() )
		{
			resultsFormatting = langMap[fmt];
		}
	}
	else if ( formatType && itemIDToString.find(formatVal) != itemIDToString.end()
		&& itemIDToString.at(formatVal) == formatType )
	{
		std::string fmt = "format_";
		fmt += formatType;
		if ( langMap.find(fmt) != langMap.end() )
		{
			resultsFormatting = langMap[fmt];
		}
		else if ( langMap.find("format") != langMap.end() )
		{
			resultsFormatting = langMap["format"];
		}
		else
		{
			return std::to_string(value);
		}
	}
	else if ( langMap.find("format") != langMap.end() )
	{
		resultsFormatting = langMap["format"];
	}
	else
	{
		return std::to_string(value);
	}

	std::string output = "";
	for ( size_t c = 0; c < resultsFormatting.size(); ++c )
	{
		if ( resultsFormatting[c] == '%' && ((c + 1) < resultsFormatting.size()) )
		{
			if ( resultsFormatting[c + 1] == 'm' )
			{
				// dist to meters
				float meters = value / (8.f);
				char buf[32];
				if ( meters >= 1000.f )
				{
					float km = meters / 1000.f;
					snprintf(buf, sizeof(buf), "%.2f km", km);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%.1f m", meters);
				}
				output += buf;
			}
			else if ( resultsFormatting[c + 1] == '$' )
			{
				int gold = value;
				// gold amounts
				char buf[32];
				if ( gold >= 1000.f )
				{
					float k = gold / 1000.f;
					snprintf(buf, sizeof(buf), "%.3fk ", k);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%d", gold);
				}
				output += buf;
			}
			else if ( resultsFormatting[c + 1] == 't' )
			{
				if ( langMap.find("format_time") != langMap.end() )
				{
					int numSymbols = 0;
					std::string fmt = langMap["format_time"];
					for ( size_t c = 0; c < fmt.size(); ++c )
					{
						if ( fmt[c] == '%' )
						{
							numSymbols++;
						}
					}
					Uint32 sec = (value / TICKS_PER_SECOND) % 60;
					Uint32 min = ((value / TICKS_PER_SECOND) / 60);
					Uint32 hour = (((value / TICKS_PER_SECOND) / 60) / 60);
					Uint32 day = ((value / TICKS_PER_SECOND) / 60) / 60 / 24;
					char buf[32] = "";
					if ( numSymbols == 2 )
					{
						// mins/secs
						min = std::min(min, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), min, sec);
						output += buf;
					}
					else if ( numSymbols == 3 )
					{
						// hours/mins/secs
						min = min % 60;
						hour = std::min(hour, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), hour, min, sec);
						output += buf;
					}
					else if ( numSymbols == 4 )
					{
						// days also
						min = min % 60;
						hour = hour % 24;
						day = std::min(day, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), day, hour, min, sec);
						output += buf;
					}
				}
				else
				{
					// ticks to seconds
					float seconds = value / (50.f);
					char buf[32];
					snprintf(buf, sizeof(buf), "%.1f", seconds);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == 'd' )
			{
				output += std::to_string(value);
			}
			else if ( resultsFormatting[c + 1] == 'p' )
			{
				if ( value >= 0 )
				{
					output += '+';
				}
				output += std::to_string(value);
			}
			else if ( resultsFormatting[c + 1] == 'h' )
			{
				real_t regen = value;
				if ( regen > 0.01 )
				{
					real_t nominalRegen = HEAL_TIME;
					regen = nominalRegen / regen;
					char buf[32];
					snprintf(buf, sizeof(buf), "%.f", regen * 100.0);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == 'e' )
			{
				real_t regen = value;
				if ( regen > 0.01 )
				{
					real_t nominalRegen = MAGIC_REGEN_TIME;
					regen = nominalRegen / regen;
					char buf[32];
					snprintf(buf, sizeof(buf), "%.f", regen * 100.0);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == '%' )
			{
				output += '%';
			}
			else if ( resultsFormatting[c + 1] == 's' )
			{
				if ( formatType )
				{
					if ( !strcmp(formatType, "stats") )
					{
						switch ( formatVal )
						{
						case STAT_STR:
							output += ItemTooltips.getItemStatShortName("STR");
							break;
						case STAT_DEX:
							output += ItemTooltips.getItemStatShortName("DEX");
							break;
						case STAT_CON:
							output += ItemTooltips.getItemStatShortName("CON");
							break;
						case STAT_INT:
							output += ItemTooltips.getItemStatShortName("INT");
							break;
						case STAT_PER:
							output += ItemTooltips.getItemStatShortName("PER");
							break;
						case STAT_CHR:
							output += ItemTooltips.getItemStatShortName("CHR");
							break;
						}
					}
					else if ( !strcmp(formatType, "class") )
					{
						std::string tmp = playerClassLangEntry(formatVal, 0);
						camelCaseString(tmp);
						output += tmp;
					}
					else if ( !strcmp(formatType, "race") )
					{
						std::string tmp = getMonsterLocalizedName(getMonsterFromPlayerRace(formatVal));
						camelCaseString(tmp);
						output += tmp;
					}
					else if ( !strcmp(formatType, "skills") )
					{
						std::string tmp = Player::SkillSheet_t::getSkillNameFromID(formatVal, true);
						camelCaseString(tmp);
						output += tmp;
					}
				}
			}
			++c;
		}
		else
		{
			output += resultsFormatting[c];
		}
	}
	return output;
}

std::vector<std::pair<std::string, Sint32>> Compendium_t::Events_t::getCustomEventValue(std::string key, 
	std::string compendiumSection, std::string compendiumContentsSelected, int specificClass)
{
	std::vector<std::pair<std::string, Sint32>> results;
	if ( customEventsValues.find(key) == customEventsValues.end() )
	{
		return results;
	}

	rapidjson::Document d;
	d.Parse(customEventsValues[key].c_str());
	if ( !d.IsObject() )
	{
		return results;
	}

	if ( d.HasMember("value") )
	{
		Events_t::Type type = MAX;
		int minValue = INT_MAX;
		int maxValue = 0;
		bool firstResult = true;
		std::string valueType = "";
		if ( d["value"].HasMember("type") )
		{
			valueType = d["value"]["type"].GetString();
			if ( valueType == "max" )
			{
				type = MAX;
			}
			else if ( valueType == "min" )
			{
				type = MIN;
			}
			else if ( valueType == "sum" )
			{
				type = SUM;
			}
			else if ( valueType == "class_sum" )
			{
				type = SUM;
			}
		}

		int specificItemId = -1;
		if ( compendiumSection == "items" || compendiumSection == "magic" )
		{
			specificItemId = specificClass;
			specificClass = -1;
		}
		else
		{
			if ( valueType != "class_sum" )
			{
				specificClass = -1;
			}
		}

		std::map<int, int> mapValueTotals;
		std::string formatType = "";
		int cycleResults = 0;
		bool foundTag = false;
		if ( d["value"].HasMember("format") )
		{
			formatType = d["value"]["format"].GetString();
		}
		if ( d["value"].HasMember("compendium_section") )
		{
			compendiumSection = d["value"]["compendium_section"].GetString();
		}
		if ( d["value"].HasMember("tags") )
		{
			for ( auto itr = d["value"]["tags"].Begin(); itr != d["value"]["tags"].End(); ++itr )
			{
				std::string name = (*itr)["name"].GetString();
				std::string cat = (*itr)["category"].GetString();

				if ( compendiumSection == "items" || compendiumSection == "magic" )
				{
					// items dont use the category header by default, either specific item or current viewed item
				}
				else
				{
					if ( cat == "" )
					{
						cat = compendiumContentsSelected;
					}
				}

				if ( valueType == "sum_items" )
				{
					auto findTag = eventIdLookup.find(name);
					if ( findTag != eventIdLookup.end() )
					{
						auto& playerTags = playerEvents[findTag->second];
						for ( auto itemId : eventItemLookup[findTag->second] )
						{
							if ( cat == "spells" )
							{
								if ( itemId >= kEventSpellOffset )
								{
									int spellID = itemId - kEventSpellOffset;
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "spellbooks" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && ::items[itemId].category == SPELLBOOK )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "equipment" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && ::items[itemId].item_slot != NO_EQUIP )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "armor" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && 
									::items[itemId].item_slot != NO_EQUIP
									&& ::items[itemId].item_slot != EQUIPPABLE_IN_SLOT_WEAPON )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "weapons" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS
									&& ::items[itemId].item_slot == EQUIPPABLE_IN_SLOT_WEAPON )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
						}
					}
					continue;
				}

				int foundId = -1;
				if ( compendiumSection == "items" || compendiumSection == "magic" )
				{
					if ( cat == "" )
					{
						if ( itemIDToString.find(specificItemId) != itemIDToString.end() )
						{
							foundId = specificItemId;
						}
					}
					else
					{
						auto find = ItemTooltips.itemNameStringToItemID.find(cat);
						if ( find != ItemTooltips.itemNameStringToItemID.end() )
						{
							foundId = find->second;
						}
					}
				}
				else
				{
					auto& eventSectionIDLookup = compendiumSection == "codex" ? eventCodexIDLookup : eventWorldIDLookup;
					auto findCat = eventSectionIDLookup.find(cat);
					if ( findCat != eventSectionIDLookup.end() )
					{
						foundId = findCat->second;
					}
				}
				if ( foundId >= 0 )
				{
					auto findTag = eventIdLookup.find(name);
					if ( findTag != eventIdLookup.end() )
					{
						auto tag = findTag->second;
						if ( playerEvents.find(tag) == playerEvents.end() )
						{
							if ( valueType == "list" )
							{
								results.push_back(std::make_pair("-", 0));
							}
							++cycleResults;
							continue;
						}
						auto& playerTags = playerEvents[tag];
						std::vector<std::pair<int, int>> codexIDs;
						codexIDs.push_back(std::make_pair(-1, foundId));

						bool foundLookup = false;
						if ( compendiumSection == "items" || compendiumSection == "magic" )
						{
							if ( eventItemLookup[tag].find(foundId) != eventItemLookup[tag].end() )
							{
								foundLookup = true;
							}
						}
						else
						{
							auto& eventSectionLookup = compendiumSection == "codex" ? eventCodexLookup : eventWorldLookup;
							if ( eventSectionLookup[tag].find(cat) != eventSectionLookup[tag].end() )
							{
								foundLookup = true;
							}
						}

						if ( foundLookup )
						{
							auto& def = events[tag];
							if ( def.attributes.find("stats") != def.attributes.end() && valueType != "max_class" )
							{
								if ( cat == "str" ) { codexIDs.back().first = STAT_STR; }
								if ( cat == "dex" ) { codexIDs.back().first = STAT_DEX; }
								if ( cat == "con" ) { codexIDs.back().first = STAT_CON; }
								if ( cat == "int" ) { codexIDs.back().first = STAT_INT; }
								if ( cat == "per" ) { codexIDs.back().first = STAT_PER; }
								if ( cat == "chr" ) { codexIDs.back().first = STAT_CHR; }
							}
							else if ( def.attributes.find("class") != def.attributes.end() )
							{
								codexIDs.clear();
								auto findClassTag = eventClassIds.find(tag);
								if ( findClassTag != eventClassIds.end() )
								{
									// iterate through classes
									int startOffsetId = -1;
									if ( def.attributes.find("skills") != def.attributes.end() )
									{
										if ( cat == "magic skill" )
										{
											startOffsetId = findClassTag->second[0] + PRO_SORCERY * kEventClassesMax;
										}
										else if ( cat == "casting skill" )
										{
											startOffsetId = findClassTag->second[0] + PRO_MYSTICISM * kEventClassesMax;
										}
										else if ( cat == "swimming skill" )
										{
											startOffsetId = findClassTag->second[0] + PRO_THAUMATURGY * kEventClassesMax;
										}
										else
										{
											for ( int i = 0; i < NUMPROFICIENCIES; ++i )
											{
												if ( cat == getSkillStringForCompendium(i) )
												{
													startOffsetId = findClassTag->second[0] + i * kEventClassesMax;
													break;
												}
											}
										}
									}

									for ( auto& classId : findClassTag->second )
									{
										if ( startOffsetId >= 0 )
										{
											if ( classId.second >= startOffsetId && classId.second < startOffsetId + kEventClassesMax )
											{
												if ( specificClass >= 0 )
												{
													if ( (classId.first % kEventClassesMax) != specificClass )
													{
														continue;
													}
												}
												codexIDs.push_back(classId);
												codexIDs.back().first = codexIDs.back().first % kEventClassesMax;
											}
										}
										else
										{
											if ( specificClass >= 0 )
											{
												if ( classId.first != specificClass )
												{
													continue;
												}
											}

											codexIDs.push_back(classId);
											if ( def.attributes.find("stats") != def.attributes.end() && valueType == "max_class" )
											{
												// we want to store the stat names rather than the class
												if ( cat == "str" ) { codexIDs.back().first = STAT_STR; }
												if ( cat == "dex" ) { codexIDs.back().first = STAT_DEX; }
												if ( cat == "con" ) { codexIDs.back().first = STAT_CON; }
												if ( cat == "int" ) { codexIDs.back().first = STAT_INT; }
												if ( cat == "per" ) { codexIDs.back().first = STAT_PER; }
												if ( cat == "chr" ) { codexIDs.back().first = STAT_CHR; }
											}
										}
									}
								}
							}
							else if ( def.attributes.find("race") != def.attributes.end() )
							{
								codexIDs.clear();
								auto findClassTag = eventClassIds.find(tag);
								if ( findClassTag != eventClassIds.end() )
								{
									// iterate through classes
									for ( auto& classId : findClassTag->second )
									{
										codexIDs.push_back(classId);
									}
								}
							}

							for ( auto& pair : codexIDs )
							{
								int codexID = pair.second;
								int classnum = pair.first;

								if ( formatType == "skills" )
								{
									if ( cat == "magic skill" )
									{
										classnum = PRO_SORCERY;
									}
									else if ( cat == "casting skill" )
									{
										classnum = PRO_MYSTICISM;
									}
									else if ( cat == "swimming skill" )
									{
										classnum = PRO_THAUMATURGY;
									}
									else
									{
										for ( int i = 0; i < NUMPROFICIENCIES; ++i )
										{
											if ( cat == getSkillStringForCompendium(i) )
											{
												classnum = i;
												break;
											}
										}
									}
								}

								if ( compendiumSection == "codex" )
								{
									if ( codexID < kEventCodexOffset )
									{
										codexID += kEventCodexOffset; // convert to offset
									}
								}
								else if ( compendiumSection == "world" )
								{
									if ( codexID < kEventWorldOffset )
									{
										codexID += kEventWorldOffset; // convert to offset
									}
								}

								auto findVal = playerTags.find(codexID);
								int val = 0;
								if ( findVal != playerTags.end() )
								{
									val = findVal->second.value;
									foundTag = true;
								}

								std::string output = "";
								int numFormats = 0;
								if ( valueType == "sum_category_max" || valueType == "sum_category_min" || valueType == "sum_category_cycle" )
								{
									int categoryValue = foundId;
									if ( formatType == "skills" )
									{
										if ( cat == "magic skill" )
										{
											categoryValue = PRO_SORCERY;
										}
										else if ( cat == "casting skill" )
										{
											categoryValue = PRO_MYSTICISM;
										}
										else if ( cat == "swimming skill" )
										{
											categoryValue = PRO_THAUMATURGY;
										}
										else
										{
											for ( int i = 0; i < NUMPROFICIENCIES; ++i )
											{
												if ( cat == getSkillStringForCompendium(i) )
												{
													categoryValue = i;
													break;
												}
											}
										}
									}
									mapValueTotals[categoryValue] += val;
								}
								else if ( valueType == "class_max_total" )
								{
									mapValueTotals[classnum] += val;
									continue;
								}
								else if ( formatType == "skills" )
								{
									output = formatEventRecordText(val, d["value"]["format"].GetString(), classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("stats") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "stats", classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("class") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "class", classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("race") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "race", classnum, eventCustomLangEntries[key]);
								}
								else if ( valueType == "cycle" )
								{
									if ( findVal != playerTags.end() )
									{
										output = formatEventRecordText(val, valueType.c_str(), cycleResults, eventCustomLangEntries[key]);
									}
									else
									{
										output = "-";
									}
								}

								results.push_back(std::make_pair(output, val));
								maxValue = std::max(maxValue, val);

								if ( findVal != playerTags.end() )
								{
									if ( firstResult )
									{
										minValue = val;
									}
									else
									{
										minValue = std::min(minValue, val);
									}
									firstResult = false;
								}

								++cycleResults;
							}
						}
					}
				}
			}
		}

		Sint32 sum = 0;
		if ( valueType == "cycle" )
		{
			bool filledEntry = false;
			for ( auto& pair : results )
			{
				if ( pair.first != "-" )
				{
					filledEntry = true;
				}
			}
			for ( auto itr = results.begin(); itr != results.end(); )
			{
				if ( filledEntry && itr->first == "-" )
				{
					itr = results.erase(itr); // don't display empty entries if something has data in it
				}
				else
				{
					++itr;
				}
			}
			return results;
		}
		else if ( valueType == "sum_items" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				sum += pair.second;
			}
			std::string output = formatEventRecordText(sum, formatType.c_str(), 0, eventCustomLangEntries[key]);
			results.push_back(std::make_pair(output, maxValue));
			return results;
		}
		else if ( valueType == "sum_category_max" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				maxValue = std::max(maxValue, pair.second);
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == maxValue )
				{
					std::string output = formatEventRecordText(maxValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, maxValue));
				}
			}
			return results;
		}
		else if ( valueType == "sum_category_cycle" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				std::string output = formatEventRecordText(pair.second, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
				results.push_back(std::make_pair(output, pair.second));
			}
			return results;
		}
		else if ( valueType == "sum_category_min" )
		{
			results.clear();
			firstResult = true;
			minValue = 0;
			for ( auto& pair : mapValueTotals )
			{
				if ( firstResult )
				{
					minValue = pair.second;
				}
				else
				{
					minValue = std::min(minValue, pair.second);
				}
				firstResult = false;
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == minValue )
				{
					std::string output = formatEventRecordText(minValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, minValue));
				}
			}
			return results;
		}
		else if ( valueType == "class_max_total" )
		{
			maxValue = 0;
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				maxValue = std::max(maxValue, pair.second);
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == maxValue )
				{
					std::string output = formatEventRecordText(maxValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, maxValue));
				}
			}
			return results;
		}
		else if ( valueType == "list" )
		{
			if ( eventCustomLangEntries[key].find("format") != eventCustomLangEntries[key].end() )
			{
				if ( results.size() >= 1 )
				{
					char buf[1024] = "";
					snprintf(buf, sizeof(buf), eventCustomLangEntries[key]["format"].c_str(),
						results[0].first != "-" ? std::to_string(results[0].second).c_str() : "-",
						results[1].first != "-" ? std::to_string(results[1].second).c_str() : "-");
					results.clear();
					results.push_back(std::make_pair(buf, 0));
				}
				else
				{
					char buf[1024] = "";
					snprintf(buf, sizeof(buf), eventCustomLangEntries[key]["format"].c_str(),
						"-",
						"-");
					results.clear();
					results.push_back(std::make_pair(buf, 0));
				}
				return results;
			}
		}
		else
		{
			for ( auto itr = results.begin(); itr != results.end(); )
			{
				if ( type == MAX && itr->second != maxValue )
				{
					itr = results.erase(itr);
				}
				else if ( type == MIN && itr->second != minValue )
				{
					itr = results.erase(itr);
				}
				else
				{
					if ( type == SUM )
					{
						sum += itr->second;
					}
					++itr;
				}
			}
		}
		if ( type == SUM && results.size() > 0 )
		{
			if ( foundTag )
			{
				results.clear();
				results.push_back(std::make_pair(formatEventRecordText(sum, nullptr, 0, eventCustomLangEntries[key]), sum));
			}
			else
			{
				results.clear();
			}
		}
		else if ( (type == MIN || type == MAX) && !foundTag )
		{
			results.clear();
		}
	}

	return results;
}

void Compendium_t::Events_t::readEventsFromFile()
{
	const std::string filename = "data/compendium/events.json";
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

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	events.clear();
	eventIdLookup.clear();
	eventClassIds.clear();
	int classIdIndex = kEventCodexClassOffset;
	int index = -1;
	for ( auto itr = d["tags"].Begin(); itr != d["tags"].End(); ++itr )
	{
		++index;
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			const EventTags id = (EventTags)std::min(index, (int)CPDM_EVENT_TAGS_MAX);
			auto& entry = events[id];
			entry.id = id;
			entry.name = itr2->name.GetString();
			eventIdLookup[entry.name] = id;


			if ( itr2->value.HasMember("type") )
			{
				std::string type = itr2->value["type"].GetString();
				if ( type == "sum" )
				{
					entry.type = SUM;
				}
				else if ( type == "max" )
				{
					entry.type = MAX;
				}
				else if ( type == "bit" )
				{
					entry.type = BITFIELD;
				}
				else if ( type == "min" )
				{
					entry.type = MIN;
				}
			}
			entry.id = index;
			if ( itr2->value.HasMember("client") )
			{
				int tmp = itr2->value["client"].GetInt();
				tmp = std::max(0, tmp);
				tmp = std::min((int)Compendium_t::Events_t::CLIENT_UPDATETYPE_MAX - 1, tmp);
				entry.clienttype = (Compendium_t::Events_t::ClientUpdateType)tmp;
			}
			if ( itr2->value.HasMember("once_per_run") )
			{
				entry.eventTrackingType = itr2->value["once_per_run"].GetBool() ? EventTrackingType::ONCE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("unique_per_run") )
			{
				entry.eventTrackingType = itr2->value["unique_per_run"].GetBool() ? EventTrackingType::UNIQUE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("unique_per_floor") )
			{
				entry.eventTrackingType = itr2->value["unique_per_floor"].GetBool() ? EventTrackingType::UNIQUE_PER_FLOOR : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("attributes") )
			{
				if ( itr2->value["attributes"].IsArray() )
				{
					for ( auto arr_itr = itr2->value["attributes"].Begin(); arr_itr != itr2->value["attributes"].End(); ++arr_itr )
					{
						if ( arr_itr->IsString() )
						{
							entry.attributes.insert(arr_itr->GetString());
						}
					}
				}
				else if ( itr2->value["attributes"].IsString() )
				{
					entry.attributes.insert(itr2->value["attributes"].GetString());
				}
			}
			if ( entry.attributes.find("class") != entry.attributes.end() || entry.attributes.find("race") != entry.attributes.end() )
			{
				if ( entry.attributes.find("skills") != entry.attributes.end() )
				{
					for ( int skillnum = 0; skillnum < 16; ++skillnum )
					{
						for ( int i = 0; i <= CLASS_PALADIN; ++i )
						{
							int index = i + skillnum * kEventClassesMax;
							eventClassIds[id][index] = (classIdIndex + index);
						}
					}
					classIdIndex += kEventClassesMax * 16;
				}
				else
				{
					for ( int i = 0; i <= CLASS_PALADIN; ++i )
					{
						eventClassIds[id][i] = (classIdIndex + i);
					}
					classIdIndex += kEventClassesMax;
				}
			}
		}
	}

	if ( d.HasMember("custom_tags") )
	{
		for ( auto itr = d["custom_tags"].MemberBegin(); itr != d["custom_tags"].MemberEnd(); ++itr )
		{
			eventIdLookup[itr->name.GetString()] = EventTags::CPDM_CUSTOM_TAG;

			rapidjson::StringBuffer os;
			os.Clear();
			rapidjson::Writer<rapidjson::StringBuffer> writer(os);
			itr->value.Accept(writer);

			customEventsValues[itr->name.GetString()] = os.GetString();
		}
	}
}

void Compendium_t::Events_t::loadItemsSaveData()
{
	const std::string filename = "savegames/compendium_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Warning: Could not locate json file %s", filename.c_str());
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

	const int bufSize = 360000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	int version = 0;
	if ( d.HasMember("version") )
	{
		version = d["version"].GetInt();
	}

	playerEvents.clear();
	for ( auto itr = d["items"].MemberBegin(); itr != d["items"].MemberEnd(); ++itr )
	{
		auto find = eventIdLookup.find(itr->name.GetString());
		if ( find == eventIdLookup.end() )
		{
			continue;
		}
		const EventTags id = (EventTags)std::min((int)find->second, (int)CPDM_EVENT_TAGS_MAX);
		for ( auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2 )
		{
			int itemType = std::stoi(itr2->name.GetString());
			Sint32 value = itr2->value.GetInt();
			if ( itemType >= kEventMonsterOffset && itemType < kEventMonsterOffset + 1000 )
			{
				eventUpdateMonster(0, id, nullptr, value, true, itemType);
				continue;
			}
			if ( itemType >= kEventWorldOffset && itemType < kEventWorldOffset + 1000 )
			{
				eventUpdateWorld(0, id, nullptr, value, true, itemType - kEventWorldOffset);
				continue;
			}
			if ( itemType >= kEventCodexOffset && itemType <= kEventCodexOffsetMax )
			{
				eventUpdateCodex(0, id, nullptr, value, true, itemType);
				continue;
			}
			if ( itemType < 0 || (itemType >= NUMITEMS && itemType < kEventSpellOffset) )
			{
				continue;
			}
			if ( itemType >= kEventSpellOffset )
			{
				eventUpdate(0, id, SPELL_ITEM, value, true, itemType - kEventSpellOffset);
			}
			else
			{
				eventUpdate(0, id, (ItemType)itemType, value, true);
			}
		}
	}

	CompendiumEntries.migrateOldSkillIndexes = false;
	if ( version == 1 )
	{
		CompendiumEntries.migrateOldSkillIndexes = true;
	}
	if ( CompendiumEntries.migrateOldSkillIndexes )
	{
		int oldClass = client_classes[0];
		std::vector<int> skillIndexes = { PRO_MYSTICISM, PRO_SORCERY, PRO_THAUMATURGY };
		for ( int i = 0; i < NUMCLASSES; ++i )
		{
			client_classes[0] = i;
			for ( auto skillID : skillIndexes )
			{
				const char* skillstr = Compendium_t::getSkillStringForCompendium(skillID);
				if ( strcmp(skillstr, "") )
				{
					Compendium_t::Events_t::eventUpdateCodex(0, Compendium_t::CPDM_CLASS_SKILL_LEGENDS, skillstr, 0, true);
					Compendium_t::Events_t::eventUpdateCodex(0, Compendium_t::CPDM_CLASS_SKILL_NOVICES, skillstr, 0, true);
					Compendium_t::Events_t::eventUpdateCodex(0, Compendium_t::CPDM_CLASS_SKILL_UPS, skillstr, 0, true);
					Compendium_t::Events_t::eventUpdateCodex(0, Compendium_t::CPDM_CLASS_SKILL_UPS_RUN_MAX, skillstr, 0, true);
					Compendium_t::Events_t::eventUpdateCodex(0, Compendium_t::CPDM_CLASS_SKILL_MAX, skillstr, 0, true);
				}
			}
		}
		client_classes[0] = oldClass;
	}
	CompendiumEntries.migrateOldSkillIndexes = false;
}

static ConsoleVariable<bool> cvar_compendiumClientSave("/compendium_client_save", false);
static ConsoleCommand ccmd_compendium_dummy_data(
	"/compendium_dummy_data", "Create test compendium data",
	[](int argc, const char** argv) {
		if ( argc < 2 )
		{
			return;
		}
		int playernum = atoi(argv[1]);
		Compendium_t::Events_t::createDummyClientData(playernum);
	});
void Compendium_t::Events_t::createDummyClientData(const int playernum)
{
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }
	for ( int i = 0; i < NUMITEMS; ++i )
	{
		for ( auto& tag : itemEventLookup[i] )
		{
			eventUpdate(playernum, tag, (ItemType)i, 1);
		}
	}
	for ( int i = 0; i < NUM_SPELLS; ++i )
	{
		for ( auto& tag : itemEventLookup[i + kEventSpellOffset] )
		{
			eventUpdate(playernum, tag, SPELL_ITEM, 1, false, i);
		}
	}
	for ( auto& pair : eventMonsterLookup  )
	{
		for ( auto monster : pair.second )
		{
			eventUpdateMonster(playernum, pair.first, nullptr, 1, false, monster);
		}
	}
	for ( auto& pair : eventWorldLookup )
	{
		for ( auto& world : pair.second )
		{
			eventUpdateWorld(playernum, pair.first, world.c_str(), 1);
		}
	}
	for ( auto& pair : eventCodexLookup )
	{
		if ( eventClassIds.find(pair.first) != eventClassIds.end() )
		{
			int oldclass = client_classes[playernum];
			for ( int c = 0; c < NUMCLASSES; ++c )
			{
				client_classes[playernum] = c;
				for ( auto& world : pair.second )
				{
					eventUpdateCodex(playernum, pair.first, world.c_str(), 1);
				}
			}
			client_classes[playernum] = oldclass;
		}
		else
		{
			for ( auto& world : pair.second )
			{
				eventUpdateCodex(playernum, pair.first, world.c_str(), 1);
			}
		}
	}
}

void Compendium_t::readUnlocksSaveData()
{
	CompendiumItems_t::unlocks.clear();
	CompendiumItems_t::itemUnlocks.clear();
	CompendiumWorld_t::unlocks.clear();
	CompendiumCodex_t::unlocks.clear();
	CompendiumMonsters_t::unlocks.clear();
	AchievementData_t::unlocks.clear();

	AchievementData_t::unlocks["experience"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["joker"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["teamwork"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["technique"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["adventure"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;

	CompendiumWorld_t::unlocks["minehead"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["hall of trials"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["portcullis"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["lever"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["door"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumMonsters_t::unlocks["human"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	for ( auto& data : CompendiumEntries.codex )
	{
		CompendiumCodex_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}
	/*CompendiumCodex_t::unlocks["class"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["classes list"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["races"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["stats metastats"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["melee"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["crits"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["flanking"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["backstabs"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
	// debug stuff
	/*for ( auto& data : CompendiumEntries.worldObjects )
	{
		CompendiumWorld_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}

	for ( auto& data : CompendiumEntries.monsters )
	{
		CompendiumMonsters_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}*/

	for ( auto& data : CompendiumEntries.items )
	{
		for ( auto& entry : data.second.items_in_category )
		{
			// debug stuff
			/*CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
			if ( entry.itemID == TOOL_TORCH
				|| entry.itemID == BRONZE_SWORD
				|| entry.itemID == WOODEN_SHIELD
				|| entry.itemID == BRONZE_AXE
				|| entry.itemID == QUARTERSTAFF
				|| entry.itemID == BRONZE_MACE
				|| (entry.itemID == SPELL_CAT && entry.spellID == SPELL_FORCEBOLT)
				|| entry.itemID == SCROLL_BLANK
				|| entry.itemID == MAGICSTAFF_OPENING
				|| entry.itemID == MAGICSTAFF_LOCKING )
			{
				CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
					? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
					entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}

	for ( auto& data : CompendiumEntries.magic )
	{
		for ( auto& entry : data.second.items_in_category )
		{
			// debug stuff
			/*CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
			if ( entry.itemID == TOOL_TORCH
				|| entry.itemID == BRONZE_SWORD
				|| entry.itemID == WOODEN_SHIELD
				|| entry.itemID == BRONZE_AXE
				|| entry.itemID == QUARTERSTAFF
				|| entry.itemID == BRONZE_MACE
				|| (entry.itemID == SPELL_ITEM && entry.spellID == SPELL_FORCEBOLT)
				|| entry.itemID == SCROLL_BLANK
				|| entry.itemID == MAGICSTAFF_OPENING
				|| entry.itemID == MAGICSTAFF_LOCKING )
			{
				CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
					? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
					entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}


	const std::string filename = "savegames/compendium_progress.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Warning: Could not locate json file %s", filename.c_str());
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

	const int bufSize = 200000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	if ( d.HasMember("items") )
	{
		for ( auto itr = d["items"].MemberBegin(); itr != d["items"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumItems_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("items_status") )
	{
		for ( auto itr = d["items_status"].MemberBegin(); itr != d["items_status"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			std::string name = itr->name.GetString();
			int id = std::stoi(name);
			CompendiumItems_t::itemUnlocks[id] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("achievements") )
	{
		for ( auto itr = d["achievements"].MemberBegin(); itr != d["achievements"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			AchievementData_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("world") )
	{
		for ( auto itr = d["world"].MemberBegin(); itr != d["world"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumWorld_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("codex") )
	{
		for ( auto itr = d["codex"].MemberBegin(); itr != d["codex"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumCodex_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("monsters") )
	{
		for ( auto itr = d["monsters"].MemberBegin(); itr != d["monsters"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumMonsters_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}
}

void Compendium_t::writeUnlocksSaveData()
{
	char path[PATH_MAX] = "";
	completePath(path, "savegames/compendium_progress.json", outputdir);

	rapidjson::Document exportDocument;
	exportDocument.SetObject();

	const int VERSION = 1;

	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumItems_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumItems_t::itemUnlocks )
	{
		obj.AddMember(rapidjson::Value(std::to_string(pair.first).c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items_status", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : AchievementData_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "achievements", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumWorld_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "world", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumCodex_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "codex", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumMonsters_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "monsters", obj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::Writer<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

void Compendium_t::Events_t::writeItemsSaveData()
{
	char path[PATH_MAX] = "";
	if ( *cvar_compendiumClientSave && multiplayer == CLIENT )
	{
		completePath(path, "savegames/compendium_items_mp.json", outputdir);
	}
	else
	{
		completePath(path, "savegames/compendium_items.json", outputdir);
	}

	rapidjson::Document exportDocument;
	exportDocument.SetObject();

	const int VERSION = 2;

	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
	rapidjson::Value itemsObj(rapidjson::kObjectType);
	for ( auto& pair : playerEvents )
	{
		const std::string& key = events[pair.first].name;
		rapidjson::Value namekey(key.c_str(), exportDocument.GetAllocator());
		itemsObj.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
		auto& obj = itemsObj[key.c_str()];
		for ( auto& itemsData : pair.second )
		{
			rapidjson::Value itemKey(std::to_string(itemsData.first).c_str(), exportDocument.GetAllocator());
			obj.AddMember(itemKey, itemsData.second.value, exportDocument.GetAllocator());
		}
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items", itemsObj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::Writer<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

bool Compendium_t::Events_t::EventVal_t::applyValue(const Sint32 val)
{
	bool first = firstValue;
	firstValue = false;
	if ( type == SUM )
	{
		value += val;
		if ( id != CPDM_SINKS_HEALTH_RESTORED )
		{
			if ( (Uint32)value >= 0x7FFFFFFF )
			{
				value = 0x7FFFFFFF;
			}
		}
		return true;
	}
	else if ( type == MAX )
	{
		if ( first )
		{
			value = val;
			return true;
		}
		if ( value == val )
		{
			return false;
		}
		value = std::max(val, value);
		return true;
	}
	else if ( type == MIN )
	{
		if ( first )
		{
			value = val;
			return true;
		}
		if ( value == val )
		{
			return false;
		}
		if ( value == 0 )
		{
			value = val;
			return true;
		}
		value = std::min(val, value);
		return true;
	}
	else if ( type == BITFIELD )
	{
		value |= val;
		return true;
	}
	else
	{
		return false;
	}
}

void onCompendiumLevelExit(const int playernum, const char* level, const bool enteringLvl, const bool died)
{
	if ( !level ) { return; }
	if ( !strcmp(level, "") ) { return; }
	if ( enteringLvl )
	{
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_ENTERED, level, 1);
	}
	else
	{
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_EXITED, level, 1);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_GOLD, level, stats[playernum]->GOLD);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_LVL, level, stats[playernum]->LVL);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MIN_LVL, level, stats[playernum]->LVL);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MIN_COMPLETION, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_COMPLETION, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);

		if ( stats[playernum]->HP <= 0 || died )
		{
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, level, 1);
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TOTAL_TIME_SPENT, "minehead",
			players[playernum]->compendiumProgress.playerGameTimeTotal);
	}
}

void Compendium_t::Events_t::updateEventsInMainLoop(const int playernum)
{
	if ( !(players[playernum] && players[playernum]->entity && stats[playernum]) )
	{
		return;
	}

	if ( ticks % TICKS_PER_SECOND == 25 )
	{
		auto entity = players[playernum]->entity;
		auto myStats = stats[playernum];
		{
			real_t resistance = 100.0 * Entity::getDamageTableMultiplier(entity, *myStats, DAMAGE_TABLE_MAGIC);
			resistance = -(resistance - 100.0);
			eventUpdateCodex(playernum, CPDM_RES_MAX, "res", (int)resistance);
			eventUpdateCodex(playernum, CPDM_CLASS_RES_MAX, "res", (int)resistance);
		}

		{
			{
				Sint32 value = statGetSTR(myStats, entity);
				value -= myStats->STR;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "str", value);
				}
			}
			{
				Sint32 value = statGetDEX(myStats, entity);
				value -= myStats->DEX;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "dex", value);
				}
			}
			{
				Sint32 value = statGetCON(myStats, entity);
				value -= myStats->CON;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "con", value);
				}
			}
			{
				Sint32 value = statGetINT(myStats, entity);
				value -= myStats->INT;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "int", value);
				}
			}
			{
				Sint32 value = statGetPER(myStats, entity);
				value -= myStats->PER;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "per", value);
				}
			}
			{
				Sint32 value = statGetCHR(myStats, entity);
				value -= myStats->CHR;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "chr", value);
				}
			}
		}

		{
			bool oldDefending = myStats->defending;
			myStats->defending = false;

			Sint32 ac = AC(myStats);

			Sint32 con = myStats->CON;
			myStats->CON = 0;
			int numBlessings = 0;
			Sint32 acFromArmor = AC(myStats);

			real_t targetACEffectiveness = Entity::getACEffectiveness(entity, myStats, true, nullptr, nullptr, numBlessings);
			int effectiveness = targetACEffectiveness * 100.0;

			myStats->CON = con;
			myStats->defending = oldDefending;

			eventUpdateCodex(playernum, CPDM_CLASS_AC_MAX, "ac", ac);
			eventUpdateCodex(playernum, CPDM_AC_MAX_FROM_BLESS, "ac", numBlessings);
			eventUpdateCodex(playernum, CPDM_AC_EFFECTIVENESS_MAX, "ac", effectiveness);
			eventUpdateCodex(playernum, CPDM_AC_EQUIPMENT_MAX, "ac", acFromArmor);
		}

		{
			eventUpdateCodex(playernum, CPDM_HP_MAX, "hp", myStats->MAXHP);
			eventUpdateCodex(playernum, CPDM_CLASS_HP_MAX, "hp", myStats->MAXHP);

			eventUpdateCodex(playernum, CPDM_MP_MAX, "mp", myStats->MAXMP);
			eventUpdateCodex(playernum, CPDM_CLASS_MP_MAX, "mp", myStats->MAXMP);
		}

		{
			int skillID = NUMPROFICIENCIES;
			if ( auto spell = players[playernum]->magic.selectedSpell() )
			{
				skillID = spell->skillID;
			}

			{
				// base PWR INT Bonus
				real_t bonus = getSpellBonusFromCasterINT(entity, myStats, skillID) * 100.0;
				real_t val = bonus;
				eventUpdateCodex(playernum, CPDM_CLASS_PWR_MAX, "pwr", (int)val);
			}

			// equip/effect bonus (minus INT)
			{
				real_t val = (getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_NONE, NUMPROFICIENCIES) * 100.0);
				// look for damage/healing spell bonus for mitre/magus hat
				if ( auto spell = getSpellFromID(SPELL_FIREBALL) )
				{
					val = std::max(val, getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_FIREBALL, spell->skillID) * 100.0);
				}
				if ( auto spell = getSpellFromID(SPELL_HEALING) )
				{
					val = std::max(val, getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_HEALING, spell->skillID) * 100.0);
				}
				real_t bonus = getSpellBonusFromCasterINT(entity, myStats, skillID);
				val -= bonus * 100.0;
				eventUpdateCodex(playernum, CPDM_PWR_MAX_EQUIP, "pwr", (int)val);
			}
		}
	}

	if ( ticks % (5 * TICKS_PER_SECOND) == 25 )
	{
		int weight = 0;
		int numDeathBoxes = 0;
		for ( node_t* node = stats[playernum]->inventory.first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( !item )
			{
				continue;
			}
			if ( itemCategory(item) == SPELL_CAT )
			{
				continue;
			}
			if ( item->type == TOOL_PLAYER_LOOT_BAG )
			{
				++numDeathBoxes;
			}
			if ( ::items[item->type].item_slot != NO_EQUIP
				&& itemIsEquipped(item, playernum) )
			{
				weight += item->getWeight();
			}
		}
		Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_WGT_EQUIPPED_MAX, "wgt", weight);
		if ( numDeathBoxes > 0 )
		{
			eventUpdate(playernum, CPDM_DEATHBOX_MOST_CARRIED, TOOL_PLAYER_LOOT_BAG, numDeathBoxes);
		}
	}
}

const char* Compendium_t::compendiumCurrentLevelToWorldString(const int currentlevel, const bool secretlevel)
{
	if ( !secretlevel )
	{
		if ( currentlevel == 0 )
		{
			return "minehead";
		}
		else if ( currentlevel == 5 || currentlevel == 10 || currentlevel == 15
			|| currentlevel == 30 )
		{
			return "transition floor";
		}
		else if ( currentlevel >= 1 && currentlevel <= 4 )
		{
			return "mines";
		}
		else if ( currentlevel >= 6 && currentlevel <= 9 )
		{
			return "swamps";
		}
		else if ( currentlevel >= 11 && currentlevel <= 14 )
		{
			return "labyrinth";
		}
		else if ( currentlevel >= 16 && currentlevel <= 19 )
		{
			return "ruins";
		}
		else if ( currentlevel == 20 )
		{
			return "herx lair";
		}
		else if ( currentlevel >= 21 && currentlevel <= 23 )
		{
			return "hell";
		}
		else if ( currentlevel == 24 )
		{
			return "molten throne";
		}
		else if ( currentlevel == 25 )
		{
			return "hamlet";
		}
		else if ( currentlevel >= 26 && currentlevel <= 29 )
		{
			return "crystal caves";
		}
		else if ( currentlevel >= 31 && currentlevel <= 34 )
		{
			return "arcane citadel";
		}
		else if ( currentlevel == 35 )
		{
			return "citadel sanctum";
		}
	}
	else
	{
		if ( currentlevel == 3 )
		{
			return "gnomish mines";
		}
		else if ( currentlevel == 4 )
		{
			return "minetown";
		}
		else if ( currentlevel == 8 )
		{
			return "temple";
		}
		else if ( currentlevel == 9 )
		{
			return "haunted castle";
		}
		else if ( currentlevel == 12 )
		{
			return "sokoban";
		}
		else if ( currentlevel == 14 )
		{
			return "minotaur maze";
		}
		else if ( currentlevel == 17 )
		{
			return "mystic library";
		}
		else if ( currentlevel == 19 || currentlevel == 20 )
		{
			return "underworld";
		}
		else if ( currentlevel == 6 || currentlevel == 7 )
		{
			return "underworld";
		}
		else if ( currentlevel == 29 )
		{
			return "cockatrice lair";
		}
		else if ( currentlevel == 34 )
		{
			return "brams castle";
		}
	}
	return "";
}

void Compendium_t::Events_t::onEndgameEvent(const int playernum, const bool tutorialend, const bool saveHighscore, const bool died)
{
	if ( players[playernum]->isLocalPlayer() )
	{
		if ( tutorialend )
		{
			if ( stats[playernum]->HP <= 0 )
			{
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, "hall of trials", 1);
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
			}
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		else
		{
			players[playernum]->compendiumProgress.updateFloorEvents();
			if ( victory )
			{
				if ( currentlevel == 35 )
				{
					onCompendiumLevelExit(playernum, "citadel sanctum", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON, "races", 1);
				}
				else if ( currentlevel == 24 )
				{
					onCompendiumLevelExit(playernum, "molten throne", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON_HELL, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_HELL_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_HELL_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON_HELL, "races", 1);
				}
				else if ( currentlevel == 20 )
				{
					onCompendiumLevelExit(playernum, "herx lair", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON_CLASSIC, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_CLASSIC_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_CLASSIC_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON_CLASSIC, "races", 1);
				}
			}
			else
			{
				const char* currentWorldString = compendiumCurrentLevelToWorldString(currentlevel, secretlevel);
				if ( strcmp(currentWorldString, "") )
				{
					if ( stats[playernum]->HP <= 0 || died )
					{
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, currentWorldString, 1);
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
					}
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TOTAL_TIME_SPENT, "minehead",
						players[playernum]->compendiumProgress.playerGameTimeTotal);
				}
			}

			if ( multiplayer == SERVER && playernum == 0 )
			{
				for ( int i = 1; i < MAXPLAYERS; ++i )
				{
					sendClientDataOverNet(i);
				}
			}
		}
	}
}

void Player::CompendiumProgress_t::updateFloorEvents()
{
	for ( auto& p1 : floorEvents )
	{
		if ( p1.first >= 0 && p1.first < Compendium_t::EventTags::CPDM_EVENT_TAGS_MAX )
		{
			Compendium_t::EventTags tag = (Compendium_t::EventTags)p1.first;
			for ( auto& p2 : p1.second )
			{
				const char* category = p2.first.c_str();
				for ( auto& p3 : p2.second )
				{
					int eventID = p3.first;
					Sint32 value = p3.second;
					if ( eventID >= Compendium_t::Events_t::kEventCodexOffset && eventID <= Compendium_t::Events_t::kEventCodexOffsetMax )
					{
						Compendium_t::Events_t::eventUpdateCodex(player.playernum, tag, category, value, false);
					}
				}
			}
		}
	}

	floorEvents.clear();
}

void Compendium_t::Events_t::onLevelChangeEvent(const int playernum, const int prevlevel, const bool prevsecretfloor, const std::string prevmapname, const bool died)
{
	if ( intro ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS )
	{
		return;
	}

	if ( players[playernum]->isLocalPlayer() )
	{
		players[playernum]->compendiumProgress.updateFloorEvents();
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			const std::string mapname = map.name;
			if ( mapname.find("Tutorial Hub") == std::string::npos
				&& mapname.find("Tutorial ") != std::string::npos )
			{
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TRIALS_ATTEMPTS, "hall of trials", 1);
			}
			if ( prevmapname.find("Tutorial Hub") == std::string::npos
				&& prevmapname.find("Tutorial ") != std::string::npos )
			{
				if ( mapname.find("Tutorial Hub") != std::string::npos )
				{
					// returning to hub from a trial, success
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TRIALS_PASSED, "hall of trials", 1);
				}
			}
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		else
		{
			const char* currentWorldString = compendiumCurrentLevelToWorldString(currentlevel, secretlevel);
			if ( strcmp(currentWorldString, "") )
			{
				onCompendiumLevelExit(playernum, currentWorldString, true, died);
			}

			if ( stats[playernum] )
			{
				int numDeathBoxes = 0;
				for ( node_t* node = stats[playernum]->inventory.first; node; node = node->next )
				{
					Item* item = (Item*)node->element;
					if ( !item )
					{
						continue;
					}
					if ( item->type == TOOL_PLAYER_LOOT_BAG )
					{
						++numDeathBoxes;
					}
				}
				if ( numDeathBoxes > 0 )
				{
					eventUpdate(playernum, CPDM_DEATHBOX_TO_EXIT, TOOL_PLAYER_LOOT_BAG, numDeathBoxes);
				}
			}

			const char* prevWorldString = compendiumCurrentLevelToWorldString(prevlevel, prevsecretfloor);
			if ( !prevsecretfloor )
			{
				if ( prevlevel == 0 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel == 5 || prevlevel == 10 || prevlevel == 15
					|| prevlevel == 30 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 1 && prevlevel <= 4 )
				{
					onCompendiumLevelExit(playernum, "mines", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 4 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "mines", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "mines", 
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "mines",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 6 && prevlevel <= 9 )
				{
					onCompendiumLevelExit(playernum, "swamps", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 9 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "swamps", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "swamps",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "swamps",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 11 && prevlevel <= 14 )
				{
					onCompendiumLevelExit(playernum, "labyrinth", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 14 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "labyrinth", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "labyrinth",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "labyrinth",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 16 && prevlevel <= 19 )
				{
					onCompendiumLevelExit(playernum, "ruins", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 19 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "ruins", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "ruins",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "ruins",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel == 20 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 21 && prevlevel <= 23 )
				{
					onCompendiumLevelExit(playernum, "hell", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 23 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "hell", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "hell",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "hell",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel == 24 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel == 25 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 26 && prevlevel <= 29 )
				{
					onCompendiumLevelExit(playernum, "crystal caves", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 29 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "crystal caves", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "crystal caves",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "crystal caves",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 31 && prevlevel <= 34 )
				{
					onCompendiumLevelExit(playernum, "arcane citadel", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 34 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "arcane citadel", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "arcane citadel",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "arcane citadel",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
			}
			else
			{
				onCompendiumLevelExit(playernum, prevWorldString, false, died);
			}
		}
	}

	if ( !players[playernum]->isLocalPlayer() ) { return; }

	if ( multiplayer == SERVER && playernum == 0 )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			sendClientDataOverNet(i);
		}
	}
}

bool allowedCompendiumProgress()
{
	if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_CUSTOM_RUN && gameModeManager.currentSession.challengeRun.isActive()
		&& gameModeManager.currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		return false; // challenge event run
	}
	if ( Mods::disableSteamAchievements || (svFlags & SV_FLAG_CHEATS) )
	{
#ifndef DEBUG_ACHIEVEMENTS
		return false;
#endif
	}
	return true;
}

static ConsoleVariable<bool> cvar_compendiumDebugSave("/compendium_debug_save", false);

void Compendium_t::Events_t::eventUpdate(int playernum, const EventTags tag, const ItemType type, 
	Sint32 value, const bool loadingValue, const int spellID)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
		else if ( def.clienttype == CLIENT_AND_SERVER )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
			}
		}
	}

	int itemType = type;
	if ( type == SPELL_ITEM && spellID >= 0 )
	{
		itemType = kEventSpellOffset + spellID;
	}

	auto find2 = eventItemLookup[tag].find(itemType);
	if ( find2 == eventItemLookup[tag].end() )
	{
		return;
	}

	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(itemType);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][itemType] += value;
	}
	
	if ( loadingValue )
	{
		if ( e.find(itemType) == e.end() )
		{
			e[itemType] = EventVal_t(tag);
		}
		auto& val = e[itemType];
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			// don't update in tutorial
		}
		else
		{
			if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
			{
				if ( clientReceiveUpdateFromServer )
				{
					// server is tracking the total for us, so don't add
					players[playernum]->compendiumProgress.itemEvents[def.name][itemType] = value;
				}
				else
				{
					players[playernum]->compendiumProgress.itemEvents[def.name][itemType] += value;
				}
				value = players[playernum]->compendiumProgress.itemEvents[def.name][itemType];
			}
			if ( e.find(itemType) == e.end() )
			{
				e[itemType] = EventVal_t(tag);
			}
			auto& val = e[itemType];
			if ( val.applyValue(value) )
			{
				if ( *cvar_compendiumDebugSave )
				{
					if ( playernum == clientnum )
					{
						writeItemsSaveData();
					}
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		if ( tag == CPDM_RUNS_COLLECTED )
		{
			{
				Monster monsterUnlock = NOTHING;
				if ( type == TOOL_SENTRYBOT )
				{
					monsterUnlock = SENTRYBOT;
				}
				else if ( type == TOOL_SPELLBOT )
				{
					monsterUnlock = SPELLBOT;
				}
				else if ( type == TOOL_GYROBOT )
				{
					monsterUnlock = GYROBOT;
				}
				else if ( type == TOOL_DUMMYBOT )
				{
					monsterUnlock = DUMMYBOT;
				}
				if ( monsterUnlock != NOTHING )
				{
					int monsterId = Compendium_t::Events_t::kEventMonsterOffset + monsterUnlock;
					auto find = Compendium_t::Events_t::monsterIDToString.find(monsterId);
					if ( find != Compendium_t::Events_t::monsterIDToString.end() )
					{
						auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
						if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
						{
							unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
						}
					}
				}
			}

			bool itemUnlocked = false;
			{
				auto& unlockStatus = Compendium_t::CompendiumItems_t::itemUnlocks[itemType];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					itemUnlocked = true;
				}
			}
			auto find = itemIDToString.find(itemType);
			if ( find != itemIDToString.end() )
			{
				auto& unlockStatus = Compendium_t::CompendiumItems_t::unlocks[find->second];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
				/*else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_VISITED )
				{
					if ( itemUnlocked )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}*/
				else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::UNLOCKED_VISITED )
				{
					if ( itemUnlocked )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::UNLOCKED_UNVISITED;
					}
				}
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateMonster(int playernum, const EventTags tag, const Entity* entity,
	Sint32 value, const bool loadingValue, const int entryID)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int monsterType = -1;
	std::string monsterStrLookup = "";
	if ( entryID >= 0 )
	{
		monsterType = entryID;
	}
	else if ( entity && entity->behavior == &actDeathGhost )
	{
		if ( monsterUniqueIDLookup.find("ghost") != monsterUniqueIDLookup.end() )
		{
			monsterType = monsterUniqueIDLookup["ghost"];
		}
	}
	else if ( entity && entity->behavior == &actMonster )
	{
		if ( auto stats = entity->getStats() )
		{
			if ( stats->type == GNOME && stats->getAttribute("gnome_type").find("gnome2") != std::string::npos )
			{
				monsterStrLookup = "gnome thief";
			}
			else if ( stats->type == SHOPKEEPER && stats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
			{
				monsterStrLookup = "mysterious shop";
			}
			else
			{
				monsterStrLookup = stats->getAttribute("special_npc");
			}

			if ( monsterStrLookup != "" && monsterUniqueIDLookup.find(monsterStrLookup) != monsterUniqueIDLookup.end() )
			{
				monsterType = monsterUniqueIDLookup[monsterStrLookup];
			}
			else if ( monsterStrLookup == "" )
			{
				monsterType = stats->type;
			}
		}
	}

	if ( monsterType == -1 )
	{
		return;
	}

	if ( monsterType < kEventMonsterOffset )
	{
		monsterType += kEventMonsterOffset; // convert to offset
	}

	auto find2 = eventMonsterLookup[tag].find(monsterType);
	if ( find2 == eventMonsterLookup[tag].end() )
	{
		return;
	}

	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];
	if ( e.find(monsterType) == e.end() )
	{
		e[monsterType] = EventVal_t(tag);
	}

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(monsterType);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] += value;
	}

	auto& val = e[monsterType];
	if ( loadingValue )
	{
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] += value;
			}
			value = players[playernum]->compendiumProgress.itemEvents[def.name][monsterType];
		}

		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		auto find = monsterIDToString.find(monsterType);
		if ( find != monsterIDToString.end() )
		{
			auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
			if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
			{
				unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateWorld(int playernum, const EventTags tag, const char* category, Sint32 value, 
	const bool loadingValue, const int entryID, const bool commitUniqueValue)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }
	if ( multiplayer == SERVER && client_disconnected[playernum] )
	{
		return;
	}

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int worldID = -1;
	if ( entryID >= 0 )
	{
		worldID = entryID;
		bool foundCategory = false;
		for ( auto& cat : eventWorldLookup[tag] )
		{
			auto find = eventWorldIDLookup.find(cat);
			if ( find != eventWorldIDLookup.end() )
			{
				if ( eventWorldIDLookup[cat] == worldID )
				{
					foundCategory = true;
					break;
				}
			}
		}
		if ( !foundCategory )
		{
			return;
		}
	}
	else
	{
		auto find2 = eventWorldLookup[tag].find(category);
		if ( find2 == eventWorldLookup[tag].end() )
		{
			return;
		}
		auto find = eventWorldIDLookup.find(category);
		if ( find != eventWorldIDLookup.end() )
		{
			worldID = find->second;
		}
	}

	if ( worldID == -1 )
	{
		return;
	}

	if ( worldID < kEventWorldOffset )
	{
		worldID += kEventWorldOffset; // convert to offset
	}


	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(worldID);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][worldID] += value;
	}

	if ( loadingValue )
	{
		if ( e.find(worldID) == e.end() )
		{
			e[worldID] = EventVal_t(tag);
		}
		auto& val = e[worldID];
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][worldID] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][worldID] += value;
			}
			if ( commitUniqueValue )
			{
				value = players[playernum]->compendiumProgress.itemEvents[def.name][worldID];
			}
			else
			{
				return;
			}
		}
		if ( e.find(worldID) == e.end() )
		{
			e[worldID] = EventVal_t(tag);
		}
		auto& val = e[worldID];
		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		auto find = worldIDToString.find(worldID);
		if ( find != worldIDToString.end() )
		{
			auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks[find->second];
			if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
			{
				if ( find->second == "merchants guild"
					|| find->second == "magicians guild"
					|| find->second == "hunters guild"
					|| find->second == "the church"
					|| find->second == "masons guild" )
				{
					// dont reveal these, revealed @ hamlet
				}
				else
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
			if ( find->second == "shop" )
			{
				// buying items triggers shopkeep stuff
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + SHOPKEEPER);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "herx lair" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "hamlet" )
			{
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["merchants guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["magicians guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["hunters guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["the church"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["masons guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "molten throne" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + DEVIL);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}

				auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["brimstone boulder"];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
			else if ( find->second == "citadel sanctum" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH_FIRE);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH_ICE);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateCodex(int playernum, const EventTags tag, const char* category, 
	Sint32 value, const bool loadingValue, const int entryID, const bool floorEvent)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }
	if ( multiplayer == SERVER && client_disconnected[playernum] )
	{
		return;
	}

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			// don't update in tutorial
			if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN
				|| def.eventTrackingType == EventTrackingType::UNIQUE_PER_FLOOR )
			{
				return; 
			}
			else if ( tag == Compendium_t::CPDM_CLASS_MOVING_TIME
				|| tag == Compendium_t::CPDM_CLASS_SNEAK_TIME )
			{
				return;
			}
			if ( category )
			{
				auto find = CompendiumEntries.codex.find(category);
				if ( find != CompendiumEntries.codex.end() )
				{
					if ( !find->second.enableTutorial )
					{
						return;
					}
				}
			}
		}

		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int codexID = -1;
	int baseCodexID = -1;
	if ( entryID >= 0 )
	{
		codexID = entryID;
		bool foundCategory = false;
		if ( def.attributes.find("class") != def.attributes.end()
			|| def.attributes.find("race") != def.attributes.end() )
		{
			auto findClassTag = eventClassIds.find(tag);
			if ( findClassTag != eventClassIds.end() )
			{
				for ( auto& pair : findClassTag->second )
				{
					if ( pair.second == ((codexID < kEventCodexOffset) ? (codexID + kEventCodexOffset) : codexID) )
					{
						foundCategory = true;
						break;
					}
				}
			}
		}
		else
		{
			for ( auto& cat : eventCodexLookup[tag] )
			{
				auto find = eventCodexIDLookup.find(cat);
				if ( find != eventCodexIDLookup.end() )
				{
					if ( (eventCodexIDLookup[cat] + kEventCodexOffset)  == codexID )
					{
						foundCategory = true;
						break;
					}
				}
			}
		}
		if ( !foundCategory )
		{
			return;
		}
	}
	else
	{
		auto find2 = eventCodexLookup[tag].find(category);
		if ( find2 == eventCodexLookup[tag].end() )
		{
			return;
		}
		auto find = eventCodexIDLookup.find(category);
		if ( find != eventCodexIDLookup.end() )
		{
			codexID = find->second;
			baseCodexID = codexID;
			if ( def.attributes.find("class") != def.attributes.end() )
			{
				auto findClassTag = eventClassIds.find(tag);
				if ( findClassTag != eventClassIds.end() )
				{
					// iterate through classes
					int classId = client_classes[playernum];
					if ( def.attributes.find("skills") != def.attributes.end() )
					{
						for ( int i = 0; i < NUMPROFICIENCIES; ++i )
						{
							if ( !strcmp(category, getSkillStringForCompendium(i)) )
							{
								classId = client_classes[playernum] + i * kEventClassesMax;
								break;
							}
						}
					}

					auto findClassId = findClassTag->second.find(classId);
					if ( findClassId != findClassTag->second.end() )
					{
						codexID = findClassId->second;
					}
					else
					{
						codexID = -1;
					}
				}
			}
			else if ( def.attributes.find("race") != def.attributes.end() )
			{
				auto findRaceTag = eventClassIds.find(tag);
				if ( findRaceTag != eventClassIds.end() )
				{
					int race = RACE_HUMAN;
					if ( stats[playernum]->playerRace > 0 && stats[playernum]->stat_appearance == 0 )
					{
						race = stats[playernum]->playerRace;
					}
					auto findRaceId = findRaceTag->second.find(race);
					if ( findRaceId != findRaceTag->second.end() )
					{
						codexID = findRaceId->second;
					}
					else
					{
						codexID = -1;
					}
				}
			}
		}
	}

	if ( codexID == -1 )
	{
		return;
	}

	if ( codexID < kEventCodexOffset )
	{
		codexID += kEventCodexOffset; // convert to offset
	}
	if ( baseCodexID >= 0 )
	{
		if ( baseCodexID < kEventCodexOffset )
		{
			baseCodexID += kEventCodexOffset; // convert to offset
		}
	}


	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];
	if ( e.find(codexID) == e.end() )
	{
		e[codexID] = EventVal_t(tag);
	}

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(codexID);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][codexID] += value;
	}

	auto& val = e[codexID];
	if ( loadingValue )
	{
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][codexID] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][codexID] += value;
			}
			value = players[playernum]->compendiumProgress.itemEvents[def.name][codexID];
		}
		else if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_FLOOR && floorEvent )
		{
			players[playernum]->compendiumProgress.floorEvents[tag][category][codexID] += value;
			return;
		}

		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		if ( baseCodexID >= 0 )
		{
			auto find = codexIDToString.find(baseCodexID);
			if ( find != codexIDToString.end() )
			{
				auto& unlockStatus = Compendium_t::CompendiumCodex_t::unlocks[find->second];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
		}
	}
}

Uint8 Compendium_t::Events_t::clientSequence = 0;
int Compendium_t::Events_t::previousCurrentLevel = 0;
bool Compendium_t::Events_t::previousSecretlevel = false;
std::map<int, std::string> Compendium_t::Events_t::clientDataStrings[MAXPLAYERS];
std::map<int, std::map<int, std::string>> Compendium_t::Events_t::clientReceiveData;
void Compendium_t::Events_t::sendClientDataOverNet(const int playernum)
{
	if ( multiplayer == SERVER ) {
		if ( playernum == 0 || client_disconnected[playernum] ) {
			return;
		}

		if ( serverPlayerEvents[playernum].empty() )
		{
			return;
		}

		rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "seq", rapidjson::Value(clientSequence));
		rapidjson::Value data(rapidjson::kObjectType);
		for ( auto& p1 : serverPlayerEvents[playernum] )
		{
			std::string key = std::to_string(p1.first);
			rapidjson::Value namekey(key.c_str(), d.GetAllocator());
			data.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
			auto& obj = data[key.c_str()];
			for ( auto& itemsData : p1.second )
			{
				rapidjson::Value itemKey(std::to_string(itemsData.first).c_str(), d.GetAllocator());
				obj.AddMember(itemKey, itemsData.second.value, d.GetAllocator());
			}
		}
		CustomHelpers::addMemberToRoot(d, "item", data);

		rapidjson::StringBuffer os;
		rapidjson::Writer<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		clientDataStrings[playernum][clientSequence] = os.GetString();
		auto& dataStr = clientDataStrings[playernum][clientSequence];

		const size_t len = dataStr.size();
		if ( len == 0 )
		{
			return;
		}

		serverPlayerEvents[playernum].clear();

		// packet header
		memset(net_packet->data, 0, NET_PACKET_SIZE);
		memcpy(net_packet->data, "CMPD", 4);
		Uint8 sequence = 0;
		// encode name
		int chunksize = 256;
		const int numchunks = 1 + (len / chunksize);
		for ( int c = 0; c < len; c += chunksize )
		{
			sequence += 1;

			if ( c + chunksize > len )
			{
				chunksize = len - c;
			}
			net_packet->data[4] = clientSequence;
			net_packet->data[5] = sequence; // chunk index
			net_packet->data[6] = numchunks; // num chunks

			std::string substr = dataStr.substr(c, chunksize);
			stringCopy((char*)net_packet->data + 7, substr.c_str(),
				256 + 1, substr.size());

			net_packet->len = 7 + substr.size();

			net_packet->address.host = net_clients[playernum - 1].host;
			net_packet->address.port = net_clients[playernum - 1].port;
			sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
		}

		++clientSequence;
		if ( clientSequence >= 255 )
		{
			clientSequence = 0;
		}

		//else
		//{
		//	// packet header
		//	memcpy(net_packet->data, "CSCN", 4);
		//	net_packet->data[4] = 0;
		//	net_packet->len = 5;
		//	for ( int i = 1; i < MAXPLAYERS; i++ ) {
		//		if ( client_disconnected[i] ) {
		//			continue;
		//		}
		//		if ( playernum != i )
		//		{
		//			continue;
		//		}
		//		net_packet->address.host = net_clients[i - 1].host;
		//		net_packet->address.port = net_clients[i - 1].port;
		//		sendPacketSafe(net_sock, -1, net_packet, i - 1);
		//	}
		//}
	}
}

void Compendium_t::readModelLimbsFromFile(std::string section)
{
	std::string fullpath = "data/compendium/" + section + "_models/";
	for ( auto& f : directoryContents(fullpath.c_str(), false, true) )
	{
		std::string inputPath = fullpath + f;
		std::string path = PHYSFS_getRealDir(inputPath.c_str()) ? PHYSFS_getRealDir(inputPath.c_str()) : "";
		if ( path != "" )
		{
			path += PHYSFS_getDirSeparator();
			inputPath = path + inputPath;
			File* fp = FileIO::open(inputPath.c_str(), "rb");
			if ( !fp )
			{
				printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
				return;
			}
			static char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("limbs") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				return;
			}
			int version = d["version"].GetInt();

			std::string filename = f.substr(0, f.find(".json"));
			auto& entry = compendiumObjectLimbs[filename];
			entry.entities.clear();
			entry.baseCamera = CompendiumView_t();

			compendiumObjectMapTiles.erase(filename);
			int w = 0;
			int h = 0;
			int index = 0;
			if ( d.HasMember("map_tiles") )
			{
				auto& m = compendiumObjectMapTiles[filename];
				if ( d["map_tiles"].HasMember("floor") 
					&& d["map_tiles"].HasMember("mid") 
					&& d["map_tiles"].HasMember("top")
					&& d["map_tiles"].HasMember("width") 
					&& d["map_tiles"].HasMember("height") )
				{
					if ( d["map_tiles"]["floor"].IsArray()
						&& d["map_tiles"]["mid"].IsArray()
						&& d["map_tiles"]["top"].IsArray() )
					{
						auto floor = d["map_tiles"]["floor"].GetArray();
						auto mid = d["map_tiles"]["mid"].GetArray();
						auto top = d["map_tiles"]["top"].GetArray();
						w = d["map_tiles"]["width"].GetInt();
						h = d["map_tiles"]["height"].GetInt();
						if ( floor.Size() == mid.Size() &&
							floor.Size() == top.Size() && floor.Size() == (w * h) )
						{
							m.first.width = w;
							m.first.height = h;
							if ( d["map_tiles"].HasMember("ceiling") )
							{
								m.first.ceiling = d["map_tiles"]["ceiling"].GetInt();
							}
							auto& tiles = m.second;
							tiles.resize(w * h * MAPLAYERS);
							for ( int z = 0; z < MAPLAYERS; ++z )
							{
								int x = 0;
								int y = 0;

								auto& arr = (z == 0) ? floor : ((z == 1) ? mid : top);
								for ( auto tile = arr.Begin(); tile != arr.End(); ++tile )
								{
									int index = z + (y * MAPLAYERS) + (x * MAPLAYERS * h);
									tiles[index] = tile->GetInt();

									// fix animated tiles so they always start on the correct index
									constexpr int numTileAtlases = sizeof(AnimatedTile::indices) / sizeof(AnimatedTile::indices[0]);
									if ( animatedtiles[tiles[index]] ) {
										auto find = tileAnimations.find(tiles[index]);
										if ( find == tileAnimations.end() ) {
											// this is not the correct index!
											for ( const auto& pair : tileAnimations ) {
												const auto& animation = pair.second;
												for ( int i = 0; i < numTileAtlases; ++i ) {
													if ( animation.indices[i] == tiles[index] ) {
														tiles[index] = animation.indices[0];
													}
												}
											}
										}
									}

									++x;
									if ( x >= w )
									{
										x = 0;
										++y;
									}
								}

							}
						}
					}
				}
			}
			if ( d.HasMember("camera") )
			{
				entry.baseCamera.inUse = true;
				auto& c = d["camera"];
				if ( c.HasMember("ang_degrees") )
				{
					entry.baseCamera.ang = PI * c["ang_degrees"].GetInt() / 180.0;
				}
				if ( c.HasMember("vang_degrees") )
				{
					entry.baseCamera.vang = PI * c["vang_degrees"].GetInt() / 180.0;
				}
				entry.baseCamera.rotateLimit = false;
				if ( c.HasMember("rotate_limit_degrees_min") )
				{
					entry.baseCamera.rotateLimitMin = PI * c["rotate_limit_degrees_min"].GetInt() / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				if ( c.HasMember("rotate_limit_degrees_max") )
				{
					entry.baseCamera.rotateLimitMax = PI * c["rotate_limit_degrees_max"].GetInt() / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				if ( c.HasMember("rotate_degrees") )
				{
					entry.baseCamera.rotate = PI * c["rotate_degrees"].GetInt() / 180.0;
				}
				else
				{
					entry.baseCamera.rotate = entry.baseCamera.rotateLimitMax - (entry.baseCamera.rotateLimitMax - entry.baseCamera.rotateLimitMin) / 2;
				}
				if ( c.HasMember("rotate_speed") )
				{
					entry.baseCamera.rotateSpeed = c["rotate_speed"].GetDouble();
				}
				if ( c.HasMember("zoom") )
				{
					entry.baseCamera.zoom = c["zoom"].GetDouble();
				}
				if ( c.HasMember("height") )
				{
					entry.baseCamera.height = c["height"].GetDouble();
				}
				if ( c.HasMember("pan") )
				{
					entry.baseCamera.pan = c["pan"].GetDouble();
				}
			}
			for ( auto itr = d["limbs"].Begin(); itr != d["limbs"].End(); ++itr )
			{
				/*if ( d.HasMember("height_offset") )
				{
					statue.heightOffset = d["height_offset"].GetDouble();
				}*/

				entry.entities.push_back(Entity(-1, 0, nullptr, nullptr));
				auto& limb = entry.entities[entry.entities.size() - 1];
				if ( index > 0 )
				{
					limb.x = entry.entities[0].x - (*itr)["x"].GetDouble();
					limb.y = entry.entities[0].y - (*itr)["y"].GetDouble();
				}
				else
				{
					limb.x = (*itr)["x"].GetDouble();
					limb.y = (*itr)["y"].GetDouble();
					limb.x += 8.0;
					limb.y += 8.0;

					if ( w > 0 && h > 0 )
					{
						// position in center of map
						if ( w % 2 == 1 )
						{
							limb.x += 16.0 * (w / 2);
						}
						else
						{
							limb.x += 16.0 * ((w - 1) / 2);
							limb.x += 8.0;
						}
						if ( h % 2 == 1 )
						{
							limb.y += 16.0 * (h / 2);
						}
						else
						{
							limb.y += 16.0 * ((h - 1) / 2);
							limb.y += 8.0;
						}
					}
				}
				limb.z = (*itr)["z"].GetDouble();
				limb.focalx = (*itr)["focalx"].GetDouble();
				limb.focaly = (*itr)["focaly"].GetDouble();
				limb.focalz = (*itr)["focalz"].GetDouble();
				limb.pitch = (*itr)["pitch"].GetDouble();
				limb.roll = (*itr)["roll"].GetDouble();
				limb.yaw = (*itr)["yaw"].GetDouble();
				if ( (*itr).HasMember("yaw_degrees") )
				{
					limb.yaw += PI * (*itr)["yaw_degrees"].GetInt() / 180.0;
				}
				if ( (*itr).HasMember("scalex") )
				{
					limb.scalex = (*itr)["scalex"].GetDouble();
				}
				if ( (*itr).HasMember("scaley") )
				{
					limb.scaley = (*itr)["scaley"].GetDouble();
				}
				if ( (*itr).HasMember("scalez") )
				{
					limb.scalez = (*itr)["scalez"].GetDouble();
				}
				limb.sprite = (*itr)["sprite"].GetInt();

				++index;
			}

			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}
}

void Compendium_t::exportCurrentMonster(Entity* monster)
{
	if ( !monster )
	{
		return;
	}

	int filenum = 0;
	std::string monsterName = monster->getStats() ? monstertypename[monster->getStats()->type] : "nothing";
	std::string testPath = "/data/compendium/monster_models/" + monsterName + std::to_string(filenum) + ".json";
	while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
	{
		++filenum;
		testPath = "/data/compendium/monster_models/" + monsterName + std::to_string(filenum) + ".json";
	}

	std::string exportFileName = monsterName + std::to_string(filenum) + ".json";

	rapidjson::Document exportDocument;
	exportDocument.SetObject();
	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));

	rapidjson::Value cameraObject(rapidjson::kObjectType);
	cameraObject.AddMember("ang_degrees", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("vang_degrees", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("zoom", 0.0, exportDocument.GetAllocator());
	cameraObject.AddMember("height", 0.0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_limit_degrees_min", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_limit_degrees_max", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_speed", 0.0, exportDocument.GetAllocator());
	CustomHelpers::addMemberToRoot(exportDocument, "camera", cameraObject);

	rapidjson::Value limbsObject(rapidjson::kObjectType);

	rapidjson::Value limbsArray(rapidjson::kArrayType);

	std::vector<Entity*> allLimbs;
	allLimbs.push_back(monster);

	for ( auto& bodypart : monster->bodyparts )
	{
		allLimbs.push_back(bodypart);
	}

	int index = 0;
	for ( auto& limb : allLimbs )
	{
		if ( limb->flags[INVISIBLE] )
		{
			continue;
		}
		rapidjson::Value limbsObj(rapidjson::kObjectType);

		if ( index != 0 )
		{
			limbsObj.AddMember("x", rapidjson::Value(monster->x - limb->x), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(monster->y - limb->y), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		else
		{
			limbsObj.AddMember("x", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		limbsObj.AddMember("pitch", rapidjson::Value(limb->pitch), exportDocument.GetAllocator());
		limbsObj.AddMember("roll", rapidjson::Value(limb->roll), exportDocument.GetAllocator());
		limbsObj.AddMember("yaw", rapidjson::Value(limb->yaw), exportDocument.GetAllocator());
		limbsObj.AddMember("focalx", rapidjson::Value(limb->focalx), exportDocument.GetAllocator());
		limbsObj.AddMember("focaly", rapidjson::Value(limb->focaly), exportDocument.GetAllocator());
		limbsObj.AddMember("focalz", rapidjson::Value(limb->focalz), exportDocument.GetAllocator());
		limbsObj.AddMember("sprite", rapidjson::Value(limb->sprite), exportDocument.GetAllocator());
		limbsObj.AddMember("scalex", rapidjson::Value(limb->scalex), exportDocument.GetAllocator());
		limbsObj.AddMember("scaley", rapidjson::Value(limb->scaley), exportDocument.GetAllocator());
		limbsObj.AddMember("scalez", rapidjson::Value(limb->scalez), exportDocument.GetAllocator());
		limbsArray.PushBack(limbsObj, exportDocument.GetAllocator());

		++index;
	}

	CustomHelpers::addMemberToRoot(exportDocument, "limbs", limbsArray);
	
	std::string outputPath = PHYSFS_getRealDir("/data/compendium/monster_models");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/compendium/monster_models/" + exportFileName;
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	return;
}

bool Compendium_t::lorePointsFirstLoad = true;
void Compendium_t::updateLorePointCounts()
{
	lorePointsFirstLoad = false;
	lorePointsFromAchievements = 0;
	lorePointsSpent = 0;
	lorePointsAchievementsTotal = 0;
	int completed = 0;
	int total = achievements.size();
	for ( auto& achData : achievements )
	{
		if ( achData.second.unlocked )
		{
			lorePointsFromAchievements += achData.second.lorePoints;
			++completed;
		}
		lorePointsAchievementsTotal += achData.second.lorePoints;
	}
	total = std::max(1, total);
	AchievementData_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumItems_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumItems_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumItems_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.items.find(pair.first);
					if ( find != CompendiumEntries.items.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}

	for ( auto& item : CompendiumEntries.items )
	{
		total += item.second.items_in_category.size();
		for ( auto& entry : item.second.items_in_category )
		{
			int type = entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID;
			auto find = Compendium_t::CompendiumItems_t::itemUnlocks.find(type);
			if ( find != Compendium_t::CompendiumItems_t::itemUnlocks.end() )
			{
				if ( find->second != LOCKED_UNKNOWN )
				{
					completed++;
				}
			}
		}
	}
	total = std::max(1, total);
	CompendiumItems_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumMagic_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumItems_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumItems_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.magic.find(pair.first);
					if ( find != CompendiumEntries.magic.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);

	for ( auto& item : CompendiumEntries.magic )
	{
		total += item.second.items_in_category.size();
		for ( auto& entry : item.second.items_in_category )
		{
			int type = entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID;
			auto find = Compendium_t::CompendiumItems_t::itemUnlocks.find(type);
			if ( find != Compendium_t::CompendiumItems_t::itemUnlocks.end() )
			{
				if ( find->second != LOCKED_UNKNOWN )
				{
					completed++;
				}
			}
		}
	}
	total = std::max(1, total);
	CompendiumMagic_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	int seenEntries = 0;
	for ( auto& pair : CompendiumWorld_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumWorld_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumWorld_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.worldObjects.find(pair.first);
					if ( find != CompendiumEntries.worldObjects.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
				++seenEntries;
			}
		}
	}
	total = std::max(1, total);
	CompendiumWorld_t::completionPercent = 100.0 * (completed / (real_t)total);
	if ( seenEntries >= (total / 2) )
	{
		steamAchievement("BARONY_ACH_ISEENTIT");
	}

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumCodex_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumCodex_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumCodex_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.codex.find(pair.first);
					if ( find != CompendiumEntries.codex.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);
	CompendiumCodex_t::completionPercent = std::min(100.0, 100.0 * (completed / (real_t)total));

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumMonsters_t::contents_unfiltered["default"] )
	{
		if ( pair.first == "crab" || pair.first == "bubbles" )
		{
			continue;
		}
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumMonsters_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumMonsters_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.monsters.find(pair.first);
					if ( find != CompendiumEntries.monsters.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);
	CompendiumMonsters_t::completionPercent = 100.0 * (completed / (real_t)total);

	Compendium_t::PointsAnim_t::countUnreadLastTicks = 0;
	Compendium_t::PointsAnim_t::countUnreadNotifs();
}

real_t Compendium_t::PointsAnim_t::anim = 0.0;
real_t Compendium_t::PointsAnim_t::animNoFunds = 0.0;
Uint32 Compendium_t::PointsAnim_t::noFundsTick = 0;
Uint32 Compendium_t::PointsAnim_t::startTicks = 0;
Sint32 Compendium_t::PointsAnim_t::pointsCurrent = 0;
Sint32 Compendium_t::PointsAnim_t::pointsChange = 0;
Sint32 Compendium_t::PointsAnim_t::txtCurrentPoints = 0;
Sint32 Compendium_t::PointsAnim_t::txtChangePoints = 0;
bool Compendium_t::PointsAnim_t::showChanged = false;
bool Compendium_t::PointsAnim_t::noFundsAnimate = false;
bool Compendium_t::PointsAnim_t::firstLoad = true;
bool Compendium_t::PointsAnim_t::mainMenuAlert = false;
Uint32 Compendium_t::PointsAnim_t::countUnreadLastTicks = 0;

void Compendium_t::PointsAnim_t::countUnreadNotifs()
{
	if ( countUnreadLastTicks == 0 || (ticks - countUnreadLastTicks) > TICKS_PER_SECOND )
	{
		Compendium_t::CompendiumMonsters_t::numUnread = 0;
		Compendium_t::CompendiumCodex_t::numUnread = 0;
		Compendium_t::CompendiumItems_t::numUnread = 0;
		Compendium_t::CompendiumMagic_t::numUnread = 0;
		Compendium_t::CompendiumWorld_t::numUnread = 0;
		Compendium_t::AchievementData_t::numUnread = 0;

		countUnreadLastTicks = ticks;

		int numUnread = 0;
		for ( auto& unlockStatus : CompendiumCodex_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumCodex_t::contentsMap.find(unlockStatus.first) != CompendiumCodex_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumCodex_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumWorld_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumWorld_t::contentsMap.find(unlockStatus.first) != CompendiumWorld_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumWorld_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumItems_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumItems_t::contentsMap.find(unlockStatus.first) != CompendiumItems_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumItems_t::numUnread;
				}
				if ( CompendiumMagic_t::contentsMap.find(unlockStatus.first) != CompendiumMagic_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumMagic_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumMonsters_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumMonsters_t::contentsMap.find(unlockStatus.first) != CompendiumMonsters_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumMonsters_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : Compendium_t::AchievementData_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( Compendium_t::AchievementData_t::contentsMap.find(unlockStatus.first) != Compendium_t::AchievementData_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::AchievementData_t::numUnread;
				}
			}
		}

		mainMenuAlert = numUnread > 0;
	}
}

void Compendium_t::PointsAnim_t::tickAnimate()
{
	const auto balance = Compendium_t::lorePointsFromAchievements - Compendium_t::lorePointsSpent;

	noFundsAnimate = false;
	{
		// constant decay for animation
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animNoFunds -= setpointDiffX;
		animNoFunds = std::max(0.0, animNoFunds);

		if ( animNoFunds > 0.001 || (ticks - noFundsTick) < TICKS_PER_SECOND * .8 )
		{
			noFundsAnimate = true;
		}
	}

	bool pauseChangeAnim = false;
	if ( pointsChange != 0 )
	{
		Uint32 duration = pointsChange > 0 ? (3 * TICKS_PER_SECOND) : (TICKS_PER_SECOND / 2);
		if ( ((ticks - startTicks) > duration) )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (anim)) / 10.0;
			anim -= setpointDiffX;
			anim = std::max(0.0, anim);

			if ( anim <= 0.0001 )
			{
				pointsChange = 0;
			}
		}
		else
		{
			pauseChangeAnim = true;

			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - anim)) / 10.0;
			anim += setpointDiffX;
			anim = std::min(1.0, anim);
			anim = 1.0;
		}
	}

	showChanged = false;
	if ( pointsChange != 0 )
	{
		Sint32 displayedChange = anim * pointsChange;
		if ( pauseChangeAnim )
		{
			displayedChange = pointsChange;
		}
		if ( abs(displayedChange) > 0 )
		{
			showChanged = true;
			//changeGoldText->setDisabled(false);
			std::string s = "+";
			if ( pointsChange < 0 )
			{
				s = "";
			}
			s += std::to_string(displayedChange);
			txtChangePoints = displayedChange;
			//changeGoldText->setText(s.c_str());
			Sint32 displayedCurrent = pointsCurrent
				+ (pointsChange - displayedChange);
			//currentGoldText->setText(std::to_string(displayedCurrentGold).c_str());
			txtCurrentPoints = displayedCurrent;
		}
	}

	if ( !showChanged )
	{
		txtChangePoints = 0;
		txtCurrentPoints = balance;
		//changeGoldText->setDisabled(true);
		//changeGoldText->setText(std::to_string(displayedChangeGold).c_str());
		//currentGoldText->setText(std::to_string(balance).c_str());
	}
}

void Compendium_t::PointsAnim_t::noFundsEvent()
{
	playSound(90, 64);
	noFundsTick = ticks;
	animNoFunds = 1.0;
}

void Compendium_t::PointsAnim_t::pointsChangeEvent(Sint32 amount)
{
	bool addedToCurrentTotal = false;
	Uint32 duration = pointsChange > 0 ? (3 * TICKS_PER_SECOND) : (TICKS_PER_SECOND / 2);
	const bool isAnimatingValue = ((ticks - startTicks) > duration);
	const auto balance = Compendium_t::lorePointsFromAchievements - Compendium_t::lorePointsSpent;
	if ( amount < 0 )
	{
		if ( pointsChange < 0
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			if ( balance + amount < 0 )
			{
				pointsChange -= balance;
			}
			else
			{
				pointsChange += amount;
			}
		}
		else
		{
			if ( balance + amount < 0 )
			{
				pointsChange = -balance;
			}
			else
			{
				pointsChange = amount;
			}
		}
	}
	else
	{
		if ( pointsChange > 0
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			pointsChange += amount;
		}
		else
		{
			pointsChange = amount;
		}
	}
	startTicks = ticks;
	anim = 0.0;
	if ( !addedToCurrentTotal )
	{
		pointsCurrent = balance;
	}
}

std::vector<Sint32> Compendium_t::CompendiumMonsters_t::Monster_t::getDisplayStat(const char* name)
{
	std::vector<Sint32> retVal;
	if ( !name )
	{
		return retVal;
	}

	bool ignoreHardcore =
		(monsterType == DUMMYBOT
			|| monsterType == GYROBOT
			|| monsterType == SENTRYBOT
			|| monsterType == SPELLBOT
			|| monsterType == NOTHING
			|| monsterType == HUMAN
			);
	bool hardcore = !intro && (svFlags & SV_FLAG_HARDCORE);

	Stat stats(1000 + monsterType);
	if ( !strcmp(name, "hp") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return hp;
		}
		if ( hp.size() > 0 )
		{
			Sint32 statMin = hp[0];
			Sint32 statMax = statMin;
			if ( hp.size() > 1 )
			{
				statMax = hp[1];
			}

			int statIncrease = ((abs(statMin) / 20 + 1) * 20);
			statMin += statIncrease - (statIncrease / 5);
			statIncrease = ((abs(statMax) / 20 + 1) * 20);
			statMax += statIncrease;
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}
	else if ( !strcmp(name, "spd") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return spd;
		}
		if ( spd.size() > 0 )
		{
			Sint32 statMin = spd[0];
			Sint32 statMax = statMin;
			if ( spd.size() > 1 )
			{
				statMax = spd[1];
			}

			int statIncrease = std::min((abs(statMin) / 4 + 1) * 1, 8);
			statMin += statIncrease - (statIncrease / 2);
			statIncrease = std::min((abs(statMax) / 4 + 1) * 1, 8);
			statMax += statIncrease;
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}
	else if ( !strcmp(name, "ac") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return ac;
		}

		Sint32 statMin = stats.CON;
		Sint32 statMax = stats.CON + stats.RANDOM_CON;
		if ( con.size() > 0 )
		{
			statMin = con[0];
			if ( con.size() > 1 )
			{
				statMax = con[1];
			}
			else
			{
				statMax = con[0] + stats.RANDOM_CON;
			}
		}

		int statIncrease = (abs(statMin) / 5 + 1) * 1;
		int minIncrease = statIncrease - (statIncrease / 2);
		statIncrease = (abs(statMax) / 5 + 1) * 1;
		int maxIncrease = statIncrease;

		if ( ac.size() > 0 )
		{
			retVal.push_back(ac[0] + minIncrease);
			if ( ac.size() > 1 )
			{
				if ( ac[1] + maxIncrease != retVal[0] )
				{
					retVal.push_back(ac[1] + maxIncrease);
				}
			}
			else
			{
				if ( ac[0] + maxIncrease != retVal[0] )
				{
					retVal.push_back(ac[0] + maxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "atk") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return atk;
		}

		Sint32 statMin = stats.STR;
		Sint32 statMax = stats.STR + stats.RANDOM_STR;
		if ( str.size() > 0 )
		{
			statMin = str[0];
			if ( str.size() > 1 )
			{
				statMax = str[1];
			}
			else
			{
				statMax = str[0] + stats.RANDOM_STR;
			}
		}

		int statIncrease = (abs(statMin) / 5 + 1) * 5;
		int minIncrease = statIncrease - (statIncrease / 4);
		statIncrease = (abs(statMax) / 5 + 1) * 5;
		int maxIncrease = statIncrease;

		if ( atk.size() > 0 )
		{
			retVal.push_back(atk[0] + minIncrease);
			if ( atk.size() > 1 )
			{
				if ( atk[1] + maxIncrease != retVal[0] )
				{
					retVal.push_back(atk[1] + maxIncrease);
				}
			}
			else
			{
				if ( atk[0] + maxIncrease != retVal[0] )
				{
					retVal.push_back(atk[0] + maxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "rangeatk") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return rangeatk;
		}

		Sint32 statMinIncrease = 0;
		Sint32 statMaxIncrease = 0;

		{
			Sint32 statMinDEX = stats.DEX;
			Sint32 statMaxDEX = stats.DEX + stats.RANDOM_DEX;
			int statIncrease = std::min((abs(statMinDEX) / 4 + 1) * 1, 8);
			int minIncrease = (statIncrease / 2);
			statIncrease = std::min((abs(statMaxDEX) / 4 + 1) * 1, 8);
			int maxIncrease = statIncrease;

			statMinIncrease += minIncrease;
			statMaxIncrease += maxIncrease;
		}
		{
			Sint32 statMinPER = stats.PER;
			Sint32 statMaxPER = stats.PER + stats.RANDOM_PER;
			int statIncrease = (abs(statMinPER) / 5 + 1) * 5;
			int minIncrease = statIncrease - (statIncrease / 4);
			statIncrease = (abs(statMaxPER) / 5 + 1) * 5;
			int maxIncrease = statIncrease;

			statMinIncrease += minIncrease;
			statMaxIncrease += maxIncrease;
		}

		if ( rangeatk.size() > 0 )
		{
			retVal.push_back(rangeatk[0] + statMinIncrease);
			if ( rangeatk.size() > 1 )
			{
				if ( rangeatk[1] + statMaxIncrease != retVal[0] )
				{
					retVal.push_back(rangeatk[1] + statMaxIncrease);
				}
			}
			else
			{
				if ( rangeatk[0] + statMaxIncrease != retVal[0] )
				{
					retVal.push_back(rangeatk[0] + statMaxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "pwr") )
	{
		return pwr;
	}
	else if ( !strcmp(name, "lvl") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return lvl;
		}

		if ( lvl.size() > 0 )
		{
			Sint32 statMin = lvl[0];
			Sint32 statMax = statMin;
			if ( lvl.size() > 1 )
			{
				statMax = lvl[1] + 1;
			}
			else
			{
				statMax = statMin + 1;
			}
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}

	return retVal;
}
#endif

std::unordered_map<std::string, Compendium_t::AchievementData_t> Compendium_t::achievements;
bool Compendium_t::AchievementData_t::achievementsNeedResort = true;
bool Compendium_t::AchievementData_t::achievementsNeedFirstData = true;
int Compendium_t::lorePointsFromAchievements = 0;
int Compendium_t::lorePointsAchievementsTotal = 0;
int Compendium_t::lorePointsSpent = 0;
std::set<std::pair<std::string, std::string>, Compendium_t::AchievementData_t::Comparator> Compendium_t::AchievementData_t::achievementNamesSorted;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::AchievementData_t::achievementCategories;
std::map<std::string, Compendium_t::AchievementData_t::CompendiumAchievementsDisplay> Compendium_t::AchievementData_t::achievementsBookDisplay;
std::unordered_set<std::string> Compendium_t::AchievementData_t::achievementUnlockedLookup;
bool Compendium_t::AchievementData_t::sortAlphabetical = false;
std::string Compendium_t::compendium_sorting = "default";
bool Compendium_t::compendium_sorting_hide_undiscovered = false;
bool Compendium_t::compendium_sorting_hide_ach_unlocked = false;