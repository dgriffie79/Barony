#include "mod_tools_private.hpp"

void jsonVecToVec(cJSON* val, std::vector<std::string>& vec )
{
	if ( !val || !cJSON_IsArray(val) ) return;
	for ( cJSON* itr = val->child; itr; itr = itr->next )
	{
		if ( cJSON_IsString(itr) && itr->valuestring )
		{
			vec.push_back(itr->valuestring);
		}
	}
}

void jsonVecToVec(cJSON* val, std::vector<Sint32>& vec)
{
	if ( !val || !cJSON_IsArray(val) ) return;
	for ( cJSON* itr = val->child; itr; itr = itr->next )
	{
		if ( cJSON_IsNumber(itr) )
		{
			vec.push_back((Sint32)itr->valueint);
		}
	}
}

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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "contents") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	cJSON* contentsArr = cJSON_GetObjectItemCaseSensitive(d, "contents");
	if ( cJSON_IsArray(contentsArr) )
	{
		for ( cJSON* itr = contentsArr->child; itr; itr = itr->next )
		{
			for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
			{
				if ( itr2->valuestring && itr2->string )
				{
					contents["default"].push_back(std::make_pair(itr2->valuestring, itr2->string));
					if ( name == "contents_monsters"
						&& (!strcmp(itr2->valuestring, "crab") || !strcmp(itr2->valuestring, "bubbles")) )
					{
						// dont read into map
					}
					else
					{
						contentsMap[itr2->valuestring] = itr2->string;
					}
				}
			}
		}
	}

	if ( cJSON_HasObjectItem(d, "contents_alphabetical") )
	{
		cJSON* contentsAlpha = cJSON_GetObjectItemCaseSensitive(d, "contents_alphabetical");
		if ( cJSON_IsArray(contentsAlpha) )
		{
			for ( cJSON* itr = contentsAlpha->child; itr; itr = itr->next )
			{
				for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
				{
					if ( itr2->valuestring && itr2->string )
					{
						contents["alphabetical"].push_back(std::make_pair(itr2->valuestring, itr2->string));
						if ( name == "contents_monsters"
							&& (!strcmp(itr2->valuestring, "crab") || !strcmp(itr2->valuestring, "bubbles")) )
						{
							// dont read into map
						}
						else
						{
							contentsMap[itr2->valuestring] = itr2->string;
						}
					}
				}
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	for ( cJSON* itr = d->child; itr; itr = itr->next )
	{
		if ( !itr->string ) continue;
		std::string key = itr->string;
		auto find = items.find(key);
		if ( find != items.end() )
		{
			find->second.blurb.clear();
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(itr, "blurb");
			if ( blurb )
			{
				jsonVecToVec(blurb, find->second.blurb);
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	items.clear();
	Compendium_t::Events_t::itemEventLookup.clear();
	Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::Events_t::itemIDToString.clear();
	Compendium_t::CompendiumItems_t::readContentsLang();

	cJSON* entries = cJSON_GetObjectItemCaseSensitive(d, "items");
	if ( entries )
	{
		for ( cJSON* itr = entries->child; itr; itr = itr->next )
		{
			std::string name = itr->string ? itr->string : "";
			cJSON* w = itr;
			auto& obj = items[name];

			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(w, "blurb");
			if ( blurb )
			{
				jsonVecToVec(blurb, obj.blurb);
			}
			cJSON* wItems = cJSON_GetObjectItemCaseSensitive(w, "items");
			if ( wItems && cJSON_IsArray(wItems) )
			{
				for ( cJSON* itr_inner = wItems->child; itr_inner; itr_inner = itr_inner->next )
				{
					for ( cJSON* itr2 = itr_inner->child; itr2; itr2 = itr2->next )
					{
						CompendiumItems_t::Codex_t::CodexItem_t item;
						item.name = itr2->string ? itr2->string : "";
						item.rotation = 0;
						cJSON* rotation = cJSON_GetObjectItemCaseSensitive(itr2, "rotation");
						if ( rotation && cJSON_IsNumber(rotation) )
						{
							item.rotation = (Sint32)rotation->valueint;
						}
						obj.items_in_category.push_back(item);
					}
				}
			}
			cJSON* lorePoints = cJSON_GetObjectItemCaseSensitive(w, "lore_points");
			if ( lorePoints && cJSON_IsNumber(lorePoints) )
			{
				obj.lorePoints = (Sint32)lorePoints->valueint;
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

			cJSON* events = cJSON_GetObjectItemCaseSensitive(w, "events");
			if ( events && cJSON_IsArray(events) )
			{
				for ( cJSON* itr_e = events->child; itr_e; itr_e = itr_e->next )
				{
					std::string eventName = itr_e->valuestring ? itr_e->valuestring : "";
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
			cJSON* eventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "events_display");
			if ( eventsDisplay && cJSON_IsArray(eventsDisplay) )
			{
				for ( cJSON* itr_ed = eventsDisplay->child; itr_ed; itr_ed = itr_ed->next )
				{
					std::string eventName = itr_ed->valuestring ? itr_ed->valuestring : "";
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

			cJSON* customEventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "custom_events_display");
			if ( customEventsDisplay && cJSON_IsArray(customEventsDisplay) )
			{
				std::vector<std::string> customEvents;
				for ( cJSON* itr_ce = customEventsDisplay->child; itr_ce; itr_ce = itr_ce->next )
				{
					customEvents.push_back(itr_ce->valuestring ? itr_ce->valuestring : "");
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
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	for ( cJSON* itr = d->child; itr; itr = itr->next )
	{
		if ( !itr->string ) continue;
		std::string key = itr->string;
		auto find = magic.find(key);
		if ( find != magic.end() )
		{
			find->second.blurb.clear();
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(itr, "blurb");
			if ( blurb )
			{
				jsonVecToVec(blurb, find->second.blurb);
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	magic.clear();
	//Compendium_t::Events_t::itemEventLookup.clear();
	//Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::CompendiumMagic_t::readContentsLang();

	cJSON* entries = cJSON_GetObjectItemCaseSensitive(d, "items");
	std::unordered_set<std::string> ignoredSpells;
	std::unordered_set<std::string> ignoredSpellbooks;

	cJSON* excludeSpells = cJSON_GetObjectItemCaseSensitive(d, "exclude_spells");
	if ( excludeSpells && cJSON_IsArray(excludeSpells) )
	{
		for ( cJSON* itr = excludeSpells->child; itr; itr = itr->next )
		{
			ignoredSpells.insert(itr->valuestring ? itr->valuestring : "");
		}
	}
	cJSON* excludeSpellbooks = cJSON_GetObjectItemCaseSensitive(d, "exclude_spellbooks");
	if ( excludeSpellbooks && cJSON_IsArray(excludeSpellbooks) )
	{
		for ( cJSON* itr = excludeSpellbooks->child; itr; itr = itr->next )
		{
			ignoredSpellbooks.insert(itr->valuestring ? itr->valuestring : "");
		}
	}

	if ( entries )
	{
		for ( cJSON* itr = entries->child; itr; itr = itr->next )
		{
			std::string name = itr->string ? itr->string : "";
			cJSON* w = itr;
			auto& obj = magic[name];

			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(w, "blurb");
			if ( blurb )
			{
				jsonVecToVec(blurb, obj.blurb);
			}
			std::set<std::string> objSpellsLookup;
			cJSON* wItems = cJSON_GetObjectItemCaseSensitive(w, "items");
			if ( wItems && cJSON_IsArray(wItems) )
			{
				for ( cJSON* itr_inner = wItems->child; itr_inner; itr_inner = itr_inner->next )
				{
					for ( cJSON* itr2 = itr_inner->child; itr2; itr2 = itr2->next )
					{
						CompendiumItems_t::Codex_t::CodexItem_t item;
						item.name = itr2->string ? itr2->string : "";
						item.rotation = 0;
						cJSON* rotation = cJSON_GetObjectItemCaseSensitive(itr2, "rotation");
						if ( rotation && cJSON_IsNumber(rotation) )
						{
							item.rotation = (Sint32)rotation->valueint;
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
		}

		cJSON* lorePoints = cJSON_GetObjectItemCaseSensitive(w, "lore_points");
		if ( lorePoints && cJSON_IsNumber(lorePoints) )
		{
			obj.lorePoints = (Sint32)lorePoints->valueint;
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

		cJSON* events = cJSON_GetObjectItemCaseSensitive(w, "events");
		if ( events && cJSON_IsArray(events) )
		{
			for ( cJSON* itr_e = events->child; itr_e; itr_e = itr_e->next )
			{
				std::string eventName = itr_e->valuestring ? itr_e->valuestring : "";
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
		cJSON* eventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "events_display");
		if ( eventsDisplay && cJSON_IsArray(eventsDisplay) )
		{
			for ( cJSON* itr_ed = eventsDisplay->child; itr_ed; itr_ed = itr_ed->next )
			{
				std::string eventName = itr_ed->valuestring ? itr_ed->valuestring : "";
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

		cJSON* customEventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "custom_events_display");
		if ( customEventsDisplay && cJSON_IsArray(customEventsDisplay) )
		{
			std::vector<std::string> customEvents;
			for ( cJSON* itr_ce = customEventsDisplay->child; itr_ce; itr_ce = itr_ce->next )
			{
				customEvents.push_back(itr_ce->valuestring ? itr_ce->valuestring : "");
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
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	for ( cJSON* itr = d->child; itr; itr = itr->next )
	{
		if ( !itr->string ) continue;
		std::string key = itr->string;
		auto find = codex.find(key);
		if ( find != codex.end() )
		{
			find->second.blurb.clear();
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(itr, "blurb");
			if ( blurb )
			{
				jsonVecToVec(blurb, find->second.blurb);
			}
			find->second.details.clear();
			cJSON* details = cJSON_GetObjectItemCaseSensitive(itr, "details");
			if ( details )
			{
				jsonVecToVec(details, find->second.details);
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
			cJSON* detailsLineHighlights = cJSON_GetObjectItemCaseSensitive(itr, "details_line_highlights");
			if ( detailsLineHighlights && cJSON_IsArray(detailsLineHighlights) )
			{
				for ( cJSON* itr2 = detailsLineHighlights->child; itr2; itr2 = itr2->next )
				{
					cJSON* color = cJSON_GetObjectItemCaseSensitive(itr2, "color");
					if ( color )
					{
						Uint8 r = 0, g = 0, b = 0;
						cJSON* rItem = cJSON_GetObjectItemCaseSensitive(color, "r");
						if ( rItem && cJSON_IsNumber(rItem) ) { r = (Uint8)rItem->valueint; }
						cJSON* gItem = cJSON_GetObjectItemCaseSensitive(color, "g");
						if ( gItem && cJSON_IsNumber(gItem) ) { g = (Uint8)gItem->valueint; }
						cJSON* bItem = cJSON_GetObjectItemCaseSensitive(color, "b");
						if ( bItem && cJSON_IsNumber(bItem) ) { b = (Uint8)bItem->valueint; }

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
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "codex") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	codex.clear();

	Compendium_t::Events_t::eventCodexIDLookup.clear();
	Compendium_t::Events_t::eventCodexLookup.clear();
	Compendium_t::Events_t::codexIDToString.clear();
	Compendium_t::CompendiumCodex_t::readContentsLang();

	cJSON* entries = cJSON_GetObjectItemCaseSensitive(d, "codex");
	if ( entries )
	{
		for ( cJSON* itr = entries->child; itr; itr = itr->next )
		{
			std::string name = itr->string ? itr->string : "";
			cJSON* w = itr;
			auto& obj = codex[name];

			cJSON* eventLookup = cJSON_GetObjectItemCaseSensitive(w, "event_lookup");
			obj.id = eventLookup && cJSON_IsNumber(eventLookup) ? (Sint32)eventLookup->valueint : 0;
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(w, "blurb");
			if ( blurb ) { jsonVecToVec(blurb, obj.blurb); }
			cJSON* details = cJSON_GetObjectItemCaseSensitive(w, "details");
			if ( details ) { jsonVecToVec(details, obj.details); }
			cJSON* img = cJSON_GetObjectItemCaseSensitive(w, "img");
			obj.imagePath = img && img->valuestring ? img->valuestring : "";
			cJSON* enableTutorial = cJSON_GetObjectItemCaseSensitive(w, "enable_tutorial");
			if ( enableTutorial && cJSON_IsBool(enableTutorial) ) { obj.enableTutorial = cJSON_IsTrue(enableTutorial); }
			cJSON* renderedImgs = cJSON_GetObjectItemCaseSensitive(w, "rendered_imgs");
			if ( renderedImgs ) { jsonVecToVec(renderedImgs, obj.renderedImagePaths); }
			cJSON* models = cJSON_GetObjectItemCaseSensitive(w, "models");
			if ( models ) { jsonVecToVec(models, obj.models); }
			cJSON* lorePoints = cJSON_GetObjectItemCaseSensitive(w, "lore_points");
			if ( lorePoints && cJSON_IsNumber(lorePoints) ) { obj.lorePoints = (Sint32)lorePoints->valueint; }
			obj.linesToHighlight.clear();
			for ( auto& line : obj.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' ) { line[0] = '\x1E'; }
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' ) { line[c] = '\x1E'; break; }
							else if ( line[c] != ' ' ) { break; }
						}
					}
				}
			}
			cJSON* featureImg = cJSON_GetObjectItemCaseSensitive(w, "feature_img");
			if ( featureImg && featureImg->valuestring ) { obj.featureImg = featureImg->valuestring; }
			cJSON* detailsLineHighlights = cJSON_GetObjectItemCaseSensitive(w, "details_line_highlights");
			if ( detailsLineHighlights && cJSON_IsArray(detailsLineHighlights) )
			{
				for ( cJSON* itr_dlh = detailsLineHighlights->child; itr_dlh; itr_dlh = itr_dlh->next )
				{
					cJSON* color = cJSON_GetObjectItemCaseSensitive(itr_dlh, "color");
					if ( color )
					{
						Uint8 r = 0, g = 0, b = 0;
						cJSON* rItem = cJSON_GetObjectItemCaseSensitive(color, "r");
						if ( rItem && cJSON_IsNumber(rItem) ) { r = (Uint8)rItem->valueint; }
						cJSON* gItem = cJSON_GetObjectItemCaseSensitive(color, "g");
						if ( gItem && cJSON_IsNumber(gItem) ) { g = (Uint8)gItem->valueint; }
						cJSON* bItem = cJSON_GetObjectItemCaseSensitive(color, "b");
						if ( bItem && cJSON_IsNumber(bItem) ) { b = (Uint8)bItem->valueint; }
						obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else { obj.linesToHighlight.push_back(0); }
				}
			}

			Compendium_t::Events_t::eventCodexIDLookup[name] = obj.id;
			Compendium_t::Events_t::codexIDToString[obj.id + Compendium_t::Events_t::kEventCodexOffset] = name;
			cJSON* events = cJSON_GetObjectItemCaseSensitive(w, "events");
			if ( events && cJSON_IsArray(events) )
			{
				for ( cJSON* itr_e = events->child; itr_e; itr_e = itr_e->next )
				{
					std::string eventName = itr_e->valuestring ? itr_e->valuestring : "";
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
			cJSON* eventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "events_display");
			if ( eventsDisplay && cJSON_IsArray(eventsDisplay) )
			{
				for ( cJSON* itr_ed = eventsDisplay->child; itr_ed; itr_ed = itr_ed->next )
				{
					std::string eventName = itr_ed->valuestring ? itr_ed->valuestring : "";
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
			cJSON* customEventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "custom_events_display");
			if ( customEventsDisplay && cJSON_IsArray(customEventsDisplay) )
			{
				std::vector<std::string> customEvents;
				for ( cJSON* itr_ce = customEventsDisplay->child; itr_ce; itr_ce = itr_ce->next )
				{
					customEvents.push_back(itr_ce->valuestring ? itr_ce->valuestring : "");
				}

				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() ) { vec2.push_back(customEvents[index]); }
						else { vec2.push_back(""); }
					}
					else { vec2.push_back(""); }
				}
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	for ( cJSON* itr = d->child; itr; itr = itr->next )
	{
		if ( !itr->string ) continue;
		std::string key = itr->string;
		auto find = worldObjects.find(key);
		if ( find != worldObjects.end() )
		{
			find->second.blurb.clear();
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(itr, "blurb");
			if ( blurb ) { jsonVecToVec(blurb, find->second.blurb); }
			find->second.details.clear();
			cJSON* details = cJSON_GetObjectItemCaseSensitive(itr, "details");
			if ( details ) { jsonVecToVec(details, find->second.details); }
			for ( auto& line : find->second.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' ) { line[0] = '\x1E'; }
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' ) { line[c] = '\x1E'; break; }
							else if ( line[c] != ' ' ) { break; }
						}
					}
				}
			}

			find->second.linesToHighlight.clear();
			cJSON* detailsLineHighlights = cJSON_GetObjectItemCaseSensitive(itr, "details_line_highlights");
			if ( detailsLineHighlights && cJSON_IsArray(detailsLineHighlights) )
			{
				for ( cJSON* itr2 = detailsLineHighlights->child; itr2; itr2 = itr2->next )
				{
					cJSON* color = cJSON_GetObjectItemCaseSensitive(itr2, "color");
					if ( color )
					{
						Uint8 r = 0, g = 0, b = 0;
						cJSON* rItem = cJSON_GetObjectItemCaseSensitive(color, "r");
						if ( rItem && cJSON_IsNumber(rItem) ) { r = (Uint8)rItem->valueint; }
						cJSON* gItem = cJSON_GetObjectItemCaseSensitive(color, "g");
						if ( gItem && cJSON_IsNumber(gItem) ) { g = (Uint8)gItem->valueint; }
						cJSON* bItem = cJSON_GetObjectItemCaseSensitive(color, "b");
						if ( bItem && cJSON_IsNumber(bItem) ) { b = (Uint8)bItem->valueint; }
						find->second.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else { find->second.linesToHighlight.push_back(0); }
				}
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "world") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	worldObjects.clear();
	Compendium_t::Events_t::eventWorldIDLookup.clear();
	Compendium_t::Events_t::eventWorldLookup.clear();
	Compendium_t::Events_t::worldIDToString.clear();
	Compendium_t::CompendiumWorld_t::readContentsLang();

	cJSON* entries = cJSON_GetObjectItemCaseSensitive(d, "world");
	if ( entries )
	{
		for ( cJSON* itr = entries->child; itr; itr = itr->next )
		{
			std::string name = itr->string ? itr->string : "";
			cJSON* w = itr;
			auto& obj = worldObjects[name];

			cJSON* eventLookup = cJSON_GetObjectItemCaseSensitive(w, "event_lookup");
			obj.id = eventLookup && cJSON_IsNumber(eventLookup) ? (Sint32)eventLookup->valueint : 0;
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(w, "blurb");
			if ( blurb ) { jsonVecToVec(blurb, obj.blurb); }
			cJSON* details = cJSON_GetObjectItemCaseSensitive(w, "details");
			if ( details ) { jsonVecToVec(details, obj.details); }
			cJSON* img = cJSON_GetObjectItemCaseSensitive(w, "img");
			obj.imagePath = img && img->valuestring ? img->valuestring : "";
			cJSON* models = cJSON_GetObjectItemCaseSensitive(w, "models");
			if ( models ) { jsonVecToVec(models, obj.models); }
			cJSON* lorePoints = cJSON_GetObjectItemCaseSensitive(w, "lore_points");
			if ( lorePoints && cJSON_IsNumber(lorePoints) ) { obj.lorePoints = (Sint32)lorePoints->valueint; }
			cJSON* featureImg = cJSON_GetObjectItemCaseSensitive(w, "feature_img");
			if ( featureImg && featureImg->valuestring ) { obj.featureImg = featureImg->valuestring; }
			obj.linesToHighlight.clear();
			for ( auto& line : obj.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' ) { line[0] = '\x1E'; }
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' ) { line[c] = '\x1E'; break; }
							else if ( line[c] != ' ' ) { break; }
						}
					}
				}
			}
			cJSON* detailsLineHighlights = cJSON_GetObjectItemCaseSensitive(w, "details_line_highlights");
			if ( detailsLineHighlights && cJSON_IsArray(detailsLineHighlights) )
			{
				for ( cJSON* itr_dlh = detailsLineHighlights->child; itr_dlh; itr_dlh = itr_dlh->next )
				{
					cJSON* color = cJSON_GetObjectItemCaseSensitive(itr_dlh, "color");
					if ( color )
					{
						Uint8 r = 0, g = 0, b = 0;
						cJSON* rItem = cJSON_GetObjectItemCaseSensitive(color, "r");
						if ( rItem && cJSON_IsNumber(rItem) ) { r = (Uint8)rItem->valueint; }
						cJSON* gItem = cJSON_GetObjectItemCaseSensitive(color, "g");
						if ( gItem && cJSON_IsNumber(gItem) ) { g = (Uint8)gItem->valueint; }
						cJSON* bItem = cJSON_GetObjectItemCaseSensitive(color, "b");
						if ( bItem && cJSON_IsNumber(bItem) ) { b = (Uint8)bItem->valueint; }
						obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else { obj.linesToHighlight.push_back(0); }
				}
			}

			Compendium_t::Events_t::eventWorldIDLookup[name] = obj.id;
			Compendium_t::Events_t::worldIDToString[obj.id + Compendium_t::Events_t::kEventWorldOffset] = name;
			cJSON* events = cJSON_GetObjectItemCaseSensitive(w, "events");
			if ( events && cJSON_IsArray(events) )
			{
				for ( cJSON* itr_e = events->child; itr_e; itr_e = itr_e->next )
				{
					std::string eventName = itr_e->valuestring ? itr_e->valuestring : "";
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
			cJSON* eventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "events_display");
			if ( eventsDisplay && cJSON_IsArray(eventsDisplay) )
			{
				for ( cJSON* itr_ed = eventsDisplay->child; itr_ed; itr_ed = itr_ed->next )
				{
					std::string eventName = itr_ed->valuestring ? itr_ed->valuestring : "";
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
			cJSON* customEventsDisplay = cJSON_GetObjectItemCaseSensitive(w, "custom_events_display");
			if ( customEventsDisplay && cJSON_IsArray(customEventsDisplay) )
			{
				std::vector<std::string> customEvents;
				for ( cJSON* itr_ce = customEventsDisplay->child; itr_ce; itr_ce = itr_ce->next )
				{
					customEvents.push_back(itr_ce->valuestring ? itr_ce->valuestring : "");
				}

				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() ) { vec2.push_back(customEvents[index]); }
						else { vec2.push_back(""); }
					}
					else { vec2.push_back(""); }
				}
			}
		}
	}
	cJSON_Delete(d);
}

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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	for ( cJSON* itr = d->child; itr; itr = itr->next )
	{
		if ( !itr->string ) continue;
		std::string key = itr->string;
		auto find = monsters.find(key);
		if ( find != monsters.end() )
		{
			find->second.blurb.clear();
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(itr, "blurb");
			if ( blurb ) { jsonVecToVec(blurb, find->second.blurb); }
			find->second.abilities.clear();
			cJSON* abilities = cJSON_GetObjectItemCaseSensitive(itr, "abilities");
			if ( abilities ) { jsonVecToVec(abilities, find->second.abilities); }
			find->second.inventory.clear();
			cJSON* inventory = cJSON_GetObjectItemCaseSensitive(itr, "inventory");
			if ( inventory ) { jsonVecToVec(inventory, find->second.inventory); }
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "monsters") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	monsters.clear();

	Compendium_t::Events_t::monsterUniqueIDLookup.clear();
	Compendium_t::Events_t::eventMonsterLookup.clear();
	Compendium_t::Events_t::monsterIDToString.clear();
	
	Compendium_t::CompendiumMonsters_t::readContentsLang();

	cJSON* uniqueTags = cJSON_GetObjectItemCaseSensitive(d, "unique_tags");
	if ( uniqueTags && cJSON_IsObject(uniqueTags) )
	{
		for ( cJSON* itr = uniqueTags->child; itr; itr = itr->next )
		{
			if ( itr->string && cJSON_IsNumber(itr) )
			{
				Compendium_t::Events_t::monsterUniqueIDLookup[itr->string] = (Sint32)itr->valueint;
			}
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

	cJSON* entries = cJSON_GetObjectItemCaseSensitive(d, "monsters");
	if ( entries )
	{
		for ( cJSON* itr = entries->child; itr; itr = itr->next )
		{
			std::string name = itr->string ? itr->string : "";
			cJSON* typeItem = cJSON_GetObjectItemCaseSensitive(itr, "type");
			if ( typeItem && typeItem->valuestring )
			{
				name = typeItem->valuestring;
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

			cJSON* m = itr;
			auto& monster = monsters[itr->string ? itr->string : ""];
			monster.monsterType = monsterType;
			cJSON* unique_npc = cJSON_GetObjectItemCaseSensitive(m, "unique_npc");
			monster.unique_npc = unique_npc && unique_npc->valuestring ? unique_npc->valuestring : "";
			cJSON* blurb = cJSON_GetObjectItemCaseSensitive(m, "blurb");
			if ( blurb ) { jsonVecToVec(blurb, monster.blurb); }
			cJSON* img = cJSON_GetObjectItemCaseSensitive(m, "img");
			if ( img && img->valuestring ) { monster.imagePath = img->valuestring; }
			cJSON* lorePoints = cJSON_GetObjectItemCaseSensitive(m, "lore_points");
			if ( lorePoints && cJSON_IsNumber(lorePoints) ) { monster.lorePoints = (Sint32)lorePoints->valueint; }
			cJSON* stats = cJSON_GetObjectItemCaseSensitive(m, "stats");
			if ( stats )
			{
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "hp"), monster.hp);
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "ac"), monster.ac);
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "spd"), monster.spd);
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "atk"), monster.atk);
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "rangeatk"), monster.rangeatk);
				jsonVecToVec(cJSON_GetObjectItemCaseSensitive(stats, "pwr"), monster.pwr);
				cJSON* strItem = cJSON_GetObjectItemCaseSensitive(stats, "str");
				if ( strItem ) { jsonVecToVec(strItem, monster.str); }
				cJSON* conItem = cJSON_GetObjectItemCaseSensitive(stats, "con");
				if ( conItem ) { jsonVecToVec(conItem, monster.con); }
				cJSON* dexItem = cJSON_GetObjectItemCaseSensitive(stats, "dex");
				if ( dexItem ) { jsonVecToVec(dexItem, monster.dex); }
				cJSON* lvlItem = cJSON_GetObjectItemCaseSensitive(stats, "lvl");
				if ( lvlItem ) { jsonVecToVec(lvlItem, monster.lvl); }
				cJSON* speciesItem = cJSON_GetObjectItemCaseSensitive(stats, "species");
				if ( speciesItem && speciesItem->valuestring )
				{
					auto species = CompendiumMonsters_t::SPECIES_NONE;
					std::string str = speciesItem->valuestring;
					if ( str == "humanoid" ) { species = CompendiumMonsters_t::SPECIES_HUMANOID; }
					else if ( str == "beast" ) { species = CompendiumMonsters_t::SPECIES_BEAST; }
					else if ( str == "beastfolk" ) { species = CompendiumMonsters_t::SPECIES_BEASTFOLK; }
					else if ( str == "undead" ) { species = CompendiumMonsters_t::SPECIES_UNDEAD; }
					else if ( str == "demonoid" ) { species = CompendiumMonsters_t::SPECIES_DEMONOID; }
					else if ( str == "construct" ) { species = CompendiumMonsters_t::SPECIES_CONSTRUCT; }
					else if ( str == "elemental" ) { species = CompendiumMonsters_t::SPECIES_ELEMENTAL; }
					monster.species = species;
				}
			}
			cJSON* abilities = cJSON_GetObjectItemCaseSensitive(m, "abilities");
			if ( abilities ) { jsonVecToVec(abilities, monster.abilities); }
			{
				int i = 0;
				cJSON* resistances = cJSON_GetObjectItemCaseSensitive(m, "resistances");
				if ( resistances && cJSON_IsArray(resistances) )
				{
					for ( cJSON* itr_r = resistances->child; itr_r && i < 4; itr_r = itr_r->next )
					{
						if ( cJSON_IsNumber(itr_r) )
						{
							monster.resistances[i] = (Sint32)itr_r->valueint;
							++i;
						}
					}
				}
			}
			cJSON* inventory = cJSON_GetObjectItemCaseSensitive(m, "inventory");
			if ( inventory ) { jsonVecToVec(inventory, monster.inventory); }
			cJSON* models = cJSON_GetObjectItemCaseSensitive(m, "models");
			if ( models ) { jsonVecToVec(models, monster.models); }
		}
	}
	cJSON_Delete(d);
}

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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	eventLangEntries.clear();
	eventCustomLangEntries.clear();
	cJSON* tags = cJSON_GetObjectItemCaseSensitive(d, "tags");
	if ( cJSON_IsArray(tags) )
	{
		for ( cJSON* itr = tags->child; itr; itr = itr->next )
		{
			for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
			{
				if ( !itr2->string ) continue;
				auto find = eventIdLookup.find(itr2->string);
				if ( find != eventIdLookup.end())
				{
					EventTags tag = eventIdLookup[find->first];
					auto& entry = eventLangEntries[tag];
					for ( cJSON* itr3 = itr2->child; itr3; itr3 = itr3->next )
					{
						if ( itr3->string && itr3->valuestring )
						{
							entry[itr3->string] = itr3->valuestring;
						}
					}
				}
			}
		}
	}
	cJSON* customTags = cJSON_GetObjectItemCaseSensitive(d, "custom_tags");
	if ( customTags && cJSON_IsObject(customTags) )
	{
		for ( cJSON* itr = customTags->child; itr; itr = itr->next )
		{
			if ( !itr->string ) continue;
			for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
			{
				if ( itr2->string && itr2->valuestring )
				{
					eventCustomLangEntries[itr->string][itr2->string] = itr2->valuestring;
				}
			}
		}
	}
	cJSON_Delete(d);
}

std::vector<std::pair<std::string, Sint32>> Compendium_t::Events_t::getCustomEventValue(std::string key, 
	std::string compendiumSection, std::string compendiumContentsSelected, int specificClass)
{
	std::vector<std::pair<std::string, Sint32>> results;
	if ( customEventsValues.find(key) == customEventsValues.end() )
	{
		return results;
	}

	cJSON* d = cJSON_Parse(customEventsValues[key].c_str());
	if ( !d || !cJSON_IsObject(d) )
	{
		if (d) cJSON_Delete(d);
		return results;
	}

	cJSON* value = cJSON_GetObjectItemCaseSensitive(d, "value");
	if ( value )
	{
		Events_t::Type type = MAX;
		int minValue = INT_MAX;
		int maxValue = 0;
		bool firstResult = true;
		std::string valueType = "";
		cJSON* valueTypeItem = cJSON_GetObjectItemCaseSensitive(value, "type");
		if ( valueTypeItem && valueTypeItem->valuestring )
		{
			valueType = valueTypeItem->valuestring;
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
		cJSON* format = cJSON_GetObjectItemCaseSensitive(value, "format");
		if ( format && format->valuestring )
		{
			formatType = format->valuestring;
		}
		cJSON* compendiumSectionItem = cJSON_GetObjectItemCaseSensitive(value, "compendium_section");
		if ( compendiumSectionItem && compendiumSectionItem->valuestring )
		{
			compendiumSection = compendiumSectionItem->valuestring;
		}
		cJSON* tags = cJSON_GetObjectItemCaseSensitive(value, "tags");
		if ( tags && cJSON_IsArray(tags) )
		{
			for ( cJSON* itr = tags->child; itr; itr = itr->next )
			{
				std::string name = "";
				std::string cat = "";
				cJSON* nameItem = cJSON_GetObjectItemCaseSensitive(itr, "name");
				if ( nameItem && nameItem->valuestring ) { name = nameItem->valuestring; }
				cJSON* catItem = cJSON_GetObjectItemCaseSensitive(itr, "category");
				if ( catItem && catItem->valuestring ) { cat = catItem->valuestring; }

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
									output = formatEventRecordText(val, format ? format->valuestring : "", classnum, eventCustomLangEntries[key]);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	events.clear();
	eventIdLookup.clear();
	eventClassIds.clear();
	int classIdIndex = kEventCodexClassOffset;
	int index = -1;
	cJSON* tags = cJSON_GetObjectItemCaseSensitive(d, "tags");
	if ( cJSON_IsArray(tags) )
	{
		for ( cJSON* itr = tags->child; itr; itr = itr->next )
		{
			++index;
			for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
			{
				const EventTags id = (EventTags)std::min(index, (int)CPDM_EVENT_TAGS_MAX);
				auto& entry = events[id];
				entry.id = id;
				entry.name = itr2->string ? itr2->string : "";
				eventIdLookup[entry.name] = id;

				cJSON* typeItem = cJSON_GetObjectItemCaseSensitive(itr2, "type");
				if ( typeItem && typeItem->valuestring )
				{
					std::string type = typeItem->valuestring;
					if ( type == "sum" ) { entry.type = SUM; }
					else if ( type == "max" ) { entry.type = MAX; }
					else if ( type == "bit" ) { entry.type = BITFIELD; }
					else if ( type == "min" ) { entry.type = MIN; }
				}
				entry.id = index;
				cJSON* client = cJSON_GetObjectItemCaseSensitive(itr2, "client");
				if ( client && cJSON_IsNumber(client) )
				{
					int tmp = (int)client->valueint;
					tmp = std::max(0, tmp);
					tmp = std::min((int)Compendium_t::Events_t::CLIENT_UPDATETYPE_MAX - 1, tmp);
					entry.clienttype = (Compendium_t::Events_t::ClientUpdateType)tmp;
				}
				cJSON* oncePerRun = cJSON_GetObjectItemCaseSensitive(itr2, "once_per_run");
				if ( oncePerRun && cJSON_IsBool(oncePerRun) )
				{
					entry.eventTrackingType = cJSON_IsTrue(oncePerRun) ? EventTrackingType::ONCE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
				}
				cJSON* uniquePerRun = cJSON_GetObjectItemCaseSensitive(itr2, "unique_per_run");
				if ( uniquePerRun && cJSON_IsBool(uniquePerRun) )
				{
					entry.eventTrackingType = cJSON_IsTrue(uniquePerRun) ? EventTrackingType::UNIQUE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
				}
				cJSON* uniquePerFloor = cJSON_GetObjectItemCaseSensitive(itr2, "unique_per_floor");
				if ( uniquePerFloor && cJSON_IsBool(uniquePerFloor) )
				{
					entry.eventTrackingType = cJSON_IsTrue(uniquePerFloor) ? EventTrackingType::UNIQUE_PER_FLOOR : EventTrackingType::ALWAYS_UPDATE;
				}
				cJSON* attributes = cJSON_GetObjectItemCaseSensitive(itr2, "attributes");
				if ( attributes )
				{
					if ( cJSON_IsArray(attributes) )
					{
						for ( cJSON* arr_itr = attributes->child; arr_itr; arr_itr = arr_itr->next )
						{
							if ( cJSON_IsString(arr_itr) && arr_itr->valuestring )
							{
								entry.attributes.insert(arr_itr->valuestring);
							}
						}
					}
					else if ( cJSON_IsString(attributes) && attributes->valuestring )
					{
						entry.attributes.insert(attributes->valuestring);
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
								int index2 = i + skillnum * kEventClassesMax;
								eventClassIds[id][index2] = (classIdIndex + index2);
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
	}

	cJSON* customTags = cJSON_GetObjectItemCaseSensitive(d, "custom_tags");
	if ( customTags && cJSON_IsObject(customTags) )
	{
		for ( cJSON* itr = customTags->child; itr; itr = itr->next )
		{
			if ( !itr->string ) continue;
			eventIdLookup[itr->string] = EventTags::CPDM_CUSTOM_TAG;

			if ( cJSON_IsString(itr) )
			{
				customEventsValues[itr->string] = itr->valuestring ? itr->valuestring : "";
			}
			else if ( cJSON_IsNumber(itr) )
			{
				char buf2[64];
				snprintf(buf2, sizeof(buf2), "%d", (int)itr->valueint);
				customEventsValues[itr->string] = buf2;
			}
			else if ( itr->child )
			{
				char* s = cJSON_Print(itr->child);
				customEventsValues[itr->string] = s ? s : "";
				if (s) free(s);
			}
		}
	}
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	int version = 0;
	cJSON* versionItem = cJSON_GetObjectItemCaseSensitive(d, "version");
	if ( versionItem && cJSON_IsNumber(versionItem) )
	{
		version = (int)versionItem->valueint;
	}

	playerEvents.clear();
	cJSON* items = cJSON_GetObjectItemCaseSensitive(d, "items");
	if ( items && cJSON_IsObject(items) )
	{
		for ( cJSON* itr = items->child; itr; itr = itr->next )
		{
			if ( !itr->string ) continue;
			auto find = eventIdLookup.find(itr->string);
			if ( find == eventIdLookup.end() )
			{
				continue;
			}
			const EventTags id = (EventTags)std::min((int)find->second, (int)CPDM_EVENT_TAGS_MAX);
			for ( cJSON* itr2 = itr->child; itr2; itr2 = itr2->next )
			{
				if ( !itr2->string || !cJSON_IsNumber(itr2) ) continue;
				int itemType = std::stoi(itr2->string);
				Sint32 value = (Sint32)itr2->valueint;
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
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		if (d) cJSON_Delete(d);
		return;
	}

	cJSON* items = cJSON_GetObjectItemCaseSensitive(d, "items");
	if ( items && cJSON_IsObject(items) )
	{
		for ( cJSON* itr = items->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumItems_t::unlocks[itr->string] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	cJSON* itemsStatus = cJSON_GetObjectItemCaseSensitive(d, "items_status");
	if ( itemsStatus && cJSON_IsObject(itemsStatus) )
	{
		for ( cJSON* itr = itemsStatus->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			int id = std::stoi(itr->string);
			CompendiumItems_t::itemUnlocks[id] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	cJSON* achievements = cJSON_GetObjectItemCaseSensitive(d, "achievements");
	if ( achievements && cJSON_IsObject(achievements) )
	{
		for ( cJSON* itr = achievements->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			AchievementData_t::unlocks[itr->string] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	cJSON* world = cJSON_GetObjectItemCaseSensitive(d, "world");
	if ( world && cJSON_IsObject(world) )
	{
		for ( cJSON* itr = world->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumWorld_t::unlocks[itr->string] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	cJSON* codex = cJSON_GetObjectItemCaseSensitive(d, "codex");
	if ( codex && cJSON_IsObject(codex) )
	{
		for ( cJSON* itr = codex->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumCodex_t::unlocks[itr->string] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	cJSON* monsters = cJSON_GetObjectItemCaseSensitive(d, "monsters");
	if ( monsters && cJSON_IsObject(monsters) )
	{
		for ( cJSON* itr = monsters->child; itr; itr = itr->next )
		{
			if ( !itr->string || !cJSON_IsNumber(itr) ) continue;
			int val = (int)itr->valueint;
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumMonsters_t::unlocks[itr->string] = static_cast<CompendiumUnlockStatus>(val);
		}
	}
	cJSON_Delete(d);
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
			FileIO::close(fp);

			cJSON* d = cJSON_Parse(buf);
			if ( !d || !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "limbs") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				if (d) cJSON_Delete(d);
				return;
			}
			cJSON* versionItem = cJSON_GetObjectItemCaseSensitive(d, "version");
			int version = versionItem && cJSON_IsNumber(versionItem) ? (int)versionItem->valueint : 0;

			std::string filename = f.substr(0, f.find(".json"));
			auto& entry = compendiumObjectLimbs[filename];
			entry.entities.clear();
			entry.baseCamera = CompendiumView_t();

			compendiumObjectMapTiles.erase(filename);
			int w = 0;
			int h = 0;
			int index = 0;
			cJSON* mapTiles = cJSON_GetObjectItemCaseSensitive(d, "map_tiles");
			if ( mapTiles )
			{
				auto& m = compendiumObjectMapTiles[filename];
				cJSON* floor = cJSON_GetObjectItemCaseSensitive(mapTiles, "floor");
				cJSON* mid = cJSON_GetObjectItemCaseSensitive(mapTiles, "mid");
				cJSON* top = cJSON_GetObjectItemCaseSensitive(mapTiles, "top");
				cJSON* widthItem = cJSON_GetObjectItemCaseSensitive(mapTiles, "width");
				cJSON* heightItem = cJSON_GetObjectItemCaseSensitive(mapTiles, "height");
				if ( floor && mid && top && widthItem && heightItem
					&& cJSON_IsArray(floor) && cJSON_IsArray(mid) && cJSON_IsArray(top)
					&& cJSON_IsNumber(widthItem) && cJSON_IsNumber(heightItem) )
				{
					cJSON* floorArr = floor;
					cJSON* midArr = mid;
					cJSON* topArr = top;
					w = (int)widthItem->valueint;
					h = (int)heightItem->valueint;
					int floorSize = cJSON_GetArraySize(floorArr);
					if ( floorSize == cJSON_GetArraySize(midArr) &&
						floorSize == cJSON_GetArraySize(topArr) && floorSize == (w * h) )
					{
						m.first.width = w;
						m.first.height = h;
						cJSON* ceiling = cJSON_GetObjectItemCaseSensitive(mapTiles, "ceiling");
						if ( ceiling && cJSON_IsNumber(ceiling) )
						{
							m.first.ceiling = (Sint32)ceiling->valueint;
						}
						auto& tiles = m.second;
						tiles.resize(w * h * MAPLAYERS);
						cJSON* arrs[3] = { floorArr, midArr, topArr };
						for ( int z = 0; z < MAPLAYERS; ++z )
						{
							int x = 0;
							int y = 0;
							cJSON* arr = arrs[z];
							for ( cJSON* tile = arr->child; tile; tile = tile->next )
							{
								int idx = z + (y * MAPLAYERS) + (x * MAPLAYERS * h);
								if ( cJSON_IsNumber(tile) )
									tiles[idx] = (Sint32)tile->valueint;

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
			cJSON* camera = cJSON_GetObjectItemCaseSensitive(d, "camera");
			if ( camera )
			{
				entry.baseCamera.inUse = true;
				cJSON* angDegrees = cJSON_GetObjectItemCaseSensitive(camera, "ang_degrees");
				if ( angDegrees && cJSON_IsNumber(angDegrees) )
					entry.baseCamera.ang = PI * (double)angDegrees->valueint / 180.0;
				cJSON* vangDegrees = cJSON_GetObjectItemCaseSensitive(camera, "vang_degrees");
				if ( vangDegrees && cJSON_IsNumber(vangDegrees) )
					entry.baseCamera.vang = PI * (double)vangDegrees->valueint / 180.0;
				entry.baseCamera.rotateLimit = false;
				cJSON* rotMin = cJSON_GetObjectItemCaseSensitive(camera, "rotate_limit_degrees_min");
				if ( rotMin && cJSON_IsNumber(rotMin) )
				{
					entry.baseCamera.rotateLimitMin = PI * (double)rotMin->valueint / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				cJSON* rotMax = cJSON_GetObjectItemCaseSensitive(camera, "rotate_limit_degrees_max");
				if ( rotMax && cJSON_IsNumber(rotMax) )
				{
					entry.baseCamera.rotateLimitMax = PI * (double)rotMax->valueint / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				cJSON* rotDeg = cJSON_GetObjectItemCaseSensitive(camera, "rotate_degrees");
				if ( rotDeg && cJSON_IsNumber(rotDeg) )
					entry.baseCamera.rotate = PI * (double)rotDeg->valueint / 180.0;
				else
					entry.baseCamera.rotate = entry.baseCamera.rotateLimitMax - (entry.baseCamera.rotateLimitMax - entry.baseCamera.rotateLimitMin) / 2;
				cJSON* rotSpeed = cJSON_GetObjectItemCaseSensitive(camera, "rotate_speed");
				if ( rotSpeed && cJSON_IsNumber(rotSpeed) )
					entry.baseCamera.rotateSpeed = rotSpeed->valuedouble;
				cJSON* zoom = cJSON_GetObjectItemCaseSensitive(camera, "zoom");
				if ( zoom && cJSON_IsNumber(zoom) )
					entry.baseCamera.zoom = zoom->valuedouble;
				cJSON* height = cJSON_GetObjectItemCaseSensitive(camera, "height");
				if ( height && cJSON_IsNumber(height) )
					entry.baseCamera.height = height->valuedouble;
				cJSON* pan = cJSON_GetObjectItemCaseSensitive(camera, "pan");
				if ( pan && cJSON_IsNumber(pan) )
					entry.baseCamera.pan = pan->valuedouble;
			}
			cJSON* limbs = cJSON_GetObjectItemCaseSensitive(d, "limbs");
			if ( limbs && cJSON_IsArray(limbs) )
			{
				for ( cJSON* itr = limbs->child; itr; itr = itr->next )
				{
					entry.entities.push_back(Entity(-1, 0, nullptr, nullptr));
					auto& limb = entry.entities[entry.entities.size() - 1];
					cJSON* xItem = cJSON_GetObjectItemCaseSensitive(itr, "x");
					cJSON* yItem = cJSON_GetObjectItemCaseSensitive(itr, "y");
					if ( index > 0 )
					{
						limb.x = entry.entities[0].x - (xItem && cJSON_IsNumber(xItem) ? xItem->valuedouble : 0.0);
						limb.y = entry.entities[0].y - (yItem && cJSON_IsNumber(yItem) ? yItem->valuedouble : 0.0);
					}
					else
					{
						limb.x = xItem && cJSON_IsNumber(xItem) ? xItem->valuedouble : 0.0;
						limb.y = yItem && cJSON_IsNumber(yItem) ? yItem->valuedouble : 0.0;
						limb.x += 8.0;
						limb.y += 8.0;
						if ( w > 0 && h > 0 )
						{
							if ( w % 2 == 1 )
								limb.x += 16.0 * (w / 2);
							else
							{
								limb.x += 16.0 * ((w - 1) / 2);
								limb.x += 8.0;
							}
							if ( h % 2 == 1 )
								limb.y += 16.0 * (h / 2);
							else
							{
								limb.y += 16.0 * ((h - 1) / 2);
								limb.y += 8.0;
							}
						}
					}
					auto getDouble = [&](const char* key) -> double {
						cJSON* item = cJSON_GetObjectItemCaseSensitive(itr, key);
						return item && cJSON_IsNumber(item) ? item->valuedouble : 0.0;
					};
					auto getInt = [&](const char* key) -> int {
						cJSON* item = cJSON_GetObjectItemCaseSensitive(itr, key);
						return item && cJSON_IsNumber(item) ? (int)item->valueint : 0;
					};
					limb.z = getDouble("z");
					limb.focalx = getDouble("focalx");
					limb.focaly = getDouble("focaly");
					limb.focalz = getDouble("focalz");
					limb.pitch = getDouble("pitch");
					limb.roll = getDouble("roll");
					limb.yaw = getDouble("yaw");
					cJSON* yawDeg = cJSON_GetObjectItemCaseSensitive(itr, "yaw_degrees");
					if ( yawDeg && cJSON_IsNumber(yawDeg) )
						limb.yaw += PI * (double)yawDeg->valueint / 180.0;
					cJSON* scalex = cJSON_GetObjectItemCaseSensitive(itr, "scalex");
					if ( scalex && cJSON_IsNumber(scalex) ) limb.scalex = scalex->valuedouble;
					cJSON* scaley = cJSON_GetObjectItemCaseSensitive(itr, "scaley");
					if ( scaley && cJSON_IsNumber(scaley) ) limb.scaley = scaley->valuedouble;
					cJSON* scalez = cJSON_GetObjectItemCaseSensitive(itr, "scalez");
					if ( scalez && cJSON_IsNumber(scalez) ) limb.scalez = scalez->valuedouble;
					limb.sprite = getInt("sprite");
					++index;
				}
			}
			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			cJSON_Delete(d);
		}
	}
}