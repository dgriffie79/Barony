#pragma once

#include "main.h"
#include "game.hpp"
#include "stat.hpp"
#include "net.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "player.hpp"
#include "mod_tools.hpp"
#include "scores.hpp"
#include "interface/ui.hpp"

//#define DEBUG_ACHIEVEMENTS

/*-------------------------------------------------------------------------------

	achievementUnlocked

	Returns true if the given achievement has been unlocked this game,
	false otherwise

-------------------------------------------------------------------------------*/

bool achievementUnlocked(const char* achName)
{
	// check internal achievement record
	auto find = Compendium_t::achievements.find(achName);
	if ( find == Compendium_t::achievements.end() )
	{
		return false;
	}
	return find->second.unlocked;
}

/*-------------------------------------------------------------------------------

	steamAchievement

	Unlocks a steam achievement

-------------------------------------------------------------------------------*/

void steamAchievement(const char* achName)
{
#ifdef DEBUG_ACHIEVEMENTS
	static ConsoleVariable<bool> cvar_achievements_debug("/achievements_debug", false);
	if ( *cvar_achievements_debug )
	{
		messagePlayer(clientnum, MESSAGE_DEBUG, "%s", achName);
	}
#endif

	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		if ( !achievementObserver.bIsAchievementAllowedDuringTutorial(achName) )
		{
			return;
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED]
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS])
		{
			return;
		}
	}
	else
	{
		if ( !gameModeManager.allowsStatisticsOrAchievements(achName, -1) )
		{
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
			|| conductGameChallenges[CONDUCT_LIFESAVING]
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS]
			|| conductGameChallenges[CONDUCT_ASSISTANCE_CLAIMED] >= GenericGUIMenu::AssistShrineGUI_t::achievementDisabledLimit
			|| Mods::disableSteamAchievements )
		{
		// cheats/mods have been enabled on savefile, disallow achievements.
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
	}

	if ( !strcmp(achName, "BARONY_ACH_BOOTS_OF_SPEED") )
	{
		conductGameChallenges[CONDUCT_BOOTS_SPEED] = 1; // to cover bases when lich or devil dies as we can't remotely update this for clients.
	}

	if ( !achievementUnlocked(achName) )
	{
		//messagePlayer(clientnum, "You've unlocked an achievement!\n [%s]",c_SteamUserStats_GetAchievementDisplayAttribute(achName,"name"));

#if defined(LOCAL_ACHIEVEMENTS)
		LocalAchievements.updateAchievement(achName, true);
#endif

		auto find = Compendium_t::achievements.find(achName);
		if ( find != Compendium_t::achievements.end() )
		{
			find->second.unlocked = true;
			find->second.unlockTime = getTime();
			auto& unlockStatus = Compendium_t::AchievementData_t::unlocks[find->second.category];
			if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
			{
				unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
			else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_VISITED )
			{
				unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
			else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::UNLOCKED_VISITED )
			{
				unlockStatus = Compendium_t::CompendiumUnlockStatus::UNLOCKED_UNVISITED;
			}
			Compendium_t::AchievementData_t::achievementUnlockedLookup.insert(achName);
			Compendium_t::AchievementData_t::achievementsNeedResort = true;
		}
	}
}

void steamUnsetAchievement(const char* achName)
{
	return;
}

/*-------------------------------------------------------------------------------

	steamAchievementClient

	Tells a client to unlock a steam achievement (server only)

-------------------------------------------------------------------------------*/

void steamAchievementClient(int player, const char* achName)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	else if ( player == 0 )
	{
		steamAchievement(achName);
	}
	else
	{
		if ( client_disconnected[player] || multiplayer == SINGLE )
		{
			return;
		}
		strcpy((char*)net_packet->data, "SACH");
		strcpy((char*)(&net_packet->data[4]), achName);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 4 + strlen(achName) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

void steamAchievementEntity(Entity* my, const char* achName)
{
	if ( !my )
	{
		return;
	}

	if ( my->behavior == &actPlayer )
	{
		steamAchievementClient(my->skill[2], achName);
	}
}

void steamStatisticUpdate(int statisticNum, ESteamStatTypes type, int value)
{
	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		if ( !achievementObserver.bIsStatisticAllowedDuringTutorial(static_cast<SteamStatIndexes>(statisticNum)) )
		{
			return;
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS] )
		{
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
	}
	else
	{
		if ( !gameModeManager.allowsStatisticsOrAchievements(nullptr, statisticNum) )
		{
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED]
			|| conductGameChallenges[CONDUCT_LIFESAVING]
			|| conductGameChallenges[CONDUCT_ASSISTANCE_CLAIMED] >= GenericGUIMenu::AssistShrineGUI_t::achievementDisabledLimit
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS]
			|| Mods::disableSteamAchievements )
		{
		// cheats/mods have been enabled on savefile, disallow statistics update.
#ifndef DEBUG_ACHIEVEMENTS
		return;
#endif
		}
	}

	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}

	bool indicateProgress = true;
	bool result = false;
	switch ( type )
	{
		case STEAM_STAT_INT:
		{
			int oldValue = g_SteamStats[statisticNum].m_iValue;
			g_SteamStats[statisticNum].m_iValue += value;
			switch ( statisticNum )
			{
				case STEAM_STAT_RHINESTONE_COWBOY:
				case STEAM_STAT_TOUGH_AS_NAILS:
				case STEAM_STAT_UNSTOPPABLE_FORCE:
				case STEAM_STAT_BOMBARDIER:
				case STEAM_STAT_IN_THE_MIX:
				case STEAM_STAT_FREE_REFILLS:
				case STEAM_STAT_TAKE_THIS_OUTSIDE:
				case STEAM_STAT_BLOOD_SPORT:
				case STEAM_STAT_IRON_GUT:
				case STEAM_STAT_BOTTLE_NOSED:
				case STEAM_STAT_BARFIGHT_CHAMP:
				case STEAM_STAT_VOLATILE:
				case STEAM_STAT_SURROGATES:
				case STEAM_STAT_KILL_COMMAND:
				case STEAM_STAT_SPICY:
				case STEAM_STAT_TRADITION:
				case STEAM_STAT_POP_QUIZ:
				case STEAM_STAT_DYSLEXIA:
				case STEAM_STAT_BOOKWORM:
				case STEAM_STAT_MONARCH:
				case STEAM_STAT_MANY_PEDI_PALP:
				case STEAM_STAT_5000_SECOND_RULE:
				case STEAM_STAT_SOCIAL_BUTTERFLY:
				case STEAM_STAT_ROLL_THE_BONES:
				case STEAM_STAT_COWBOY_FROM_HELL:
				case STEAM_STAT_SELF_FLAGELLATION:
				case STEAM_STAT_CHOPPING_BLOCK:
				case STEAM_STAT_IF_YOU_LOVE_SOMETHING:
				case STEAM_STAT_RAGE_AGAINST:
				case STEAM_STAT_GUERILLA_RADIO:
				case STEAM_STAT_ITS_A_LIVING:
				case STEAM_STAT_FASCIST:
				case STEAM_STAT_I_NEEDED_THAT:
				case STEAM_STAT_DUNGEONSEED:
				case STEAM_STAT_RUNG_OUT:
				case STEAM_STAT_CALL_LOCKSMITH:
				case STEAM_STAT_PREMIUM_LOOTBOX:
				case STEAM_STAT_WITCHES_BREW:
				case STEAM_STAT_HOBBYIST:
				case STEAM_STAT_BLESSED_ADDITION:
				case STEAM_STAT_THATS_A_WRAP:
				case STEAM_STAT_MERCENARY_ARMY:
				case STEAM_STAT_COLONIST:
				case STEAM_STAT_PRICKLY_PERSONALITY:
				case STEAM_STAT_BOOM_DYNAMITE:
				case STEAM_STAT_DOESNT_COUNT:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					break;
				case STEAM_STAT_PITCH_PERFECT:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_ALTER_EGO:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 1000 && ((oldValue / 1000) < (g_SteamStats[statisticNum].m_iValue / 1000)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 5000) < (g_SteamStats[statisticNum].m_iValue / 5000)) )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_BAD_BLOOD:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 20 && ((oldValue / 20) < (g_SteamStats[statisticNum].m_iValue / 20)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 50) < (g_SteamStats[statisticNum].m_iValue / 50)) )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_SOURCE_ENGINE:
				case STEAM_STAT_TOUCHE:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 100 && ((oldValue / 100) < (g_SteamStats[statisticNum].m_iValue / 100)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 200) < (g_SteamStats[statisticNum].m_iValue / 200)) )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_PAY_TO_WIN:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 100 && ((oldValue / 100) < (g_SteamStats[statisticNum].m_iValue / 100)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 500) < (g_SteamStats[statisticNum].m_iValue / 500)) )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_SUPER_SHREDDER:
				case STEAM_STAT_SMASH_MELEE:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 25 && ((oldValue / 25) < (g_SteamStats[statisticNum].m_iValue / 25)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 50) < (g_SteamStats[statisticNum].m_iValue / 50)) ) // show every 50.
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_OVERCLOCKED:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 30 && ((oldValue / 30) < (g_SteamStats[statisticNum].m_iValue / 30)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 60) < (g_SteamStats[statisticNum].m_iValue / 60)) ) // show every 60.
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_SERIAL_THRILLA:
				case STEAM_STAT_TRASH_COMPACTOR:
				case STEAM_STAT_TORCHERER:
				case STEAM_STAT_FIXER_UPPER:
				case STEAM_STAT_LET_HIM_COOK:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 10 && ((oldValue / 10) < (g_SteamStats[statisticNum].m_iValue / 10)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 25) < (g_SteamStats[statisticNum].m_iValue / 25)) ) // show every 25.
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_DIPLOMA_LVLS:
				case STEAM_STAT_EXTRA_CREDIT_LVLS:
				case STEAM_STAT_BACK_TO_BASICS:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					indicateProgress = false;
					break;
				case STEAM_STAT_DAPPER_1:
				case STEAM_STAT_DAPPER_2:
				case STEAM_STAT_DAPPER_3:
					g_SteamStats[statisticNum].m_iValue =
						std::min((Uint32)g_SteamStats[statisticNum].m_iValue, (Uint32)steamStatAchStringsAndMaxVals[statisticNum].second);
					indicateProgress = false;
					break;
				case STEAM_STAT_DAPPER:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == g_SteamStats[statisticNum].m_iValue )
					{
						indicateProgress = false;
					}
					else
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_DIPLOMA:
				case STEAM_STAT_EXTRA_CREDIT:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == g_SteamStats[statisticNum].m_iValue )
					{
						indicateProgress = false;
					}
					else
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_TUTORIAL_ENTERED:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( oldValue == 0 )
					{
						achievementObserver.updateGlobalStat(STEAM_GSTAT_TUTORIAL_ENTERED, -1);
					}
					indicateProgress = false;
					break;
				default:
					break;
			}
			break;
		}
		case STEAM_STAT_FLOAT:
			break;
		default:
			break;
	}

#if defined(LOCAL_ACHIEVEMENTS)
	LocalAchievements.updateStatistic(statisticNum, g_SteamStats[statisticNum].m_iValue);
#endif

	if ( indicateProgress )
	{
		steamIndicateStatisticProgress(statisticNum, type);
	}
#ifdef DEBUG_ACHIEVEMENTS
	static ConsoleVariable<bool> cvar_statistics_debug("/statistics_debug", false);
	if ( *cvar_statistics_debug )
	{
		messagePlayer(clientnum, MESSAGE_DEBUG, "%s: %d, %d", steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
			g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
	}
#endif
}

void steamStatisticUpdateClient(int player, int statisticNum, ESteamStatTypes type, int value)
{
	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		if ( !achievementObserver.bIsStatisticAllowedDuringTutorial(static_cast<SteamStatIndexes>(statisticNum)) )
		{
			return;
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED]
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS] )
		{
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
	}
	else
	{
		if ( !gameModeManager.allowsStatisticsOrAchievements(nullptr, statisticNum) )
		{
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
		if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
			|| conductGameChallenges[CONDUCT_LIFESAVING]
			|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS]
			|| conductGameChallenges[CONDUCT_ASSISTANCE_CLAIMED] >= GenericGUIMenu::AssistShrineGUI_t::achievementDisabledLimit
			|| Mods::disableSteamAchievements )
		{
			// cheats/mods have been enabled on savefile, disallow statistics update.
#ifndef DEBUG_ACHIEVEMENTS
			return;
#endif
		}
	}

	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( player == 0 )
	{
		steamStatisticUpdate(statisticNum, type, value);
		return;
	}
	else if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	else
	{
		if ( client_disconnected[player] || multiplayer == SINGLE )
		{
			return;
		}
		strcpy((char*)net_packet->data, "SSTA");
		net_packet->data[4] = static_cast<Uint8>(statisticNum);
		net_packet->data[5] = static_cast<Uint8>(type);
		SDLNet_Write16(value, &net_packet->data[6]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

void indicateAchievementProgressAndUnlock(const char* achName, int currentValue, int maxValue)
{
#if defined(LOCAL_ACHIEVEMENTS)
	UIToastNotificationManager.createStatisticUpdateNotification(achName, currentValue, maxValue);
#endif
	if ( currentValue == maxValue )
	{
		steamAchievement(achName);
	}
}

void steamIndicateStatisticProgress(int statisticNum, ESteamStatTypes type)
{
	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}

	int iVal = g_SteamStats[statisticNum].m_iValue;
	float fVal = g_SteamStats[statisticNum].m_flValue;
	if ( type == STEAM_STAT_INT )
	{
		switch ( statisticNum )
		{
			// below are 30-50 max value
			case STEAM_STAT_RHINESTONE_COWBOY:
			case STEAM_STAT_TOUGH_AS_NAILS:
			case STEAM_STAT_UNSTOPPABLE_FORCE:
			case STEAM_STAT_BOMBARDIER:
			case STEAM_STAT_IN_THE_MIX:
			case STEAM_STAT_FREE_REFILLS:
			case STEAM_STAT_BLOOD_SPORT:
			case STEAM_STAT_BARFIGHT_CHAMP:
			case STEAM_STAT_SURROGATES:
			case STEAM_STAT_KILL_COMMAND:
			case STEAM_STAT_DYSLEXIA:
			case STEAM_STAT_BOOKWORM:
			case STEAM_STAT_5000_SECOND_RULE:
			case STEAM_STAT_SOCIAL_BUTTERFLY:
			case STEAM_STAT_ROLL_THE_BONES:
			case STEAM_STAT_COWBOY_FROM_HELL:
			case STEAM_STAT_SELF_FLAGELLATION:
			case STEAM_STAT_FASCIST:
			case STEAM_STAT_ITS_A_LIVING:
			case STEAM_STAT_CHOPPING_BLOCK:
			case STEAM_STAT_MANY_PEDI_PALP:
			case STEAM_STAT_WITCHES_BREW:
			case STEAM_STAT_HOBBYIST:
			case STEAM_STAT_BLESSED_ADDITION:
			case STEAM_STAT_THATS_A_WRAP:
			case STEAM_STAT_BOOM_DYNAMITE:
			case STEAM_STAT_COLONIST:
			case STEAM_STAT_DOESNT_COUNT:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 5 == 0) )
					{
						indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					}
				}
				break;
			// below are 20 max value
			case STEAM_STAT_IRON_GUT:
			case STEAM_STAT_BOTTLE_NOSED:
			case STEAM_STAT_VOLATILE:
			case STEAM_STAT_TRADITION:
			case STEAM_STAT_POP_QUIZ:
			case STEAM_STAT_MONARCH:
			case STEAM_STAT_RAGE_AGAINST:
			case STEAM_STAT_GUERILLA_RADIO:
			case STEAM_STAT_RUNG_OUT:
			case STEAM_STAT_CALL_LOCKSMITH:
			case STEAM_STAT_PREMIUM_LOOTBOX:
			case STEAM_STAT_MERCENARY_ARMY:
			case STEAM_STAT_PRICKLY_PERSONALITY:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 4 == 0) )
					{
						indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					}
				}
				break;
			case STEAM_STAT_BAD_BLOOD:
			case STEAM_STAT_ALTER_EGO:
			case STEAM_STAT_PAY_TO_WIN:
			case STEAM_STAT_SOURCE_ENGINE:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
						iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
				}
				break;
			// below is 100 max value
			case STEAM_STAT_SERIAL_THRILLA:
			case STEAM_STAT_TRASH_COMPACTOR:
			case STEAM_STAT_TORCHERER:
			case STEAM_STAT_FIXER_UPPER:
			case STEAM_STAT_SMASH_MELEE:
			case STEAM_STAT_PITCH_PERFECT:
			case STEAM_STAT_LET_HIM_COOK:
			// below are 1000 max value
			case STEAM_STAT_SUPER_SHREDDER:
			case STEAM_STAT_TOUCHE:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
						iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
				}
				break;
			// below is 600 max value
			case STEAM_STAT_OVERCLOCKED:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
						iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
				}
				break;
			// below are 100 max value
			case STEAM_STAT_IF_YOU_LOVE_SOMETHING:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || iVal == 5 || (iVal > 0 && iVal % 10 == 0) || (iVal > 0 && iVal % 25 == 0) )
					{
						indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					}
				}
				break;
			// below are 10 max value
			case STEAM_STAT_TAKE_THIS_OUTSIDE:
			case STEAM_STAT_SPICY:
			case STEAM_STAT_I_NEEDED_THAT:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 2 == 0) )
					{
						indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					}
				}
				break;
			case STEAM_STAT_DIPLOMA:
			case STEAM_STAT_EXTRA_CREDIT:
			case STEAM_STAT_DUNGEONSEED:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
						iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
				}
				break;
			case STEAM_STAT_DAPPER:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal >= 5 && iVal % 5 == 0 )
					{
						indicateAchievementProgressAndUnlock(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					}
				}
				break;
			default:
				break;
		}
#ifdef DEBUG_ACHIEVEMENTS
		static ConsoleVariable<bool> cvar_statistics_indicate_debug("/statistics_indicate_debug", false);
		if ( *cvar_statistics_indicate_debug )
		{
			messagePlayer(clientnum, MESSAGE_DEBUG, "%s: %d, %d", steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
				iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
		}
#endif
	}
}
