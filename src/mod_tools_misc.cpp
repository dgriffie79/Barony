/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_misc.cpp - Statue manager, debug timers, glyph renderer, script text parser, monster data, shop consumables (converted from rapidjson to cJSON)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

Uint32 StatueManager_t::statueId = 0;
int StatueManager_t::processStatueExport()
{
	if ( !exportActive )
	{
		return 0;
	}

	Entity* player = uidToEntity(editingPlayerUid);

	if ( exportRotations >= 4 || !player )
	{
		if ( player ) // save the file.
		{
			std::string outputPath = PHYSFS_getRealDir("/data/statues");
			outputPath.append(PHYSFS_getDirSeparator());
			std::string fileName = "data/statues/" + exportFileName;
			outputPath.append(fileName.c_str());

			exportRotations = 0;
			exportFileName = "";
			exportActive = false;

			File* fp = FileIO::open(outputPath.c_str(), "wb");
			if ( !fp )
			{
				return 0;
			}
			char* json = cJSON_Print(exportDocument);
			if ( json )
			{
				fp->write(json, sizeof(char), strlen(json));
				cJSON_free(json);
			}
			FileIO::close(fp);
			return 2; // success
		}
		else
		{
			exportRotations = 0;
			exportFileName = "";
			exportActive = false;
			return 0;
		}
	}

	bool newDocument = false;

	if ( exportFileName == "" ) // find a new filename
	{
		int filenum = 0;
		std::string testPath = "/data/statues/statue" + std::to_string(filenum) + ".json";
		while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
		{
			++filenum;
			testPath = "/data/statues/statue" + std::to_string(filenum) + ".json";
		}
		exportFileName = "statue" + std::to_string(filenum) + ".json";
		newDocument = true;
	}

	if ( newDocument )
	{
		if ( exportDocument )
		{
			cJSON_Delete(exportDocument);
		}
		exportDocument = cJSON_CreateObject();
		cJSON_AddNumberToObject(exportDocument, "version", 1);
		cJSON_AddNumberToObject(exportDocument, "statue_id", (double)local_rng.rand());
		cJSON_AddNumberToObject(exportDocument, "height_offset", 0);
		cJSON_AddObjectToObject(exportDocument, "limbs");
	}

	cJSON* limbsArray = cJSON_CreateArray();

	std::vector<Entity*> allLimbs;
	allLimbs.push_back(player);

	for ( auto& bodypart : player->bodyparts )
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
			cJSON_AddNumberToObject(limbsObj, "x", player->x - limb->x);
			cJSON_AddNumberToObject(limbsObj, "y", player->y - limb->y);
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
		cJSON_AddItemToArray(limbsArray, limbsObj);

		++index;
	}

	cJSON* limbsObj2 = cJSON_GetObjectItem(exportDocument, "limbs");
	if ( limbsObj2 )
	{
		cJSON_AddItemToObject(limbsObj2, directionKeys[exportRotations].c_str(), limbsArray);
	}
	else
	{
		cJSON_Delete(limbsArray);
	}
	++exportRotations;
	return 1;
}

void StatueManager_t::resetStatueEditor()
{
	if ( editingPlayerUid != 0 )
	{
		client_disconnected[1] = true;
	}
	editingPlayerUid = 0;
	StatueManager.activeEditing = false;
}

void StatueManager_t::refreshAllStatues()
{
	node_t* nextnode = nullptr;
	for ( node_t* node = map.entities->first; node; node = nextnode )
	{
		nextnode = node->next;
		auto entity = (Entity*)node->element;
		if ( entity->behavior == &actStatue )
		{
			entity->statueInit = 0;
			node_t* nextnode2 = nullptr;
			for ( node_t* node2 = entity->children.first; node2; node2 = nextnode2 )
			{
				nextnode2 = node2->next;
				auto entity2 = (Entity*)node2->element;
				list_RemoveNode(entity2->mynode);
				list_RemoveNode(node2);
			}
		}
	}
}

void StatueManager_t::readAllStatues()
{
	std::string baseDir = "data/statues";
	auto files = physfsGetFileNamesInDirectory(baseDir.c_str());
	for ( auto& file : files )
	{
		std::string checkFile = baseDir + '/' + file;
		PHYSFS_Stat stat;
		if ( PHYSFS_stat(checkFile.c_str(), &stat) == 0 ) { continue; }

		if ( stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
		{
			auto files2 = physfsGetFileNamesInDirectory(checkFile.c_str());
			for ( auto& file2 : files2 )
			{
				std::string checkFile2 = checkFile + '/' + file2;
				if ( PHYSFS_stat(checkFile2.c_str(), &stat) == 0 ) { continue; }

				if ( stat.filetype != PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
				{
					readStatueFromFile(0, checkFile2);
				}
			}
		}
		else
		{
			readStatueFromFile(0, checkFile);
		}
	}
}

void StatueManager_t::readStatueFromFile(int index, std::string filename)
{
	std::string fileName = "/data/statues/statue" + std::to_string(index) + ".json";
	if ( filename != "" )
	{
		fileName = filename;
	}
	if ( PHYSFS_getRealDir(fileName.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(fileName.c_str());
		if (!inputPath.empty()) {
			inputPath.append(PHYSFS_getDirSeparator());
		}
		inputPath.append(fileName);

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
		if ( !d )
		{
			printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
			return;
		}
		if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "limbs") || !cJSON_HasObjectItem(d, "statue_id") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			cJSON_Delete(d);
			return;
		}
		int version = cJSON_GetObjectItem(d, "version")->valueint;
		cJSON* statueIdItem = cJSON_GetObjectItem(d, "statue_id");
		Uint32 statueId = (Uint32)(statueIdItem ? statueIdItem->valuedouble : 0);
		auto findStatue = allStatues.find(statueId);
		if ( findStatue != allStatues.end() )
		{
			allStatues.erase(findStatue);
		}
		allStatues.insert(std::make_pair(statueId, Statue_t()));
		cJSON* limbsObj = cJSON_GetObjectItem(d, "limbs");
		if ( limbsObj )
		{
			for ( cJSON* limb_itr = limbsObj->child; limb_itr; limb_itr = limb_itr->next )
			{
				auto& statue = allStatues[statueId];
				cJSON* heightItem = cJSON_GetObjectItem(d, "height_offset");
				if ( heightItem )
				{
					statue.heightOffset = heightItem->valuedouble;
				}
				if ( cJSON_IsArray(limb_itr) )
				{
					cJSON* dir_itr = NULL;
					cJSON_ArrayForEach(dir_itr, limb_itr)
					{
						std::string direction = limb_itr->string;
						auto& limbVector = statue.limbs[direction];
						limbVector.push_back(Statue_t::StatueLimb_t());
						auto& limb = limbVector[limbVector.size() - 1];
						cJSON* xItem = cJSON_GetObjectItem(dir_itr, "x");
						if ( xItem ) limb.x = xItem->valuedouble;
						cJSON* yItem = cJSON_GetObjectItem(dir_itr, "y");
						if ( yItem ) limb.y = yItem->valuedouble;
						cJSON* zItem = cJSON_GetObjectItem(dir_itr, "z");
						if ( zItem ) limb.z = zItem->valuedouble;
						cJSON* focalxItem = cJSON_GetObjectItem(dir_itr, "focalx");
						if ( focalxItem ) limb.focalx = focalxItem->valuedouble;
						cJSON* focalyItem = cJSON_GetObjectItem(dir_itr, "focaly");
						if ( focalyItem ) limb.focaly = focalyItem->valuedouble;
						cJSON* focalzItem = cJSON_GetObjectItem(dir_itr, "focalz");
						if ( focalzItem ) limb.focalz = focalzItem->valuedouble;
						cJSON* pitchItem = cJSON_GetObjectItem(dir_itr, "pitch");
						if ( pitchItem ) limb.pitch = pitchItem->valuedouble;
						cJSON* rollItem = cJSON_GetObjectItem(dir_itr, "roll");
						if ( rollItem ) limb.roll = rollItem->valuedouble;
						cJSON* yawItem = cJSON_GetObjectItem(dir_itr, "yaw");
						if ( yawItem ) limb.yaw = yawItem->valuedouble;
						cJSON* spriteItem = cJSON_GetObjectItem(dir_itr, "sprite");
						if ( spriteItem ) limb.sprite = spriteItem->valueint;
					}
				}
			}
		}
		cJSON_Delete(d);
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
}

void DebugTimers_t::printAllTimepoints()
{
	int posy = 100;
	for ( auto& keyValue : timepoints )
	{
		printTimepoints(keyValue.first, posy);
		posy += 16;
	}
}

void DebugTimers_t::printTimepoints(std::string key, int& posy)
{
	if ( !font8x8_bmp || intro ) {
		return;
	}
	auto& points = timepoints[key];
	if ( points.empty() ) { return; }
	int starty = posy;
	int index = 0;
	std::string output = "";
	auto previousPoint = points[0];
	for ( auto& point : points )
	{
		double timediff = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(point.second - previousPoint.second).count();
		char outputBuf[1024] = "";
		snprintf(outputBuf, sizeof(outputBuf), "[%d]['%s'] %4.5fms\n", index, point.first.c_str(), timediff);
		output += outputBuf;
		posy += 8;
		if ( index > 0 )
		{
			previousPoint = point;
		}
		++index;
	}
	printTextFormatted(font8x8_bmp, 8, starty, "%s:\n%s", key.c_str(), output.c_str());
}

bool GlyphRenderer_t::readFromFile()
{
	if ( PHYSFS_getRealDir("/data/keyboard_glyph_config.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/keyboard_glyph_config.json");
		inputPath.append("/data/keyboard_glyph_config.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		FileIO::close(fp);

		cJSON* d = cJSON_Parse(buf);
		if ( !d )
		{
			printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
			return false;
		}
		if ( !cJSON_HasObjectItem(d, "version") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			cJSON_Delete(d);
			return false;
		}

		allGlyphs.clear();
		cJSON* item = cJSON_GetObjectItem(d, "rendered_glyph_folder");
		if ( item ) renderedGlyphFolder = item->valuestring;
		item = cJSON_GetObjectItem(d, "base_glyph_folder");
		if ( item ) baseSourceFolder = item->valuestring;
		item = cJSON_GetObjectItem(d, "small_key_unpressed_path");
		if ( item ) baseUnpressedGlyphPath = item->valuestring;
		item = cJSON_GetObjectItem(d, "small_key_pressed_path");
		if ( item ) basePressedGlyphPath = item->valuestring;
		int render_offsety = 0;
		item = cJSON_GetObjectItem(d, "base_render_offset_y");
		if ( item ) render_offsety = item->valueint;
		pressedRenderedPrefix = "Pressed_";
		item = cJSON_GetObjectItem(d, "pressed_rendered_glyph_prefix");
		if ( item ) pressedRenderedPrefix = item->valuestring;
		unpressedRenderedPrefix = "Unpressed_";
		item = cJSON_GetObjectItem(d, "unpressed_rendered_glyph_prefix");
		if ( item ) unpressedRenderedPrefix = item->valuestring;

		std::string basePath = baseSourceFolder;
		basePath += '/';
		std::string baseRenderedPath = basePath;
		baseRenderedPath += renderedGlyphFolder;
		baseRenderedPath += '/';

		cJSON* glyphsArr = cJSON_GetObjectItem(d, "glyphs");
		if ( glyphsArr )
		{
			cJSON* glyph_itr = NULL;
			cJSON_ArrayForEach(glyph_itr, glyphsArr)
			{
				if ( !cJSON_HasObjectItem(glyph_itr, "keyname") )
				{
					printlog("[JSON]: Glyph entry does not have keyname, skipping...");
					continue;
				}
				cJSON* keynameItem = cJSON_GetObjectItem(glyph_itr, "keyname");
				std::string keyname = keynameItem->valuestring;
				int keycode = SDL_GetKeyFromName(keyname.c_str());
				if ( keycode == SDLK_UNKNOWN )
				{
					printlog("[JSON]: Glyph name: %s could not find a keycode, skipping...", keyname.c_str());
					continue;
				}
				allGlyphs[keycode] = GlyphData_t();
				auto& glyphData = allGlyphs[keycode];
				glyphData.keycode = keycode;
				glyphData.keyname = keyname;
				cJSON* folderItem = cJSON_GetObjectItem(glyph_itr, "folder");
				if ( !folderItem )
				{
					printlog("[JSON]: Glyph entry does not have base folder entry, skipping...");
					continue;
				}
				glyphData.folder = folderItem->valuestring;
				cJSON* pathItem = cJSON_GetObjectItem(glyph_itr, "path");
				if ( !pathItem )
				{
					printlog("[JSON]: Glyph entry does not have glyph path name, skipping...");
					continue;
				}
				glyphData.filename = pathItem->valuestring;
				cJSON* customOffsetItem = cJSON_GetObjectItem(glyph_itr, "custom_render_offset_y");
				if ( customOffsetItem )
				{
					if ( customOffsetItem->valueint != 0 )
					{
						glyphData.render_offsety = customOffsetItem->valueint;
					}
					else
					{
						glyphData.render_offsety = render_offsety;
					}
				}
				else
				{
					glyphData.render_offsety = render_offsety;
				}
				cJSON* pressedBgItem = cJSON_GetObjectItem(glyph_itr, "pressed_glyph_background_path");
				if ( pressedBgItem )
				{
					glyphData.pressedGlyphPath = pressedBgItem->valuestring;
				}
				else
				{
					glyphData.pressedGlyphPath = basePressedGlyphPath;
				}
				cJSON* unpressedBgItem = cJSON_GetObjectItem(glyph_itr, "unpressed_glyph_background_path");
				if ( unpressedBgItem )
				{
					glyphData.unpressedGlyphPath = unpressedBgItem->valuestring;
				}
				else
				{
					glyphData.unpressedGlyphPath = baseUnpressedGlyphPath;
				}
			}
		}

		for ( auto& keyValue : allGlyphs )
		{
			auto& glyphData = keyValue.second;
			glyphData.fullpath = "";
			glyphData.pressedRenderedFullpath = "";
			glyphData.unpressedRenderedFullpath = "";
			
			glyphData.fullpath = basePath;
			glyphData.fullpath += glyphData.folder;
			glyphData.fullpath += '/';
			glyphData.fullpath += glyphData.filename;

			if ( !PHYSFS_getRealDir(glyphData.fullpath.c_str()) )
			{
				printlog("[JSON]: Glyph path: %s not detected; won't be able to render!", glyphData.fullpath.c_str());
				glyphData.fullpath = "";
			}

			const std::string renderedPath = baseRenderedPath + glyphData.folder + '/';

			glyphData.unpressedRenderedFullpath = renderedPath;
			glyphData.unpressedRenderedFullpath += unpressedRenderedPrefix;
			glyphData.unpressedRenderedFullpath += glyphData.filename;
			if ( glyphData.unpressedRenderedFullpath[0] == '/' )
			{
				glyphData.unpressedRenderedFullpath.erase((size_t)0, (size_t)1);
			}
			if ( !PHYSFS_getRealDir(glyphData.unpressedRenderedFullpath.c_str()) )
			{
				printlog("[JSON]: Glyph path: %s not detected", glyphData.unpressedRenderedFullpath.c_str());
			}

			glyphData.pressedRenderedFullpath = renderedPath;
			glyphData.pressedRenderedFullpath += pressedRenderedPrefix;
			glyphData.pressedRenderedFullpath += glyphData.filename;
			if ( glyphData.pressedRenderedFullpath[0] == '/' )
			{
				glyphData.pressedRenderedFullpath.erase((size_t)0, (size_t)1);
			}
			if ( !PHYSFS_getRealDir(glyphData.pressedRenderedFullpath.c_str()) )
			{
				printlog("[JSON]: Glyph path: %s not detected", glyphData.pressedRenderedFullpath.c_str());
			}
		}
		cJSON_Delete(d);
		printlog("[JSON]: Successfully read json file %s, processed %d glyphs", inputPath.c_str(), allGlyphs.size());
		return true;
	}
	printlog("[JSON]: Error: Could not locate json file %s", "/data/keyboard_glyph_config.json");
	return false;
}

void GlyphRenderer_t::renderGlyphsToPNGs()
{
	printlog("[Glyph Export]: Starting export...");
	int errors = 0;
	for ( auto& keyValue : allGlyphs )
	{
		std::string pressedPath = baseSourceFolder;
		pressedPath += '/';
		pressedPath += keyValue.second.pressedGlyphPath;
		if ( pressedPath[0] == '/' )
		{
			pressedPath.erase((size_t)0, (size_t)1);
		}

		std::string unpressedPath = baseSourceFolder;
		unpressedPath += '/';
		unpressedPath += keyValue.second.unpressedGlyphPath;
		if ( unpressedPath[0] == '/' )
		{
			unpressedPath.erase((size_t)0, (size_t)1);
		}

		auto& glyphData = keyValue.second;

		Image* base = Image::get(unpressedPath.c_str());
		if ( base->getWidth() != 0 )
		{
			SDL_Surface* srcSurf = const_cast<SDL_Surface*>(base->getSurf());
			SDL_Rect pos{ 0, 0, (int)base->getWidth(), (int)base->getHeight() };
			SDL_Surface* sprite = SDL_CreateRGBSurface(0, pos.w, pos.h, 32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
			SDL_SetSurfaceAlphaMod(srcSurf, 255);
			SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);

			std::string keyPath = keyValue.second.fullpath;
			if ( keyPath[0] == '/' )
			{
				keyPath.erase((size_t)0, (size_t)1);
			}
			auto key = Image::get(keyPath.c_str());
			if ( key->getWidth() != 0 )
			{
				SDL_Surface* keySurf = const_cast<SDL_Surface*>(key->getSurf());
				SDL_Rect keyPos{ 0, 0, (int)key->getWidth(), (int)key->getHeight() };
				keyPos.x = pos.w / 2 - keyPos.w / 2;
				keyPos.y = keyValue.second.render_offsety;

				SDL_BlitSurface(keySurf, nullptr, sprite, &keyPos);

				std::string writePath = keyValue.second.unpressedRenderedFullpath;

				if ( writePath[0] == '/' )
				{
					writePath.erase((size_t)0, (size_t)1);
				}

				if ( SDL_SavePNG(sprite, writePath.c_str()) == 0 )
				{
					printlog("[Glyph Export]: Successfully exported unpressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
				else
				{
					printlog("[Glyph Export]: Failed exporting unpressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
			}
			else
			{
				printlog("[Glyph Export]: Failed exporting unpressed glyph: %s", keyValue.second.keyname.c_str());
				++errors;
			}
			if ( sprite )
			{
				SDL_FreeSurface(sprite);
				sprite = nullptr;
			}
		}
		else
		{
			++errors;
		}

		base = Image::get(pressedPath.c_str());
		if ( base->getWidth() != 0 )
		{
			SDL_Surface* srcSurf = const_cast<SDL_Surface*>(base->getSurf());
			SDL_Rect pos{ 0, 0, (int)base->getWidth(), (int)base->getHeight() };
			SDL_Surface* sprite = SDL_CreateRGBSurface(0, pos.w, pos.h, 32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
			SDL_SetSurfaceAlphaMod(srcSurf, 255);
			SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);

			std::string keyPath = keyValue.second.fullpath;
			if ( keyPath[0] == '/' )
			{
				keyPath.erase((size_t)0, (size_t)1);
			}
			auto key = Image::get(keyPath.c_str());
			if ( key->getWidth() != 0 )
			{
				SDL_Surface* keySurf = const_cast<SDL_Surface*>(key->getSurf());
				SDL_Rect keyPos{ 0, 0, (int)key->getWidth(), (int)key->getHeight() };
				keyPos.x = pos.w / 2 - keyPos.w / 2;
				keyPos.y = keyValue.second.render_offsety;

				SDL_BlitSurface(keySurf, nullptr, sprite, &keyPos);

				std::string writePath = keyValue.second.pressedRenderedFullpath;
				if ( writePath[0] == '/' )
				{
					writePath.erase((size_t)0, (size_t)1);
				}

				if ( SDL_SavePNG(sprite, writePath.c_str()) == 0 )
				{
					printlog("[Glyph Export]: Successfully exported pressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
				else
				{
					printlog("[Glyph Export]: Failed exporting pressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
			}
			else
			{
				printlog("[Glyph Export]: Failed exporting pressed glyph: %s", keyValue.second.keyname.c_str());
				++errors;
			}
			if ( sprite )
			{
				SDL_FreeSurface(sprite);
				sprite = nullptr;
			}
		}
		else
		{
			++errors;
		}

	}

	printlog("[Glyph Export]: Completed export of %d glyphs with %d errors.", allGlyphs.size(), errors);
}

void ScriptTextParser_t::readAllScripts()
{
	allEntries.clear();

	std::string baseDir = "/data/scripts";
	auto files = physfsGetFileNamesInDirectory(baseDir.c_str());
	for ( auto& file : files )
	{
		std::string checkFile = baseDir + '/' + file;
		PHYSFS_Stat stat;
		if ( PHYSFS_stat(checkFile.c_str(), &stat) == 0 ) { continue; }

		if ( stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
		{
			auto files2 = physfsGetFileNamesInDirectory(checkFile.c_str());
			for ( auto& file2 : files2 )
			{
				std::string checkFile2 = checkFile + '/' + file2;
				if ( PHYSFS_stat(checkFile2.c_str(), &stat) == 0 ) { continue; }

				if ( stat.filetype != PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
				{
					readFromFile(checkFile2);
				}
			}
		}
		else
		{
			readFromFile(checkFile);
		}
	}
}

bool ScriptTextParser_t::readFromFile(const std::string& filename)
{
	if ( filename.find(".json") == std::string::npos )
	{
		return false;
	}
	if ( PHYSFS_getRealDir(filename.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(filename.c_str());
		inputPath.append(filename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		FileIO::close(fp);

		cJSON* d = cJSON_Parse(buf);
		if ( !d )
		{
			printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
			return false;
		}
		if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "script_entries") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			cJSON_Delete(d);
			return false;
		}

		Uint32 defaultFontColor = 0xFFFFFFFF;
		Uint32 defaultFontOutlineColor = 0;
		Uint32 defaultFontHighlightColor = 0xFFFFFFFF;
		Uint32 defaultFontHighlight2Color = 0xFFFFFFFF;

		auto readColor = [](cJSON* parent, const char* key) -> Uint32 {
			cJSON* colorObj = cJSON_GetObjectItem(parent, key);
			if ( !colorObj ) return 0xFFFFFFFF;
			cJSON* r = cJSON_GetObjectItem(colorObj, "r");
			cJSON* g = cJSON_GetObjectItem(colorObj, "g");
			cJSON* b = cJSON_GetObjectItem(colorObj, "b");
			cJSON* a = cJSON_GetObjectItem(colorObj, "a");
			return makeColor(
				r ? r->valueint : 255,
				g ? g->valueint : 255,
				b ? b->valueint : 255,
				a ? a->valueint : 255);
		};

		cJSON* defaultAttr = cJSON_GetObjectItem(d, "default_attributes");
		if ( defaultAttr )
		{
			if ( cJSON_HasObjectItem(defaultAttr, "font_color") )
				defaultFontColor = readColor(defaultAttr, "font_color");
			if ( cJSON_HasObjectItem(defaultAttr, "font_outline_color") )
				defaultFontOutlineColor = readColor(defaultAttr, "font_outline_color");
			if ( cJSON_HasObjectItem(defaultAttr, "font_highlight_color") )
				defaultFontHighlightColor = readColor(defaultAttr, "font_highlight_color");
			if ( cJSON_HasObjectItem(defaultAttr, "font_highlight2_color") )
				defaultFontHighlight2Color = readColor(defaultAttr, "font_highlight2_color");
		}

		cJSON* scriptEntries = cJSON_GetObjectItem(d, "script_entries");
		if ( scriptEntries )
		{
			for ( cJSON* entry_itr = scriptEntries->child; entry_itr; entry_itr = entry_itr->next )
			{
				std::string key = entry_itr->string;
				allEntries[key] = Entry_t();
				auto& entry = allEntries[key];
				entry.name = key;
				entry.fontColor = defaultFontColor;
				entry.fontOutlineColor = defaultFontOutlineColor;
				entry.fontHighlightColor = defaultFontHighlightColor;
				entry.fontHighlight2Color = defaultFontHighlight2Color;

				cJSON* signArr = cJSON_GetObjectItem(entry_itr, "sign");
				if ( signArr && cJSON_IsArray(signArr) )
				{
					entry.objectType = OBJ_SIGN;
					cJSON* text_itr = NULL;
					cJSON_ArrayForEach(text_itr, signArr)
					{
						if ( text_itr->valuestring )
							entry.rawText.push_back(text_itr->valuestring);
					}
					cJSON* vars = cJSON_GetObjectItem(entry_itr, "variables");
					if ( vars && cJSON_IsArray(vars) )
					{
						cJSON* var_itr = NULL;
						cJSON_ArrayForEach(var_itr, vars)
						{
							Entry_t::Variable_t variable;
							variable.type = TEXT;
							cJSON* typeItem = cJSON_GetObjectItem(var_itr, "type");
							if ( typeItem )
							{
								std::string typeTxt = typeItem->valuestring;
								if ( typeTxt == "text" )
									variable.type = TEXT;
								else if ( typeTxt == "input_glyph" )
									variable.type = GLYPH;
								else if ( typeTxt == "image" )
									variable.type = IMG;
							}
							cJSON* valItem = cJSON_GetObjectItem(var_itr, "value");
							if ( valItem ) variable.value = valItem->valuestring;
							cJSON* sizexItem = cJSON_GetObjectItem(var_itr, "sizex");
							if ( sizexItem ) variable.sizex = sizexItem->valueint;
							cJSON* sizeyItem = cJSON_GetObjectItem(var_itr, "sizey");
							if ( sizeyItem ) variable.sizey = sizeyItem->valueint;
							entry.variables.push_back(variable);
						}
					}
					for ( size_t s = 0; s < entry.rawText.size(); ++s )
						entry.padPerLine.push_back(0);
					cJSON* attr = cJSON_GetObjectItem(entry_itr, "attributes");
					if ( attr )
					{
						cJSON* fontItem = cJSON_GetObjectItem(attr, "font");
						if ( fontItem ) entry.font = fontItem->valuestring;
						cJSON* hjustItem = cJSON_GetObjectItem(attr, "horizontal_justify");
						if ( hjustItem )
						{
							std::string s = hjustItem->valuestring;
							if ( s == "center" ) entry.hjustify = Field::justify_t::CENTER;
							else if ( s == "left" ) entry.hjustify = Field::justify_t::LEFT;
							else if ( s == "right" ) entry.hjustify = Field::justify_t::RIGHT;
						}
						cJSON* vjustItem = cJSON_GetObjectItem(attr, "vertical_justify");
						if ( vjustItem )
						{
							std::string s = vjustItem->valuestring;
							if ( s == "center" ) entry.vjustify = Field::justify_t::CENTER;
							else if ( s == "top" ) entry.vjustify = Field::justify_t::TOP;
							else if ( s == "bottom" ) entry.vjustify = Field::justify_t::BOTTOM;
						}
						cJSON* topPadItem = cJSON_GetObjectItem(attr, "top_padding");
						if ( topPadItem ) entry.padTopY = topPadItem->valueint;
						cJSON* linePadItem = cJSON_GetObjectItem(attr, "line_padding");
						if ( linePadItem )
						{
							if ( cJSON_IsNumber(linePadItem) )
							{
								for ( auto& s : entry.padPerLine )
									s = linePadItem->valueint;
							}
							else if ( cJSON_IsArray(linePadItem) )
							{
								size_t s = 0;
								cJSON* arr_itr = NULL;
								cJSON_ArrayForEach(arr_itr, linePadItem)
								{
									entry.padPerLine[s] = arr_itr->valueint;
									++s;
									if ( s >= entry.padPerLine.size() ) break;
								}
							}
						}
						if ( cJSON_HasObjectItem(attr, "font_color") )
							entry.fontColor = readColor(attr, "font_color");
						if ( cJSON_HasObjectItem(attr, "font_outline_color") )
							entry.fontOutlineColor = readColor(attr, "font_outline_color");
						if ( cJSON_HasObjectItem(attr, "font_highlight_color") )
							entry.fontHighlightColor = readColor(attr, "font_highlight_color");
						if ( cJSON_HasObjectItem(attr, "font_highlight2_color") )
							entry.fontHighlight2Color = readColor(attr, "font_highlight2_color");
						cJSON* wordHL = cJSON_GetObjectItem(attr, "word_highlights");
						if ( wordHL && cJSON_IsArray(wordHL) )
						{
							int lineNumber = 0;
							cJSON* highlight_itr = NULL;
							cJSON_ArrayForEach(highlight_itr, wordHL)
							{
								if ( cJSON_IsArray(highlight_itr) )
								{
									cJSON* line_itr = NULL;
									cJSON_ArrayForEach(line_itr, highlight_itr)
									{
										entry.wordHighlights.push_back(lineNumber + line_itr->valueint);
									}
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
						cJSON* wordHL2 = cJSON_GetObjectItem(attr, "word_highlights2");
						if ( wordHL2 && cJSON_IsArray(wordHL2) )
						{
							int lineNumber = 0;
							cJSON* highlight_itr = NULL;
							cJSON_ArrayForEach(highlight_itr, wordHL2)
							{
								if ( cJSON_IsArray(highlight_itr) )
								{
									cJSON* line_itr = NULL;
									cJSON_ArrayForEach(line_itr, highlight_itr)
									{
										entry.wordHighlights2.push_back(lineNumber + line_itr->valueint);
									}
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
						cJSON* inlineImgItem = cJSON_GetObjectItem(attr, "inline_img_adjust_x");
						if ( inlineImgItem ) entry.imageInlineTextAdjustX = inlineImgItem->valueint;
						cJSON* video = cJSON_GetObjectItem(attr, "video");
						if ( video )
						{
							cJSON* v = cJSON_GetObjectItem(video, "path");
							if ( v ) entry.signVideoContent.path = v->valuestring;
							v = cJSON_GetObjectItem(video, "x");
							if ( v ) entry.signVideoContent.pos.x = v->valueint;
							v = cJSON_GetObjectItem(video, "y");
							if ( v ) entry.signVideoContent.pos.y = v->valueint;
							v = cJSON_GetObjectItem(video, "w");
							if ( v ) entry.signVideoContent.pos.w = v->valueint;
							v = cJSON_GetObjectItem(video, "h");
							if ( v ) entry.signVideoContent.pos.h = v->valueint;
							v = cJSON_GetObjectItem(video, "background_img");
							if ( v ) entry.signVideoContent.bgPath = v->valuestring;
							v = cJSON_GetObjectItem(video, "background_border");
							if ( v ) entry.signVideoContent.imgBorder = v->valueint;
						}
					}
				}
				else
				{
					cJSON* scriptItem = cJSON_GetObjectItem(entry_itr, "script");
					if ( scriptItem )
					{
						entry.objectType = OBJ_SCRIPT;
						entry.formattedText = scriptItem->valuestring;
					}
					else
					{
						const char* bubbleTypes[] = { "bubble_sign", "bubble_grave", "bubble_dialogue", "message" };
						ObjectType_t objTypes[] = { OBJ_BUBBLE_SIGN, OBJ_BUBBLE_GRAVE, OBJ_BUBBLE_DIALOGUE, OBJ_MESSAGE };
						for ( int bi = 0; bi < 4; ++bi )
						{
							cJSON* bubbleArr = cJSON_GetObjectItem(entry_itr, bubbleTypes[bi]);
							if ( bubbleArr && cJSON_IsArray(bubbleArr) )
							{
								entry.objectType = objTypes[bi];
								cJSON* text_itr = NULL;
								cJSON_ArrayForEach(text_itr, bubbleArr)
								{
									if ( text_itr->valuestring )
										entry.rawText.push_back(text_itr->valuestring);
								}
								entry.formattedText = "";
								for ( auto& str : entry.rawText )
								{
									if ( entry.formattedText != "" )
										entry.formattedText += '\n';
									entry.formattedText += str;
								}
								if ( objTypes[bi] == OBJ_MESSAGE )
								{
									cJSON* vars = cJSON_GetObjectItem(entry_itr, "variables");
									if ( vars && cJSON_IsArray(vars) )
									{
										cJSON* var_itr = NULL;
										cJSON_ArrayForEach(var_itr, vars)
										{
											Entry_t::Variable_t variable;
											variable.type = TEXT;
											cJSON* typeItem = cJSON_GetObjectItem(var_itr, "type");
											if ( typeItem )
											{
												std::string typeTxt = typeItem->valuestring;
												if ( typeTxt == "color_r" ) variable.type = COLOR_R;
												else if ( typeTxt == "color_g" ) variable.type = COLOR_G;
												else if ( typeTxt == "color_b" ) variable.type = COLOR_B;
											}
											cJSON* valItem = cJSON_GetObjectItem(var_itr, "value");
											if ( valItem )
											{
												if ( cJSON_IsNumber(valItem) )
													variable.numericValue = valItem->valueint;
												else if ( cJSON_IsString(valItem) )
													variable.value = valItem->valuestring;
											}
											entry.variables.push_back(variable);
										}
									}
								}
								break;
							}
						}
					}
				}
			}
		}
		cJSON_Delete(d);
		printlog("[JSON]: Successfully read json file %s, processed %d script variables", inputPath.c_str(), allEntries.size());
		return true;
	}
	printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
	return false;
}

void ScriptTextParser_t::writeWorldSignsToFile()
{
	cJSON* exportDocument = cJSON_CreateObject();
	cJSON_AddNumberToObject(exportDocument, "version", 1);
	cJSON* objScriptEntries = cJSON_AddObjectToObject(exportDocument, "script_entries");

	char suffix = 'a';
	char suffix2 = 'a';
	bool doubleSuffix = false;

	SDL_Color fontColor{ 255, 255, 255, 255 };
	SDL_Color fontOutline{ 29, 16, 11, 255 };
	SDL_Color fontHighlight{ 255, 0, 255, 255 };

	cJSON* objDefaultAttributes = cJSON_CreateObject();
	{
		auto addColor = [](cJSON* parent, const char* name, SDL_Color& c) {
			cJSON* obj = cJSON_CreateObject();
			cJSON_AddNumberToObject(obj, "r", c.r);
			cJSON_AddNumberToObject(obj, "g", c.g);
			cJSON_AddNumberToObject(obj, "b", c.b);
			cJSON_AddNumberToObject(obj, "a", c.a);
			cJSON_AddItemToObject(parent, name, obj);
		};
		addColor(objDefaultAttributes, "font_color", fontColor);
		addColor(objDefaultAttributes, "font_outline_color", fontOutline);
		addColor(objDefaultAttributes, "font_highlight_color", fontHighlight);
		addColor(objDefaultAttributes, "font_highlight2_color", fontHighlight);
	}
	cJSON_AddItemToObject(exportDocument, "default_attributes", objDefaultAttributes);

	for ( auto node = map.entities->first; node != NULL; node = node->next )
	{
		auto entity = (Entity*)node->element;
		if ( entity->behavior == &actFloorDecoration && entity->sprite == 991 /* sign */ )
		{
			std::string key = map.filename;
			size_t find = key.find(".lmp");
			key.erase(find, strlen(".lmp"));
			if ( doubleSuffix ) { key += suffix2; }
			key += suffix;
			if ( suffix == 'z' )
			{
				doubleSuffix = true;
				suffix = 'a';
			}
			suffix += 1;
			
			printlog("Sign '%s': x: %d y: %d", key.c_str(), static_cast<int>(entity->x) >> 4, static_cast<int>(entity->y) >> 4);

			cJSON* objEntry = cJSON_CreateObject();
			cJSON* arrSignText = cJSON_CreateArray();

			char buf[256] = "";
			int totalChars = 0;
			for ( int i = 8; i < 60; ++i )
			{
				if ( i == 28 )
					continue;
				if ( entity->skill[i] != 0 )
				{
					for ( int c = 0; c < 4; ++c )
					{
						buf[totalChars] = static_cast<char>((entity->skill[i] >> (c * 8)) & 0xFF);
						++totalChars;
					}
				}
			}
			if ( buf[totalChars] != '\0' )
				buf[totalChars] = '\0';
			std::string output = buf;

			std::vector<std::string> signText;
			int line = 0;
			signText.push_back("");
			for ( int i = 0; i < output.size(); ++i )
			{
				if ( i == 0 && output[0] == '#' )
					continue;
				if ( output[i] == '\0' )
					break;
				if ( output[i] == '\\' && (i + 1) < output.size() && output[i + 1] == 'n' )
				{
					++i;
					++line;
					signText.push_back("");
					continue;
				}
				signText[line] += output[i];
			}

			for ( auto& line : signText )
			{
				cJSON_AddItemToArray(arrSignText, cJSON_CreateString(line.c_str()));
			}
			cJSON_AddItemToObject(objEntry, "sign", arrSignText);

			cJSON* objAttributes = cJSON_CreateObject();
			{
				cJSON_AddStringToObject(objAttributes, "font", "fonts/pixel_maz_multiline.ttf#16#2");
				cJSON_AddStringToObject(objAttributes, "horizontal_justify", "center");
				cJSON_AddStringToObject(objAttributes, "vertical_justify", "center");
				cJSON_AddNumberToObject(objAttributes, "line_padding", 0);
				cJSON_AddNumberToObject(objAttributes, "top_padding", 0);
				cJSON_AddNumberToObject(objAttributes, "inline_img_adjust_x", 0);
				cJSON_AddArrayToObject(objAttributes, "word_highlights");
				cJSON_AddArrayToObject(objAttributes, "word_highlights2");
			}
			cJSON_AddItemToObject(objEntry, "attributes", objAttributes);
			cJSON_AddArrayToObject(objEntry, "variables");

			cJSON_AddItemToObject(objScriptEntries, key.c_str(), objEntry);
		}
	}

	std::string outputPath = PHYSFS_getRealDir("/data/scripts");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/scripts/script.json";
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
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
}



void MonsterData_t::loadMonsterDataJSON()
{
	if ( PHYSFS_getRealDir("/data/monster_data.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/monster_data.json");
		inputPath.append("/data/monster_data.json");

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
		if ( !d )
		{
			printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
			return;
		}
		if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "monsters") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			cJSON_Delete(d);
			return;
		}

		monsterDataEntries.clear();

		cJSON* basePathItem = cJSON_GetObjectItem(d, "base_path");
		const std::string baseIconPath = basePathItem ? basePathItem->valuestring : "";

		cJSON* monstersObj = cJSON_GetObjectItem(d, "monsters");
		if ( monstersObj )
		{
			for ( cJSON* itr = monstersObj->child; itr; itr = itr->next )
			{
				std::string monsterTypeName = itr->string;
				int monsterType = NOTHING;
				for ( int i = 0; i < NUMMONSTERS; ++i )
				{
					if ( monsterTypeName == monstertypename[i] )
					{
						monsterType = i;
						break;
					}
				}

				monsterDataEntries[monsterType] = MonsterDataEntry_t(monsterType);
				auto& entry = monsterDataEntries[monsterType];

				for ( cJSON* entry_itr = itr->child; entry_itr; entry_itr = entry_itr->next )
				{
					std::string key = entry_itr->string;
					if ( key == "specialNPCs" )
					{
						for ( cJSON* special_itr = entry_itr->child; special_itr; special_itr = special_itr->next )
						{
							bool foundIcon = false;
							std::string iconPath = "";
							cJSON* iconItem = cJSON_GetObjectItem(special_itr, "icon");
							if ( iconItem )
							{
								iconPath = iconItem->valuestring;
								if ( iconPath.size() > 0 )
								{
									foundIcon = true;
									iconPath = baseIconPath + iconPath;
								}
							}
							cJSON* modelsItem = cJSON_GetObjectItem(special_itr, "models");
							if ( modelsItem )
							{
								std::vector<int> models;
								if ( cJSON_IsArray(modelsItem) )
								{
									cJSON* array_itr = NULL;
									cJSON_ArrayForEach(array_itr, modelsItem)
									{
										models.push_back(array_itr->valueint);
									}
								}
								else if ( cJSON_IsNumber(modelsItem) )
								{
									models.push_back(modelsItem->valueint);
								}
								assert(models.size() > 0);
								int baseModel = models[0];
								cJSON* baseModelItem = cJSON_GetObjectItem(special_itr, "base_model");
								if ( baseModelItem )
								{
									baseModel = baseModelItem->valueint;
								}

								bool noOverrideIcon = false;
								cJSON* noOvItem = cJSON_GetObjectItem(special_itr, "no_override_icon");
								if ( noOvItem )
								{
									noOverrideIcon = cJSON_IsTrue(noOvItem);
								}
								entry.specialNPCs[special_itr->string] = MonsterDataEntry_t::SpecialNPCEntry_t();
								auto& specialNPC = entry.specialNPCs[special_itr->string];
								specialNPC.internalName = special_itr->string;
								cJSON* localizedName = cJSON_GetObjectItem(special_itr, "localized_name");
								if ( localizedName ) specialNPC.name = localizedName->valuestring;
								specialNPC.baseModel = baseModel;
								cJSON* shortName = cJSON_GetObjectItem(special_itr, "localized_short_name");
								if ( shortName ) specialNPC.shortname = shortName->valuestring;
								if ( foundIcon && noOverrideIcon )
								{
									specialNPC.uniqueIcon = iconPath;
								}
								for ( auto m : models )
								{
									entry.modelIndexes.insert(m);
									if ( foundIcon )
									{
										if ( !noOverrideIcon )
										{
											entry.iconSpritesAndPaths[m].iconPath = iconPath;
											entry.iconSpritesAndPaths[m].key = special_itr->string;
											entry.keyToSpriteLookup[special_itr->string].push_back(m);
										}
									}
									specialNPC.modelIndexes.insert(m);
								}
							}
						}
					}
					else
					{
						bool isPlayerSprite = (key.find("player") != std::string::npos) || monsterType == HUMAN;

						cJSON* iconItem = cJSON_GetObjectItem(entry_itr, "icon");
						if ( iconItem )
						{
							std::string iconPath = iconItem->valuestring;
							if ( iconPath.size() > 0 )
							{
								iconPath = baseIconPath + iconPath;
							}
							if ( key == "default" )
							{
								entry.defaultIconPath = iconPath;
								cJSON* shortName = cJSON_GetObjectItem(entry_itr, "localized_short_name");
								if ( shortName ) entry.defaultShortDisplayName = shortName->valuestring;
							}
							cJSON* modelsItem = cJSON_GetObjectItem(entry_itr, "models");
							if ( modelsItem )
							{
								std::vector<int> models;
								if ( cJSON_IsArray(modelsItem) )
								{
									cJSON* array_itr = NULL;
									cJSON_ArrayForEach(array_itr, modelsItem)
									{
										models.push_back(array_itr->valueint);
									}
								}
								else if ( cJSON_IsNumber(modelsItem) )
								{
									models.push_back(modelsItem->valueint);
								}

								for ( auto m : models )
								{
									if ( isPlayerSprite )
									{
										entry.playerModelIndexes.insert(m);
									}
									entry.modelIndexes.insert(m);
									entry.iconSpritesAndPaths[m].iconPath = iconPath;
									entry.iconSpritesAndPaths[m].key = entry_itr->string;
									entry.keyToSpriteLookup[entry_itr->string].push_back(m);
								}
							}
						}
					}
				}
			}
		}

		// validate data
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			for ( auto sprite : monsterSprites[i] )
			{
				if ( monsterDataEntries[i].modelIndexes.find(sprite) == monsterDataEntries[i].modelIndexes.end() )
				{
					printlog("[JSON]: Error: Could not find monster %s model index: %d", monstertypename[i], sprite);
				}
				if ( Entity::isPlayerHeadSprite(sprite) )
				{
					if ( monsterData.monsterDataEntries[i].playerModelIndexes.find(sprite) == monsterDataEntries[i].playerModelIndexes.end() )
					{
						printlog("[JSON]: Error: Could not find player %s model index: %d", monstertypename[i], sprite);
					}
				}
			}
		}
		cJSON_Delete(d);
		printlog("[JSON]: Successfully read json file %s, processed %d monsters", inputPath.c_str(), monsterDataEntries.size());
		return;
	}
	printlog("[JSON]: Error: Could not locate json file %s", "/data/monster_data.json");
}



std::map<int, std::vector<ShopkeeperConsumables_t::StoreSlots_t>> ShopkeeperConsumables_t::entries;
int ShopkeeperConsumables_t::consumableBuyValueMult = 100;
void ShopkeeperConsumables_t::readFromFile()
{
	const std::string filename = "data/shop_consumables.json";
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
	if ( !d )
	{
		printlog("[JSON]: Error: Could not parse json file %s", inputPath.c_str());
		return;
	}
	if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "store_types") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		cJSON_Delete(d);
		return;
	}

	cJSON* multItem = cJSON_GetObjectItem(d, "consumable_buy_value_multiplier");
	if ( multItem ) consumableBuyValueMult = multItem->valueint;

	entries.clear();
	cJSON* storeTypes = cJSON_GetObjectItem(d, "store_types");
	if ( storeTypes )
	{
		for ( cJSON* shoptypes = storeTypes->child; shoptypes; shoptypes = shoptypes->next )
		{
			int shoptype = -1;
			const std::string shopname = shoptypes->string;
			if ( shopname == "arms_armor" )
				shoptype = SHOP_TYPE_ARMS_ARMOR;
			else if ( shopname == "hats" )
				shoptype = SHOP_TYPE_HAT;
			else if ( shopname == "jewelry" )
				shoptype = SHOP_TYPE_JEWELRY;
			else if ( shopname == "books" )
				shoptype = SHOP_TYPE_BOOKS;
			else if ( shopname == "potions" )
				shoptype = SHOP_TYPE_POTIONS;
			else if ( shopname == "staffs" )
				shoptype = SHOP_TYPE_STAFFS;
			else if ( shopname == "food" )
				shoptype = SHOP_TYPE_FOOD;
			else if ( shopname == "hardware" )
				shoptype = SHOP_TYPE_HARDWARE;
			else if ( shopname == "hunting" )
				shoptype = SHOP_TYPE_HUNTING;
			else if ( shopname == "general" )
				shoptype = SHOP_TYPE_GENERAL;
			if ( shoptype == -1 )
				continue;

			cJSON* slotsObj = cJSON_GetObjectItem(shoptypes, "slots");
			if ( !slotsObj )
				continue;

			for ( cJSON* slots = slotsObj->child; slots; slots = slots->next )
			{
				cJSON* tradeReqItem = cJSON_GetObjectItem(slots, "trading_req");
				int tradeRequirement = tradeReqItem ? tradeReqItem->valueint : 0;

				cJSON* slotItems = cJSON_GetObjectItem(slots, "items");

				entries[shoptype].push_back(StoreSlots_t());
				auto& storeSlotData = entries[shoptype].at(entries[shoptype].size() - 1);

				storeSlotData.slotTradingReq = tradeRequirement;
				if ( slotItems && cJSON_IsArray(slotItems) )
				{
					cJSON* slot_itr = NULL;
					cJSON_ArrayForEach(slot_itr, slotItems)
					{
						storeSlotData.itemEntries.push_back(ItemEntry());
						auto& itemEntry = storeSlotData.itemEntries.at(storeSlotData.itemEntries.size() - 1);

						auto readStringOrArray = [](cJSON* parent, const char* key, std::vector<std::string>& out) {
							cJSON* member = cJSON_GetObjectItem(parent, key);
							if ( !member ) return;
							if ( cJSON_IsString(member) )
							{
								out.push_back(member->valuestring);
							}
							else if ( cJSON_IsArray(member) )
							{
								cJSON* arr = NULL;
								cJSON_ArrayForEach(arr, member)
								{
									if ( arr->valuestring )
										out.push_back(arr->valuestring);
								}
							}
						};
						auto readIntOrArray = [](cJSON* parent, const char* key, auto& out) {
							cJSON* member = cJSON_GetObjectItem(parent, key);
							if ( !member ) return;
							if ( cJSON_IsNumber(member) )
							{
								out.push_back(static_cast<typename std::decay<decltype(out)>::type::value_type>(member->valueint));
							}
							else if ( cJSON_IsArray(member) )
							{
								cJSON* arr = NULL;
								cJSON_ArrayForEach(arr, member)
								{
									out.push_back(static_cast<typename std::decay<decltype(out)>::type::value_type>(arr->valueint));
								}
							}
						};

						{
							std::vector<std::string> strings;
							readStringOrArray(slot_itr, "type", strings);
							for ( auto& s : strings )
							{
								if ( s == "empty" )
								{
									itemEntry.type.clear();
									break;
								}
								bool found = false;
								for ( int i = 0; i < NUMITEMS; ++i )
								{
									if ( s.compare(itemNameStrings[i + 2]) == 0 )
									{
										itemEntry.type.push_back(static_cast<ItemType>(i));
										found = true;
										break;
									}
								}
								assert(found);
							}
						}
						if ( itemEntry.type.empty() )
							itemEntry.emptyItemEntry = true;
						{
							std::vector<std::string> strings;
							readStringOrArray(slot_itr, "status", strings);
							for ( auto& s : strings )
							{
								if ( s == "broken" ) itemEntry.status.push_back(BROKEN);
								else if ( s == "decrepit" ) itemEntry.status.push_back(DECREPIT);
								else if ( s == "worn" ) itemEntry.status.push_back(WORN);
								else if ( s == "serviceable" ) itemEntry.status.push_back(SERVICABLE);
								else if ( s == "excellent" ) itemEntry.status.push_back(EXCELLENT);
							}
						}
						readIntOrArray(slot_itr, "beatitude", itemEntry.beatitude);
						readIntOrArray(slot_itr, "count", itemEntry.count);
						{
							cJSON* member = cJSON_GetObjectItem(slot_itr, "appearance");
							if ( member && !cJSON_IsString(member) )
							{
								std::vector<int> ints;
								readIntOrArray(slot_itr, "appearance", ints);
								for ( auto& i : ints )
									itemEntry.appearance.push_back((Uint32)i);
							}
						}
						{
							cJSON* member = cJSON_GetObjectItem(slot_itr, "identified");
							if ( member )
							{
								if ( cJSON_IsBool(member) )
								{
									itemEntry.identified.push_back(cJSON_IsTrue(member));
								}
								else if ( cJSON_IsArray(member) )
								{
									cJSON* arr = NULL;
									cJSON_ArrayForEach(arr, member)
									{
										itemEntry.identified.push_back(cJSON_IsTrue(arr));
									}
								}
							}
						}
						cJSON* spawnPct = cJSON_GetObjectItem(slot_itr, "spawn_percent_chance");
						if ( spawnPct ) itemEntry.percentChance = spawnPct->valueint;
						cJSON* dropPct = cJSON_GetObjectItem(slot_itr, "drop_percent_chance");
						if ( dropPct ) itemEntry.dropChance = dropPct->valueint;
						cJSON* weightChance = cJSON_GetObjectItem(slot_itr, "slot_weighted_chance");
						if ( weightChance ) itemEntry.weightedChance = weightChance->valueint;
					}
				}
			}
		}
	}
	cJSON_Delete(d);
	printlog("[JSON]: Successfully read json file %s, processed %d shop consumables", inputPath.c_str(), entries.size());
}
