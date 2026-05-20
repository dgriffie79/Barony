/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_tooltips.cpp - Item and spell tooltip formatting
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

Uint32 ItemTooltips_t::itemsJsonHashRead = 0;
const Uint32 ItemTooltips_t::kItemsJsonHash = 2516917045;

void ItemTooltips_t::setSpellValueIfKeyPresent(ItemTooltips_t::spellItem_t& t, rapidjson::Value::ConstMemberIterator item_itr, Uint32& hash, Uint32& hashShift, const char* key, int& toSet)
{
	t.hasExpandedJSON = true;
}
void ItemTooltips_t::setSpellValueIfKeyPresent(ItemTooltips_t::spellItem_t& t, rapidjson::Value::ConstMemberIterator item_itr, Uint32& hash, Uint32& hashShift, const char* key, real_t& toSet)
{
	t.hasExpandedJSON = true;
}

#ifdef EDITOR
void lowercaseString(std::string& str)
{
	if ( str.size() < 1 ) { return; }
	for ( auto& letter : str )
	{
		if ( letter >= 'A' && letter <= 'Z' )
		{
			letter = tolower(letter);
		}
	}
}
#endif

void hashSpellProp(Uint32& hash, Uint32& hashShift, int& toSet)
{
	hash += (Uint32)((Uint32)abs(toSet) << (hashShift % 32)); ++hashShift;
}

void hashSpellProp(Uint32& hash, Uint32& hashShift, real_t& toSet)
{
	hash += (Uint32)(static_cast<Uint32>(abs(toSet) * 100000) << (hashShift % 32)); ++hashShift;
}

void ItemTooltips_t::readItemsFromFile()
{
	printlog("loading items...\n");
	if ( !PHYSFS_getRealDir("items/items.json") )
	{
		printlog("[JSON]: Error: Could not find file: items/items.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/items/items.json");
	inputPath.append("/items/items.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	const int bufSize = 600000;
	static char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);
	if ( !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	int itemsRead = 0;

	tmpItems.clear();

	for ( rapidjson::Value::ConstMemberIterator item_itr = d["items"].MemberBegin(); 
		item_itr != d["items"].MemberEnd(); ++item_itr )
	{
		tmpItem_t t;
		t.internalName = item_itr->name.GetString();
		t.itemId = item_itr->value["item_id"].GetInt();
		t.fpIndex = item_itr->value["first_person_model_index"].GetInt();
		t.tpIndex = item_itr->value["third_person_model_index"].GetInt();
		if ( item_itr->value.HasMember("third_person_model_index_short") )
		{
			t.tpShortIndex = item_itr->value["third_person_model_index_short"].GetInt();
		}
		t.gold = item_itr->value["gold_value"].GetInt();
		t.weight = item_itr->value["weight_value"].GetInt();
		t.itemLevel = item_itr->value["item_level"].GetInt();
		t.category = item_itr->value["item_category"].GetString();
		t.equipSlot = item_itr->value["equip_slot"].GetString();

		for ( rapidjson::Value::ConstValueIterator pathArray_itr = item_itr->value["item_images"].Begin();
			pathArray_itr != item_itr->value["item_images"].End(); 
			++pathArray_itr )
		{
			t.imagePaths.push_back(pathArray_itr->GetString());
		}

		if ( item_itr->value.HasMember("stats") )
		{
			for ( rapidjson::Value::ConstMemberIterator stat_itr = item_itr->value["stats"].MemberBegin(); 
				stat_itr != item_itr->value["stats"].MemberEnd(); ++stat_itr )
			{
				t.attributes[stat_itr->name.GetString()] = stat_itr->value.GetInt();
			}
		}

		if ( item_itr->value.HasMember("tooltip") )
		{
			if ( item_itr->value["tooltip"].HasMember("type") )
			{
				t.tooltip = item_itr->value["tooltip"]["type"].GetString();
			}
		}

		if ( item_itr->value.HasMember("icon_label_path") )
		{
			t.iconLabelPath = item_itr->value["icon_label_path"].GetString();
		}

		tmpItems.push_back(t);
		++itemsRead;
	}

	printlog("[JSON]: Successfully read %d items from '%s'", itemsRead, inputPath.c_str());

	//itemValueTable.clear();
	//itemValueTableByCategory.clear();
	Uint32 shift = 0;
	Uint32 hash = 0;
	for ( int i = 0; i < NUMITEMS && i < itemsRead; ++i )
	{
		assert(i == tmpItems[i].itemId);
		items[i].level = tmpItems[i].itemLevel;
		items[i].gold_value = tmpItems[i].gold;
		items[i].weight = tmpItems[i].weight;
		items[i].fpindex = tmpItems[i].fpIndex;
		items[i].index = tmpItems[i].tpIndex;
		items[i].indexShort = tmpItems[i].tpShortIndex;
		items[i].tooltip = tmpItems[i].tooltip;
		items[i].attributes.clear();
		items[i].attributes = tmpItems[i].attributes;
		if ( i == SPELL_ITEM )
		{
			items[i].variations = 1;
		}
		else
		{
			items[i].variations = tmpItems[i].imagePaths.size();
		}
		list_FreeAll(&items[i].images);
		items[i].images.first = NULL;
		items[i].images.last = NULL;
		for ( int j = 0; j < tmpItems[i].imagePaths.size(); ++j )
		{
			//auto s = static_cast<string_t*>(list_Node(&items[i].images, j)->element);
			//assert(!strcmp(s->data, tmpItems[i].imagePaths[j].c_str()));

			string_t* string = (string_t*)malloc(sizeof(string_t));
			const size_t len = 64;
			string->data = (char*)malloc(sizeof(char) * len);
			memset(string->data, 0, sizeof(char) * len);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[i].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			stringCopy(string->data, tmpItems[i].imagePaths[j].c_str(), len - 1, tmpItems[i].imagePaths[j].size());
		}
		if ( tmpItems[i].category.compare("WEAPON") == 0 )
		{
			items[i].category = WEAPON;
		}
		else if ( tmpItems[i].category.compare("ARMOR") == 0 )
		{
			items[i].category = ARMOR;
		}
		else if ( tmpItems[i].category.compare("AMULET") == 0 )
		{
			items[i].category = AMULET;
		}
		else if ( tmpItems[i].category.compare("POTION") == 0 )
		{
			items[i].category = POTION;
		}
		else if ( tmpItems[i].category.compare("SCROLL") == 0 )
		{
			items[i].category = SCROLL;
		}
		else if ( tmpItems[i].category.compare("MAGICSTAFF") == 0 )
		{
			items[i].category = MAGICSTAFF;
		}
		else if ( tmpItems[i].category.compare("RING") == 0 )
		{
			items[i].category = RING;
		}
		else if ( tmpItems[i].category.compare("SPELLBOOK") == 0 )
		{
			items[i].category = SPELLBOOK;
		}
		else if ( tmpItems[i].category.compare("TOOL") == 0 )
		{
			items[i].category = TOOL;
		}
		else if ( tmpItems[i].category.compare("FOOD") == 0 )
		{
			items[i].category = FOOD;
		}
		else if ( tmpItems[i].category.compare("BOOK") == 0 )
		{
			items[i].category = BOOK;
		}
		else if ( tmpItems[i].category.compare("THROWN") == 0 )
		{
			items[i].category = THROWN;
		}
		else if ( tmpItems[i].category.compare("SPELL_CAT") == 0 )
		{
			items[i].category = SPELL_CAT;
		}
		else if ( tmpItems[i].category.compare("TOME_SPELL") == 0 )
		{
			items[i].category = TOME_SPELL;
		}
		else
		{
			items[i].category = GEM;
		}

		items[i].item_slot = ItemEquippableSlot::NO_EQUIP;
		if ( tmpItems[i].equipSlot.compare("mainhand") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON;
		}
		else if ( tmpItems[i].equipSlot.compare("offhand") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD;
		}
		else if ( tmpItems[i].equipSlot.compare("gloves") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES;
		}
		else if ( tmpItems[i].equipSlot.compare("cloak") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK;
		}
		else if ( tmpItems[i].equipSlot.compare("boots") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS;
		}
		else if ( tmpItems[i].equipSlot.compare("torso") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE;
		}
		else if ( tmpItems[i].equipSlot.compare("amulet") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET;
		}
		else if ( tmpItems[i].equipSlot.compare("ring") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING;
		}
		else if ( tmpItems[i].equipSlot.compare("mask") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK;
		}
		else if ( tmpItems[i].equipSlot.compare("helm") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM;
		}

		hash += (Uint32)((Uint32)items[i].weight << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)items[i].gold_value << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)items[i].level << (shift % 32)); ++shift;
		/*{
			auto pair = std::make_pair(items[i].value, i);
			auto lower = std::lower_bound(itemValueTable.begin(), itemValueTable.end(), pair,
				[](const auto& lhs, const auto& rhs) {
					return lhs < rhs;
			});
			itemValueTable.insert(lower, pair);
		}
		{
			auto pair = std::make_pair(items[i].value, i);
			auto lower = std::lower_bound(itemValueTableByCategory[items[i].category].begin(), 
				itemValueTableByCategory[items[i].category].end(), pair,
				[](const auto& lhs, const auto& rhs) {
					return lhs < rhs;
				});
			itemValueTableByCategory[items[i].category].insert(lower, pair);
		}*/
	}

	spellItems.clear();

	int spellsRead = 0;
	for ( rapidjson::Value::ConstMemberIterator spell_itr = d["spells"].MemberBegin();
		spell_itr != d["spells"].MemberEnd(); ++spell_itr )
	{
		spellItem_t t;
		t.internalName = spell_itr->name.GetString();
		//hash += djb2Hash(const_cast<char*>(t.internalName.c_str()));
		t.name = spell_itr->value["spell_name"].GetString();
		t.name_lowercase = t.name;
		lowercaseString(t.name_lowercase);
		t.id = spell_itr->value["spell_id"].GetInt();
		//hash += (Uint32)((Uint32)t.id << (shift % 32)); ++shift;
		t.spellTypeStr = spell_itr->value["spell_type"].GetString();
		t.spellType = SPELL_TYPE_DEFAULT;
		if ( t.spellTypeStr == "PROJECTILE" )
		{
			t.spellType = SPELL_TYPE_PROJECTILE;
		}
		else if ( t.spellTypeStr == "AREA" )
		{
			t.spellType = SPELL_TYPE_AREA;
		}
		else if ( t.spellTypeStr == "SELF" )
		{
			t.spellType = SPELL_TYPE_SELF;
		}
		else if ( t.spellTypeStr == "SELF_SUSTAIN" )
		{
			t.spellType = SPELL_TYPE_SELF_SUSTAIN;
		}
		else if ( t.spellTypeStr == "PROJECTILE_SHORT_X3" )
		{
			t.spellType = SPELL_TYPE_PROJECTILE_SHORT_X3;
		}
		else if ( t.spellTypeStr == "TOUCH_FLOOR" )
		{
			t.spellType = SPELL_TYPE_TOUCH_FLOOR;
		}
		else if ( t.spellTypeStr == "TOUCH_WALL" )
		{
			t.spellType = SPELL_TYPE_TOUCH_WALL;
		}
		else if ( t.spellTypeStr == "TOUCH_ENEMY" )
		{
			t.spellType = SPELL_TYPE_TOUCH_ENEMY;
		}
		else if ( t.spellTypeStr == "TOUCH_ALLY" )
		{
			t.spellType = SPELL_TYPE_TOUCH_ALLY;
		}
		else if ( t.spellTypeStr == "TOUCH_ENTITY" )
		{
			t.spellType = SPELL_TYPE_TOUCH_ENTITY;
		}
		else if ( t.spellTypeStr == "DIVINE_TARGET" )
		{
			t.spellType = SPELL_TYPE_DIVINE_TARGET;
		}

		//hash += djb2Hash(const_cast<char*>(t.spellTypeStr.c_str()));

		if ( spell_itr->value.HasMember("format_tags") )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = spell_itr->value["format_tags"].Begin();
				arr_itr != spell_itr->value["format_tags"].End(); ++arr_itr )
			{
				t.spellFormatTags.push_back(arr_itr->GetString());
			}
		}

		if ( spell_itr->value.HasMember("spellbook_item_icon_padding") )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = spell_itr->value["spellbook_item_icon_padding"].Begin();
				arr_itr != spell_itr->value["spellbook_item_icon_padding"].End(); ++arr_itr )
			{
				t.spellbookItemIconPaddingLines.push_back(arr_itr->GetInt());
			}
		}

		for ( rapidjson::Value::ConstValueIterator arr_itr = spell_itr->value["effect_tags"].Begin();
			arr_itr != spell_itr->value["effect_tags"].End(); ++arr_itr )
		{
			t.spellTagsStr.push_back(arr_itr->GetString());
			//hash += djb2Hash(const_cast<char*>(t.spellTagsStr.back().c_str()));
			if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "DAMAGE" )
			{
				t.spellTags.insert(SPELL_TAG_DAMAGE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "STATUS" )
			{
				t.spellTags.insert(SPELL_TAG_STATUS_EFFECT);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "UTILITY" )
			{
				t.spellTags.insert(SPELL_TAG_UTILITY);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "HEALING" )
			{
				t.spellTags.insert(SPELL_TAG_HEALING);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "CURE" )
			{
				t.spellTags.insert(SPELL_TAG_CURE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "BUFF" )
			{
				t.spellTags.insert(SPELL_TAG_BUFF);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "BASIC_HIT_MESSAGE" )
			{
				t.spellTags.insert(SPELL_TAG_BASIC_HIT_MESSAGE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "TRACK_SPELL_HITS" )
			{
				t.spellTags.insert(SPELL_TAG_TRACK_HITS);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_SCALE_SPELLBOOK" )
			{
				t.spellTags.insert(SPELL_TAG_SPELLBOOK_SCALING);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "BONUS_AS_EFFECT_POWER" )
			{
				t.spellTags.insert(SPELL_TAG_BONUS_AS_EFFECT_POWER);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_EVENT" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_DEFAULT);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_DMG" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_DMG);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_EFFECT" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_EFFECT);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_SUMMON" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_SUMMON);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_SHAPESHIFT" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_SHAPESHIFT);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_SUSTAIN" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_SUSTAIN);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "SPELL_LEVEL_ASSIST" )
			{
				t.spellLevelTags.insert(spell_t::SPELL_LEVEL_EVENT_ASSIST);
			}
		}

		t.spellbookInternalName = spell_itr->value["spellbook_internal_name"].GetString();
		t.magicstaffInternalName = spell_itr->value["magicstaff_internal_name"].GetString();
		t.fociInternalName = "";
		if ( spell_itr->value.HasMember("foci_internal_name") )
		{
			t.fociInternalName = spell_itr->value["foci_internal_name"].GetString();
		}
		for ( int i = 0; i < NUMITEMS; ++i )
		{
			if ( items[i].category != SPELLBOOK && items[i].category != MAGICSTAFF && !itemTypeIsFoci((ItemType)i) )
			{
				continue;
			}
			if ( t.spellbookInternalName == tmpItems[i].internalName )
			{
				t.spellbookId = i;
				if ( !items[i].hasAttribute("spellbook_spell") )
				{
					items[i].attributes["spellbook_spell"] = t.id;
				}
			}
			if ( t.magicstaffInternalName == tmpItems[i].internalName )
			{
				t.magicstaffId = i;
				items[i].attributes["magicstaff_spell"] = t.id;
			}
			if ( t.fociInternalName == tmpItems[i].internalName )
			{
				t.fociId = i;
				items[i].attributes["foci_spell"] = t.id;
			}
		}

		//hash += djb2Hash(const_cast<char*>(t.spellbookInternalName.c_str()));
		//hash += djb2Hash(const_cast<char*>(t.magicstaffInternalName.c_str()));
		//hash += djb2Hash(const_cast<char*>(t.fociInternalName.c_str()));
		
		t.hasExpandedJSON = false;

		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "mana", t.mana);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "duration", t.duration);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "duration_mult", t.duration_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "duration2", t.duration2);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "duration2_mult", t.duration2_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "damage", t.damage);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "damage_mult", t.damage_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "damage2", t.damage2);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "damage2_mult", t.damage2_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "distance", t.distance);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "distance_mult", t.distance_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "life_time", t.life_time);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "life_mult", t.life_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "cast_time", t.cast_time);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "cast_time_mult", t.cast_time_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "radius", t.radius);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "radius_mult", t.radius_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "difficulty", t.difficulty);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "sustain_mana", t.sustain_mana);
		if ( t.sustain_mana > 0 )
		{
			t.sustain_duration = std::max(1, t.duration); // set a sane default
		}
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "sustain_duration", t.sustain_duration);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "sustain_mult", t.sustain_mult);
		setSpellValueIfKeyPresent(t, spell_itr, hash, shift, "drop_table", t.drop_table);

		if ( spell_itr->value.HasMember("school") )
		{
			std::string school = spell_itr->value["school"].GetString();
			if ( school == "sorcery" )
			{
				t.skillID = PRO_SORCERY;
				//hash += (Uint32)((Uint32)t.skillID << (shift % 32)); ++shift;
			}
			else if ( school == "mysticism" )
			{
				t.skillID = PRO_MYSTICISM;
				//hash += (Uint32)((Uint32)t.skillID << (shift % 32)); ++shift;
			}
			else if ( school == "thaumaturgy" )
			{
				t.skillID = PRO_THAUMATURGY;
				//hash += (Uint32)((Uint32)t.skillID << (shift % 32)); ++shift;
			}
			else
			{
				assert(false && "invalid school from items.json!");
				//hash += (Uint32)((Uint32)1 << (shift % 32)); ++shift;
			}
		}

		spellNameStringToSpellID[t.internalName] = t.id;
		assert(spellItems.find(t.id) == spellItems.end()); // check we haven't got duplicate key
		spellItems.insert(std::make_pair(t.id, t));
		++spellsRead;
	}
	printlog("[JSON]: Successfully read %d spells from '%s'", spellsRead, inputPath.c_str());

	for ( int i = 0; i < NUM_SPELLS; ++i )
	{
		auto find = spellItems.find(i);
		if ( find != spellItems.end() )
		{
			spellItem_t& t = find->second;
			hash += djb2Hash(const_cast<char*>(t.internalName.c_str()));
			hash += (Uint32)((Uint32)t.id << (shift % 32)); ++shift;
			hash += djb2Hash(const_cast<char*>(t.spellTypeStr.c_str()));
			for ( auto& tag : t.spellTagsStr )
			{
				hash += djb2Hash(const_cast<char*>(tag.c_str()));
			}

			hash += djb2Hash(const_cast<char*>(t.spellbookInternalName.c_str()));
			hash += djb2Hash(const_cast<char*>(t.magicstaffInternalName.c_str()));
			hash += djb2Hash(const_cast<char*>(t.fociInternalName.c_str()));

			hashSpellProp(hash, shift, t.mana);
			hashSpellProp(hash, shift, t.duration);
			hashSpellProp(hash, shift, t.duration_mult);
			hashSpellProp(hash, shift, t.duration2);
			hashSpellProp(hash, shift, t.duration2_mult);
			hashSpellProp(hash, shift, t.damage);
			hashSpellProp(hash, shift, t.damage_mult);
			hashSpellProp(hash, shift, t.damage2);
			hashSpellProp(hash, shift, t.damage2_mult);
			hashSpellProp(hash, shift, t.distance);
			hashSpellProp(hash, shift, t.distance_mult);
			hashSpellProp(hash, shift, t.life_time);
			hashSpellProp(hash, shift, t.life_mult);
			hashSpellProp(hash, shift, t.cast_time);
			hashSpellProp(hash, shift, t.cast_time_mult);
			hashSpellProp(hash, shift, t.radius);
			hashSpellProp(hash, shift, t.radius_mult);
			hashSpellProp(hash, shift, t.difficulty);
			hashSpellProp(hash, shift, t.sustain_mana);

			hashSpellProp(hash, shift, t.sustain_duration);
			hashSpellProp(hash, shift, t.sustain_mult);
			hashSpellProp(hash, shift, t.drop_table);

			if ( t.skillID >= 0 )
			{
				hash += (Uint32)((Uint32)t.skillID << (shift % 32)); ++shift;
			}
			else
			{
				hash += (Uint32)((Uint32)1 << (shift % 32)); ++shift;
			}
		}
	}

	itemsJsonHashRead = hash;
	if ( itemsJsonHashRead != kItemsJsonHash )
	{
		printlog("[JSON]: Notice: items.json unknown hash, achievements are disabled: %d", itemsJsonHashRead);
	}
	else
	{
		printlog("[JSON]: items.json hash verified successfully.");
	}

	// validation against old items.txt
	/*for ( int i = 0; i < NUMITEMS; ++i )
	{
		assert(i == tmpItems[i].itemId);
		assert(items[i].level == tmpItems[i].itemLevel);
		assert(items[i].value == tmpItems[i].gold);
		assert(items[i].weight == tmpItems[i].weight);
		assert(items[i].fpindex == tmpItems[i].fpIndex);
		assert(items[i].index == tmpItems[i].tpIndex);
		if ( i != SPELL_ITEM )
		{
			assert(items[i].variations == tmpItems[i].imagePaths.size());
		}
		for ( int j = 0; j < items[i].variations; ++j )
		{
			auto s = static_cast<string_t*>(list_Node(&items[i].images, j)->element);
			assert(!strcmp(s->data, tmpItems[i].imagePaths[j].c_str()));
		}
		assert(items[i].index == tmpItems[i].tpIndex);
		assert(!strcmp(itemNameStrings[i + 2], tmpItems[i].itemName.c_str()));
	}*/
}

void ItemTooltips_t::readItemLocalizationsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/lang/item_names.json") )
	{
		printlog("[JSON]: Error: Could not find file: lang/item_names.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/lang/item_names.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemLocalizationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/lang/item_names.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 17); // 131kb
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( d.HasMember("items") )
	{
		if ( forceLoadBaseDirectory )
		{
			itemNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator items_itr = d["items"].MemberBegin();
			items_itr != d["items"].MemberEnd(); ++items_itr )
		{
			if ( items_itr->value.HasMember("name_identified") )
			{
				itemNameLocalizations[items_itr->name.GetString()].name_identified = items_itr->value["name_identified"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: item '%s' has no member 'name_identified'!", items_itr->name.GetString());
			}
			if ( items_itr->value.HasMember("name_unidentified") )
			{
				itemNameLocalizations[items_itr->name.GetString()].name_unidentified = items_itr->value["name_unidentified"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: item '%s' has no member 'name_unidentified'!", items_itr->name.GetString());
			}
		}
	}

	if ( d.HasMember("spell_names") )
	{
		if ( forceLoadBaseDirectory )
		{
			spellNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator spell_itr = d["spell_names"].MemberBegin();
			spell_itr != d["spell_names"].MemberEnd(); ++spell_itr )
		{
			if ( spell_itr->value.HasMember("name") )
			{
				spellNameLocalizations[spell_itr->name.GetString()] = spell_itr->value["name"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: spell '%s' has no member 'name'!", spell_itr->name.GetString());
			}
		}
	}

	printlog("[JSON]: Successfully read %d item names, %d spell names from '%s'", itemNameLocalizations.size(), spellNameLocalizations.size(), inputPath.c_str());
	assert(itemNameLocalizations.size() == (NUMITEMS));
#ifndef NDEBUG
	//assert(spellNameLocalizations.size() == (NUM_SPELLS - 1)); // ignore SPELL_NONE
#endif

	// apply localizations
	for ( int i = 0; i < NUMITEMS; ++i )
	{
		items[i].setIdentifiedName("default_identified_item_name");
		items[i].setUnidentifiedName("default_unidentified_item_name");
	}
	for ( auto& item : tmpItems )
	{
		if ( item.itemId >= WOODEN_SHIELD && item.itemId < NUMITEMS )
		{
			items[item.itemId].setIdentifiedName(itemNameLocalizations[item.internalName].name_identified);
			items[item.itemId].setUnidentifiedName(itemNameLocalizations[item.internalName].name_unidentified);
		}
	}
	for ( auto& spell : spellItems )
	{
		spell.second.name = spellNameLocalizations[spell.second.internalName];
		spell.second.name_lowercase = spell.second.name;
		lowercaseString(spell.second.name_lowercase);
	}

	/*for ( auto i : itemValueTable )
	{
		printlog("itemValueTable %4d | %s", items[i.second].value, items[i.second].getIdentifiedName());
	}
	for ( int cat = 0; cat < NUMCATEGORIES; ++cat )
	{
		for ( auto i : itemValueTableByCategory[cat] )
		{
			printlog("itemValueTableByCategory %2d | %4d | %s", cat,
				items[i.second].value, items[i.second].getIdentifiedName());
		}
	}*/
}

void ItemTooltips_t::readBookLocalizationsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/lang/book_names.json") )
	{
		printlog("[JSON]: Error: Could not find file: lang/book_names.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/lang/book_names.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readBookLocalizationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/lang/book_names.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 13);
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( d.HasMember("book_names") )
	{
		if ( forceLoadBaseDirectory )
		{
			bookNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator books_itr = d["book_names"].MemberBegin();
			books_itr != d["book_names"].MemberEnd(); ++books_itr )
		{
			bookNameLocalizations[books_itr->name.GetString()] = books_itr->value.GetString();
		}
	}

	printlog("[JSON]: Successfully read %d book names from '%s'", bookNameLocalizations.size(), inputPath.c_str());
}

#ifndef EDITOR
void ItemTooltips_t::readTooltipsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/items/item_tooltips.json") )
	{
		printlog("[JSON]: Error: Could not find file: items/item_tooltips.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/items/item_tooltips.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readTooltipsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/items/item_tooltips.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 20); // 1mb
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( forceLoadBaseDirectory )
	{
		adjectives.clear();
	}
	if ( d.HasMember("adjectives") )
	{
		for ( rapidjson::Value::ConstMemberIterator adj_itr = d["adjectives"].MemberBegin();
			adj_itr != d["adjectives"].MemberEnd(); ++adj_itr )
		{
			std::map<std::string, std::string> m;
			for ( rapidjson::Value::ConstMemberIterator inner_itr = adj_itr->value.MemberBegin();
				inner_itr != adj_itr->value.MemberEnd(); ++inner_itr )
			{
				m[inner_itr->name.GetString()] = inner_itr->value.GetString();
			}
			adjectives[adj_itr->name.GetString()] = m;
		}
	}

	if ( d.HasMember("default_text_colors") )
	{
		defaultHeadingTextColor = makeColor( 
			d["default_text_colors"]["heading"]["r"].GetInt(), 
			d["default_text_colors"]["heading"]["g"].GetInt(), 
			d["default_text_colors"]["heading"]["b"].GetInt(),
			d["default_text_colors"]["heading"]["a"].GetInt());
		defaultIconTextColor = makeColor(
			d["default_text_colors"]["icons"]["r"].GetInt(),
			d["default_text_colors"]["icons"]["g"].GetInt(),
			d["default_text_colors"]["icons"]["b"].GetInt(), 
			d["default_text_colors"]["icons"]["a"].GetInt());
		defaultDescriptionTextColor = makeColor(
			d["default_text_colors"]["description"]["r"].GetInt(),
			d["default_text_colors"]["description"]["g"].GetInt(), 
			d["default_text_colors"]["description"]["b"].GetInt(),
			d["default_text_colors"]["description"]["a"].GetInt());
		defaultDetailsTextColor = makeColor(
			d["default_text_colors"]["details"]["r"].GetInt(), 
			d["default_text_colors"]["details"]["g"].GetInt(), 
			d["default_text_colors"]["details"]["b"].GetInt(), 
			d["default_text_colors"]["details"]["a"].GetInt());
		defaultPositiveTextColor = makeColor(
			d["default_text_colors"]["positive_color"]["r"].GetInt(),
			d["default_text_colors"]["positive_color"]["g"].GetInt(),
			d["default_text_colors"]["positive_color"]["b"].GetInt(),
			d["default_text_colors"]["positive_color"]["a"].GetInt());
		defaultNegativeTextColor = makeColor(
			d["default_text_colors"]["negative_color"]["r"].GetInt(),
			d["default_text_colors"]["negative_color"]["g"].GetInt(),
			d["default_text_colors"]["negative_color"]["b"].GetInt(),
			d["default_text_colors"]["negative_color"]["a"].GetInt());
		defaultStatusEffectTextColor = makeColor(
			d["default_text_colors"]["status_effect"]["r"].GetInt(),
			d["default_text_colors"]["status_effect"]["g"].GetInt(),
			d["default_text_colors"]["status_effect"]["b"].GetInt(),
			d["default_text_colors"]["status_effect"]["a"].GetInt());
		defaultFaintTextColor = makeColor(
			d["default_text_colors"]["faint_text"]["r"].GetInt(),
			d["default_text_colors"]["faint_text"]["g"].GetInt(),
			d["default_text_colors"]["faint_text"]["b"].GetInt(),
			d["default_text_colors"]["faint_text"]["a"].GetInt());
	}

	if ( forceLoadBaseDirectory )
	{
		templates.clear();
	}
	if ( d.HasMember("templates") )
	{
		for ( rapidjson::Value::ConstMemberIterator template_itr = d["templates"].MemberBegin();
			template_itr != d["templates"].MemberEnd(); ++template_itr )
		{
			if ( !template_itr->value.IsArray() )
			{
				printlog("[JSON]: Error: template entry for template %s did not have [] format!", template_itr->name.GetString());
			}
			else
			{
				std::string template_name = template_itr->name.GetString();
				if ( templates.find(template_name) != templates.end() )
				{
					templates[template_name].clear();
				}
				for ( auto lines = template_itr->value.Begin();
					lines != template_itr->value.End(); ++lines )
				{
					templates[template_name].push_back(lines->GetString());
				}
			}
		}
	}

	if ( forceLoadBaseDirectory )
	{
		tooltips.clear();
	}
	std::unordered_set<std::string> tagsRead;

	if ( d.HasMember("tooltips") )
	{
		for ( rapidjson::Value::ConstMemberIterator tooltipType_itr = d["tooltips"].MemberBegin();
			tooltipType_itr != d["tooltips"].MemberEnd(); ++tooltipType_itr )
		{
			ItemTooltip_t tooltip;
			tooltip.setColorHeading(this->defaultHeadingTextColor);
			tooltip.setColorDescription(this->defaultDescriptionTextColor);
			tooltip.setColorDetails(this->defaultDetailsTextColor);
			tooltip.setColorPositive(this->defaultPositiveTextColor);
			tooltip.setColorNegative(this->defaultNegativeTextColor);
			tooltip.setColorStatus(this->defaultStatusEffectTextColor);
			tooltip.setColorFaintText(this->defaultFaintTextColor);

			if ( tooltipType_itr->value.HasMember("icons") )
			{
				if ( !tooltipType_itr->value["icons"].IsArray() )
				{
					printlog("[JSON]: Error: 'icons' entry for tooltip %s did not have [] format", tooltipType_itr->name.GetString());
				}
				else
				{
					for ( auto icons = tooltipType_itr->value["icons"].Begin();
						icons != tooltipType_itr->value["icons"].End(); ++icons )
					{
						// you need to FindMember() if getting objects from an array...
						auto textMember = icons->FindMember("text");
						auto iconPathMember = icons->FindMember("icon_path");
						if ( !textMember->value.IsString() || !iconPathMember->value.IsString() )
						{
							printlog("[JSON]: Error: Icon text or path was not string!");
							continue;
						}

						tooltip.icons.push_back(ItemTooltipIcons_t(iconPathMember->value.GetString(), textMember->value.GetString()));

						Uint32 color = this->defaultIconTextColor;
						if ( icons->HasMember("color") && icons->FindMember("color")->value.HasMember("r") )
						{
							// icons->FindMember("color")->value.isObject() always returning true?? so check for "r" member instead
							color = makeColor(
								icons->FindMember("color")->value["r"].GetInt(),
								icons->FindMember("color")->value["g"].GetInt(),
								icons->FindMember("color")->value["b"].GetInt(),
								icons->FindMember("color")->value["a"].GetInt());
						}
						tooltip.icons[tooltip.icons.size() - 1].setColor(color);
						if ( icons->HasMember("conditional_attribute") )
						{
							tooltip.icons[tooltip.icons.size() - 1].setConditionalAttribute(icons->FindMember("conditional_attribute")->value.GetString());
						}
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("description") )
			{
				if ( tooltipType_itr->value["description"].IsString() )
				{
					//printlog("[JSON]: Found template string '%s' for tooltip '%s'", tooltipType_itr->value["description"].GetString(), tooltipType_itr->name.GetString());
					if ( templates.find(tooltipType_itr->value["description"].GetString()) != templates.end() )
					{
						tooltip.descriptionText = templates[tooltipType_itr->value["description"].GetString()];
					}
					else
					{
						printlog("[JSON]: Error: Could not find template tag '%s'", tooltipType_itr->value["description"].GetString());
					}
				}
				else
				{
					for ( auto descriptions = tooltipType_itr->value["description"].Begin();
						descriptions != tooltipType_itr->value["description"].End(); ++descriptions )
					{
						tooltip.descriptionText.push_back(descriptions->GetString());
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("details") )
			{
				if ( !tooltipType_itr->value["details"].IsArray() )
				{
					printlog("[JSON]: Error: 'details' entry for tooltip '%s' did not have [] format!", tooltipType_itr->name.GetString());
				}
				else
				{
					for ( auto details_itr = tooltipType_itr->value["details"].Begin();
						details_itr != tooltipType_itr->value["details"].End(); ++details_itr )
					{
						for ( auto keyValue_itr = details_itr->MemberBegin();
							keyValue_itr != details_itr->MemberEnd(); ++keyValue_itr )
						{
							tagsRead.insert(keyValue_itr->name.GetString());
							std::vector<std::string> detailEntry;
							if ( keyValue_itr->value.IsString() )
							{
								//printlog("[JSON]: Found template string '%s' for tooltip '%s'", keyValue_itr->value.GetString(), tooltipType_itr->name.GetString());
								if ( templates.find(keyValue_itr->value.GetString()) != templates.end() )
								{
									detailEntry = templates[keyValue_itr->value.GetString()];
								}
								else
								{
									printlog("[JSON]: Error: Could not find template tag '%s'", keyValue_itr->value.GetString());
								}
							}
							else
							{
								for ( auto detailTag = keyValue_itr->value.Begin();
									detailTag != keyValue_itr->value.End(); ++detailTag )
								{
									detailEntry.push_back(detailTag->GetString());
								}
							}
							tooltip.detailsText[keyValue_itr->name.GetString()] = detailEntry;
							tooltip.detailsTextInsertOrder.push_back(keyValue_itr->name.GetString());
						}
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("size") )
			{
				if ( tooltipType_itr->value["size"].HasMember("min_width") )
				{
					tooltip.minWidths["default"] = tooltipType_itr->value["size"]["min_width"].GetInt();
				}
				else
				{
					tooltip.minWidths["default"] = 0;
				}
				if ( tooltipType_itr->value["size"].HasMember("max_width") )
				{
					tooltip.maxWidths["default"] = tooltipType_itr->value["size"]["max_width"].GetInt();
				}
				else
				{
					tooltip.maxWidths["default"] = 0;
				}
				if ( tooltipType_itr->value["size"].HasMember("max_header_width") )
				{
					tooltip.headerMaxWidths["default"] = tooltipType_itr->value["size"]["max_header_width"].GetInt();
				}
				else
				{
					tooltip.headerMaxWidths["default"] = 0;
				}

				if ( tooltipType_itr->value["size"].HasMember("item_overrides") )
				{
					for ( auto itemOverride_itr = tooltipType_itr->value["size"]["item_overrides"].MemberBegin();
						itemOverride_itr != tooltipType_itr->value["size"]["item_overrides"].MemberEnd(); ++itemOverride_itr )
					{
						if ( itemOverride_itr->value.HasMember("min_width") )
						{
							tooltip.minWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["min_width"].GetInt();
						}
						if ( itemOverride_itr->value.HasMember("max_width") )
						{
							tooltip.maxWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["max_width"].GetInt();
						}
						if ( itemOverride_itr->value.HasMember("max_header_width") )
						{
							tooltip.headerMaxWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["max_header_width"].GetInt();
						}
					}
				}
			}

			tooltips[tooltipType_itr->name.GetString()] = tooltip;
		}
	}

	printlog("[JSON]: Successfully read %d item tooltips from '%s'", tooltips.size(), inputPath.c_str());
	/*for ( auto tmp : tagsRead )
	{
		printlog("%s", tmp.c_str());
	}*/
}

std::string& ItemTooltips_t::getItemStatusAdjective(Uint32 itemType, Status status)
{
	if ( itemType >= ARTIFACT_ORB_BLUE && itemType <= ARTIFACT_ORB_GREEN )
	{
		if ( adjectives.find("jewelry_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["jewelry_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["jewelry_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["jewelry_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["jewelry_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["jewelry_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( itemType == TOOL_SENTRYBOT || itemType == TOOL_SPELLBOT
		|| itemType == TOOL_GYROBOT || itemType == TOOL_DUMMYBOT )
	{
		if ( adjectives.find("tinkering_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["tinkering_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["tinkering_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["tinkering_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["tinkering_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["tinkering_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == ARMOR 
		|| items[itemType].category == WEAPON
		|| items[itemType].category == MAGICSTAFF
		|| items[itemType].category == TOOL
		|| items[itemType].category == THROWN
		|| itemType == POTION_EMPTY )
	{
		if ( adjectives.find("equipment_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["equipment_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["equipment_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["equipment_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["equipment_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["equipment_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == AMULET
		|| items[itemType].category == RING
		|| items[itemType].category == GEM )
	{
		if ( adjectives.find("jewelry_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["jewelry_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["jewelry_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["jewelry_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["jewelry_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["jewelry_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == SCROLL
		|| items[itemType].category == SPELLBOOK
		|| items[itemType].category == TOME_SPELL
		|| items[itemType].category == BOOK )
	{
		if ( adjectives.find("book_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["book_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["book_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["book_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["book_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["book_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == FOOD )
	{
		if ( adjectives.find("food_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["food_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["food_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["food_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["food_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["food_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == POTION )
	{
		if ( adjectives.find("potion_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["potion_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["potion_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["potion_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["potion_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["potion_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemBeatitudeAdjective(Sint16 beatitude)
{
	if ( adjectives.find("beatitude_status") == adjectives.end() )
	{
		return defaultString;
	}

	if ( beatitude > 0 )
	{
		return adjectives["beatitude_status"]["blessed"];
	}
	else if ( beatitude < 0 )
	{
		return adjectives["beatitude_status"]["cursed"];
	}
	else
	{
		return adjectives["beatitude_status"]["uncursed"];
	}
}

std::string& ItemTooltips_t::getProficiencyLevelName(Sint32 proficiencyLevel)
{
	if ( adjectives.find("proficiency_levels") == adjectives.end() )
	{
		return defaultString;
	}

	if ( proficiencyLevel >= SKILL_LEVEL_LEGENDARY )
	{
		return adjectives["proficiency_levels"]["legend"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_MASTER )
	{
		return adjectives["proficiency_levels"]["master"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_EXPERT )
	{
		return adjectives["proficiency_levels"]["expert"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_SKILLED )
	{
		return adjectives["proficiency_levels"]["skilled"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_BASIC )
	{
		return adjectives["proficiency_levels"]["basic"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_NOVICE )
	{
		return adjectives["proficiency_levels"]["novice"];
	}
	else
	{
		return adjectives["proficiency_levels"]["none"];
	}
}

bool ItemTooltips_t::bIsSpellDamageOrHealingType(spell_t* spell)
{
	if ( !spell )
	{
		return false;
	}
	if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING) != spellItems[spell->ID].spellTags.end()
		|| spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_DAMAGE) != spellItems[spell->ID].spellTags.end() )
	{
		return true;
	}
	return false;
}

bool ItemTooltips_t::bSpellHasBasicHitMessage(const int spellID)
{
	if ( spellItems.find(spellID) != spellItems.end() )
	{
		auto& entry = spellItems[spellID];
		if ( entry.spellTags.find(SPELL_TAG_BASIC_HIT_MESSAGE) != entry.spellTags.end() )
		{
			return true;
		}
	}
	return false;
}

int ItemTooltips_t::getSpellDamageOrHealAmount(const int player, spell_t* spell, Item* spellbook, const bool excludePlayerStats)
{
#ifdef EDITOR
	return 0;
#else
	if ( !spell )
	{
		return 0;
	}
	node_t* rootNode = spell->elements.first;
	spellElement_t* elementRoot = nullptr;
	if ( rootNode )
	{
		elementRoot = (spellElement_t*)(rootNode->element);
	}
	int damage = 0;
	int mana = 0;
	int heal = 0;
	spellElement_t* primaryElement = nullptr;
	real_t damageMult = 1.0;
	if ( elementRoot )
	{
		node_t* primaryNode = elementRoot->elements.first;
		if ( primaryNode )
		{
			primaryElement = (spellElement_t*)(primaryNode->element);
			if ( primaryElement )
			{
				damage = primaryElement->getDamage();
				heal = primaryElement->getDamage();
				damageMult = primaryElement->getDamageMult();
			}
		}
		else
		{
			damage = elementRoot->getDamage();
			heal = elementRoot->getDamage();
			damageMult = elementRoot->getDamageMult();
		}
		if ( player >= 0 && players[player] )
		{
			int bonus = 0;
			if ( spellbook && itemCategory(spellbook) == MAGICSTAFF && spellbook->type != MAGICSTAFF_SCEPTER )
			{
				// no modifier.
			}
			else
			{
				if ( spellbook && (itemCategory(spellbook) == SPELLBOOK || itemTypeIsFoci(spellbook->type)) )
				{
					bonus = getSpellbookBonusPercent(
						excludePlayerStats ? nullptr : players[player]->entity, 
						excludePlayerStats ? nullptr : stats[player], 
						spellbook);
				}
				damage += damageMult * (damage * (bonus * 0.01
					+ getBonusFromCasterOfSpellElement(
						excludePlayerStats ? nullptr : players[player]->entity,
						excludePlayerStats ? nullptr : stats[player], primaryElement, spell ? spell->ID : SPELL_NONE, spell->skillID)));
				heal += damageMult * (heal * (bonus * 0.01
					+ getBonusFromCasterOfSpellElement(
						excludePlayerStats ? nullptr : players[player]->entity,
						excludePlayerStats ? nullptr : stats[player], primaryElement, spell ? spell->ID : SPELL_NONE, spell->skillID)));
			}
		}
		if ( spell->ID == SPELL_HEALING || spell->ID == SPELL_EXTRAHEALING )
		{
			damage = heal;
		}
		if ( spell->ID == SPELL_MUSHROOM )
		{
			if ( !excludePlayerStats )
			{
				if ( player >= 0 && players[player] )
				{
					int bonusEffect = 0;
					if ( stats[player]->type == MYCONID && stats[player]->getEffectActive(EFF_GROWTH) >= 2 )
					{
						bonusEffect = std::max(bonusEffect, stats[player]->getEffectActive(EFF_GROWTH) - 1);
					}
					damage += damage * (bonusEffect * 1.0);
				}
			}
		}
	}
	return damage;
#endif
}

std::string ItemTooltips_t::getSpellDescriptionText(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell || spellItems.find(spell->ID) == spellItems.end() )
	{
		return defaultString;
	}
	std::string templateName = "template_desc_";
	templateName += spellItems[spell->ID].internalName;

	if ( templates.find(templateName) == templates.end() )
	{
		return defaultString;
	}

	std::string str;
	for ( auto it = templates[templateName].begin();
		it != templates[templateName].end(); ++it )
	{
		str += *it;
		if ( std::next(it) != ItemTooltips.templates[templateName].end() )
		{
			str += '\n';
		}
	}
	return str;
#endif
}

std::string& ItemTooltips_t::getIconLabel(Item& item)
{
#ifndef EDITOR
	if ( item.type == SPELL_ITEM && !item.spellNotifyIcon ) { return defaultString; }
	return tmpItems[item.type].iconLabelPath;
#endif
}

const char* spellValueToTier(int val)
{
	if ( val == 1 ) { return "I"; }
	if ( val == 2 ) { return "II"; }
	if ( val == 3 ) { return "III"; }
	if ( val == 4 ) { return "IV"; }
	if ( val == 5 ) { return "V"; }
	return "I";
}

std::string ItemTooltips_t::getSpellIconFormatText(const int player, Item& item, std::string& format, const spell_t* spell, const int iconIndex, const bool compendiumTooltipIntro)
{
	if ( !spell ) { return defaultString; }
	auto def = spellItems.find(spell->ID);
	if ( def == spellItems.end() )
	{
		return defaultString;
	}

	enum Comparators
	{
		COMP_NONE,
		COMP_MIN,
		COMP_MAX,
		COMP_HEAL_OTHER,
		COMP_INCOHERENCE,
		COMP_WEAKNESS,
		COMP_BLOOD_WAVES,
		COMP_MAGICIANS_ARMOR,
		COMP_BREATHE_FIRE,
		COMP_BREATHE_FIRE2,
		COMP_SIGIL,
		COMP_SANCTUARY
	};
	enum Values
	{
		VAL_NUM,
		VAL_TIER
	};
	Values valueType = VAL_NUM;
	Comparators comparator = COMP_NONE;
	Entity* caster = compendiumTooltipIntro ? nullptr : players[player]->entity;
	Stat* myStats = compendiumTooltipIntro ? nullptr : stats[player];

	real_t bonusPercent = 0.0;
	if ( itemCategory(&item) == SPELLBOOK )
	{
		bonusPercent = getSpellbookBonusPercent(
			caster,
			myStats,
			&item) / 100.0;
	}

	for ( auto& formatTag : def->second.spellFormatTags )
	{
		if ( formatTag.size() == 0 ) { continue; }
		if ( (formatTag.at(0) - '1') != iconIndex ) { continue; }
		std::vector<int> vals;
		size_t index = 2;
		std::string str;
		size_t offset = 1;
		while ( index + offset < formatTag.size() )
		{
			if ( (index + offset == (formatTag.size() - 1)) || formatTag[index + offset] == ' ' )
			{
				std::string key = (index + offset == (formatTag.size() - 1)) ? formatTag.substr(index) : formatTag.substr(index, offset);
				if ( key == "min" )
				{
					comparator = COMP_MIN;
				}
				else if ( key == "dmg" )
				{
					vals.push_back(getSpellDamageFromID(spell->ID, caster, myStats, caster, bonusPercent, false));
				}
				else if ( key == "dmg2" )
				{
					vals.push_back(getSpellDamageSecondaryFromID(spell->ID, caster, myStats, caster, bonusPercent, false));
				}
				else if ( key == "dur" )
				{
					vals.push_back(getSpellEffectDurationFromID(spell->ID, caster, myStats, caster, bonusPercent));
				}
				else if ( key == "dur2" )
				{
					vals.push_back(getSpellEffectDurationSecondaryFromID(spell->ID, caster, myStats, caster, bonusPercent));
				}
				else if ( key == "tier" )
				{
					valueType = VAL_TIER;
				}
				else if ( key == "comp_heal_other" )
				{
					comparator = COMP_HEAL_OTHER;
				}
				else if ( key == "comp_incoherence" )
				{
					comparator = COMP_INCOHERENCE;
				}
				else if ( key == "comp_weakness" )
				{
					comparator = COMP_WEAKNESS;
				}
				else if ( key == "comp_blood_waves" )
				{
					comparator = COMP_BLOOD_WAVES;
				}
				else if ( key == "comp_magicians_armor" )
				{
					comparator = COMP_MAGICIANS_ARMOR;
				}
				else if ( key == "comp_breathe_fire" )
				{
					comparator = COMP_BREATHE_FIRE;
				}
				else if ( key == "comp_breathe_fire2" )
				{
					comparator = COMP_BREATHE_FIRE2;
				}
				else if ( key == "comp_sigil" )
				{
					comparator = COMP_SIGIL;
				}
				else if ( key == "comp_sanctuary" )
				{
					comparator = COMP_SANCTUARY;
				}
				index += offset + 1;
				offset = 1;
			}
			else
			{
				++offset;
			}
		}

		if ( vals.size() )
		{
			if ( comparator == COMP_HEAL_OTHER && vals.size() >= 4 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));
				real_t healTicks = vals[0] * (vals[1] / 100.0) * vals[2];
				snprintf(buf, sizeof(buf), format.c_str(), (int)(healTicks / vals[3]));
				str = buf;
			}
			else if ( (comparator == COMP_INCOHERENCE || comparator == COMP_WEAKNESS) && vals.size() >= 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));
				int effectStrength = std::min(vals[0], vals[1]);
				int reduction = 100.0 * std::min(0.9, 0.2 + (effectStrength - 1) * 0.1);
				snprintf(buf, sizeof(buf), format.c_str(), -reduction);
				str = buf;
			}
			else if ( comparator == COMP_MAGICIANS_ARMOR && vals.size() >= 3 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));

				int instances = vals[2];
				instances *= ((myStats ? myStats->getModifiedProficiency(spell->skillID) : 0) + statGetINT(myStats, caster)) / std::max(1, vals[1]);
				int maxInstances = vals[0];
				instances = std::min(std::max(1, instances), maxInstances);

				snprintf(buf, sizeof(buf), format.c_str(), instances);
				str = buf;
			}
			else if ( comparator == COMP_BREATHE_FIRE && vals.size() >= 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));

				snprintf(buf, sizeof(buf), format.c_str(), std::min(10, vals[1]), vals[0]);
				str = buf;
			}
			else if ( comparator == COMP_BREATHE_FIRE2 )
			{
				int maxStrength = 10;
				int minStrength = 2;
				if ( myStats )
				{
					if ( myStats->type == SALAMANDER
						&& myStats->getEffectActive(EFF_SALAMANDER_HEART) == 2 )
					{
						minStrength += 3;
					}
					maxStrength = std::min(maxStrength, minStrength + std::max(0, statGetCHR(myStats, caster)) / 5);
				}
				else
				{
					maxStrength = minStrength;
				}

				snprintf(buf, sizeof(buf), format.c_str(), maxStrength);
				str = buf;
			}
			else if ( comparator == COMP_SIGIL && vals.size() >= 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));

				int tier = std::min(vals[0], vals[1]);
				int dmgMult = 10 + 10 * tier;
				int healMult = 10 + 10 * tier;

				snprintf(buf, sizeof(buf), format.c_str(), healMult, dmgMult);
				str = buf;
			}
			else if ( comparator == COMP_SANCTUARY && vals.size() >= 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));

				int tier = std::min(vals[0], vals[1]);
				int reduction = std::min(80, std::max(0, 10 + (15 * tier)));

				snprintf(buf, sizeof(buf), format.c_str(), reduction);
				str = buf;
			}
			else if ( (comparator == COMP_BLOOD_WAVES) && vals.size() >= 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));
				spellElement_t* element = nullptr;
				if ( spell->elements.first )
				{
					if ( element = (spellElement_t*)spell->elements.first->element )
					{
						if ( element->elements.first && element->elements.first->element )
						{
							element = (spellElement_t*)element->elements.first->element;
						}
					}
				}
				int damage = vals[0] + std::max(1, vals[1] * statGetINT(myStats, caster)) * (1.0 + bonusPercent + getBonusFromCasterOfSpellElement(caster, myStats, element, spell->ID, spell->skillID));
				snprintf(buf, sizeof(buf), format.c_str(), damage);
				str = buf;
			}
			else if ( vals.size() == 1 || comparator == COMP_MIN || comparator == COMP_MAX )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));
				if ( vals.size() == 2 )
				{
					if ( comparator == COMP_MIN )
					{
						if ( valueType == VAL_TIER )
						{
							snprintf(buf, sizeof(buf), format.c_str(), spellValueToTier(std::min(vals[0], vals[1])));
						}
						else
						{
							snprintf(buf, sizeof(buf), format.c_str(), std::min(vals[0], vals[1]));
						}
					}
					else if ( comparator == COMP_MAX )
					{
						snprintf(buf, sizeof(buf), format.c_str(), std::max(vals[0], vals[1]));
					}
				}
				else
				{
					if ( valueType == VAL_TIER )
					{
						snprintf(buf, sizeof(buf), format.c_str(), spellValueToTier(vals[0]));
					}
					else
					{
						snprintf(buf, sizeof(buf), format.c_str(), vals[0]);
					}
				}
				str = buf;
			}
			else if ( vals.size() == 2 )
			{
				char buf[128];
				memset(buf, 0, sizeof(buf));
				snprintf(buf, sizeof(buf), format.c_str(), vals[0], vals[1]);
				str = buf;
			}
			return str;
		}
	}
	return defaultString;
}

std::string ItemTooltips_t::getSpellIconText(const int player, Item& item, const bool compendiumTooltipIntro)
{
#ifndef EDITOR
	spell_t* spell = nullptr;
	
	if ( itemCategory(&item) == SPELLBOOK )
	{
		spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
	}
	else if ( itemCategory(&item) == MAGICSTAFF )
	{
		for ( auto& s : spellItems )
		{
			if ( s.second.magicstaffId == item.type )
			{
				spell = getSpellFromID(s.first);
				break;
			}
		}
	}
	else
	{
		spell = getSpellFromItem(player, &item, false);
	}
	if ( !spell || spellItems.find(spell->ID) == spellItems.end() )
	{
		return defaultString;
	}
	std::string templateName = "template_icon_";
	templateName += spellItems[spell->ID].internalName;

	if ( templates.find(templateName) == templates.end() )
	{
		return defaultString;
	}

	std::string str;
	for ( auto it = templates[templateName].begin();
		it != templates[templateName].end(); ++it )
	{
		str += *it;
		if ( std::next(it) != ItemTooltips.templates[templateName].end() )
		{
			str += '\n';
		}
	}

	if ( spellItems[spell->ID].internalName == "spell_summon" )
	{
		int numSummons = 1;
		if ( !compendiumTooltipIntro )
		{
			if ( (statGetINT(stats[player], players[player]->entity)
				+ stats[player]->getModifiedProficiency(spell->skillID)) >= SKILL_LEVEL_EXPERT )
			{
				numSummons = 2;
			}
		}
		char buf[128];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), str.c_str(), numSummons);
		str = buf;
	}
	else if ( spell->ID == SPELL_BREATHE_FIRE || spell->ID == SPELL_BLOOD_WAVES )
	{
		std::string result = getSpellIconFormatText(player, item, str, spell, 0, compendiumTooltipIntro);
		if ( result != "" )
		{
			str = result;
		}
	}
	else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING) != spellItems[spell->ID].spellTags.end()
		|| spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_DAMAGE) != spellItems[spell->ID].spellTags.end() )
	{
		char buf[128];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), str.c_str(), getSpellDamageOrHealAmount(player, spell, &item, compendiumTooltipIntro));
		str = buf;
	}
	else if ( spellItems[spell->ID].spellFormatTags.size() )
	{
		std::string result = getSpellIconFormatText(player, item, str, spell, 0, compendiumTooltipIntro);
		if ( result != "" )
		{
			str = result;
		}
	}

	return str;
#else
	return std::string("");
#endif
}

real_t ItemTooltips_t::getSpellSustainCostPerSecond(int spellID)
{
	real_t cost = 0.0;
	if ( auto spell = getSpellFromID(spellID) )
	{
		if ( spell_isChanneled(spell) )
		{
			if ( spell->elements.first )
			{
				if ( spellElement_t* element = (spellElement_t*)spell->elements.first->element )
				{
					if ( element->channeledMana > 0 )
					{
						return element->duration / (real_t)TICKS_PER_SECOND;
					}
				}
			}
		}
	}
	/*switch ( spellID )
	{
		case SPELL_REFLECT_MAGIC:
			cost = 6.0;
			break;
		case SPELL_LEVITATION:
			cost = 0.6;
			break;
		case SPELL_INVISIBILITY:
			cost = 1.0;
			break;
		case SPELL_LIGHT:
			cost = 15.0;
			break;
		case SPELL_VAMPIRIC_AURA:
			cost = 0.33;
			break;
		case SPELL_AMPLIFY_MAGIC:
			cost = 0.25;
			break;
		default:
			break;
	}*/
	return cost;
}

std::string& ItemTooltips_t::getSpellTypeString(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell )
	{
		return defaultString;
	}
	switch ( spellItems[spell->ID].spellType )
	{
		case SPELL_TYPE_AREA:
			return adjectives["spell_strings"]["spell_type_area"];
			break;
		case SPELL_TYPE_PROJECTILE:
			return adjectives["spell_strings"]["spell_type_projectile"];
			break;
		case SPELL_TYPE_SELF:
			return adjectives["spell_strings"]["spell_type_self"];
			break;
		case SPELL_TYPE_SELF_SUSTAIN:
			return adjectives["spell_strings"]["spell_type_self_sustain"];
			break;
		case SPELL_TYPE_PROJECTILE_SHORT_X3:
			return adjectives["spell_strings"]["spell_type_projectile_3x"];
			break;
		case SPELL_TYPE_TOUCH_FLOOR:
			return adjectives["spell_strings"]["spell_type_touch_floor"];
			break;
		case SPELL_TYPE_TOUCH_WALL:
			return adjectives["spell_strings"]["spell_type_touch_wall"];
			break;
		case SPELL_TYPE_TOUCH_ENEMY:
			return adjectives["spell_strings"]["spell_type_touch_enemy"];
			break;
		case SPELL_TYPE_TOUCH_ALLY:
			return adjectives["spell_strings"]["spell_type_touch_ally"];
			break;
		case SPELL_TYPE_TOUCH_ENTITY:
			return adjectives["spell_strings"]["spell_type_touch_entity"];
			break;
		case SPELL_TYPE_DIVINE_TARGET:
			return adjectives["spell_strings"]["spell_type_divine_target"];
			break;
		case SPELL_TYPE_DEFAULT:
		default:
			return defaultString;
			break;
	}
#endif
}

std::string ItemTooltips_t::getCostOfSpellString(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell )
	{
		return defaultString;
	}
	char buf[64];
	memset(buf, 0, sizeof(buf));
	if ( spell->ID == SPELL_DOMINATE )
	{
		std::string templateName = "template_spell_cost_dominate";
		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
	}
	else if ( spell->ID == SPELL_DEMON_ILLUSION )
	{
		std::string templateName = "template_spell_cost_demon_illusion";
		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
	}
	else if ( spell->ID == SPELL_LEAD_BOLT || spell->ID == SPELL_MERCURY_BOLT
		|| spell->ID == SPELL_FORGE_METAL_SCRAP || spell->ID == SPELL_FORGE_MAGIC_SCRAP )
	{
		std::string templateName = "template_spell_cost_gold";
		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell), getGoldCostOfSpell(spell, player));
	}
	else
	{
		std::string templateName = "template_spell_cost";
		real_t sustainCostPerSecond = getSpellSustainCostPerSecond(spell->ID);
		if ( sustainCostPerSecond > 0.01 )
		{
			templateName = "template_spell_cost_sustained";
		}

		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
		if ( players[player] && players[player]->entity )
		{
			if ( sustainCostPerSecond > 0.01 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					getCostOfSpell(spell, players[player]->entity), sustainCostPerSecond);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell, players[player]->entity));
			}
		}
		else
		{
			if ( sustainCostPerSecond > 0.01 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell), sustainCostPerSecond);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
			}
		}
	}
	return buf;
#endif
}

node_t* ItemTooltips_t::getSpellNodeFromSpellID(int spellID)
{
	node_t* spellImageNode = nullptr;
	if ( spellID >= NUM_SPELLS || spellID < SPELL_NONE )
	{
		return nullptr;
	}

	if ( arachnophobia_filter )
	{
		if ( spellID == SPELL_SPIDER_FORM )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_FORM);
		}
		else if ( spellID == SPELL_SPRAY_WEB )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_WEB);
		}
		else
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
		}
	}
	else
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
	}
	return spellImageNode;
}

std::string ItemTooltips_t::getSpellIconPath(const int player, Item& item, int spellID)
{
#ifdef EDITOR
	return "items/images/null.png";
#else
	node_t* spellImageNode = nullptr;
	if ( itemCategory(&item) == MAGICSTAFF )
	{
		spell_t* spell = nullptr;
		for ( auto& s : spellItems )
		{
			if ( s.second.magicstaffId == item.type )
			{
				spell = getSpellFromID(s.first);
				break;
			}
		}
		if ( spell )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, spell->ID);
		}
		else
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, 0);
		}
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, getSpellIDFromSpellbook(item.type));
	}
	else if ( itemCategory(&item) == TOME_SPELL )
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, item.getTomeSpellID());
	}
	else if ( item.type == TOOL_SPELLBOT )
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, item.status < EXCELLENT ? SPELL_FORCEBOLT : SPELL_MAGICMISSILE);
	}
	else if ( item.type == SPELL_ITEM )
	{
		if ( spellID > SPELL_NONE )
		{
			spellImageNode = getSpellNodeFromSpellID(spellID);
		}
		else
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( spell )
			{
				spellImageNode = getSpellNodeFromSpellID(spell->ID);
			}
			else
			{
				spellImageNode = getSpellNodeFromSpellID(SPELL_NONE);
			}
		}
	}
	if ( spellImageNode )
	{
		string_t* string = (string_t*)spellImageNode->element;
		if ( string )
		{
			return string->data;
		}
	}
	return "items/images/null.png";
#endif
}

std::string& ItemTooltips_t::getItemPotionAlchemyAdjective(const int player, Uint32 itemType)
{
#ifdef EDITOR
	return defaultString;
#else
	if ( adjectives.find("potion_alchemy_types") == adjectives.end() )
	{
		return defaultString;
	}
	if ( clientLearnedAlchemyIngredients[player].find(itemType) == clientLearnedAlchemyIngredients[player].end() )
	{
		return adjectives["potion_alchemy_types"]["unknown"];
	}
	else if ( GenericGUI[player].isItemBaseIngredient(itemType) )
	{
		return adjectives["potion_alchemy_types"]["base_ingredient"];
	}
	else if ( GenericGUI[player].isItemSecondaryIngredient(itemType) )
	{
		return adjectives["potion_alchemy_types"]["secondary_ingredient"];
	}
	else
	{
		return adjectives["potion_alchemy_types"]["no_ingredient"];
	}
#endif
}

std::string& ItemTooltips_t::getItemPotionHarmAllyAdjective(Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	if ( adjectives.find("potion_ally_damage") == adjectives.end() )
	{
		return defaultString;
	}

	if ( items[item.type].hasAttribute("POTION_TYPE_GOOD_EFFECT") 
		|| items[item.type].hasAttribute("POTION_TYPE_HEALING") /*item.doesPotionHarmAlliesOnThrown()*/ )
	{
		return adjectives["potion_ally_damage"]["no_harm_ally"];
	}
	else
	{
		return adjectives["potion_ally_damage"]["harm_ally"];
	}
#endif
}

std::string& ItemTooltips_t::getItemProficiencyName(int proficiency)
{
	if ( adjectives.find("proficiency_types") == adjectives.end() )
	{
		return defaultString;
	}

	switch ( proficiency )
	{
		case PRO_SWORD:
			return adjectives["proficiency_types"]["sword"];
		case PRO_AXE:
			return adjectives["proficiency_types"]["axe"];
		case PRO_MACE:
			return adjectives["proficiency_types"]["mace"];
		case PRO_POLEARM:
			return adjectives["proficiency_types"]["polearm"];
		case PRO_UNARMED:
			return adjectives["proficiency_types"]["unarmed"];
		case PRO_SHIELD:
			return adjectives["proficiency_types"]["shield"];
		case PRO_RANGED:
			return adjectives["proficiency_types"]["ranged"];
		case PRO_SORCERY:
			return adjectives["proficiency_types"]["magic"];
		default:
			return defaultString;
	}
}

std::string& ItemTooltips_t::getItemSlotName(ItemEquippableSlot slotname)
{
	switch ( slotname )
	{
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET:
			return adjectives["equipment_slot_types"]["amulet"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING:
			return adjectives["equipment_slot_types"]["ring"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE:
			return adjectives["equipment_slot_types"]["breastpiece"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM:
			return adjectives["equipment_slot_types"]["helm"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS:
			return adjectives["equipment_slot_types"]["boots"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES:
			return adjectives["equipment_slot_types"]["gloves"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK:
			return adjectives["equipment_slot_types"]["cloak"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK:
			return adjectives["equipment_slot_types"]["mask"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON:
			return adjectives["equipment_slot_types"]["mainhand"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD:
			return adjectives["equipment_slot_types"]["offhand"];
		default:
			break;
	}
	return adjectives["equipment_slot_types"]["unknown"];
}

std::string& ItemTooltips_t::getItemStatShortName(const char* attr)
{
    const std::string attribute = attr;
	if ( attribute == "STR" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "DEX" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "CON" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "INT" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "PER" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "CHR" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "AC" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemStatFullName(const char* attr)
{
    const std::string attribute = attr;
	if ( attribute == "STR" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "DEX" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "CON" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "INT" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "PER" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "CHR" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "AC" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemEquipmentEffectsForIconText(std::string& attribute)
{
	if ( adjectives["equipment_effects_icon_text"].find(attribute) != adjectives["equipment_effects_icon_text"].end() )
	{
		return adjectives["equipment_effects_icon_text"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemEquipmentEffectsForAttributesText(std::string& attribute)
{
	if ( adjectives["equipment_effects_attributes_text"].find(attribute) != adjectives["equipment_effects_attributes_text"].end() )
	{
		return adjectives["equipment_effects_attributes_text"][attribute];
	}
	return defaultString;
}

Sint32 getStatAttributeBonusFromItem(const int player, Item& item, std::string& attribute)
{
#ifndef EDITOR
	Sint32 stat = 0;
	bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
	if ( item.beatitude >= 0 || cursedItemIsBuff )
	{
		stat += items[item.type].attributes[attribute];
	}
	else if ( items[item.type].attributes[attribute] < 0 )
	{
		stat += items[item.type].attributes[attribute];
	}
	stat += (cursedItemIsBuff ? abs(item.beatitude) : item.beatitude);
	return stat;
#else
	return 0;
#endif
}

void ItemTooltips_t::formatItemIcon(const int player, std::string tooltipType, Item& item, std::string& str, int iconIndex, std::string& conditionalAttribute, Frame* parentFrame)
{
#ifndef EDITOR
	//auto itemTooltip = tooltips[tooltipType];
	static Stat itemDummyStat(0);
	static char buf[1024];
	memset(buf, 0, sizeof(buf));

	bool compendiumTooltip = false;
	bool compendiumTooltipIntro = false;
	if ( parentFrame && !strcmp(parentFrame->getName(), "compendium") )
	{
		compendiumTooltip = true;
		if ( intro )
		{
			compendiumTooltipIntro = true;
		}
	}

	if ( conditionalAttribute.find("magicstaff_") != std::string::npos )
	{
		if ( str == "" )
		{
			str = getSpellIconText(player, item, compendiumTooltipIntro);
		}
		return;
	}
	else if ( conditionalAttribute.find("SPELL_") != std::string::npos )
	{
		if ( conditionalAttribute == "SPELL_ICON_MANACOST" )
		{
			str = getCostOfSpellString(player, item);
		}
		else if ( conditionalAttribute == "SPELL_ICON_EFFECT" )
		{
			str = getSpellIconText(player, item, compendiumTooltipIntro);
		}
		return;
	}
	else if ( item.type == SPELL_ITEM && conditionalAttribute.find("spell_") != std::string::npos )
	{
		if ( auto spell = getSpellFromItem(player, &item, false) )
		{
			auto def = spellItems.find(spell->ID);
			if ( def != spellItems.end() )
			{
				if ( def->second.spellFormatTags.size() )
				{
					std::string result = getSpellIconFormatText(player, item, str, spell, iconIndex, compendiumTooltipIntro);
					if ( result != "" )
					{
						str = result;
					}
				}
			}
		}
		return;
	}
	else if ( conditionalAttribute.find("SPELLBOOK_") != std::string::npos )
	{
		if ( conditionalAttribute == "SPELLBOOK_SPELLINFO_LEARNED" )
		{
			if ( iconIndex == 1 )
			{
				if ( auto spell = getSpellFromID(getSpellIDFromSpellbook(item.type)) )
				{
					auto def = spellItems.find(spell->ID);
					if ( def != spellItems.end() )
					{
						if ( def->second.spellFormatTags.size() )
						{
							std::string result = getSpellIconFormatText(player, item, str, spell, iconIndex, compendiumTooltipIntro);
							if ( result != "" )
							{
								str = result;
							}
						}
					}
				}
			}
			else
			{
				str = getSpellIconText(player, item, compendiumTooltipIntro);
				if ( itemCategory(&item) == SPELLBOOK )
				{
					if ( auto spell = getSpellFromID(getSpellIDFromSpellbook(item.type)) )
					{
						if ( iconIndex < ItemTooltips.spellItems[spell->ID].spellbookItemIconPaddingLines.size() )
						{
							for ( int i = 0; i < ItemTooltips.spellItems[spell->ID].spellbookItemIconPaddingLines[iconIndex]; ++i )
							{
								str += '\n';
							}
						}
					}
				}
			}
			return;
		}
		else if ( conditionalAttribute == "SPELLBOOK_SPELLINFO_UNLEARNED"
			|| conditionalAttribute == "SPELLBOOK_SPELLINFO_TOME" )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( spell )
			{
				snprintf(buf, sizeof(buf), str.c_str(), spell->getSpellName());
			}
		}
		else if ( conditionalAttribute == "SPELLBOOK_CAST_BONUS"
			&& items[item.type].hasAttribute(conditionalAttribute) )
		{
			int spellBookBonusPercent = 0;
			spellBookBonusPercent += getSpellbookBonusPercent(
				compendiumTooltipIntro ? nullptr : players[player]->entity, 
				compendiumTooltipIntro ? nullptr : stats[player], &item);
			spellBookBonusPercent *= ((items[item.type].attributes["SPELLBOOK_CAST_BONUS"]) / 100.0);

			int spellID = itemCategory(&item) == SPELLBOOK ? getSpellIDFromSpellbook(item.type) : item.getTomeSpellID();
			if ( spellItems.find(spellID) == spellItems.end() )
			{
				return;
			}
			SpellItemTypes spellType = spellItems[spellID].spellType;

			if ( spellItems[spellID].spellTags.find(SPELL_TAG_BONUS_AS_EFFECT_POWER) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_pwr_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_pwr_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_pwr_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_DAMAGE) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_damage_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_damage_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_damage_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_HEALING) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_heal_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_heal_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_heal_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_STATUS_EFFECT) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_duration_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_duration_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_duration_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_CURE) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].end() )
					{
						str += '\n';
					}
				}
				int bonusSeconds = 10 * ((spellBookBonusPercent * 4) / 100.0); // 25% = 10 seconds, 50% = 20 seconds.
				snprintf(buf, sizeof(buf), str.c_str(), bonusSeconds);
				str = buf;
				return;
			}

			snprintf(buf, sizeof(buf), str.c_str(), spellBookBonusPercent);
		}
		else
		{
			return;
		}
		str = buf;
		return;
	}
	else if ( tooltipType.find("tooltip_tool_bomb") != std::string::npos )
	{
		if ( conditionalAttribute.find("BOMB_ATK") != std::string::npos )
		{
			int baseDamage = items[item.type].attributes["BOMB_ATK"];
			int baseSpellDamage = 0;
			if ( item.type == TOOL_FREEZE_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr, compendiumTooltipIntro);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr, compendiumTooltipIntro);
			}
			int bonusFromPER = std::max(0, statGetPER(stats[player], players[player]->entity)) * items[item.type].attributes["BOMB_DMG_PER_MULT"];
			if ( compendiumTooltipIntro )
			{
				bonusFromPER = 0;
			}
			bonusFromPER /= 100;
			snprintf(buf, sizeof(buf), str.c_str(), baseDamage + bonusFromPER + baseSpellDamage);
			str = buf;
		}
		return;
	}
	else if ( item.type == TOOL_SENTRYBOT || item.type == TOOL_SPELLBOT
		|| item.type == TOOL_GYROBOT || item.type == TOOL_DUMMYBOT )
	{
		switch ( item.type )
		{
			case TOOL_SENTRYBOT: itemDummyStat.type = SENTRYBOT; break;
			case TOOL_SPELLBOT: itemDummyStat.type = SPELLBOT; break;
			case TOOL_GYROBOT: itemDummyStat.type = GYROBOT; break;
			case TOOL_DUMMYBOT: itemDummyStat.type = DUMMYBOT; break;
			default:
				break;
		}
		Entity::tinkerBotSetStats(&itemDummyStat, item.status);
		if ( conditionalAttribute.find("TINKERBOT_RANGEDATK") != std::string::npos )
		{
			int baseDamage = items[CROSSBOW].attributes["ATK"] + 1;
			int statDMG = itemDummyStat.PER + itemDummyStat.DEX;
			int skillBonus = SKILL_LEVEL_MASTER / 20;
			snprintf(buf, sizeof(buf), str.c_str(), baseDamage + statDMG + skillBonus);
			str = buf;
		}
		else if ( conditionalAttribute.find("TINKERBOT_MAGICATK") != std::string::npos )
		{
			int spellID = item.status == EXCELLENT ? SPELL_MAGICMISSILE : SPELL_FORCEBOLT;
			int spellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(spellID), nullptr, compendiumTooltipIntro);
			snprintf(buf, sizeof(buf), str.c_str(), spellDamage);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_HPAC" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.MAXHP, itemDummyStat.CON);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_HP" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.MAXHP);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_AC" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.CON);
			str = buf;
		}
		return;
	}
	else if ( conditionalAttribute == "EFF_MONOCLE_APPRAISE" )
	{
		int appraisalMult = 200;
		if ( item.beatitude > 0 )
		{
			appraisalMult = 400;
		}

		snprintf(buf, sizeof(buf), str.c_str(), appraisalMult);
		str = buf;
		return;
	}
	else if ( conditionalAttribute.compare("") != 0 && items[item.type].hasAttribute(conditionalAttribute) )
	{
		if ( conditionalAttribute == "STR" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "DEX" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "CON" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "INT" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "PER" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "CHR" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute.find("EFF_") != std::string::npos )
		{
			if ( conditionalAttribute == "EFF_PARALYZE" )
			{
				if ( item.type == TOOL_BEARTRAP )
				{
					snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes["EFF_PARALYZE"] / TICKS_PER_SECOND);
				}
			}
			else if ( conditionalAttribute == "EFF_LIFESAVING" )
			{
				if ( item.type == AMULET_LIFESAVING )
				{
					real_t restore = 0.5;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						restore += 0.5 * std::min(2, abs(item.beatitude));
					}
					restore *= 100.0;
					snprintf(buf, sizeof(buf), str.c_str(), (int)restore);
				}
			}
			else if ( conditionalAttribute == "EFF_BLEEDING" )
			{
				if ( item.type == TOOL_BEARTRAP )
				{
					snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes["EFF_BLEEDING"] / TICKS_PER_SECOND);
				}
			}
			else if ( conditionalAttribute == "EFF_REGENERATION" )
			{
				if ( item.type == RING_REGENERATION )
				{
					int healring = std::min(2, std::max((shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude) + 1, 1));
					snprintf(buf, sizeof(buf), str.c_str(), healring,
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else
				{
					int healring = 1;
					snprintf(buf, sizeof(buf), str.c_str(), healring,
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute == "EFF_MP_REGENERATION" )
			{
				int manaring = 1;
				snprintf(buf, sizeof(buf), str.c_str(), manaring,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_CLOAK_GUARDIAN1" )
			{
				real_t res = 0.75;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					res = std::max(0.25, 0.75 - 0.25 * (abs(item.beatitude)));
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(res * 100))),
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_CLOAK_GUARDIAN2" )
			{
				real_t res = 0.5;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					res = std::max(0.25, 0.5 - 0.25 * (abs(item.beatitude)));
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(res * 100))),
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_MARIGOLD1"
				|| conditionalAttribute == "EFF_MARIGOLD1_HUNGER" )
			{
				int foodMod = (svFlags & SV_FLAG_HUNGER) ? 5 : 3;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					foodMod += 3 * std::min(2, (int)abs(item.beatitude));
				}
				snprintf(buf, sizeof(buf), str.c_str(), foodMod,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_RING_RESOLVE" )
			{
				int effect = 10;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					effect += 10 * std::min(2, (int)abs(item.beatitude));
				}
				snprintf(buf, sizeof(buf), str.c_str(), effect,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_COLDRESIST" )
			{
				if ( item.type == HAT_WARM )
				{
					real_t coldMultiplier = 1.0;
					if ( !(players[player]->entity && players[player]->entity->effectShapeshift != NOTHING) )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							coldMultiplier = std::max(0.0, 0.5 - 0.25 * (abs(item.beatitude)));
						}
						else
						{
							coldMultiplier = 0.50;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(coldMultiplier * 100))),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute == "EFF_TECH_GOGGLES1" )
			{
				real_t speedFactor = 1.0;
				if ( item.type == MASK_TECH_GOGGLES )
				{
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						speedFactor = std::min(speedFactor + (1 + abs(item.beatitude)) * 0.5, 3.0);
					}
					else
					{
						speedFactor = speedFactor + 0.5;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((speedFactor - 1.0) * 100));
			}
			else if ( conditionalAttribute == "EFF_EYEPATCH" )
			{
				int bonus = 0;
				if ( item.type == MASK_EYEPATCH )
				{
					bonus = 2;
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						bonus += abs(item.beatitude);
					}
					else if ( item.beatitude < 0 )
					{
						bonus = 2;
					}
				}
				bonus = std::max(-6, std::min(bonus, 4));
				snprintf(buf, sizeof(buf), str.c_str(), bonus);
			}
			else if ( conditionalAttribute == "EFF_STRAFE" )
			{
				double backpedalMultiplier = 0.25;
				if ( item.type == HAT_BANDANA )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						backpedalMultiplier += 0.5 * (1 + abs(item.beatitude)) * 0.25;
						backpedalMultiplier = std::min(0.75, backpedalMultiplier);
					}
					else
					{
						backpedalMultiplier += 0.5 * (1 + abs(item.beatitude)) * 0.25;
						backpedalMultiplier = std::min(0.75, backpedalMultiplier);
					}
				}
				int multBackpedal = 100 * (backpedalMultiplier - 0.25);
				snprintf(buf, sizeof(buf), str.c_str(), multBackpedal);
			}
			else if ( conditionalAttribute == "EFF_MASK_GOLDEN" )
			{
				int equipmentBonus = 100;
				if ( item.type == MASK_GOLDEN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						equipmentBonus -= 50 * (1 + abs(item.beatitude));
						equipmentBonus = std::max(-50, equipmentBonus);
					}
					else
					{
						equipmentBonus -= 50 * (abs(item.beatitude));
						equipmentBonus = std::max(0, equipmentBonus);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100 - equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_PIPE" )
			{
				int chance = 0;
				if ( item.type == MASK_PIPE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance = std::min(25 + (10 * abs(item.beatitude)), 50);
					}
					else
					{
						chance = std::min(25 + (10 * abs(item.beatitude)), 50);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), chance);
			}
			else if ( conditionalAttribute == "EFF_HOOD_APPRENTICE" )
			{
				int chance = 0;
				if ( item.type == HAT_HOOD_APPRENTICE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance = std::min(30 + (10 * abs(item.beatitude)), 50);
					}
					else
					{
						chance = std::min(30 + (10 * abs(item.beatitude)), 50);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), chance);
			}
			else if ( conditionalAttribute == "EFF_HOOD_ASSASSIN" )
			{
				int bonus = 0;
				if ( item.type == HAT_HOOD_ASSASSIN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(4 + (2 * abs(item.beatitude)), 8);
					}
					else
					{
						bonus = 4;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), bonus);
			}
			else if ( conditionalAttribute == "EFF_HOOD_WHISPERS" )
			{
				/*int bonus = 0;
				if ( item.type == HAT_HOOD_WHISPERS )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(50 + (10 * abs(item.beatitude)), 100);
					}
					else
					{
						bonus = 50;
					}
				}*/

				int val = ((compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20)) + 2) * 2; // backstab dmg
				if ( skillCapstoneUnlocked(player, PRO_STEALTH) )
				{
					val *= 2;
				}

				real_t equipmentModifier = 0.0;
				real_t bonusModifier = 1.0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentModifier += (std::min(50 + (10 * abs(item.beatitude)), 100)) / 100.0;
				}
				else
				{
					equipmentModifier = 0.5;
					bonusModifier = 0.5;
				}
				val = ((val * equipmentModifier) * bonusModifier);

				snprintf(buf, sizeof(buf), str.c_str(), val);
			}
			else if ( conditionalAttribute == "EFF_THORNS" )
			{
				int dmg = 0;
				if ( item.type == MASK_MOUTHKNIFE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						dmg = (1 + abs(item.beatitude)) * 2;
					}
					else
					{
						dmg = -2 * (1 + abs(item.beatitude));
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), abs(dmg));
			}
			else if ( conditionalAttribute == "EFF_CHEF" )
			{
				real_t foodMult = 1.0;
				if ( item.type == HAT_CHEF )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						if ( svFlags & SV_FLAG_HUNGER )
						{
							foodMult += 0.2 + abs(item.beatitude) * 0.1;
						}
						else
						{
							foodMult += 0.5 + abs(item.beatitude) * 0.25;
						}
					}
					else
					{
						foodMult += 0.2;
					}
					foodMult = std::max(0.2, foodMult);
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)(foodMult * 100) - 100);
			}
			else if ( conditionalAttribute == "EFF_CHEF2" )
			{
				int chance = 0;
				if ( item.type == HAT_CHEF )
				{
					chance = 20;
					bool cursedChef = false;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance -= 5 * abs(item.beatitude);
						chance = std::max(10, chance);
					}
					else
					{
						chance -= 5 * abs(item.beatitude);
						chance = std::max(10, chance);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100.0 / chance);
			}
			else if ( conditionalAttribute == "EFF_INSPIRATION" )
			{
				int inspiration = 0;
				if ( item.type == HAT_LAURELS
					|| item.type == HAT_TURBAN
					|| item.type == HAT_CROWN )
				{
					if ( item.beatitude >= 0 )
					{
						inspiration = std::min(300, 25 + (item.beatitude * 25));
					}
					else if ( shouldInvertEquipmentBeatitude(stats[player]) )
					{
						inspiration = std::min(300, 25 + (abs(item.beatitude) * 25));
					}
					else
					{
						inspiration = 25;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), inspiration);
			}
			else if ( conditionalAttribute == "EFF_CELEBRATION" )
			{
				int hpMod = 0;
				if ( item.type == HAT_CROWN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						hpMod += std::min(50, ((20 + 10 * (abs(item.beatitude)))));
					}
					else
					{
						hpMod = 20;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), hpMod);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_REGEN" )
			{
				int regen = 0;
				if ( item.type == HAT_LAURELS )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						regen = 1 + abs(item.beatitude) * 1;
					}
					else
					{
						regen = 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 2 * regen);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_TRAPRESIST" )
			{
				int resist = 0;
				if ( item.type == HAT_TURBAN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						resist = std::min(100, 50 + abs(item.beatitude) * 25);
					}
					else
					{
						resist = 50;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), resist);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_DMGRESIST" )
			{
				int resist = 0;
				if ( item.type == HAT_CROWNED_HELM )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						resist = std::min(50, 20 + abs(item.beatitude) * 10);
					}
					else
					{
						resist = 20;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), resist);
			}
			else if ( conditionalAttribute == "EFF_SPRIG" )
			{
				real_t mult = 0.0;
				if ( item.type == MASK_GRASS_SPRIG )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						mult = std::min(1.25 + (0.25 * abs(item.beatitude)), 2.0);
					}
					else
					{
						mult = 1.25;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), ((1.0 / mult)));
			}
			else if ( conditionalAttribute == "EFF_SKILL_MELEE_STEEL" )
			{
				int equipmentBonus = 0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * 5);
				}
				else
				{
					equipmentBonus += 5;
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_SKILL_MELEE_ARTIFACT" )
			{
				int equipmentBonus = 0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * 10);
				}
				else
				{
					equipmentBonus += 10;
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_BOUNTY" )
			{
				int equipmentBonus = 0;
				if ( item.type == HAT_BOUNTYHUNTER )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						if ( abs(item.beatitude) >= 2 )
						{
							equipmentBonus += 2;
						}
						else
						{
							equipmentBonus += 1;
						}
					}
					else
					{
						equipmentBonus += 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_RANGED_DISTANCE" )
			{
				int dropOffModifier = 0;
				if ( item.type == HAT_BYCOCKET )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						dropOffModifier = std::min(3, 1 + abs(item.beatitude));
					}
					else
					{
						dropOffModifier = 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), dropOffModifier);
			}
			else if ( conditionalAttribute == "EFF_RANGED_FIRERATE" )
			{
				int equipmentBonus = 0;
				if ( item.type == HAT_BYCOCKET )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						equipmentBonus -= std::min(30, 10 + 10 * abs(item.beatitude));
					}
					else
					{
						equipmentBonus -= 30;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), -equipmentBonus);
			}
			else if ( conditionalAttribute.find("EFF_SKILL_") != std::string::npos )
			{
				int skill = std::stoi(conditionalAttribute.substr(strlen("EFF_SKILL_"), std::string::npos));
				int equipmentBonus = 0;
				if ( (skill == PRO_TRADING && item.type == MASK_GOLDEN)
					|| (skill == PRO_LEADERSHIP && item.type == HAT_PLUMED_CAP)
					|| (skill == PRO_RANGED && item.type == HAT_BOUNTYHUNTER)
					|| (skill == PRO_STEALTH && item.type == HAT_HOOD_WHISPERS)
					|| (skill == PRO_MYSTICISM && (item.type == HAT_CIRCLET))
					|| (skill == PRO_SORCERY && (item.type == HAT_CIRCLET_SORCERY))
					|| (skill == PRO_THAUMATURGY && (item.type == HAT_CIRCLET_THAUMATURGY))
					|| (skill == PRO_ALCHEMY && item.type == MASK_HAZARD_GOGGLES)
					|| ((skill == PRO_MYSTICISM || skill == PRO_SORCERY || skill == PRO_THAUMATURGY)
						&& itemTypeIsFoci(item.type)) )
				{
					if ( itemTypeIsFoci(item.type) )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							equipmentBonus = 10;
							equipmentBonus += 5 * std::min(10, abs(item.beatitude));
						}
						else
						{
							equipmentBonus = 10;
						}
					}
					else
					{
						int bonus = 10;
						if ( skill == PRO_MYSTICISM || skill == PRO_SORCERY || skill == PRO_THAUMATURGY )
						{
							bonus = 5;
						}
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * bonus);
						}
						else
						{
							equipmentBonus += bonus;
						}
					}
				}

				std::string skillName = Player::SkillSheet_t::getSkillNameFromID(skill);
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus, skillName.c_str());
			}
			else if ( conditionalAttribute == "EFF_BOULDER_RES" )
			{
				real_t mult = 1.0;
				if ( item.type == HAT_TOPHAT )
				{
					mult = 0.0;
				}
				else if ( item.type == HELM_MINING )
				{
					mult = 0.5;
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						mult -= 0.25 * abs(item.beatitude);
						mult = std::max(0.0, mult);
					}
					else
					{
						mult = 0.5;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100 - (int)(mult * 100));
			}
			else if ( conditionalAttribute.find("EFF_CAST_TOUCHSPEED_") != std::string::npos
				|| conditionalAttribute.find("EFF_CAST_TOUCH_") != std::string::npos )
			{
				real_t bonus = 0.0;
				if ( (item.type == ROBE_HEALER)
					|| (item.type == ROBE_WIZARD)
					|| (item.type == ROBE_CULTIST)
					|| (item.type == ROBE_MONK) )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(0.5, 0.2 + 0.1 * abs(item.beatitude));
					}
					else
					{
						bonus = 0.2;
					}
				}
				else if ( item.type == SHAWL )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(1.0, 0.35 + 0.35 * abs(item.beatitude));
					}
					else
					{
						bonus = 0.35;
					}
				}

				std::string skillName = "";
				if ( conditionalAttribute.find("SORCERY") != std::string::npos )
				{
					skillName = Player::SkillSheet_t::getSkillNameFromID(PRO_SORCERY);
				}
				else if ( conditionalAttribute.find("MYSTICISM") != std::string::npos )
				{
					skillName = Player::SkillSheet_t::getSkillNameFromID(PRO_MYSTICISM);
				}
				else if ( conditionalAttribute.find("THAUMATURGY") != std::string::npos )
				{
					skillName = Player::SkillSheet_t::getSkillNameFromID(PRO_THAUMATURGY);
				}
				if ( skillName != "" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						skillName.c_str());
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100));
				}
			}
			else if ( conditionalAttribute.find("EFF_PWR") != std::string::npos )
			{
				real_t bonus = 0.0;
				if ( conditionalAttribute == "EFF_PWR" )
				{
					if ( item.type == HAT_CIRCLET
						|| item.type == HAT_CIRCLET_SORCERY
						|| item.type == HAT_CIRCLET_THAUMATURGY
						|| item.type == HAT_CIRCLET_WISDOM )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.05 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.05;
						}
					}
					else if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else if ( conditionalAttribute == "EFF_PWR_DMG"
					|| conditionalAttribute == "EFF_PWR_SORCERY"
					|| conditionalAttribute == "EFF_PWR_MYSTICISM"
					|| conditionalAttribute == "EFF_PWR_THAUMATURGY" )
				{
					if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else if ( conditionalAttribute == "EFF_PWR_HEAL" )
				{
					if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute == "EFF_CHAIN_RESIST" || conditionalAttribute == "EFF_QUILT_RESIST"
				|| conditionalAttribute == "EFF_BONE_RESIST" || conditionalAttribute == "EFF_BANDIT_LEATHER" )
			{
				real_t base = 0.1;
				real_t bonus = 0.05;
				if ( items[item.type].item_slot == ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE )
				{
					base = 0.2;
				}
				if ( item.type == BANDIT_BREASTPIECE )
				{
					base = 0.15;
					real_t mod = 0.0;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						mod = Entity::getDamageTableEquipmentMod(*stats[player], item, base, bonus);
					}
					else
					{
						mod = base;
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(mod * 100));
				}
				else
				{
					real_t mod = 0.0;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						mod = Entity::getDamageTableEquipmentMod(*stats[player], item, base, bonus);
					}
					else
					{
						mod = base;
					}
					char tmp[128] = "";
					if ( ItemTooltips.templates["template_armor_resist_icon"].size() )
					{
						std::string skillnames = "";
						if ( conditionalAttribute == "EFF_CHAIN_RESIST" )
						{
							skillnames += getItemProficiencyName(PRO_SWORD);
							skillnames += "/";
							skillnames += getItemProficiencyName(PRO_AXE);
						}
						else if ( conditionalAttribute == "EFF_QUILT_RESIST" )
						{
							skillnames += getItemProficiencyName(PRO_POLEARM);
							skillnames += "/";
							skillnames += getItemProficiencyName(PRO_RANGED);
						}
						else if ( conditionalAttribute == "EFF_BONE_RESIST" )
						{
							skillnames += getItemProficiencyName(PRO_UNARMED);
							skillnames += "/";
							skillnames += getItemProficiencyName(PRO_MACE);
						}
						snprintf(tmp, sizeof(tmp), ItemTooltips.templates["template_armor_resist_icon"][0].c_str(),
							skillnames.c_str());
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(mod * 100),
						tmp);
				}
			}
			else if ( conditionalAttribute.find("EFF_FOCI_MAGE") != std::string::npos )
			{
				int spellID = getSpellIDFromFoci(item.type);
				if ( spellID != SPELL_NONE )
				{
					if ( auto spell = getSpellFromID(spellID) )
					{
						char buf[128];
						memset(buf, 0, sizeof(buf));
						snprintf(buf, sizeof(buf), str.c_str(), getSpellDamageOrHealAmount(player, spell, &item, compendiumTooltipIntro));
						str = buf;
					}
				}
				return;
			}
			else if ( conditionalAttribute == "EFF_INSTRUMENT_MP_COST" )
			{
				if ( itemTypeIsInstrument(item.type) )
				{
					snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes[conditionalAttribute]);
					str = buf;
					return;
				}
			}
			else if ( conditionalAttribute.find("EFF_INSTRUMENT") != std::string::npos )
			{
				Sint32 CHR = statGetCHR(stats[player], players[player]->entity);
				if ( compendiumTooltipIntro ) { CHR = 0; }
				Uint8 effectStrength = std::max(1, std::min(255, CHR + 1));
				int tier = 1;
				std::string tierString = "I";
				int nextCHR = Stat::kEnsembleBreakPointTier2;
				if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier4 )
				{
					nextCHR = 0;
					tierString = "IV";
					tier = 4;
				}
				else if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier3 )
				{
					nextCHR = Stat::kEnsembleBreakPointTier4;
					tierString = "III";
					tier = 3;

				}
				else if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier2 )
				{
					nextCHR = Stat::kEnsembleBreakPointTier3;
					tierString = "II";
					tier = 2;
				}

				int eff1_min = 0;
				int eff2_min = 0;
				int effTier_min = 0;
				int eff1 = 0;
				int eff2 = 0;
				int effTier = 0;
				if ( item.type == INSTRUMENT_FLUTE )
				{
					eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_1, effectStrength);
					eff2 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_2, effectStrength);
					effTier = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_TIER, effectStrength);

					eff1_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_1, 1);
					eff2_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_2, 1);
					effTier_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_TIER, 1);
				}
				else if ( item.type == INSTRUMENT_LUTE )
				{
					eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_1, effectStrength);
					eff2 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_2, effectStrength);
					effTier = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_TIER, effectStrength);

					eff1_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_1, 1);
					eff2_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_2, 1);
					effTier_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_TIER, 1);
				}
				else if ( item.type == INSTRUMENT_LYRE )
				{
					eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_1, effectStrength);
					eff2 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_2, effectStrength);
					effTier = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_TIER, effectStrength);

					eff1_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_1, 1);
					eff2_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_2, 1);
					effTier_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_TIER, 1);
				}
				else if ( item.type == INSTRUMENT_HORN )
				{
					eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_1, effectStrength);
					eff2 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_2, effectStrength);
					effTier = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_TIER, effectStrength);

					eff1_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_1, 1);
					eff2_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_2, 1);
					effTier_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_TIER, 1);
				}
				else if ( item.type == INSTRUMENT_DRUM )
				{
					eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_1, effectStrength);
					eff2 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_2, effectStrength);
					effTier = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_TIER, effectStrength);

					eff1_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_1, 1);
					eff2_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_2, 1);
					effTier_min = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_TIER, 1);
				}

				snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), eff1_min, eff1, effTier_min, effTier);
			}
			else if ( conditionalAttribute.find("EFF_FOCI_") != std::string::npos )
			{
				Sint32 CHR = statGetCHR(stats[player], players[player]->entity);
				if ( compendiumTooltipIntro ) { CHR = 0; }
				int tier = 1;
				std::string tierString = "I";
				if ( CHR >= 3 && CHR <= 7 )
				{
					tier = 1;
				}
				else if ( CHR >= 8 && CHR <= 14 )
				{
					tierString = "II";
					tier = 2;
				}
				else if ( CHR >= 15 && CHR <= 29 )
				{
					tierString = "III";
					tier = 3;
				}
				else if ( CHR >= 30 )
				{
					tierString = "IV";
					tier = 4;
				}

				if ( conditionalAttribute == "EFF_FOCI_PEACE" )
				{
					int effect = getSpellEffectDurationSecondaryFromID(SPELL_FOCI_LIGHT_PEACE, nullptr, nullptr, nullptr);
					int effectMax = effect - (tier - 1) * getSpellDamageSecondaryFromID(SPELL_FOCI_LIGHT_PEACE, nullptr, nullptr, nullptr);
					snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), effect / TICKS_PER_SECOND, effectMax / TICKS_PER_SECOND);
				}
				else if ( conditionalAttribute == "EFF_FOCI_PURITY" )
				{
					int effect = getSpellEffectDurationSecondaryFromID(SPELL_FOCI_LIGHT_PURITY, nullptr, nullptr, nullptr);
					int effectMax = effect - (tier - 1) * getSpellDamageSecondaryFromID(SPELL_FOCI_LIGHT_PURITY, nullptr, nullptr, nullptr);
					snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), effect / TICKS_PER_SECOND, effectMax / TICKS_PER_SECOND);
				}
				else if ( conditionalAttribute == "EFF_FOCI_JUSTICE" )
				{
					int effect = getSpellEffectDurationSecondaryFromID(SPELL_FOCI_LIGHT_JUSTICE, nullptr, nullptr, nullptr);
					int effectMax = effect + (tier - 1) * getSpellDamageSecondaryFromID(SPELL_FOCI_LIGHT_JUSTICE, nullptr, nullptr, nullptr);
					snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), effect, effectMax);
				}
				else if ( conditionalAttribute == "EFF_FOCI_SANCTUARY" )
				{
					int effect = getSpellDamageFromID(SPELL_FOCI_LIGHT_SANCTUARY, nullptr, nullptr, nullptr);
					int effectMax = effect + (tier - 1) * getSpellDamageSecondaryFromID(SPELL_FOCI_LIGHT_SANCTUARY, nullptr, nullptr, nullptr);
					snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), effect, effectMax);
				}
				else if ( conditionalAttribute == "EFF_FOCI_PROVIDENCE" )
				{
					int effect = getSpellEffectDurationSecondaryFromID(SPELL_FOCI_LIGHT_PROVIDENCE, nullptr, nullptr, nullptr);
					int effectMax = effect + (tier - 1) * getSpellDamageSecondaryFromID(SPELL_FOCI_LIGHT_PROVIDENCE, nullptr, nullptr, nullptr);
					snprintf(buf, sizeof(buf), str.c_str(), tierString.c_str(), effect, effectMax);
				}
				else
				{
					return;
				}
			}
			else if ( conditionalAttribute.find("EFF_ARTIFACT_") != std::string::npos )
			{
				real_t amount = 0.0;
				real_t percent = getArtifactWeaponEffectChance(item.type, *stats[player], &amount);
				if ( conditionalAttribute == "EFF_ARTIFACT_SWORD" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_AXE" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, (amount * 100) - 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_MACE" )
				{
					amount = amount / MAGIC_REGEN_TIME;
					snprintf(buf, sizeof(buf), str.c_str(), 100 * amount);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_SPEAR" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_BOW" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount);
				}
			}
			else if ( conditionalAttribute == "EFF_FLAIL" )
			{
				int percent = (50 + (compendiumTooltipIntro ? 0 : stats[player]->getModifiedProficiency(PRO_MACE) * 25 / 100.0)) - 100;
				snprintf(buf, sizeof(buf), str.c_str(), percent);
			}
			else if ( conditionalAttribute == "EFF_SHILLELAGH" )
			{
				real_t variance = 20;
				real_t baseSkillModifier = 50.0; // 40-60 base
				real_t skillModifier = baseSkillModifier - (variance / 2) + (compendiumTooltipIntro ? 0 : stats[player]->getModifiedProficiency(PRO_MYSTICISM) / 2.0);

				int valueLow = skillModifier;
				int valueHigh = skillModifier + variance;
				valueHigh = std::min(100, valueHigh);

				snprintf(buf, sizeof(buf), str.c_str(), valueLow, valueHigh);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
		}
		else if ( conditionalAttribute == "AC" )
		{
			Sint32 AC = item.armorGetAC(stats[player]);
			snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName("AC").c_str());
		}
		else if ( conditionalAttribute == "FOCI_MP_COST" )
		{
			int mpCost = 0;
			int spellID = getSpellIDFromFoci(item.type);
			if ( spellID != SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					mpCost = getCostOfSpell(spell, compendiumTooltipIntro ? nullptr : players[player]->entity);
				}
			}

			snprintf(buf, sizeof(buf), str.c_str(), mpCost);
		}
		else
		{
			return;
		}
		str = buf;
		return;
	}

	if ( tooltipType.find("tooltip_armor") != std::string::npos
		|| tooltipType.find("tooltip_offhand") != std::string::npos
		|| tooltipType.find("tooltip_ring") != std::string::npos )
	{
		Sint32 AC = item.armorGetAC(stats[player]);
		snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName("AC").c_str());
	}
	else if ( tooltipType.find("tooltip_mace") != std::string::npos
		|| tooltipType.find("tooltip_sword") != std::string::npos
		|| tooltipType.find("tooltip_whip") != std::string::npos
		|| tooltipType.find("tooltip_polearm") != std::string::npos
		|| tooltipType.find("tooltip_thrown") != std::string::npos
		|| tooltipType.find("tooltip_boomerang") != std::string::npos
		|| tooltipType.find("tooltip_gem") != std::string::npos
		|| tooltipType.find("tooltip_ranged") != std::string::npos
		|| tooltipType.find("tooltip_quiver") != std::string::npos
		|| tooltipType.compare("tooltip_tool_pickaxe") == 0 
		|| tooltipType.compare("tooltip_magicstaff_scepter") == 0
		)
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.find("tooltip_axe") != std::string::npos )
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		atk += 1;
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.compare("tooltip_food_tin") == 0 )
	{
		std::string cookingMethod, protein, sides;
		item.foodTinGetDescription(cookingMethod, protein, sides);
		snprintf(buf, sizeof(buf), str.c_str(), protein.c_str());
	}
	else if ( tooltipType.find("tooltip_potion") != std::string::npos )
	{
		if ( items[item.type].hasAttribute("POTION_TYPE_HEALING") )
		{
			if ( item.type == POTION_HEALING || item.type == POTION_EXTRAHEALING || item.type == POTION_RESTOREMAGIC )
			{
				int healthVal = item.potionGetEffectHealth(players[player]->entity, stats[player]);

				if ( item.type == POTION_HEALING )
				{
					const int statBonus = compendiumTooltipIntro ? 0 : (2 * std::max(0, statGetCON(stats[player], players[player]->entity)));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_EXTRAHEALING )
				{
					const int statBonus = compendiumTooltipIntro ? 0 : (4 * std::max(0, statGetCON(stats[player], players[player]->entity)));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_RESTOREMAGIC )
				{
					const int statBonus = std::min(30, 2 * (compendiumTooltipIntro ? 0 : (std::max(0, statGetINT(stats[player], players[player]->entity)))));
					healthVal += statBonus;
				}

				snprintf(buf, sizeof(buf), str.c_str(), healthVal);
			}
			else if ( item.type == POTION_BOOZE )
			{
				if ( iconIndex == 1 )
				{
					auto oldBeatitude = item.beatitude;
					item.beatitude = std::max((Sint16)0, item.beatitude);
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND, 
						item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
					item.beatitude = oldBeatitude;
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth(players[player]->entity, stats[player]));
				}
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth(players[player]->entity, stats[player]));
			}
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_DMG") )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDamage(players[player]->entity, stats[player]));
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_GOOD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND, 
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_BAD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND,
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
	}
	else if ( tooltipType.find("tooltip_tool_beartrap") != std::string::npos )
	{
		const int atk = 10 + 3 * (item.status + item.beatitude);
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.find("tooltip_jewel") != std::string::npos )
	{
		int tier = std::max(DECREPIT, item.status);
		snprintf(buf, sizeof(buf), str.c_str(), adjectives["jewel_levels"][std::to_string(tier)].c_str(), getItemStatusAdjective(item.type, item.status).c_str());
	}
	else if ( tooltipType.find("tooltip_scroll") != std::string::npos )
	{
		if ( conditionalAttribute == "SCROLL_LABEL" )
		{
			if ( compendiumTooltip )
			{
				snprintf(buf, sizeof(buf), str.c_str(), "???"); // hide labels in compendium
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), item.getScrollLabel());
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}
	str = buf;
#endif
}

void ItemTooltips_t::formatItemDescription(const int player, std::string tooltipType, Item& item, std::string& str)
{
	if ( tooltipType.find("tooltip_spell_") != std::string::npos )
	{
		str = getSpellDescriptionText(player, item);
	}
	else if ( item.type == TOOL_SENTRYBOT || item.type == TOOL_SPELLBOT
		|| item.type == TOOL_GYROBOT || item.type == TOOL_DUMMYBOT )
	{
		if ( item.status == BROKEN )
		{
			str = "";
			for ( auto it = ItemTooltips.templates["template_tinkerbot_broken_description"].begin();
				it != ItemTooltips.templates["template_tinkerbot_broken_description"].end(); ++it )
			{
				str += *it;
				if ( std::next(it) != ItemTooltips.templates["template_tinkerbot_broken_description"].end() )
				{
					str += '\n';
				}
			}
		}
	}
	return;
}

void ItemTooltips_t::formatItemDetails(const int player, std::string tooltipType, Item& item, std::string& str, std::string detailTag, Frame* parentFrame)
{
#ifndef EDITOR
	if ( !stats[player] )
	{
		str = "";
		return;
	}
	if ( players[player] && !players[player]->isLocalPlayer() )
	{
		str = "";
		return;
	}

	bool compendiumTooltip = false;
	bool compendiumTooltipIntro = false;
	if ( parentFrame && !strcmp(parentFrame->getName(), "compendium") )
	{
		compendiumTooltip = true;
		if ( intro )
		{
			compendiumTooltipIntro = true;
		}
	}

	//auto itemTooltip = ItemTooltips.tooltips[tooltipType];

	memset(buf, 0, sizeof(buf));

	if ( tooltipType.find("tooltip_armor") != std::string::npos 
		|| tooltipType.find("tooltip_offhand") != std::string::npos
		|| tooltipType.find("tooltip_amulet") != std::string::npos
		|| tooltipType.find("tooltip_ring") != std::string::npos )
	{
		if ( detailTag.compare("armor_base_ac") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("AC") ? items[item.type].attributes["AC"] : 0	);
		}
		else if ( detailTag.compare("armor_shield_bonus") == 0 )
		{
			bool excludeSkill = compendiumTooltipIntro;
			if ( tooltipType.find("tooltip_offhand") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					stats[player]->getActiveShieldBonus(false, excludeSkill, &item),
					getItemProficiencyName(PRO_SHIELD).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					stats[player]->getPassiveShieldBonus(false, excludeSkill),
					getItemProficiencyName(PRO_SHIELD).c_str(),
					stats[player]->getActiveShieldBonus(false, excludeSkill, &item),
					getItemProficiencyName(PRO_SHIELD).c_str());
			}
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude, 
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("shield_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_SHIELD) / 10);
			int durabilityBonus = skillLVL * 10;
			if ( itemCategory(&item) == ARMOR )
			{
				durabilityBonus *= 2;
			}
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("shield_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("knuckle_skill_modifier") == 0 )
		{
			int atk = compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(PRO_UNARMED) / 20)); // 0 - 5
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("knuckle_knockback_modifier") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				items[item.type].hasAttribute("KNOCKBACK") ? items[item.type].attributes["KNOCKBACK"] : 0);
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				(int)(((!compendiumTooltipIntro && stats[player]) ? statGetSTR(stats[player], players[player]->entity) : 0) * 100 * Entity::PlayerAttackMeleeStatFactor));
		}
		else if ( detailTag.compare("ring_unarmed_atk") == 0 )
		{
			int atk = 1 + (shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_UNARMED) / 20);
			int durabilityBonus = skillLVL * 20;
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("weapon_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("equipment_fragile_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("FRAGILE") ? -items[item.type].attributes["FRAGILE"] : 0);
		}
		else if ( detailTag.compare("equipment_stat_bonus") == 0 )
		{
			std::vector<std::string> statNames = { "STR", "DEX", "CON", "INT", "PER", "CHR" };
			int baseStatBonus = 0;
			int beatitudeStatBonus = 0;
			bool found = false;
			for ( auto& stat : statNames )
			{
				if ( items[item.type].hasAttribute(stat) )
				{
					found = true;
					baseStatBonus = items[item.type].attributes[stat];
					beatitudeStatBonus = getStatAttributeBonusFromItem(player, item, stat) - baseStatBonus;

					snprintf(buf, sizeof(buf), str.c_str(), baseStatBonus, stat.c_str(),
						beatitudeStatBonus, stat.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
					break;
				}
			}
			if ( !found )
			{
				return;
			}
		}
		else if ( detailTag.find("EFF_") != std::string::npos )
		{
			if ( adjectives["equipment_effects_attributes_text"].find(detailTag)
				== adjectives["equipment_effects_attributes_text"].end() )
			{
				return;
			}

			if ( detailTag == "EFF_FEATHER" )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					items[item.type].hasAttribute(detailTag) ? items[item.type].attributes["EFF_FEATHER"] : 0,
					getItemEquipmentEffectsForAttributesText(detailTag).c_str());
			}
			else if ( detailTag == "EFF_WARNING" )
			{
				int beatitude = shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude;
				int radius = std::max(3, 11 + 5 * beatitude);
				snprintf(buf, sizeof(buf), str.c_str(), radius, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else if ( detailTag == "EFF_HOOD_WHISPERS" )
			{
				//int val = (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20 + 2) * 2; // backstab dmg
				//if ( skillCapstoneUnlocked(player, PRO_STEALTH) )
				//{
				//	val *= 2;
				//}

				//real_t equipmentModifier = 0.0;
				//real_t bonusModifier = 1.0;
				//if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				//{
				//	equipmentModifier += (std::min(50 + (10 * abs(item.beatitude)), 100)) / 100.0;
				//}
				//else
				//{
				//	equipmentModifier = 0.5;
				//	bonusModifier = 0.5;
				//}
				//val = ((val * equipmentModifier) * bonusModifier);

				std::string skillName = Player::SkillSheet_t::getSkillNameFromID(PRO_STEALTH);
				snprintf(buf, sizeof(buf), str.c_str(), skillName.c_str());
			}
			else if ( detailTag == "EFF_SILKEN_BOW" )
			{
				int baseBonus = 5;
				int chanceBonus = 0;
				if ( item.type == HAT_SILKEN_BOW )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						baseBonus = 3 + 1 * std::min(5, abs(item.beatitude));
						if ( !compendiumTooltipIntro )
						{
							chanceBonus += std::min(10, (stats[player]->getModifiedProficiency(PRO_LEADERSHIP)
								+ std::max(0, 3 * statGetCHR(stats[player], players[player]->entity))) / 10);
						}

						if ( baseBonus + chanceBonus > 15 )
						{
							chanceBonus -= (baseBonus + chanceBonus) - 15;
						}
					}
					else
					{
						baseBonus = 1;
					}
				}

				std::string skillName = Player::SkillSheet_t::getSkillNameFromID(PRO_LEADERSHIP);
				snprintf(buf, sizeof(buf), str.c_str(), baseBonus,
					chanceBonus, skillName.c_str(), getItemStatShortName("CHR").c_str());
			}
			else
			{
				return;
			}
		}
		else if ( detailTag.compare("equipment_on_cursed_sideeffect") == 0
			|| detailTag.compare("ring_on_cursed_sideeffect") == 0 
			|| detailTag.compare("armor_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("artifact_armor_on_degraded") == 0 )
		{
			int statusModifier = std::max(DECREPIT, item.status) - 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_gem") != std::string::npos )
	{
		int proficiency = PRO_RANGED;
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("thrown_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				(int)(((!compendiumTooltipIntro && stats[player]) ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0) * 100 * Entity::PlayerAttackThrownStatFactor));
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 10);
			snprintf(buf, sizeof(buf), str.c_str(), skillLVL,
				getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag == "EFF_FEATHER" )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute(detailTag) ? items[item.type].attributes["EFF_FEATHER"] : 0,
				getItemEquipmentEffectsForAttributesText(detailTag).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_tool_beartrap") != std::string::npos )
	{
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("beartrap_degrade_on_use_cursed") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 100, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("beartrap_degrade_on_use") == 0 )
		{
			int chanceDegrade = 0;
			switch ( item.status )
			{
				case SERVICABLE:
					chanceDegrade = 4;
					break;
				case WORN:
					chanceDegrade = 10;
					break;
				case DECREPIT:
					chanceDegrade = 25;
					break;
				default:
					break;
			}
			snprintf(buf, sizeof(buf), str.c_str(),	chanceDegrade, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				item.beatitude * 3,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("on_degraded") == 0 )
		{
			int statusModifier = item.status * 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType == "tooltip_jewel" )
	{
		if ( detailTag == "jewel_limit" )
		{
			int allowedFollowers = std::min(8, std::max(4, compendiumTooltipIntro ? 0 : 2 * (stats[player]->getModifiedProficiency(PRO_LEADERSHIP) / 20)));
			snprintf(buf, sizeof(buf), str.c_str(), allowedFollowers);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_thrown") != std::string::npos
		|| tooltipType.find("tooltip_boomerang") != std::string::npos )
	{
		int proficiency = PRO_RANGED;
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("thrown_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				(int)(((!compendiumTooltipIntro && stats[player]) ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0) * 100 * Entity::PlayerAttackThrownStatFactor));
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 20);
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * thrownDamageSkillMultipliers[std::min(skillLVL, 5)] - 100),
				getItemProficiencyName(proficiency).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType == "tooltip_instrument" )
	{
		Entity* caster = compendiumTooltipIntro ? nullptr : players[player]->entity;
		Stat* myStats = compendiumTooltipIntro ? nullptr : stats[player];
		if ( detailTag == "armor_on_cursed_sideeffect" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag == "instrument_duration" )
		{
			Sint32 CHR = statGetCHR(stats[player], players[player]->entity);
			if ( compendiumTooltipIntro ) { CHR = 0; }

			int skillLVL = std::max(0, myStats ? myStats->getModifiedProficiency(PRO_APPRAISAL) : 0);
			int durationMinimum = TICKS_PER_SECOND;
			durationMinimum += (skillLVL / 20) * TICKS_PER_SECOND;

			int duration = 1;
			duration += 4 * skillLVL * (TICKS_PER_SECOND / 25);
			duration += (skillLVL / 20) * TICKS_PER_SECOND;

			int durationMax = TICKS_PER_SECOND * 60 * 5;
			duration = std::min(duration, durationMax);
			duration = std::max(duration, durationMinimum);

			int chargeTimeBase = 4 * TICKS_PER_SECOND;
			int chargeTime = std::max(TICKS_PER_SECOND / 2,
				chargeTimeBase - (TICKS_PER_SECOND / 20) * CHR);

			real_t chargeRatio = (100.0 * chargeTimeBase / (real_t)chargeTime) - 100.0;

			snprintf(buf, sizeof(buf), str.c_str(), 
				chargeTime / (real_t)TICKS_PER_SECOND, duration / (real_t)TICKS_PER_SECOND);
		}
		else if ( detailTag == "instrument_casting" )
		{
			Sint32 CHR = statGetCHR(stats[player], players[player]->entity);
			if ( compendiumTooltipIntro ) { CHR = 0; }
			Uint8 effectStrength = std::max(1, std::min(255, CHR + 1));
			int tier = 1;
			std::string tierString = "I";
			int nextCHR = Stat::kEnsembleBreakPointTier2;
			if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier4 )
			{
				nextCHR = 0;
				tier = 4;
			}
			else if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier3 )
			{
				nextCHR = Stat::kEnsembleBreakPointTier4;
				tier = 3;

			}
			else if ( effectStrength - 1 >= Stat::kEnsembleBreakPointTier2 )
			{
				nextCHR = Stat::kEnsembleBreakPointTier3;
				tier = 2;
			}

			char nextChrStr[64];
			if ( nextCHR > 0 )
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "%+d %s", nextCHR, ItemTooltips_t::getItemStatShortName("CHR").c_str());
			}
			else
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "N/A");
			}

			char nextChrStatStr[64];
			int nextCHRStat = effectStrength + 1;
			int eff1 = 0;
			if ( item.type == INSTRUMENT_FLUTE )
			{
				eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_1, effectStrength);
			}
			else if ( item.type == INSTRUMENT_LUTE )
			{
				eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_1, effectStrength);
			}
			else if ( item.type == INSTRUMENT_LYRE )
			{
				eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_1, effectStrength);
			}
			else if ( item.type == INSTRUMENT_HORN )
			{
				eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_1, effectStrength);
			}
			else if ( item.type == INSTRUMENT_DRUM )
			{
				eff1 = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_1, effectStrength);
			}
			for ( int i = effectStrength + 1; i < effectStrength + 10 && i <= 255; ++i )
			{
				int eff1_test = 0;
				if ( item.type == INSTRUMENT_FLUTE )
				{
					eff1_test = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_FLUTE_EFF_1, i);
				}
				else if ( item.type == INSTRUMENT_LUTE )
				{
					eff1_test = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LUTE_EFF_1, i);
				}
				else if ( item.type == INSTRUMENT_LYRE )
				{
					eff1_test = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_LYRE_EFF_1, i);
				}
				else if ( item.type == INSTRUMENT_HORN )
				{
					eff1_test = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_HORN_EFF_1, i);
				}
				else if ( item.type == INSTRUMENT_DRUM )
				{
					eff1_test = stats[player]->getEnsembleEffectBonus(Stat::ENSEMBLE_DRUM_EFF_1, i);
				}
				if ( eff1_test > eff1 )
				{
					nextCHRStat = std::max(1, i - 1);
					break;
				}
			}

			if ( nextCHRStat > 0 )
			{
				snprintf(nextChrStatStr, sizeof(nextChrStatStr), "%+d %s", nextCHRStat, ItemTooltips_t::getItemStatShortName("CHR").c_str());
			}
			else
			{
				snprintf(nextChrStatStr, sizeof(nextChrStatStr), "N/A");
			}

			snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes["EFF_INSTRUMENT_MP_COST"], 
				nextChrStatStr, nextChrStr);
		}
	}
	else if ( tooltipType == "tooltip_foci_light"
		|| tooltipType == "tooltip_foci_dark"
		|| tooltipType == "tooltip_foci_mage" )
	{
		Entity* caster = compendiumTooltipIntro ? nullptr : players[player]->entity;
		Stat* myStats = compendiumTooltipIntro ? nullptr : stats[player];
		if ( detailTag.compare("armor_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag == "spell_damage_bonus"
			|| detailTag == "EFF_FOCI_MAGE_ARCS" )
		{
			int spellID = getSpellIDFromFoci(item.type);
			if ( spellID != SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					int baseDamage = getSpellDamageOrHealAmount(-1, spell, nullptr, compendiumTooltipIntro);
					real_t bonusINTPercent = 100.0 * getBonusFromCasterOfSpellElement(
						compendiumTooltipIntro ? nullptr : players[player]->entity,
						compendiumTooltipIntro ? nullptr : stats[player],
						nullptr, spell ? spell->ID : SPELL_NONE, spell->skillID);
					real_t mult = 1.0;
					if ( detailTag == "EFF_FOCI_MAGE_ARCS" )
					{
						spellElement_t* element = nullptr;
						if ( spell->elements.first )
						{
							if ( element = (spellElement_t*)spell->elements.first->element )
							{
								if ( element->elements.first && element->elements.first->element )
								{
									element = (spellElement_t*)element->elements.first->element;
								}
							}
						}
						if ( element )
						{
							baseDamage = element->getDamageSecondary();
						}
						mult = getSpellPropertyFromID(spell_t::SPELLPROP_DAMAGE_SECONDARY_MULT, spell->ID, compendiumTooltipIntro ? nullptr : players[player]->entity, nullptr, nullptr);
						snprintf(buf, sizeof(buf), str.c_str(), (int)(baseDamage * (1.0 + bonusINTPercent * mult / 100.0)));
					}
					else
					{
						mult = getSpellPropertyFromID(spell_t::SPELLPROP_DAMAGE_MULT, spell->ID, compendiumTooltipIntro ? nullptr : players[player]->entity, nullptr, nullptr);
						std::string damageOrHealing = adjectives["spell_strings"]["damage"];
						/*std::string statName = getItemStatShortName("INT");
						if ( spell->skillID == PRO_MYSTICISM )
						{
							statName += '/';
							statName += getItemStatShortName("CHR");
						}
						else if ( spell->skillID == PRO_THAUMATURGY )
						{
							statName += '/';
							statName += getItemStatShortName("CON");
						}*/

						snprintf(buf, sizeof(buf), str.c_str(), damageOrHealing.c_str(), baseDamage, damageOrHealing.c_str(),
							bonusINTPercent * mult, damageOrHealing.c_str());
					}
				}
			}
		}
		else if ( detailTag == "mage_foci_bless_bonus" )
		{
			int bless = shouldInvertEquipmentBeatitude(myStats) ?
				abs(item.beatitude) :
				std::max((Sint16)0, item.beatitude);

			int bonus = bless * 5;
			snprintf(buf, sizeof(buf), str.c_str(), bonus, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag == "foci_mage_cast" )
		{
			int baseChargeTime = TICKS_PER_SECOND;
			int effectDuration = TICKS_PER_SECOND;
			int spellID = getSpellIDFromFoci(item.type);
			if ( spellID != SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					spellElement_t* element = nullptr;
					if ( spell->elements.first )
					{
						if ( element = (spellElement_t*)spell->elements.first->element )
						{
							if ( element->elements.first && element->elements.first->element )
							{
								element = (spellElement_t*)element->elements.first->element;
							}
						}
					}
					if ( element )
					{
						baseChargeTime = element->getChanneledManaDuration();
						effectDuration = element->duration;
					}
				}
			}
			snprintf(buf, sizeof(buf), str.c_str(), baseChargeTime / (real_t)TICKS_PER_SECOND, effectDuration / (real_t)TICKS_PER_SECOND);
		}
		else if ( detailTag == "foci_charge_text" )
		{
			int nextCHRBonus = 8;

			int tier = 1;
			Sint32 CHR = statGetCHR(myStats, caster);
			if ( CHR >= 3 && CHR <= 7 )
			{
				nextCHRBonus = 8;
			}
			else if ( CHR >= 8 && CHR <= 14 )
			{
				nextCHRBonus = 15;
			}
			else if ( CHR >= 15 && CHR <= 29 )
			{
				nextCHRBonus = 30;
			}
			else if ( CHR >= 30 )
			{
				nextCHRBonus = 0;
			}
			char nextChrStr[64];
			if ( nextCHRBonus > 0 )
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "%+d %s", nextCHRBonus, ItemTooltips_t::getItemStatShortName("CHR").c_str());
			}
			else
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "N/A");
			}
			int effectDuration = TICKS_PER_SECOND;
			int baseChargeTime = TICKS_PER_SECOND;
			int spellID = getSpellIDFromFoci(item.type);
			if ( spellID != SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					spellElement_t* element = nullptr;
					if ( spell->elements.first )
					{
						if ( element = (spellElement_t*)spell->elements.first->element )
						{
							if ( element->elements.first && element->elements.first->element )
							{
								element = (spellElement_t*)element->elements.first->element;
							}
						}
					}
					if ( element )
					{
						effectDuration = element->duration;
						baseChargeTime = element->getChanneledManaDuration();
					}
				}
			}

			snprintf(buf, sizeof(buf), str.c_str(), baseChargeTime / (real_t)TICKS_PER_SECOND, effectDuration / (real_t)TICKS_PER_SECOND, nextChrStr);
		}
		else if ( detailTag == "foci_mp_cost" )
		{
			int mpCost = 5;
			int spellID = getSpellIDFromFoci(item.type);
			int freeMPInterval1 = 0;
			int freeMPInterval2 = 0;
			int moveSpeed = 0;
			std::string skillName = "";
			int chargeRefire = 0;
			int nextCHRBonus = 8;
			if ( spellID != SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					mpCost = getCostOfSpell(spell, caster);
					int bless = shouldInvertEquipmentBeatitude(myStats) ?
						abs(item.beatitude) :
						std::max((Sint16)0, item.beatitude);

					mpCost = getSpellPropertyFromID(spell_t::SPELLPROP_FOCI_SECONDARY_MANA_COST, spellID,
						caster, myStats, caster);

					int tier = bless > 0 ? std::min(2, bless) : 0;
					Sint32 CHR = statGetCHR(myStats, caster);
					if ( CHR > 0 )
					{
						if ( CHR >= 3 && CHR <= 7 )
						{
							tier += 1;
							nextCHRBonus = 8;
						}
						else if ( CHR >= 8 && CHR <= 14 )
						{
							tier += 2;
							nextCHRBonus = 15;
						}
						else if ( CHR >= 15 && CHR <= 29 )
						{
							tier += 3;
							nextCHRBonus = 30;
						}
						else if ( CHR >= 30 && CHR <= 59 )
						{
							tier += 4;
							nextCHRBonus = 60;
						}
						else if ( CHR >= 60 )
						{
							tier += 5;
							nextCHRBonus = 0;
						}
					}
					tier = std::min(tier, 5);
					if ( tier == 1 ) { freeMPInterval1 = 1; freeMPInterval2 = 4; }
					if ( tier == 2 ) { freeMPInterval1 = 1; freeMPInterval2 = 3; }
					if ( tier == 3 ) { freeMPInterval1 = 1; freeMPInterval2 = 2; }
					if ( tier == 4 ) { freeMPInterval1 = 2; freeMPInterval2 = 3; }
					if ( tier == 5 ) { freeMPInterval1 = 3; freeMPInterval2 = 4; }

					moveSpeed = !myStats ? 0 : myStats->getModifiedProficiency(spell->skillID);
					skillName = Player::SkillSheet_t::getSkillNameFromID(spell->skillID);

					real_t modifier = std::min(100, !myStats ? 0 : myStats->getModifiedProficiency(spell->skillID)) / 100.0;

					spellElement_t* element = nullptr;
					if ( spell->elements.first )
					{
						if ( element = (spellElement_t*)spell->elements.first->element )
						{
							if ( element->elements.first && element->elements.first->element )
							{
								element = (spellElement_t*)element->elements.first->element;
							}
						}
					}

					chargeRefire = 0;
					if ( element )
					{
						int result = element->getChanneledManaDuration();
						int modifiedResult = getSpellPropertyFromID(spell_t::SPELLPROP_FOCI_REFIRE_TICKS, spellID,
							caster, myStats, caster);

						chargeRefire = (100.0 - (100.0 * modifiedResult / (real_t)result));
					}
				}
			}

			std::string fmt1 = "-";
			std::string fmt2 = "-";
			if ( freeMPInterval1 > 0 && freeMPInterval2 > 0 )
			{
				fmt1 = std::to_string(freeMPInterval1);
				fmt2 = std::to_string(freeMPInterval2);
			}
			char nextChrStr[64];
			if ( nextCHRBonus > 0 )
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "%+d %s", nextCHRBonus, ItemTooltips_t::getItemStatShortName("CHR").c_str());
			}
			else
			{
				snprintf(nextChrStr, sizeof(nextChrStr), "N/A");
			}
			snprintf(buf, sizeof(buf), str.c_str(),
				mpCost, fmt1.c_str(), fmt2.c_str(), 
				nextChrStr,
				skillName.c_str(), 
				chargeRefire, moveSpeed);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_mace") != std::string::npos
		|| tooltipType.find("tooltip_axe") != std::string::npos
		|| tooltipType.find("tooltip_sword") != std::string::npos
		|| tooltipType.find("tooltip_polearm") != std::string::npos
		|| tooltipType.find("tooltip_whip") != std::string::npos
		|| tooltipType.compare("tooltip_tool_pickaxe") == 0
		|| tooltipType.compare("tooltip_magicstaff_scepter") == 0
		|| tooltipType.find("tooltip_ranged") != std::string::npos
		|| tooltipType.find("tooltip_quiver") != std::string::npos )
	{
		int proficiency = PRO_SWORD;
		if ( tooltipType.find("tooltip_mace") != std::string::npos )
		{
			proficiency = PRO_MACE;
		}
		else if ( tooltipType.find("tooltip_axe") != std::string::npos )
		{
			proficiency = PRO_AXE;
		}
		else if ( tooltipType.find("tooltip_sword") != std::string::npos )
		{
			proficiency = PRO_SWORD;
		}
		else if ( tooltipType.find("tooltip_polearm") != std::string::npos )
		{
			proficiency = PRO_POLEARM;
		}
		else if ( tooltipType.find("tooltip_ranged") != std::string::npos
			|| tooltipType.compare("tooltip_whip") == 0 )
		{
			proficiency = PRO_RANGED;
		}

		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			if ( proficiency == PRO_AXE )
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] + 1 : 0);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
			}
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude, 
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("on_degraded") == 0 )
		{
			int statusModifier = item.status - 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("artifact_weapon_on_degraded") == 0 )
		{
			int statusModifier = (item.status - 3) * 2;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("weapon_skill_modifier") == 0 )
		{
			if ( proficiency == PRO_POLEARM )
			{
				//int weaponEffectiveness = -8 + (stats[player]->PROFICIENCIES[proficiency] / 3); // -8% to +25%
				int weaponEffectiveness = -25 + compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(proficiency) / 2)); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
			else
			{
				int weaponEffectiveness = -25 + compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(proficiency) / 2)); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
		}
		else if ( detailTag.compare("weapon_skill_modifier_range") == 0 )
		{
			real_t variance = 20;
			real_t baseSkillModifier = 50.0; // 40-60 base
			ItemType itemType = item.type;
			Entity::setMeleeDamageSkillModifiers(compendiumTooltipIntro ? nullptr : players[player]->entity, 
				nullptr, proficiency, baseSkillModifier, variance, &itemType);
			real_t lowest = baseSkillModifier - (variance / 2) + (compendiumTooltipIntro ? 0 : stats[player]->getModifiedProficiency(proficiency) / 2.0);
			lowest = std::min(100.0, std::max(0.0, lowest));
			real_t highest = std::min(100.0, lowest + variance);

			snprintf(buf, sizeof(buf), str.c_str(), (int)lowest, (int)highest, getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			if ( item.type == TOOL_WHIP )
			{
				int atk = (stats[player] ? statGetDEX(stats[player], players[player]->entity) : 0);
				atk += (stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
				atk = std::min(atk / 2, atk);
				if ( compendiumTooltipIntro )
				{
					atk = 0;
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)(atk * 100 * Entity::PlayerAttackMeleeStatFactor));
			}
			else if ( item.type == MAGICSTAFF_SCEPTER )
			{
				int atk = (stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
				atk = std::min(atk / 2, atk);
				snprintf(buf, sizeof(buf), str.c_str(), (int)(atk * 100 * Entity::PlayerAttackMeleeStatFactor));
			}
			else if ( item.type == RAPIER )
			{
				int atk = (stats[player] ? statGetDEX(stats[player], players[player]->entity) : 0);
				snprintf(buf, sizeof(buf), str.c_str(), (int)(atk * 100 * Entity::PlayerAttackMeleeStatFactor));
			}
			else if ( proficiency == PRO_RANGED )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					(int)(((!compendiumTooltipIntro && stats[player]) ? statGetDEX(stats[player], players[player]->entity) : 0) * 100 * Entity::PlayerAttackRangedStatFactor));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					(int)(((!compendiumTooltipIntro && stats[player]) ? statGetSTR(stats[player], players[player]->entity) : 0) * 100 * Entity::PlayerAttackMeleeStatFactor));
			}
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 20);
			int durabilityBonus = skillLVL * 20;
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_bonus_exp") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				items[item.type].hasAttribute("BONUS_SKILL_EXP") ? items[item.type].attributes["BONUS_SKILL_EXP"] : 0,
				getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("equipment_fragile_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("FRAGILE") ? -items[item.type].attributes["FRAGILE"] : 0);
		}
		else if ( detailTag.compare("weapon_ranged_armor_pierce") == 0
			|| detailTag.compare("whip_base_effects") == 0 )
		{
			int statChance = std::min(std::max((stats[player] ? statGetPER(stats[player], players[player]->entity) : 0), 0), 50); // 0 to 50 value.
			if ( compendiumTooltipIntro )
			{
				statChance = 0;
			}
			statChance += (items[item.type].hasAttribute("ARMOR_PIERCE") ? items[item.type].attributes["ARMOR_PIERCE"] : 0);
			snprintf(buf, sizeof(buf), str.c_str(), statChance);
		}
		else if ( detailTag.compare("weapon_ranged_quiver_augment") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemSlotName(ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD).c_str());
		}
		else if ( detailTag.compare("weapon_ranged_rate_of_fire") == 0 )
		{
			int rof = (items[item.type].hasAttribute("RATE_OF_FIRE") ? items[item.type].attributes["RATE_OF_FIRE"] : 0);
			if ( rof > 0 )
			{
				rof -= 100;
				rof *= -1;
				snprintf(buf, sizeof(buf), str.c_str(), rof);
			}
		}
		else if ( detailTag == "spell_damage_bonus" )
		{
			if ( item.type == MAGICSTAFF_SCEPTER )
			{
				if ( detailTag.compare("spell_damage_bonus") == 0 )
				{
					spell_t* spell = getSpellFromID(SPELL_SCEPTER_BLAST);
					if ( !spell ) { return; }

					int baseDamage = getSpellDamageOrHealAmount(-1, spell, nullptr, compendiumTooltipIntro);

					real_t mult = getSpellPropertyFromID(spell_t::SPELLPROP_DAMAGE_MULT, spell->ID, compendiumTooltipIntro ? nullptr : players[player]->entity, nullptr, nullptr);

					real_t bonusINTPercent = 100.0 * getBonusFromCasterOfSpellElement(
						compendiumTooltipIntro ? nullptr : players[player]->entity,
						compendiumTooltipIntro ? nullptr : stats[player],
						nullptr, spell ? spell->ID : SPELL_NONE, spell->skillID);

					std::string damageOrHealing = adjectives["spell_strings"]["damage"];
					std::string statName = getItemStatShortName("INT");
					if ( spell->skillID == PRO_MYSTICISM )
					{
						statName += '/';
						statName += getItemStatShortName("CHR");
					}
					else if ( spell->skillID == PRO_THAUMATURGY )
					{
						statName += '/';
						statName += getItemStatShortName("CON");
					}

					snprintf(buf, sizeof(buf), str.c_str(), damageOrHealing.c_str(), baseDamage, damageOrHealing.c_str(),
						bonusINTPercent * mult, damageOrHealing.c_str());
				}
			}
		}
		else if ( detailTag == "weapon_parry_ac" )
		{
			Entity* my = compendiumTooltipIntro ? nullptr : players[player]->entity;
			Stat* myStats = compendiumTooltipIntro ? nullptr : stats[player];
			int parryBonus = Stat::getParryingACBonus(myStats, &item, true, false, proficiency);
			snprintf(buf, sizeof(buf), str.c_str(), parryBonus);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_potion") != std::string::npos )
	{
		if ( detailTag.compare("default") == 0 || detailTag.compare("potion_additional_effects") == 0 )
		{
			return;
		}
		else if ( detailTag.compare("potion_polymorph_duration") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / (60 * TICKS_PER_SECOND),
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / (60 * TICKS_PER_SECOND) );
		}
		else if ( detailTag.compare("potion_restoremagic_bonus") == 0 )
		{
			if ( stats[player] && statGetINT(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), std::min(30, 2 * std::max(0, statGetINT(stats[player], players[player]->entity))));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_healing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 2 * std::max(0, statGetCON(stats[player], players[player]->entity)));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_extrahealing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 4 * std::max(0, statGetCON(stats[player], players[player]->entity)));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_on_blessed") == 0 )
		{
			if ( item.type == POTION_CUREAILMENT )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					item.potionGetEffectDurationRandom(players[player]->entity, stats[player]) / TICKS_PER_SECOND, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else if ( item.type == POTION_WATER )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 20 * item.beatitude, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else
			{
				return;
			}
		}
		else if ( detailTag.compare("potion_on_cursed") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("alchemy_details") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemPotionAlchemyAdjective(player, item.type).c_str());
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.beatitude, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("potion_damage") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), BASE_THROWN_DAMAGE);
		}
		else if ( detailTag.compare("potion_multiplier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_ALCHEMY) / 20);
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * potionDamageSkillMultipliers[std::min(skillLVL, 5)] - 100), 
				getItemPotionHarmAllyAdjective(item).c_str());
		}
	}
	else if ( tooltipType.compare("tooltip_tool_lockpick") == 0 )
	{
		if ( detailTag.compare("lockpick_chestsdoors_unlock_chance") == 0 )
		{
			int chance = stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 2; // lockpick chests/doors
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) == SKILL_LEVEL_LEGENDARY )
			{
				chance = 100;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_chests_scrap_chance") == 0 )
		{
			int chance = std::min(100, stats[player]->getModifiedProficiency(PRO_LOCKPICKING) + 50);
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 10)))); // disarm arrow traps
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_BASIC )
			{
				chance = 0;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_automaton_disarm") == 0 )
		{
			int chance = 0;
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) >= SKILL_LEVEL_EXPERT )
			{
				chance = 100; // lockpick automatons
			}
			else
			{
				chance = (100 - 100 / (static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 20 + 1))); // lockpick automatons
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_skeletonkey") == 0 )
	{
		Sint32 PER = statGetPER(
			compendiumTooltipIntro ? nullptr : stats[player], 
			compendiumTooltipIntro ? nullptr : players[player]->entity);
		if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 10)))); // disarm arrow traps
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_BASIC )
			{
				chance = 0;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_decoy") == 0 )
	{
		if ( detailTag.compare("tool_decoy_range") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), decoyBoxRange);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_food") != std::string::npos )
	{
		if ( detailTag.compare("food_puke_chance") == 0 )
		{
			int pukeChance = item.foodGetPukeChance(nullptr);
			real_t chance = 100;
			if ( item.status == BROKEN )
			{
				chance = 100.0;
			}
			else if ( pukeChance == 100 )
			{
				chance = 0.0;
			}
			else
			{
				chance *= (1.0 / std::max(1, pukeChance));
			}
			snprintf(buf, sizeof(buf), str.c_str(), 
				static_cast<int>(chance), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("food_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_spellbook") != std::string::npos
		|| tooltipType.find("tooltip_tome") != std::string::npos )
	{
		if ( detailTag.compare("spellbook_cast_bonus") == 0 )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( !spell ) { return; }

			int intBonus = getSpellbookBaseINTBonus(
				compendiumTooltipIntro ? nullptr : players[player]->entity,
				compendiumTooltipIntro ? nullptr : stats[player],
				spell->skillID);
			real_t mult = ((items[item.type].attributes["SPELLBOOK_CAST_BONUS"]) / 100.0);
			intBonus *= mult;
			int beatitudeBonus = (mult * getSpellbookBonusPercent(
				compendiumTooltipIntro ? nullptr : players[player]->entity, 
				compendiumTooltipIntro ? nullptr : stats[player], &item)) - intBonus;

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
			}
			else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_CURE)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["duration"];
			}
			else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["duration"];
			}
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_BONUS_AS_EFFECT_POWER)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["effect"];
			}

			std::string statName = getItemStatShortName("INT");
			if ( spell->skillID == PRO_MYSTICISM )
			{
				statName += '/';
				statName += getItemStatShortName("CHR");
			}
			else if ( spell->skillID == PRO_THAUMATURGY )
			{
				statName += '/';
				statName += getItemStatShortName("CON");
			}

			snprintf(buf, sizeof(buf), str.c_str(),
				intBonus, damageOrHealing.c_str(), statName.c_str(),
				beatitudeBonus, damageOrHealing.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("spellbook_cast_success") == 0 )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( !spell ) { return; }

			int spellcastingAbility = getSpellcastingAbilityFromUsingSpellbook(spell, players[player]->entity, stats[player]);
			int chance = ((10 - (spellcastingAbility / 10)) * 20 / 3.0); // 33% after rolling to fizzle, 66% success
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spellbook_extramana_chance") == 0 )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( !spell ) { return; }

			int spellcastingAbility = getSpellcastingAbilityFromUsingSpellbook(spell, players[player]->entity, stats[player]);
			int chance = (10 - (spellcastingAbility / 10)) * 10;
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spellbook_magic_requirement") == 0 )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( !spell ) { return; }

			int skillLVL = std::min(100, stats[player]->getModifiedProficiency(spell->skillID) + statGetINT(stats[player], players[player]->entity));
			if ( !playerLearnedSpellbook(player, &item) && (spell && spell->difficulty > skillLVL) )
			{
				str.insert((size_t)0, 1, '^'); // red line character
			}

			if ( spell )
			{
				snprintf(buf, sizeof(buf), str.c_str(), spell->difficulty, getProficiencyLevelName(spell->difficulty).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0, getProficiencyLevelName(0).c_str());
			}
		}
		else if ( detailTag.compare("spellbook_magic_current") == 0 )
		{
			spell_t* spell = nullptr;
			if ( itemCategory(&item) == SPELLBOOK )
			{
				spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			}
			else if ( itemCategory(&item) == TOME_SPELL )
			{
				spell = getSpellFromID(item.getTomeSpellID());
			}
			if ( !spell ) { return; }

			int skillLVL = std::min(100, stats[player]->getModifiedProficiency(spell->skillID) + statGetINT(stats[player], players[player]->entity));
			if ( !playerLearnedSpellbook(player, &item) && (spell && spell->difficulty > skillLVL) )
			{
				str.insert((size_t)0, 1, '^'); // red line character
			}
			Sint32 INT = stats[player] ? statGetINT(stats[player], players[player]->entity) : 0;
			Sint32 skill = stats[player] ? stats[player]->getModifiedProficiency(spell->skillID) : 0;
			Sint32 total = std::min(SKILL_LEVEL_LEGENDARY, INT + skill);
			if ( str.find("%s") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(), Player::SkillSheet_t::getSkillNameFromID(spell->skillID, true).c_str(), INT + skill, getProficiencyLevelName(INT + skill).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), INT + skill, getProficiencyLevelName(INT + skill).c_str());
			}
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_spell_item") == 0 )
	{
		if ( detailTag == "spell_cast_time" )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			Entity* caster = compendiumTooltipIntro ? nullptr : players[player]->entity;
			Stat* casterStats = compendiumTooltipIntro ? nullptr : stats[player];
			real_t baseCastTime = spell->cast_time * 20;
			real_t modifiedCastTime = getSpellPropertyFromID(spell_t::SPELLPROP_MODIFIED_SPELL_CAST_TIME, spell->ID, caster, casterStats, nullptr) * 20;
			real_t diff = -(baseCastTime - modifiedCastTime);
			if ( abs(diff) < 0.001 )
			{
				diff = 0.0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), baseCastTime / (real_t)TICKS_PER_SECOND, (diff) / (real_t)TICKS_PER_SECOND);
		}
		else if ( detailTag == "spell_distance" )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			real_t dist = 0.0;
			real_t diff = 0.0;
			if ( spell->rangefinder != SpellRangefinderType::RANGEFINDER_NONE )
			{
				Entity* caster = compendiumTooltipIntro ? nullptr : players[player]->entity;
				Stat* casterStats = compendiumTooltipIntro ? nullptr : stats[player];
				dist = spell->distance;
				real_t modifiedDistance = getSpellPropertyFromID(spell_t::SPELLPROP_MODIFIED_DISTANCE, spell->ID, caster, casterStats, nullptr);
				diff = (modifiedDistance - dist);
				if ( abs(diff) < 0.001 )
				{
					diff = 0.0;
				}
			}
			snprintf(buf, sizeof(buf), str.c_str(), dist / 16.0, (diff) / 16.0);
		}
		else if ( detailTag.compare("spell_damage_bonus") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			//int totalDamage = getSpellDamageOrHealAmount(player, spell, nullptr);
			/*Sint32 oldINT = stats[player]->INT;
			Sint32 oldCHR = stats[player]->CHR;
			Sint32 oldCON = stats[player]->CON;
			stats[player]->INT = 0;
			stats[player]->CHR = 0;
			stats[player]->CON = 0;*/

			int baseDamage = getSpellDamageOrHealAmount(-1, spell, nullptr, compendiumTooltipIntro);

			real_t mult = getSpellPropertyFromID(spell_t::SPELLPROP_DAMAGE_MULT, spell->ID, compendiumTooltipIntro ? nullptr : players[player]->entity, nullptr, nullptr);

			/*real_t bonusEquipPercent = 100.0 * getBonusFromCasterOfSpellElement(
				compendiumTooltipIntro ? nullptr : players[player]->entity, 
				compendiumTooltipIntro ? nullptr : stats[player], 
				nullptr, spell ? spell->ID : SPELL_NONE, spell->skillID);

			stats[player]->INT = oldINT;
			stats[player]->CHR = oldCHR;
			stats[player]->CON = oldCON;*/

			real_t bonusINTPercent = 100.0 * getBonusFromCasterOfSpellElement(
				compendiumTooltipIntro ? nullptr : players[player]->entity,
				compendiumTooltipIntro ? nullptr : stats[player],
				nullptr, spell ? spell->ID : SPELL_NONE, spell->skillID);
			//bonusINTPercent -= bonusEquipPercent;

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
			}
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_BONUS_AS_EFFECT_POWER)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["effect"];
			}

			std::string statName = getItemStatShortName("INT");
			if ( spell->skillID == PRO_MYSTICISM )
			{
				statName += '/';
				statName += getItemStatShortName("CHR");
			}
			else if ( spell->skillID == PRO_THAUMATURGY )
			{
				statName += '/';
				statName += getItemStatShortName("CON");
			}

			if ( spell->ID == SPELL_HOLY_BEAM && templates["template_spell_damage_bonus_pwr_dual"].size() )
			{
				int damage = getSpellDamageFromID(SPELL_HOLY_BEAM, nullptr, nullptr, nullptr, 0.0, false);
				real_t damageMult = mult;
				int healing = getSpellDamageSecondaryFromID(SPELL_HOLY_BEAM, nullptr, nullptr, nullptr, 0.0, false);
				real_t healingMult = getSpellPropertyFromID(spell_t::SPELLPROP_DAMAGE_SECONDARY_MULT, spell->ID, compendiumTooltipIntro ? nullptr : players[player]->entity, nullptr, nullptr);
				snprintf(buf, sizeof(buf), templates["template_spell_damage_bonus_pwr_dual"][0].c_str(), adjectives["spell_strings"]["effect"].c_str(), damage, adjectives["spell_strings"]["damage"].c_str(),
					bonusINTPercent * damageMult, adjectives["spell_strings"]["damage"].c_str(),
					healing, adjectives["spell_strings"]["healing"].c_str(),
					bonusINTPercent * healingMult, adjectives["spell_strings"]["healing"].c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), damageOrHealing.c_str(), baseDamage, damageOrHealing.c_str(), 
					bonusINTPercent * mult, damageOrHealing.c_str());// , statName.c_str(), bonusEquipPercent* mult, damageOrHealing.c_str());
			}
		}
		else if ( detailTag.compare("spell_cast_success") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			int spellcastingAbility = std::min(std::max(0, stats[player]->getModifiedProficiency(spell->skillID)
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = ((100 - (spellcastingAbility)) / 3.0); // 33% after rolling to fizzle, 66% success
			if ( str.find("%s") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(), Player::SkillSheet_t::getSkillNameFromID(spell->skillID, true).c_str(), chance);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), chance);
			}
		}
		else if ( detailTag.compare("spell_cast_success1") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			if ( str.find("%s") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(), Player::SkillSheet_t::getSkillNameFromID(spell->skillID, true).c_str());
			}
		}
		else if ( detailTag.compare("spell_cast_success2") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			int spellcastingAbility = std::min(std::max(0, stats[player]->getModifiedProficiency(spell->skillID)
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = ((100 - (spellcastingAbility)) / 3.0); // 33% after rolling to fizzle, 66% success
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spell_extramana_chance") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			int spellcastingAbility = std::min(std::max(0, stats[player]->getModifiedProficiency(spell->skillID)
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = (10 - (spellcastingAbility / 10)) * 10;
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("attribute_spell_charm") == 0 )
		{
			int leaderChance = ((statGetCHR(stats[player], players[player]->entity) + 
				stats[player]->getModifiedProficiency(PRO_LEADERSHIP)) / 20) * 5;
			int intChance = (statGetINT(stats[player], players[player]->entity) * 2);
			if ( compendiumTooltipIntro )
			{
				leaderChance = 0;
				intChance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), intChance, leaderChance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_magicstaff") == 0 )
	{
		if ( detailTag.compare("magicstaff_charm_degrade_chance") == 0 )
		{
			int degradeChance = 100;
			if ( item.status > WORN )
			{
				degradeChance = 33;
			}
			snprintf(buf, sizeof(buf), str.c_str(), degradeChance, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("magicstaff_degrade_chance") == 0 )
		{
			int degradeChance = 33;
			snprintf(buf, sizeof(buf), str.c_str(), degradeChance);
		}
		else if ( detailTag.compare("attribute_spell_charm") == 0 )
		{
			int leaderChance = ((statGetCHR(stats[player], players[player]->entity) +
				stats[player]->getModifiedProficiency(PRO_LEADERSHIP)) / 20) * 10;
			if ( compendiumTooltipIntro )
			{
				leaderChance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), leaderChance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_scroll") == 0 )
	{
		if ( detailTag.compare("scroll_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_tool_bomb") != std::string::npos )
	{
		if ( detailTag.compare("tool_bomb_base_atk") == 0 )
		{
			int baseDmg = (items[item.type].hasAttribute("BOMB_ATK") ? items[item.type].attributes["BOMB_ATK"] : 0);
			int baseSpellDamage = 0;
			if ( item.type == TOOL_FREEZE_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr, compendiumTooltipIntro);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr, compendiumTooltipIntro);
			}
			snprintf(buf, sizeof(buf), str.c_str(), baseDmg + baseSpellDamage);
		}
		else if ( detailTag.compare("tool_bomb_per_atk") == 0 )
		{
			int perMult = (items[item.type].hasAttribute("BOMB_DMG_PER_MULT") ? items[item.type].attributes["BOMB_DMG_PER_MULT"] : 0);
			int perDmg = std::max(0, statGetPER(
				compendiumTooltipIntro ? nullptr : stats[player], 
				compendiumTooltipIntro ? nullptr : players[player]->entity)) * perMult / 100.0;
			snprintf(buf, sizeof(buf), str.c_str(), perDmg, perMult);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_spellbot") == 0 || tooltipType.compare("tooltip_tool_sentrybot") == 0
		|| tooltipType.compare("tooltip_tool_gyrobot") == 0 )
	{
		if ( detailTag.compare("tinkerbot_status_bonus") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("spellbot_rate_of_fire") == 0 )
		{
			real_t bow = 2.0;
			if ( item.type == TOOL_SPELLBOT )
			{
				if ( item.status >= EXCELLENT )
				{
					bow = 1.2;
				}
				else if ( item.status >= SERVICABLE )
				{
					bow = 1.5;
				}
				else if ( item.status >= WORN )
				{
					bow = 1.8;
				}
				else
				{
					bow = 2;
				}
			}
			real_t rof = fabs((1.0 - (2 / bow)) * 100);
			snprintf(buf, sizeof(buf), str.c_str(), (int)rof);
		}
		else if ( detailTag.compare("tinkerbot_turn_rate") == 0 )
		{
			real_t ratio = 64.0;
			if ( item.status >= EXCELLENT )
			{
				ratio = 2.0;
			}
			else if ( item.status >= SERVICABLE )
			{
				ratio = 4.0;
			}
			else if ( item.status >= WORN )
			{
				ratio = 16.0;
			}
			else
			{
				ratio = 64.0;
			}
			real_t turnRate = (64.0 / ratio) - 1.0;
			snprintf(buf, sizeof(buf), str.c_str(), (int)turnRate);
		}
		else if ( detailTag.compare("gyrobot_info_interact") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( detailTag == "duck_info" )
	{
		std::string ownerName = adjectives["duck_default"]["unknown"];
		std::string duckName = adjectives["duck_default"]["unknown"];
		int owner = item.getDuckPlayer();
		if ( owner >= 0 && owner < MAXPLAYERS && !compendiumTooltipIntro )
		{
			if ( !client_disconnected[owner] && stats[owner] )
			{
				ownerName = stats[owner]->name;
			}
		}
		if ( owner == player && !compendiumTooltipIntro )
		{
			for ( auto& duck : players[player]->mechanics.ducksInARow )
			{
				if ( duck.first == ((item.appearance % items[TOOL_DUCK].variations) / MAXPLAYERS) )
				{
					if ( duck.second >= 15 * 60 * TICKS_PER_SECOND )
					{
						if ( adjectives["duck_titles"].size() >= 1 )
						{
							int index = uniqueGameKey % adjectives["duck_titles"].size();
							index += 8 * player;
							if ( index >= adjectives["duck_titles"].size() )
							{
								index = index % adjectives["duck_titles"].size();
							}
							duckName = adjectives["duck_titles"][std::to_string(index)];
						}
					}
					break;
				}
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), ownerName.c_str(), duckName.c_str());
	}
	else
	{
		return;
	}
	str = buf;
#endif
}

void ItemTooltips_t::stripOutHighlightBracketText(std::string& str, std::string& bracketText)
{
	for ( auto it = str.begin(); it != str.end(); ++it )
	{
		bool foundBracket = false;
		if ( *it == '[' )
		{
			foundBracket = true;
		}
		else if ( *it == '+' )
		{
			auto next_it = std::next(it);
			if ( next_it != str.end() && *next_it == '[' )
			{
				foundBracket = true;
				bracketText += *it;
				// don't edit original string ---- *it = ' ';
				it = next_it;
			}
		}

		if ( foundBracket )
		{
			while ( *it != '\0' && *it != '\n' )
			{
				bracketText += *it;
				if ( *it == ']' )
				{
					// don't edit original string ---- *it = ' ';
					break;
				}
				// don't edit original string ---- *it = ' ';
				it = std::next(it);
			}
		}
		else if ( it != str.end() && *it != '\n' )
		{
			bracketText += ' ';
		}
		if ( *it == '\n' )
		{
			bracketText += '\n';
		}
	}
}

bool charIsWordSeparator(char c)
{
	if ( c == ' ' || c == '\n' || c == '\r' || c == '\0' )
	{
		return true;
	}
	return false;
}

void ItemTooltips_t::getWordIndexesItemDetails(void* field, std::string& str, std::string& highlightValues, std::string& positiveValues, std::string& negativeValues,
	std::map<int, Uint32>& highlightIndexes, std::map<int, Uint32>& positiveIndexes, std::map<int, Uint32>& negativeIndexes, ItemTooltip_t& tooltip)
{
	positiveIndexes.clear();
	negativeIndexes.clear();
	highlightIndexes.clear();
	((Field*)field)->clearWordsToHighlight();
	int wordIndex = 0;
	bool prevCharWasWordSeparator = false;
	int numLines = 0;
	for ( size_t c = 0; c < str.size(); ++c )
	{
		if ( str[c] == '\n' )
		{
			wordIndex = -1;
			++numLines;
			prevCharWasWordSeparator = true;
			continue;
		}

		if ( prevCharWasWordSeparator && !charIsWordSeparator(str[c]) )
		{
			++wordIndex;
			prevCharWasWordSeparator = false;
			if ( !(c + 1 < str.size() && charIsWordSeparator(str[c+1])) )
			{
				continue;
			}
		}

		if ( charIsWordSeparator(str[c]) )
		{
			prevCharWasWordSeparator = true;
		}
		else
		{
			prevCharWasWordSeparator = false;
			if ( c < positiveValues.size() )
			{
				if ( positiveValues[c] == str[c] )
				{
					positiveIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
			if ( c < negativeValues.size() )
			{
				if ( negativeValues[c] == str[c] )
				{
					negativeIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
			if ( c < highlightValues.size() )
			{
				if ( highlightValues[c] == str[c] )
				{
					highlightIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
		}
	}

	for ( auto& p : positiveIndexes )
	{
		Uint32 color = tooltip.positiveTextColor;
		((Field*)field)->addWordToHighlight(p.first, color);
		//messagePlayer(0, "Positives: %d", p.first);
	}
	for ( auto& n : negativeIndexes )
	{
		Uint32 color = tooltip.negativeTextColor;
		((Field*)field)->addWordToHighlight(n.first, color);
		//messagePlayer(0, "Negatives: %d", n.first);
	}
	for ( auto& h : highlightIndexes )
	{
		Uint32 color = tooltip.statusEffectTextColor;
		((Field*)field)->addWordToHighlight(h.first, color);
		//messagePlayer(0, "Highlights: %d", h.first);
	}
}

void ItemTooltips_t::stripOutPositiveNegativeItemDetails(std::string& str, std::string& positiveValues, std::string& negativeValues)
{
	size_t index = 0;
	for ( auto it = str.begin(); it != str.end(); )
	{
		int sign = 0;
		if ( *it == '+' )
		{
			sign = 1;
		}
		else if ( *it == '-' )
		{
			sign = -1;
		}

		if ( sign != 0 )
		{
			if ( std::next(it) != str.end() )
			{
				char peekCharacter = *(std::next(it));
				if ( !(peekCharacter >= '0' && peekCharacter <= '9') )
				{
					sign = 0; // don't highlight +text, only +0 numbers
				}
			}
		}

		if ( sign != 0 )
		{
			bool addSpace = false;
			while ( *it != '\0' && *it != ' ' && *it != '\n' )
			{
				if ( *it == '*' )
				{
					// replace with bullet symbol
					*it = '\x1E';
				}
				if ( sign > 0 )
				{
					positiveValues += *it;
					negativeValues += ' ';
				}
				else
				{
					negativeValues += *it;
					positiveValues += ' ';
				}
				// don't edit original string ---- *it = ' ';
				it = std::next(it);
				addSpace = true;

				if ( it == str.end() )
				{
					break;
				}
			}

			if ( addSpace )
			{
				positiveValues += ' ';
				negativeValues += ' ';
			}
		}
		else
		{
			bool addSpace = true;
			if ( *it == '(' )
			{
				if ( str.find("(?)", std::distance(str.begin(), it)) == std::distance(str.begin(), it) )
				{
					addSpace = false;
					positiveValues += ' ';
					negativeValues += "(?)";
					// don't edit original string ---- *it = ' ';
					for ( size_t i = 0; i < strlen("(?)") - 1; ++i )
					{
						positiveValues += ' ';
						it = std::next(it);
						// don't edit original string ---- *it = ' ';
					}
				}
			}
			else if ( *it == '[' )
			{
				// look for matching brace.
				while ( it != str.end() && *it != ']' && *it != ' ' && *it != '\0' && *it != ' ' && *it != '\n' )
				{
					if ( *it == '*' )
					{
						// replace with bullet symbol
						*it = '\x1E';
					}
					positiveValues += ' ';
					negativeValues += ' ';
					it = std::next(it);
				}
				if ( it != str.end() && (*it == '\0' || *it == '\n') )
				{
					addSpace = false;
				}
			}
			else if ( *it == '^' )
			{
				// cursed line
				it = str.erase(it); // skip the '^'
				while ( it != str.end() && *it != '\0' && *it != '\n' )
				{
					if ( *it == '*' )
					{
						// replace with bullet symbol
						*it = '\x1E';
					}
					positiveValues += ' ';
					negativeValues += *it;
					// don't edit original string ---- *it = ' ';
					it = std::next(it);
				}
				if ( it != str.end() && (*it == '\0' || *it == '\n') )
				{
					addSpace = false;
				}
			}
			else if ( *it == '*' )
			{
				// replace with bullet symbol
				*it = '\x1E';
			}
			else if ( *it == '\0' || *it == '\n' )
			{
				addSpace = false;
			}

			if ( addSpace )
			{
				positiveValues += ' ';
				negativeValues += ' ';
			}
		}

		if ( it != str.end() )
		{
			if ( *it == '\n' )
			{
				positiveValues += '\n';
				negativeValues += '\n';
			}
		}
		else
		{
			break;
		}

		++it;
		index = std::distance(str.begin(), it);
	}
}
#endif // !EDITOR
