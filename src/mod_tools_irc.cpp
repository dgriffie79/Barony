/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_irc.cpp - IRC chat handler
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

#ifndef NINTENDO
bool IRCHandler_t::readFromFile()
{
	if ( PHYSFS_getRealDir("/data/twitchchat.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/twitchchat.json");
		inputPath.append("/data/twitchchat.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}
		auth.oauth = "";
		auth.chatroom = "";
		auth.username = "";
		if ( d.HasMember("oauth_key") )
		{
			auth.oauth = d["oauth_key"].GetString();
		}
		if ( d.HasMember("username") )
		{
			auth.username = d["username"].GetString();
		}
		if ( d.HasMember("channel") )
		{
			auth.chatroom = d["channel"].GetString();
		}

		if ( !auth.oauth.compare("") || !auth.chatroom.compare("") || !auth.username.compare("") )
		{
			printlog("[JSON]: Error in one or more data values. Check syntax and try again.");
			return false;
		}
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		return true;
	}
	return false;
}

void IRCHandler_t::disconnect()
{
	if ( net_ircsocketset )
	{
		SDLNet_TCP_DelSocket(net_ircsocketset, net_ircsocket);
	}
	net_ircsocketset = nullptr;
	if ( net_ircsocket )
	{
		SDLNet_TCP_Close(net_ircsocket);
	}
	net_ircsocket = nullptr;
	bSocketConnected = false;
}

bool IRCHandler_t::connect()
{
	if ( SDLNet_ResolveHost(&ip, "irc.chat.twitch.tv", 6667) == -1 )
	{
		return false;
	}
	bSocketConnected = false;
	if ( !readFromFile() )
	{
		return false;
	}
	if ( !(net_ircsocket = SDLNet_TCP_Open(&ip)) )
	{
		return false;
	}
	net_ircsocketset = SDLNet_AllocSocketSet(1);
	if ( !net_ircsocketset )
	{
		return false;
	}
	SDLNet_TCP_AddSocket(net_ircsocketset, net_ircsocket);
	bSocketConnected = true;

	std::string data = "PASS oauth:" + auth.oauth + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	data = "NICK " + auth.username + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	data = "JOIN #" + auth.chatroom + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	return true;
}

int IRCHandler_t::packetSend(std::string data)
{
	if ( !bSocketConnected )
	{
		return -1;
	}
	int sentBytes = SDLNet_TCP_Send(net_ircsocket, data.data(), data.length());
	return sentBytes;
}

int IRCHandler_t::packetReceive()
{
	if ( !bSocketConnected )
	{
		return 0;
	}

	if ( SDLNet_CheckSockets(net_ircsocketset, 0) )
	{
		if ( SDLNet_SocketReady(net_ircsocketset) )
		{
			std::fill(recvBuffer.begin(), recvBuffer.end(), '\0');
			int receiveLen = SDLNet_TCP_Recv(net_ircsocket, &recvBuffer[0], MAX_BUFFER_LEN);
			if ( receiveLen <= 0 )
			{
				printlog("[IRCHandler]: Error in packetReceive: %s", SDLNet_GetError());
				return 0;
			}
			return receiveLen;
		}
	}
	return 0;
}

void IRCHandler_t::run()
{
	if ( !bSocketConnected )
	{
		return;
	}

	while ( int receiveLen = packetReceive() )
	{
		// check incoming messages.
		std::string msg(recvBuffer.cbegin(), recvBuffer.cend());
		handleMessage(msg);
	}
}

void IRCHandler_t::handleMessage(std::string& msg)
{
	if ( intro )
	{
		return;
	}
	if ( msg.length() <= 1 )
	{
		return;
	}
	msg = msg.substr(0, msg.find("\r\n"));
	printlog("Recv: %s", msg.c_str());

	if ( msg.find("PING :tmi.twitch.tv") != std::string::npos )
	{
		packetSend("PING :tmi.twitch.tv\r\n");
		return;
	}

	std::string msgPrefix = "PRIVMSG #" + auth.chatroom + " :";
	auto findMsg = msg.find(msgPrefix);
	if ( findMsg != std::string::npos )
	{
		if ( msg.find("!") != std::string::npos )
		{
			std::string user = msg.substr(1, msg.find("!") - 1);
			std::string formattedMsg = msg.substr(msgPrefix.length() + findMsg);
			messagePlayer(clientnum, MESSAGE_MISC, "IRC: [@%s]: %s", user.c_str(), formattedMsg.c_str());
		}
		return;
	}
}
#endif // !NINTENDO
