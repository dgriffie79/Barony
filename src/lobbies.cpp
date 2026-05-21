/*-------------------------------------------------------------------------------

BARONY
File: lobbies.cpp
Desc: contains code for matchmaking handlers

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.h"
#include "menu.h"
#include "game.h"
#include "lobbies.h"
#include "draw.h"
#include "player.h"
#include "scores.hpp"
#include "interface/interface.hpp"
#include "colors.hpp"
#include "net.h"

LobbyHandler_t LobbyHandler;

std::string LobbyHandler_t::getCurrentRoomKey() const
{
    return "";
}

std::string LobbyHandler_t::getLobbyJoinFailedConnectString(int result)
{
	char buf[1024] = "";
	switch ( result )
	{
		case EResult_LobbyFailures::LOBBY_GAME_IN_PROGRESS:
			snprintf(buf, 1023, "Unable to join lobby:\nGame in progress not joinable.");
			break;
		case EResult_LobbyFailures::LOBBY_USING_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\n%s", Language::get(1381));
			break;
		case EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\nOnly new characters allowed.");
			break;
		case EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\nIncompatible save game.");
			break;
		case EResult_LobbyFailures::LOBBY_JOIN_CANCELLED:
			snprintf(buf, 1023, "Lobby join cancelled.\nSafely leaving lobby.");
			break;
		case EResult_LobbyFailures::LOBBY_NO_OWNER:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby has no host.");
			break;
		case EResult_LobbyFailures::LOBBY_NOT_FOUND:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby no longer exists.");
			break;
		case EResult_LobbyFailures::LOBBY_TOO_MANY_PLAYERS:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby is full.");
			break;
		case EResult_LobbyFailures::LOBBY_SAVEGAME_REQUIRES_DLC:
			snprintf(buf, 1023, "Unable to join lobby:\n%s", Language::get(6100));
			break;

		case EResult_LobbyFailures::LOBBY_JOIN_TIMEOUT:
			snprintf(buf, 1023, "Unable to join lobby:\nTimeout waiting for host.");
			break;
		default:
			snprintf(buf, 1023, "Unable to join lobby:\nError code: %d.", result);
			break;
	}
	printlog("[Lobbies Error]: %s", buf);
	return buf;
}



void LobbyHandler_t::handleLobbyListRequests()
{
}

void LobbyHandler_t::updateSearchResults()
{
	numLobbyDisplaySearchResults = 0;
	for ( auto& result : lobbyDisplayedSearchResults )
	{
		result.first = -1;
		result.second = LOBBY_DISABLE;
	}
}

LobbyHandler_t::LobbyServiceType LobbyHandler_t::getDisplayedResultLobbyType(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return LOBBY_DISABLE;
	}
	return lobbyDisplayedSearchResults.at(selection).second;
}
Sint32 LobbyHandler_t::getDisplayedResultLobbyIndex(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return -1;
	}
	return lobbyDisplayedSearchResults.at(selection).first;
}



void LobbyHandler_t::filterLobbyButton(button_t* my)
{
	LobbyHandler.showLobbyFilters = !LobbyHandler.showLobbyFilters;
}

void LobbyHandler_t::searchLobbyWithFilter(button_t* my)
{
	LobbyHandler.showLobbyFilters = false;
}

void LobbyHandler_t::drawLobbyFilters()
{
}
