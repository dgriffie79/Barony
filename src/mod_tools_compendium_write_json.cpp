#include "mod_tools_private.hpp"
#include <cstring>
#include <cstdlib>

static ConsoleVariable<bool> cvar_compendiumClientSave("/compendium_client_save", false);

void Compendium_t::writeUnlocksSaveData()
{
	char path[PATH_MAX] = "";
	completePath(path, "savegames/compendium_progress.json", outputdir);

	cJSON* exportDocument = cJSON_CreateObject();

	const int VERSION = 1;

	cJSON_AddNumberToObject(exportDocument, "version", VERSION);

	cJSON* obj = cJSON_CreateObject();
	for ( auto& pair : CompendiumItems_t::unlocks )
	{
		cJSON_AddNumberToObject(obj, pair.first.c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "items", obj);

	obj = cJSON_CreateObject();
	for ( auto& pair : CompendiumItems_t::itemUnlocks )
	{
		cJSON_AddNumberToObject(obj, std::to_string(pair.first).c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "items_status", obj);

	obj = cJSON_CreateObject();
	for ( auto& pair : AchievementData_t::unlocks )
	{
		cJSON_AddNumberToObject(obj, pair.first.c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "achievements", obj);

	obj = cJSON_CreateObject();
	for ( auto& pair : CompendiumWorld_t::unlocks )
	{
		cJSON_AddNumberToObject(obj, pair.first.c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "world", obj);

	obj = cJSON_CreateObject();
	for ( auto& pair : CompendiumCodex_t::unlocks )
	{
		cJSON_AddNumberToObject(obj, pair.first.c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "codex", obj);

	obj = cJSON_CreateObject();
	for ( auto& pair : CompendiumMonsters_t::unlocks )
	{
		cJSON_AddNumberToObject(obj, pair.first.c_str(), (int)pair.second);
	}
	cJSON_AddItemToObject(exportDocument, "monsters", obj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		cJSON_Delete(exportDocument);
		return;
	}
	char* str = cJSON_Print(exportDocument);
	fp->write(str, sizeof(char), strlen(str));
	free(str);
	FileIO::close(fp);
	cJSON_Delete(exportDocument);

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

	cJSON* exportDocument = cJSON_CreateObject();

	const int VERSION = 2;

	cJSON_AddNumberToObject(exportDocument, "version", VERSION);
	cJSON* itemsObj = cJSON_CreateObject();
	for ( auto& pair : playerEvents )
	{
		const std::string& key = events[pair.first].name;
		cJSON* inner = cJSON_CreateObject();
		for ( auto& itemsData : pair.second )
		{
			cJSON_AddNumberToObject(inner, std::to_string(itemsData.first).c_str(), itemsData.second.value);
		}
		cJSON_AddItemToObject(itemsObj, key.c_str(), inner);
	}
	cJSON_AddItemToObject(exportDocument, "items", itemsObj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		cJSON_Delete(exportDocument);
		return;
	}
	char* str = cJSON_Print(exportDocument);
	fp->write(str, sizeof(char), strlen(str));
	free(str);
	FileIO::close(fp);
	cJSON_Delete(exportDocument);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

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

		cJSON* d = cJSON_CreateObject();
		cJSON_AddNumberToObject(d, "seq", clientSequence);
		cJSON* data = cJSON_CreateObject();
		for ( auto& p1 : serverPlayerEvents[playernum] )
		{
			std::string key = std::to_string(p1.first);
			cJSON* inner = cJSON_CreateObject();
			for ( auto& itemsData : p1.second )
			{
				cJSON_AddNumberToObject(inner, std::to_string(itemsData.first).c_str(), itemsData.second.value);
			}
			cJSON_AddItemToObject(data, key.c_str(), inner);
		}
		cJSON_AddItemToObject(d, "item", data);

		char* jsonStr = cJSON_PrintUnformatted(d);
		clientDataStrings[playernum][clientSequence] = jsonStr;
		auto& dataStr = clientDataStrings[playernum][clientSequence];
		free(jsonStr);
		cJSON_Delete(d);

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

	cJSON* exportDocument = cJSON_CreateObject();
	cJSON_AddNumberToObject(exportDocument, "version", 1);

	cJSON* cameraObject = cJSON_CreateObject();
	cJSON_AddNumberToObject(cameraObject, "ang_degrees", 0);
	cJSON_AddNumberToObject(cameraObject, "vang_degrees", 0);
	cJSON_AddNumberToObject(cameraObject, "zoom", 0.0);
	cJSON_AddNumberToObject(cameraObject, "height", 0.0);
	cJSON_AddNumberToObject(cameraObject, "rotate_limit_degrees_min", 0);
	cJSON_AddNumberToObject(cameraObject, "rotate_limit_degrees_max", 0);
	cJSON_AddNumberToObject(cameraObject, "rotate_speed", 0.0);
	cJSON_AddItemToObject(exportDocument, "camera", cameraObject);

	cJSON* limbsArray = cJSON_CreateArray();

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
		cJSON* limbsObj = cJSON_CreateObject();

		if ( index != 0 )
		{
			cJSON_AddNumberToObject(limbsObj, "x", monster->x - limb->x);
			cJSON_AddNumberToObject(limbsObj, "y", monster->y - limb->y);
			cJSON_AddNumberToObject(limbsObj, "z", limb->z);
		}
		else
		{
			cJSON_AddNumberToObject(limbsObj, "x", 0);
			cJSON_AddNumberToObject(limbsObj, "y", 0);
			cJSON_AddNumberToObject(limbsObj, "z", limb->z);
		}
		cJSON_AddNumberToObject(limbsObj, "pitch", limb->pitch);
		cJSON_AddNumberToObject(limbsObj, "roll", limb->roll);
		cJSON_AddNumberToObject(limbsObj, "yaw", limb->yaw);
		cJSON_AddNumberToObject(limbsObj, "focalx", limb->focalx);
		cJSON_AddNumberToObject(limbsObj, "focaly", limb->focaly);
		cJSON_AddNumberToObject(limbsObj, "focalz", limb->focalz);
		cJSON_AddNumberToObject(limbsObj, "sprite", limb->sprite);
		cJSON_AddNumberToObject(limbsObj, "scalex", limb->scalex);
		cJSON_AddNumberToObject(limbsObj, "scaley", limb->scaley);
		cJSON_AddNumberToObject(limbsObj, "scalez", limb->scalez);
		cJSON_AddItemToArray(limbsArray, limbsObj);

		++index;
	}

	cJSON_AddItemToObject(exportDocument, "limbs", limbsArray);
	
	std::string outputPath = PHYSFS_getRealDir("/data/compendium/monster_models");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/compendium/monster_models/" + exportFileName;
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		cJSON_Delete(exportDocument);
		return;
	}
	char* jsonStr = cJSON_Print(exportDocument);
	fp->write(jsonStr, sizeof(char), strlen(jsonStr));
	free(jsonStr);
	FileIO::close(fp);
	cJSON_Delete(exportDocument);

	return;
}