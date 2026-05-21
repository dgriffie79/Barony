/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_compendium.cpp - Compendium (bestiary, items, magic, codex, world, events, achievements)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"


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









std::map<std::string, int> Compendium_t::Events_t::monsterUniqueIDLookup;
std::map<Compendium_t::EventTags, std::set<int>> Compendium_t::Events_t::eventMonsterLookup;


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