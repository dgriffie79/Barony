/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_misc.cpp - Statue manager, debug timers, glyph renderer, script text parser, monster data, shop consumables
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
			rapidjson::StringBuffer os;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
			exportDocument.Accept(writer);
			fp->write(os.GetString(), sizeof(char), os.GetSize());
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
		if ( exportDocument.IsObject() )
		{
			exportDocument.RemoveAllMembers();
		}
		exportDocument.SetObject();
		CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(exportDocument, "statue_id", rapidjson::Value(local_rng.rand()));
		CustomHelpers::addMemberToRoot(exportDocument, "height_offset", rapidjson::Value(0));
		rapidjson::Value limbsObject(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(exportDocument, "limbs", limbsObject);
	}

	rapidjson::Value limbsArray(rapidjson::kArrayType);

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
		rapidjson::Value limbsObj(rapidjson::kObjectType);

		if ( index != 0 )
		{
			limbsObj.AddMember("x", rapidjson::Value(player->x - limb->x), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(player->y - limb->y), exportDocument.GetAllocator());
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
		limbsArray.PushBack(limbsObj, exportDocument.GetAllocator());

		++index;
	}

	CustomHelpers::addMemberToSubkey(exportDocument, "limbs", directionKeys[exportRotations], limbsArray);
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
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("limbs") || !d.HasMember("statue_id") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}
		int version = d["version"].GetInt();
		Uint32 statueId = d["statue_id"].GetUint();
		auto findStatue = allStatues.find(statueId);
		if ( findStatue != allStatues.end() )
		{
			allStatues.erase(findStatue);
		}
		allStatues.insert(std::make_pair(statueId, Statue_t()));
		for ( rapidjson::Value::ConstMemberIterator limb_itr = d["limbs"].MemberBegin(); limb_itr != d["limbs"].MemberEnd(); ++limb_itr )
		{
			auto& statue = allStatues[statueId];
			if ( d.HasMember("height_offset") )
			{
				statue.heightOffset = d["height_offset"].GetDouble();
			}
			for ( rapidjson::Value::ConstValueIterator dir_itr = limb_itr->value.Begin(); dir_itr != limb_itr->value.End(); ++dir_itr )
			{
				const rapidjson::Value& attributes = *dir_itr;
				std::string direction = limb_itr->name.GetString();
				auto& limbVector = statue.limbs[direction];
				limbVector.push_back(Statue_t::StatueLimb_t());
				auto& limb = limbVector[limbVector.size() - 1];
				limb.x = attributes["x"].GetDouble();
				limb.y = attributes["y"].GetDouble();
				limb.z = attributes["z"].GetDouble();
				limb.focalx = attributes["focalx"].GetDouble();
				limb.focaly = attributes["focaly"].GetDouble();
				limb.focalz = attributes["focalz"].GetDouble();
				limb.pitch = attributes["pitch"].GetDouble();
				limb.roll = attributes["roll"].GetDouble();
				limb.yaw = attributes["yaw"].GetDouble();
				limb.sprite = attributes["sprite"].GetInt();
			}
		}

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
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}

		allGlyphs.clear();
		if ( d.HasMember("rendered_glyph_folder") )
		{
			renderedGlyphFolder = d["rendered_glyph_folder"].GetString();
		}
		if ( d.HasMember("base_glyph_folder") )
		{
			baseSourceFolder = d["base_glyph_folder"].GetString();
		}
		if ( d.HasMember("small_key_unpressed_path") )
		{
			baseUnpressedGlyphPath = d["small_key_unpressed_path"].GetString();
		}
		if ( d.HasMember("small_key_pressed_path") )
		{
			basePressedGlyphPath = d["small_key_pressed_path"].GetString();
		}
		int render_offsety = 0;
		if ( d.HasMember("base_render_offset_y") )
		{
			render_offsety = d["base_render_offset_y"].GetInt();
		}
		pressedRenderedPrefix = "Pressed_";
		if ( d.HasMember("pressed_rendered_glyph_prefix") )
		{
			pressedRenderedPrefix = d["pressed_rendered_glyph_prefix"].GetString();
		}
		unpressedRenderedPrefix = "Unpressed_";
		if ( d.HasMember("unpressed_rendered_glyph_prefix") )
		{
			unpressedRenderedPrefix = d["unpressed_rendered_glyph_prefix"].GetString();
		}

		std::string basePath = baseSourceFolder;
		basePath += '/';
		std::string baseRenderedPath = basePath;
		baseRenderedPath += renderedGlyphFolder;
		baseRenderedPath += '/';

		for ( rapidjson::Value::ConstValueIterator glyph_itr = d["glyphs"].Begin(); glyph_itr != d["glyphs"].End(); ++glyph_itr )
		{
			const rapidjson::Value& attributes = *glyph_itr;
			if ( !attributes.HasMember("keyname") )
			{
				printlog("[JSON]: Glyph entry does not have keyname, skipping...");
				continue;
			}
			std::string keyname = attributes["keyname"].GetString();
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
			if ( !attributes.HasMember("folder") )
			{
				printlog("[JSON]: Glyph entry does not have base folder entry, skipping...");
				continue;
			}
			glyphData.folder = attributes["folder"].GetString();
			if ( !attributes.HasMember("path") )
			{
				printlog("[JSON]: Glyph entry does not have glyph path name, skipping...");
				continue;
			}
			glyphData.filename = attributes["path"].GetString();
			if ( attributes.HasMember("custom_render_offset_y") )
			{
				if ( attributes["custom_render_offset_y"].GetInt() != 0 )
				{
					glyphData.render_offsety = attributes["custom_render_offset_y"].GetInt();
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
			if ( attributes.HasMember("pressed_glyph_background_path") )
			{
				glyphData.pressedGlyphPath = attributes["pressed_glyph_background_path"].GetString();
			}
			else
			{
				glyphData.pressedGlyphPath = basePressedGlyphPath;
			}
			if ( attributes.HasMember("unpressed_glyph_background_path") )
			{
				glyphData.unpressedGlyphPath = attributes["unpressed_glyph_background_path"].GetString();
			}
			else
			{
				glyphData.unpressedGlyphPath = baseUnpressedGlyphPath;
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

			if ( !PHYSFS_getRealDir(glyphData.fullpath.c_str()) ) // you need single forward '/' slashes for getRealDir to report true
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
			// successfully loaded, do unpressed glyph
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
				// successfully loaded
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
			// successfully loaded, do pressed glyph
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
				// successfully loaded
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
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("script_entries") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}

		Uint32 defaultFontColor = 0xFFFFFFFF;
		Uint32 defaultFontOutlineColor = 0;
		Uint32 defaultFontHighlightColor = 0xFFFFFFFF;
		Uint32 defaultFontHighlight2Color = 0xFFFFFFFF;
		if ( d.HasMember("default_attributes") )
		{
			if ( d["default_attributes"].HasMember("font_color") )
			{
				defaultFontColor = makeColor(
					d["default_attributes"]["font_color"]["r"].GetInt(),
					d["default_attributes"]["font_color"]["g"].GetInt(),
					d["default_attributes"]["font_color"]["b"].GetInt(),
					d["default_attributes"]["font_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_outline_color") )
			{
				defaultFontOutlineColor = makeColor(
					d["default_attributes"]["font_outline_color"]["r"].GetInt(),
					d["default_attributes"]["font_outline_color"]["g"].GetInt(),
					d["default_attributes"]["font_outline_color"]["b"].GetInt(),
					d["default_attributes"]["font_outline_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_highlight_color") )
			{
				defaultFontHighlightColor = makeColor(
					d["default_attributes"]["font_highlight_color"]["r"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["g"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["b"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_highlight2_color") )
			{
				defaultFontHighlight2Color = makeColor(
					d["default_attributes"]["font_highlight2_color"]["r"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["g"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["b"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["a"].GetInt());
			}
		}

		for ( rapidjson::Value::ConstMemberIterator entry_itr = d["script_entries"].MemberBegin(); entry_itr != d["script_entries"].MemberEnd(); ++entry_itr )
		{
			std::string key = entry_itr->name.GetString();
			allEntries[key] = Entry_t();
			auto& entry = allEntries[key];
			entry.name = key;
			entry.fontColor = defaultFontColor;
			entry.fontOutlineColor = defaultFontOutlineColor;
			entry.fontHighlightColor = defaultFontHighlightColor;
			entry.fontHighlight2Color = defaultFontHighlight2Color;
			if ( entry_itr->value.HasMember("sign") )
			{
				entry.objectType = OBJ_SIGN;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["sign"].Begin(); text_itr != entry_itr->value["sign"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				if ( entry_itr->value.HasMember("variables") )
				{
					for ( rapidjson::Value::ConstValueIterator var_itr = entry_itr->value["variables"].Begin();
						var_itr != entry_itr->value["variables"].End(); ++var_itr )
					{
						Entry_t::Variable_t variable;
						variable.type = TEXT;
						if ( (*var_itr).HasMember("type") )
						{
							std::string typeTxt = (*var_itr)["type"].GetString();
							if ( typeTxt == "text" )
							{
								variable.type = TEXT;
							}
							else if ( typeTxt == "input_glyph" )
							{
								variable.type = GLYPH;
							}
							else if ( typeTxt == "image" )
							{
								variable.type = IMG;
							}
						}
						if ( (*var_itr).HasMember("value") )
						{
							variable.value = (*var_itr)["value"].GetString();
						}
						if ( (*var_itr).HasMember("sizex") )
						{
							variable.sizex = (*var_itr)["sizex"].GetInt();
						}
						if ( (*var_itr).HasMember("sizey") )
						{
							variable.sizey = (*var_itr)["sizey"].GetInt();
						}
						entry.variables.push_back(variable);
					}
				}
				for ( size_t s = 0; s < entry.rawText.size(); ++s )
				{
					entry.padPerLine.push_back(0);
				}
				if ( entry_itr->value.HasMember("attributes") )
				{
					if ( entry_itr->value["attributes"].HasMember("font") )
					{
						entry.font = entry_itr->value["attributes"]["font"].GetString();
					}
					if ( entry_itr->value["attributes"].HasMember("horizontal_justify") )
					{
						std::string s = entry_itr->value["attributes"]["horizontal_justify"].GetString();
						if ( s == "center" )
						{
							entry.hjustify = Field::justify_t::CENTER;
						}
						else if ( s == "left" )
						{
							entry.hjustify = Field::justify_t::LEFT;
						}
						else if ( s == "right" )
						{
							entry.hjustify = Field::justify_t::RIGHT;
						}
					}
					if ( entry_itr->value["attributes"].HasMember("vertical_justify") )
					{
						std::string s = entry_itr->value["attributes"]["vertical_justify"].GetString();
						if ( s == "center" )
						{
							entry.vjustify = Field::justify_t::CENTER;
						}
						else if ( s == "top" )
						{
							entry.vjustify = Field::justify_t::TOP;
						}
						else if ( s == "bottom" )
						{
							entry.vjustify = Field::justify_t::BOTTOM;
						}
					}
					if ( entry_itr->value["attributes"].HasMember("top_padding") )
					{
						entry.padTopY = entry_itr->value["attributes"]["top_padding"].GetInt();
					}
					if ( entry_itr->value["attributes"].HasMember("line_padding") )
					{
						if ( entry_itr->value["attributes"]["line_padding"].IsInt() )
						{
							for ( auto& s : entry.padPerLine )
							{
								s = entry_itr->value["attributes"]["line_padding"].GetInt();
							}
						}
						else if ( entry_itr->value["attributes"]["line_padding"].IsArray() )
						{
							size_t s = 0;
							for ( auto arr_itr = entry_itr->value["attributes"]["line_padding"].Begin();
								arr_itr != entry_itr->value["attributes"]["line_padding"].End(); ++arr_itr )
							{
								entry.padPerLine[s] = arr_itr->GetInt();
								++s;
								if ( s >= entry.padPerLine.size() )
								{
									break;
								}
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("font_color") )
					{
						entry.fontColor = makeColor(
							entry_itr->value["attributes"]["font_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_outline_color") )
					{
						entry.fontOutlineColor = makeColor(
							entry_itr->value["attributes"]["font_outline_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_highlight_color") )
					{
						entry.fontHighlightColor = makeColor(
							entry_itr->value["attributes"]["font_highlight_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_highlight2_color") )
					{
						entry.fontHighlight2Color = makeColor(
							entry_itr->value["attributes"]["font_highlight2_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("word_highlights") )
					{
						if ( entry_itr->value["attributes"]["word_highlights"].IsArray() )
						{
							int lineNumber = 0;
							for ( auto highlight_itr = entry_itr->value["attributes"]["word_highlights"].Begin();
								highlight_itr != entry_itr->value["attributes"]["word_highlights"].End(); ++highlight_itr )
							{
								for ( auto line_itr = (*highlight_itr).Begin(); line_itr != (*highlight_itr).End(); ++line_itr )
								{
									entry.wordHighlights.push_back(lineNumber + line_itr->GetInt());
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("word_highlights2") )
					{
						if ( entry_itr->value["attributes"]["word_highlights2"].IsArray() )
						{
							int lineNumber = 0;
							for ( auto highlight_itr = entry_itr->value["attributes"]["word_highlights2"].Begin();
								highlight_itr != entry_itr->value["attributes"]["word_highlights2"].End(); ++highlight_itr )
							{
								for ( auto line_itr = (*highlight_itr).Begin(); line_itr != (*highlight_itr).End(); ++line_itr )
								{
									entry.wordHighlights2.push_back(lineNumber + line_itr->GetInt());
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("inline_img_adjust_x") )
					{
						entry.imageInlineTextAdjustX = entry_itr->value["attributes"]["inline_img_adjust_x"].GetInt();
					}
					if ( entry_itr->value["attributes"].HasMember("video") )
					{
						if ( entry_itr->value["attributes"]["video"].HasMember("path") )
						{
							entry.signVideoContent.path = entry_itr->value["attributes"]["video"]["path"].GetString();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("x") )
						{
							entry.signVideoContent.pos.x = entry_itr->value["attributes"]["video"]["x"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("y") )
						{
							entry.signVideoContent.pos.y = entry_itr->value["attributes"]["video"]["y"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("w") )
						{
							entry.signVideoContent.pos.w = entry_itr->value["attributes"]["video"]["w"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("h") )
						{
							entry.signVideoContent.pos.h = entry_itr->value["attributes"]["video"]["h"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("background_img") )
						{
							entry.signVideoContent.bgPath = entry_itr->value["attributes"]["video"]["background_img"].GetString();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("background_border") )
						{
							entry.signVideoContent.imgBorder = entry_itr->value["attributes"]["video"]["background_border"].GetInt();
						}
					}
				}
			}
			else if ( entry_itr->value.HasMember("script") )
			{
				entry.objectType = OBJ_SCRIPT;
				entry.formattedText = entry_itr->value["script"].GetString();
			}
			else if ( entry_itr->value.HasMember("bubble_sign") )
			{
				entry.objectType = OBJ_BUBBLE_SIGN;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["bubble_sign"].Begin(); text_itr != entry_itr->value["bubble_sign"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				entry.formattedText = "";
				for ( auto& str : entry.rawText )
				{
					if ( entry.formattedText != "" )
					{
						entry.formattedText += '\n';
					}
					entry.formattedText += str;
				}
			}
			else if ( entry_itr->value.HasMember("bubble_grave") )
			{
				entry.objectType = OBJ_BUBBLE_GRAVE;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["bubble_grave"].Begin(); text_itr != entry_itr->value["bubble_grave"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				entry.formattedText = "";
				for ( auto& str : entry.rawText )
				{
					if ( entry.formattedText != "" )
					{
						entry.formattedText += '\n';
					}
					entry.formattedText += str;
				}
			}
			else if ( entry_itr->value.HasMember("bubble_dialogue") )
			{
				entry.objectType = OBJ_BUBBLE_DIALOGUE;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["bubble_dialogue"].Begin(); text_itr != entry_itr->value["bubble_dialogue"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				entry.formattedText = "";
				for ( auto& str : entry.rawText )
				{
					if ( entry.formattedText != "" )
					{
						entry.formattedText += '\n';
					}
					entry.formattedText += str;
				}
			}
			else if ( entry_itr->value.HasMember("message") )
			{
				entry.objectType = OBJ_MESSAGE;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["message"].Begin(); text_itr != entry_itr->value["message"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				entry.formattedText = "";
				for ( auto& str : entry.rawText )
				{
					if ( entry.formattedText != "" )
					{
						entry.formattedText += '\n';
					}
					entry.formattedText += str;
				}
				if ( entry_itr->value.HasMember("variables") )
				{
					for ( rapidjson::Value::ConstValueIterator var_itr = entry_itr->value["variables"].Begin();
						var_itr != entry_itr->value["variables"].End(); ++var_itr )
					{
						Entry_t::Variable_t variable;
						variable.type = TEXT;
						if ( (*var_itr).HasMember("type") )
						{
							std::string typeTxt = (*var_itr)["type"].GetString();
							if ( typeTxt == "color_r" )
							{
								variable.type = COLOR_R;
							}
							else if ( typeTxt == "color_g" )
							{
								variable.type = COLOR_G;
							}
							else if ( typeTxt == "color_b" )
							{
								variable.type = COLOR_B;
							}
						}
						if ( (*var_itr).HasMember("value") )
						{
							if ( (*var_itr)["value"].IsInt() )
							{
								variable.numericValue = (*var_itr)["value"].GetInt();
							}
							else if ( (*var_itr)["value"].IsString() )
							{
								variable.value = (*var_itr)["value"].GetString();
							}
						}
						entry.variables.push_back(variable);
					}
				}
			}
		}

		printlog("[JSON]: Successfully read json file %s, processed %d script variables", inputPath.c_str(), allEntries.size());
		return true;
	}
	printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
	return false;
}

void ScriptTextParser_t::writeWorldSignsToFile()
{
	rapidjson::Document exportDocument;
	exportDocument.SetObject();
	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));
	rapidjson::Value objScriptEntries(rapidjson::kObjectType);

	char suffix = 'a';
	char suffix2 = 'a';
	bool doubleSuffix = false;

	SDL_Color fontColor{ 255, 255, 255, 255 };
	SDL_Color fontOutline{ 29, 16, 11, 255 };
	SDL_Color fontHighlight{ 255, 0, 255, 255 };

	rapidjson::Value objDefaultAttributes(rapidjson::kObjectType);
	{
		rapidjson::Value objFontColor(rapidjson::kObjectType);
		objFontColor.AddMember("r", rapidjson::Value(fontColor.r), exportDocument.GetAllocator());
		objFontColor.AddMember("g", rapidjson::Value(fontColor.g), exportDocument.GetAllocator());
		objFontColor.AddMember("b", rapidjson::Value(fontColor.b), exportDocument.GetAllocator());
		objFontColor.AddMember("a", rapidjson::Value(fontColor.a), exportDocument.GetAllocator());

		rapidjson::Value objFontOutlineColor(rapidjson::kObjectType);
		objFontOutlineColor.AddMember("r", rapidjson::Value(fontOutline.r), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("g", rapidjson::Value(fontOutline.g), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("b", rapidjson::Value(fontOutline.b), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("a", rapidjson::Value(fontOutline.a), exportDocument.GetAllocator());

		rapidjson::Value objFontHighlightColor(rapidjson::kObjectType);
		objFontHighlightColor.AddMember("r", rapidjson::Value(fontHighlight.r), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("g", rapidjson::Value(fontHighlight.g), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("b", rapidjson::Value(fontHighlight.b), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("a", rapidjson::Value(fontHighlight.a), exportDocument.GetAllocator());

		rapidjson::Value objFontHighlight2Color(rapidjson::kObjectType);
		objFontHighlight2Color.AddMember("r", rapidjson::Value(fontHighlight.r), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("g", rapidjson::Value(fontHighlight.g), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("b", rapidjson::Value(fontHighlight.b), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("a", rapidjson::Value(fontHighlight.a), exportDocument.GetAllocator());

		objDefaultAttributes.AddMember("font_color", objFontColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_outline_color", objFontOutlineColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_highlight_color", objFontHighlightColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_highlight2_color", objFontHighlight2Color, exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "default_attributes", objDefaultAttributes);

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

			rapidjson::Value objEntry(rapidjson::kObjectType);
			rapidjson::Value arrSignText(rapidjson::kArrayType);

			// assemble the string.
			char buf[256] = "";
			int totalChars = 0;
			for ( int i = 8; i < 60; ++i )
			{
				if ( i == 28 ) // circuit_status
				{
					continue;
				}
				if ( entity->skill[i] != 0 )
				{
					for ( int c = 0; c < 4; ++c )
					{
						buf[totalChars] = static_cast<char>((entity->skill[i] >> (c * 8)) & 0xFF);
						//messagePlayer(0, "%d %d", i, c);
						++totalChars;
					}
				}
			}
			if ( buf[totalChars] != '\0' )
			{
				buf[totalChars] = '\0';
			}
			std::string output = buf;

			std::vector<std::string> signText;
			int line = 0;
			signText.push_back("");
			for ( int i = 0; i < output.size(); ++i )
			{
				if ( i == 0 && output[0] == '#' )
				{
					continue;
				}
				if ( output[i] == '\0' )
				{
					break;
				}
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
				rapidjson::Value lineString(line.c_str(), exportDocument.GetAllocator());
				arrSignText.PushBack(lineString, exportDocument.GetAllocator());
			}
			objEntry.AddMember("sign", arrSignText, exportDocument.GetAllocator());

			rapidjson::Value objAttributes(rapidjson::kObjectType);
			{
				objAttributes.AddMember("font", rapidjson::StringRef("fonts/pixel_maz_multiline.ttf#16#2"), exportDocument.GetAllocator());
				objAttributes.AddMember("horizontal_justify", rapidjson::StringRef("center"), exportDocument.GetAllocator());
				objAttributes.AddMember("vertical_justify", rapidjson::StringRef("center"), exportDocument.GetAllocator());
				objAttributes.AddMember("line_padding", rapidjson::Value(0), exportDocument.GetAllocator());
				objAttributes.AddMember("top_padding", rapidjson::Value(0), exportDocument.GetAllocator());
				objAttributes.AddMember("inline_img_adjust_x", rapidjson::Value(0), exportDocument.GetAllocator());
				rapidjson::Value objWordHighlights(rapidjson::kArrayType);
				objAttributes.AddMember("word_highlights", objWordHighlights, exportDocument.GetAllocator());
				rapidjson::Value objWordHighlights2(rapidjson::kArrayType);
				objAttributes.AddMember("word_highlights2", objWordHighlights2, exportDocument.GetAllocator());
			}
			objEntry.AddMember("attributes", objAttributes, exportDocument.GetAllocator());

			rapidjson::Value objVariables(rapidjson::kArrayType);
			objEntry.AddMember("variables", objVariables, exportDocument.GetAllocator());

			rapidjson::Value entryName(key.c_str(), exportDocument.GetAllocator());
			objScriptEntries.AddMember(entryName, objEntry, exportDocument.GetAllocator());
		}
	}

	CustomHelpers::addMemberToRoot(exportDocument, "script_entries", objScriptEntries);

	std::string outputPath = PHYSFS_getRealDir("/data/scripts");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/scripts/script.json";
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
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("monsters") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}

		monsterDataEntries.clear();

		const std::string baseIconPath = d["base_path"].GetString();

		for ( auto itr = d["monsters"].MemberBegin(); itr != d["monsters"].MemberEnd(); ++itr )
		{
			std::string monsterTypeName = itr->name.GetString();
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

			for ( auto entry_itr = itr->value.MemberBegin(); entry_itr != itr->value.MemberEnd(); ++entry_itr )
			{
				std::string key = entry_itr->name.GetString();
				if ( key == "specialNPCs" )
				{
					for ( auto special_itr = entry_itr->value.MemberBegin(); special_itr != entry_itr->value.MemberEnd(); ++special_itr )
					{
						bool foundIcon = false;
						std::string iconPath = "";
						if ( special_itr->value.HasMember("icon") )
						{
							iconPath = special_itr->value["icon"].GetString();
							if ( iconPath.size() > 0 )
							{
								foundIcon = true;
								iconPath = baseIconPath + iconPath;
							}
						}
						if ( special_itr->value.HasMember("models") )
						{
							std::vector<int> models;
							if ( special_itr->value["models"].IsArray() )
							{
								for ( auto array_itr = special_itr->value["models"].Begin(); array_itr != special_itr->value["models"].End(); ++array_itr )
								{
									models.push_back(array_itr->GetInt());
								}
							}
							else if ( special_itr->value["models"].IsInt() )
							{
								models.push_back(special_itr->value["models"].GetInt());
							}
							assert(models.size() > 0);
							int baseModel = models[0];
							if ( special_itr->value.HasMember("base_model") )
							{
								baseModel = special_itr->value["base_model"].GetInt();
							}

							bool noOverrideIcon = false; // special case, handling monsters with unique icons but no unique sprites
							if ( special_itr->value.HasMember("no_override_icon") )
							{
								noOverrideIcon = special_itr->value["no_override_icon"].GetBool();
							}
							entry.specialNPCs[special_itr->name.GetString()] = MonsterDataEntry_t::SpecialNPCEntry_t();
							auto& specialNPC = entry.specialNPCs[special_itr->name.GetString()];
							specialNPC.internalName = special_itr->name.GetString();
							specialNPC.name = special_itr->value["localized_name"].GetString();
							specialNPC.baseModel = baseModel;
							if ( special_itr->value.HasMember("localized_short_name") )
							{
								specialNPC.shortname = special_itr->value["localized_short_name"].GetString();
							}
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
										entry.iconSpritesAndPaths[m].key = special_itr->name.GetString();
										entry.keyToSpriteLookup[special_itr->name.GetString()].push_back(m);
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

					if ( entry_itr->value.HasMember("icon") )
					{
						std::string iconPath = entry_itr->value["icon"].GetString();
						if ( iconPath.size() > 0 )
						{
							iconPath = baseIconPath + iconPath;
						}
						if ( key == "default" )
						{
							entry.defaultIconPath = iconPath;
							if ( entry_itr->value.HasMember("localized_short_name") )
							{
								entry.defaultShortDisplayName = entry_itr->value["localized_short_name"].GetString();
							}
						}
						if ( entry_itr->value.HasMember("models") )
						{
							std::vector<int> models;
							if ( entry_itr->value["models"].IsArray() )
							{
								for ( auto array_itr = entry_itr->value["models"].Begin(); array_itr != entry_itr->value["models"].End(); ++array_itr )
								{
									models.push_back(array_itr->GetInt());
								}
							}
							else if ( entry_itr->value["models"].IsInt() )
							{
								models.push_back(entry_itr->value["models"].GetInt());
							}

							for ( auto m : models )
							{
								if ( isPlayerSprite )
								{
									entry.playerModelIndexes.insert(m);
								}
								entry.modelIndexes.insert(m);
								entry.iconSpritesAndPaths[m].iconPath = iconPath;
								entry.iconSpritesAndPaths[m].key = entry_itr->name.GetString();
								entry.keyToSpriteLookup[entry_itr->name.GetString()].push_back(m);
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
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("store_types") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	consumableBuyValueMult = 100;
	if ( d.HasMember("consumable_buy_value_multiplier") )
	{
		consumableBuyValueMult = d["consumable_buy_value_multiplier"].GetInt();
	}

	entries.clear();
	for ( auto shoptypes = d["store_types"].MemberBegin(); shoptypes != d["store_types"].MemberEnd(); ++shoptypes )
	{
		int shoptype = -1;
		const std::string shopname = shoptypes->name.GetString();
		if ( shopname == "arms_armor" )
		{
			shoptype = SHOP_TYPE_ARMS_ARMOR;
		}
		else if ( shopname == "hats" )
		{
			shoptype = SHOP_TYPE_HAT;
		}
		else if ( shopname == "jewelry" )
		{
			shoptype = SHOP_TYPE_JEWELRY;
		}
		else if ( shopname == "books" )
		{
			shoptype = SHOP_TYPE_BOOKS;
		}
		else if ( shopname == "potions" )
		{
			shoptype = SHOP_TYPE_POTIONS;
		}
		else if ( shopname == "staffs" )
		{
			shoptype = SHOP_TYPE_STAFFS;
		}
		else if ( shopname == "food" )
		{
			shoptype = SHOP_TYPE_FOOD;
		}
		else if ( shopname == "hardware" )
		{
			shoptype = SHOP_TYPE_HARDWARE;
		}
		else if ( shopname == "hunting" )
		{
			shoptype = SHOP_TYPE_HUNTING;
		}
		else if ( shopname == "general" )
		{
			shoptype = SHOP_TYPE_GENERAL;
		}
		if ( shoptype == -1 )
		{
			continue;
		}

		if ( !shoptypes->value.HasMember("slots") )
		{
			continue;
		}

		for ( auto slots = shoptypes->value["slots"].MemberBegin(); slots != shoptypes->value["slots"].MemberEnd(); ++slots )
		{
			auto& slot = slots->value;
			int tradeRequirement = slot["trading_req"].GetInt();

			auto& slotItems = slot["items"];

			entries[shoptype].push_back(StoreSlots_t());
			auto& storeSlotData = entries[shoptype].at(entries[shoptype].size() - 1);

			storeSlotData.slotTradingReq = tradeRequirement;
			for ( auto slot_itr = slotItems.Begin(); slot_itr != slotItems.End(); ++slot_itr )
			{
				storeSlotData.itemEntries.push_back(ItemEntry());
				auto& itemEntry = storeSlotData.itemEntries.at(storeSlotData.itemEntries.size() - 1);

				{
					auto& member = (*slot_itr)["type"];
					bool isArr = member.IsArray();
					std::vector<std::string> strings;
					if ( !isArr )
					{
						strings.push_back(member.GetString());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							strings.push_back(arr->GetString());
						}
					}
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
				{
					itemEntry.emptyItemEntry = true;
				}
				{
					auto& member = (*slot_itr)["status"];
					bool isArr = member.IsArray();
					std::vector<std::string> strings;
					if ( !isArr )
					{
						strings.push_back(member.GetString());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							strings.push_back(arr->GetString());
						}
					}
					for ( auto& s : strings )
					{
						if ( s == "broken" )
						{
							itemEntry.status.push_back(BROKEN);
						}
						else if ( s == "decrepit" )
						{
							itemEntry.status.push_back(DECREPIT);
						}
						else if ( s == "worn" )
						{
							itemEntry.status.push_back(WORN);
						}
						else if ( s == "serviceable" )
						{
							itemEntry.status.push_back(SERVICABLE);
						}
						else if ( s == "excellent" )
						{
							itemEntry.status.push_back(EXCELLENT);
						}
					}
				}
				{
					auto& member = (*slot_itr)["beatitude"];
					bool isArr = member.IsArray();
					std::vector<int> ints;
					if ( !isArr )
					{
						ints.push_back(member.GetInt());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							ints.push_back(arr->GetInt());
						}
					}
					for ( auto& i : ints )
					{
						itemEntry.beatitude.push_back(i);
					}
				}
				{
					auto& member = (*slot_itr)["count"];
					bool isArr = member.IsArray();
					std::vector<int> ints;
					if ( !isArr )
					{
						ints.push_back(member.GetInt());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							ints.push_back(arr->GetInt());
						}
					}
					for ( auto& i : ints )
					{
						itemEntry.count.push_back(i);
					}
				}
				{
					auto& member = (*slot_itr)["appearance"];
					bool isArr = member.IsArray();
					std::vector<Uint32> ints;
					if ( !member.IsString() )
					{
						if ( !isArr )
						{
							ints.push_back(member.GetUint());
						}
						else
						{
							for ( auto arr = member.Begin(); arr != member.End(); ++arr )
							{
								ints.push_back(arr->GetUint());
							}
						}
						for ( auto& i : ints )
						{
							itemEntry.appearance.push_back(i);
						}
					}
				}
				{
					auto& member = (*slot_itr)["identified"];
					bool isArr = member.IsArray();
					std::vector<bool> bools;
					if ( !isArr )
					{
						bools.push_back(member.GetBool());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							bools.push_back(arr->GetBool());
						}
					}
					for ( auto b : bools )
					{
						itemEntry.identified.push_back(b);
					}
				}
				itemEntry.percentChance = (*slot_itr)["spawn_percent_chance"].GetInt();
				itemEntry.dropChance = (*slot_itr)["drop_percent_chance"].GetInt();
				itemEntry.weightedChance = (*slot_itr)["slot_weighted_chance"].GetInt();
			}
		}
	}

	printlog("[JSON]: Successfully read json file %s, processed %d shop consumables", inputPath.c_str(), entries.size());
}

