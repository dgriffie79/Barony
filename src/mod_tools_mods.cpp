/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_mods.cpp - Mod loading and management
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

std::vector<int> Mods::modelsListModifiedIndexes;
std::vector<int> Mods::soundsListModifiedIndexes;
std::vector<std::pair<SDL_Surface**, std::string>> Mods::systemResourceImagesToReload;
std::vector<std::pair<std::string, std::string>> Mods::mountedFilepaths;
std::vector<std::pair<std::string, std::string>> Mods::mountedFilepathsSaved;
std::set<std::string> Mods::mods_loaded_local;
std::set<std::string> Mods::mods_loaded_workshop;
std::list<std::string> Mods::localModFoldernames;
int Mods::numCurrentModsLoaded = -1;
bool Mods::modelsListRequiresReloadUnmodded = false;
bool Mods::soundListRequiresReloadUnmodded = false;
bool Mods::tileListRequireReloadUnmodded = false;
bool Mods::spriteImagesRequireReloadUnmodded = false;
bool Mods::booksRequireReloadUnmodded = false;
bool Mods::musicRequireReloadUnmodded = false;
bool Mods::langRequireReloadUnmodded = false;
bool Mods::monsterLimbsRequireReloadUnmodded = false;
bool Mods::systemImagesReloadUnmodded = false;
bool Mods::customContentLoadedFirstTime = false;
bool Mods::disableSteamAchievements = false;
bool Mods::lobbyDisableSteamAchievements = false;
bool Mods::isLoading = false;
void Mods::updateModCounts()
{
	mods_loaded_local.clear();
	mods_loaded_workshop.clear();
	for ( auto& mod : mountedFilepaths )
	{
		bool found = false;
		if ( mod.first.find("371970") != std::string::npos )
		{
			if ( mod.first.find("workshop") != std::string::npos )
			{
				if ( mod.first.find("content") != std::string::npos )
				{
					found = true;
					Mods::mods_loaded_workshop.insert(mod.first);
				}
			}
		}
		if ( !found )
		{
			Mods::mods_loaded_local.insert(mod.first);
		}
	}
}



bool Mods::verifyMapFiles(const char* folder, bool ignoreBaseFolder)
{
	std::map<std::string, int> newMapHashes;
	std::string fullpath;
	if ( !folder )
	{
		fullpath = "maps/";
	}
	else
	{
		fullpath = folder;
		fullpath += PHYSFS_getDirSeparator();
		fullpath += "maps/";
	}
	for ( auto& f : directoryContents(fullpath.c_str(), false, true) )
	{
		const std::string mapPath = "maps/" + f;
		auto path = PHYSFS_getRealDir(mapPath.c_str());
		if ( path && ignoreBaseFolder && !strcmp(path, "./") )
		{
			continue;
		}

		map_t m;
		m.tiles = nullptr;
		m.entities = (list_t*)malloc(sizeof(list_t));
		m.entities->first = nullptr;
		m.entities->last = nullptr;
		m.creatures = new list_t;
		m.creatures->first = nullptr;
		m.creatures->last = nullptr;
		m.worldUI = new list_t;
		m.worldUI->first = nullptr;
		m.worldUI->last = nullptr;
		if ( path )
		{
			int maphash = 0;
			const std::string fullMapPath = path + (PHYSFS_getDirSeparator() + mapPath);
			int result = loadMap(fullMapPath.c_str(), &m, m.entities, m.creatures, &maphash);
			if ( result >= 0 ) {
				bool fileExistsInTable = false;
				if ( !verifyMapHash(fullMapPath.c_str(), maphash, &fileExistsInTable) )
				{
					if ( fileExistsInTable || strcmp(path, "./") ) 
					{
						// return false if map exists in map hash table, or if hash check failed and mod folder contains an unknown map
						return false;
					}
				}
			}
		}
		if ( m.entities ) {
			list_FreeAll(m.entities);
			free(m.entities);
		}
		if ( m.creatures ) {
			list_FreeAll(m.creatures);
			delete m.creatures;
		}
		if ( m.worldUI ) {
			list_FreeAll(m.worldUI);
			delete m.worldUI;
		}
		if ( m.tiles ) {
			free(m.tiles);
		}
	}
	return true;
}

void Mods::verifyAchievements(const char* fullpath, bool ignoreBaseFolder)
{
	if ( physfsIsMapLevelListModded() )
	{
		disableSteamAchievements = true;
	}

	if ( PHYSFS_getRealDir("/data/gameplaymodifiers.json") )
	{
		disableSteamAchievements = true;
	}
	else if ( PHYSFS_getRealDir("/data/monstercurve.json") )
	{
		disableSteamAchievements = true;
	}
	else if ( !verifyMapFiles(fullpath, ignoreBaseFolder) )
	{
		disableSteamAchievements = true;
	}
	if ( ItemTooltips_t::itemsJsonHashRead != ItemTooltips_t::kItemsJsonHash )
	{
		disableSteamAchievements = true;
	}
}

bool Mods::isPathInMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry
			return true;
		}
	}
	return false;
}

bool Mods::removePathFromMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry, remove from list.
			Mods::mountedFilepaths.erase(it);
			return true;
		}
	}
	return false;
}

bool Mods::clearAllMountedPaths()
{
	bool success = true;
	char** i;
	for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )
	{
        const std::string xmas = (std::string(datadir) + "/") + holidayThemeDirs[HolidayTheme::THEME_XMAS];
        const std::string halloween = (std::string(datadir) + "/") + holidayThemeDirs[HolidayTheme::THEME_HALLOWEEN];
    
		std::string line = *i;
		if (line.compare(outputdir) != 0 &&
            line.compare(datadir) != 0 &&
            line.compare(halloween) != 0 &&
            line.compare(xmas) != 0 &&
            line.compare("./") != 0) // don't unmount the base directories
		{
			if ( PHYSFS_unmount(*i) == 0 )
			{
				success = false;
				printlog("[%s] unsuccessfully removed from the search path.\n", line.c_str());
			}
			else
			{
				printlog("[%s] is removed from the search path.\n", line.c_str());
			}
		}
	}
	Mods::numCurrentModsLoaded = -1;
	PHYSFS_freeList(*i);
	return success;
}

bool Mods::mountAllExistingPaths()
{
	bool success = true;
	std::vector<std::pair<std::string, std::string>>::iterator it;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		std::pair<std::string, std::string> itpair = *it;
		if ( PHYSFS_mount(itpair.first.c_str(), NULL, 0) )
		{
			printlog("[%s] is in the search path.\n", itpair.first.c_str());
		}
		else
		{
			printlog("[%s] unsuccessfully added to search path.\n", itpair.first.c_str());
			success = false;
		}
	}
	Mods::numCurrentModsLoaded = Mods::mountedFilepaths.size();
	return success;
}

void Mods::loadModels(int start, int end) {
	start = std::clamp(start, 0, (int)nummodels - 1);
	end = std::clamp(end, 0, (int)nummodels);

	if ( start >= end ) {
		return;
	}

	//messagePlayer(clientnum, Language::get(2354));
	printlog(Language::get(2355), start, end);

	loading = true;
	//createLoadingScreen(5);
	doLoadingScreen();

	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, sizeof(name));
		if ( c >= start && c < end ) {
			if (polymodels[c].vao) {
				GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
			}
			if (polymodels[c].positions) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
			}
			if (polymodels[c].colors) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
			}
			if (polymodels[c].normals) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
			}
		}
	}

	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done, start, end]() {
		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, sizeof(name));
		if ( c >= start && c < end )
		{
			if ( models[c] != NULL )
			{
				if ( models[c]->data )
				{
					free(models[c]->data);
				}
				free(models[c]);
				if ( polymodels[c].faces )
				{
					free(polymodels[c].faces);
					polymodels[c].faces = nullptr;
				}
				models[c] = loadVoxel(name);
			}
		}
	}
	FileIO::close(fp);
	generatePolyModels(start, end, true);
	loading_done = true;
	return 0;
		});
	while ( !loading_done )
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	generateVBOs(start, end);
}

void Mods::unloadMods(bool force)
{
	isLoading = true;
	loading = true;
	createLoadingScreen(5);
	doLoadingScreen();

	// start loading
	mountedFilepathsSaved = mountedFilepaths;
	clearAllMountedPaths();
	mountedFilepaths.clear();
	Mods::disableSteamAchievements = false;
    if (force) {
        modelsListModifiedIndexes.clear();
        for (int c = 0; c < nummodels; ++c) {
            modelsListModifiedIndexes.push_back(c);
        }
        for (int c = 0; c < numsounds; ++c) {
            soundsListModifiedIndexes.push_back(c);
        }
        for (const auto& pair : systemResourceImages) {
            Mods::systemResourceImagesToReload.push_back(pair);
        }
		Mods::tileListRequireReloadUnmodded = true;
		Mods::modelsListRequiresReloadUnmodded = true;
		Mods::spriteImagesRequireReloadUnmodded = true;
		Mods::musicRequireReloadUnmodded = true;
		Mods::langRequireReloadUnmodded = true;
		Mods::monsterLimbsRequireReloadUnmodded = true;
		Mods::systemImagesReloadUnmodded = true;
    }
	updateLoadingScreen(10);
	doLoadingScreen();

	// update tiles
	if (Mods::tileListRequireReloadUnmodded)
	{
		physfsReloadTiles(true);
		Mods::tileListRequireReloadUnmodded = false;
	}
	doLoadingScreen();

	// reload sprites
	if (Mods::spriteImagesRequireReloadUnmodded)
	{
		physfsReloadSprites(true);
		Mods::spriteImagesRequireReloadUnmodded = false;
	}
	doLoadingScreen();

	// reload system images
	if (Mods::systemImagesReloadUnmodded)
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = false;
		systemResourceImagesToReload.clear();
	}

	updateLoadingScreen(20);
	doLoadingScreen();

	static int modelsIndexUpdateStart = 1;
	static int modelsIndexUpdateEnd = nummodels;

	static std::vector<polymodel_t> polymodelsToUpdate;
	polymodelsToUpdate.reserve(nummodels);

	// begin async load process
	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done]() {
		initGameDatafilesAsync(true);

		// update sounds
		if (Mods::soundListRequiresReloadUnmodded || !Mods::soundsListModifiedIndexes.empty())
		{
			physfsReloadSounds(true);
			Mods::soundListRequiresReloadUnmodded = false;
		}
		Mods::soundsListModifiedIndexes.clear();
		updateLoadingScreen(30);

		// update models
		if (Mods::modelsListRequiresReloadUnmodded || !Mods::modelsListModifiedIndexes.empty())
		{
			physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd);
			for (int c = 0; c < nummodels; ++c) {
				if (polymodels[c].faces) {
					free(polymodels[c].faces);
					polymodels[c].faces = nullptr;
				}

				polymodelsToUpdate.emplace_back(polymodel_t());
				polymodelsToUpdate.back().faces = nullptr;
				polymodelsToUpdate.back().vao = polymodels[c].vao;
				polymodelsToUpdate.back().positions = polymodels[c].positions;
				polymodelsToUpdate.back().colors = polymodels[c].colors;
				polymodelsToUpdate.back().normals = polymodels[c].normals;
			}
			free(polymodels);
			polymodels = nullptr;
			generatePolyModels(0, nummodels, false);
			Mods::modelsListRequiresReloadUnmodded = false;
		}
		Mods::modelsListModifiedIndexes.clear();
		updateLoadingScreen(60);

		updateLoadingScreen(70);

		// reload lang file
		if (Mods::langRequireReloadUnmodded)
		{
			Language::reset();
			Language::reloadLanguage();
			Mods::langRequireReloadUnmodded = false;
		}

		// reload monster limb offsets
		if (Mods::monsterLimbsRequireReloadUnmodded)
		{
			physfsReloadMonsterLimbFiles();
			Mods::monsterLimbsRequireReloadUnmodded = false;
		}

		updateLoadingScreen(80);

		loading_done = true;
		return 0;
		});
	while (!loading_done)
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// final loading steps
	initGameDatafiles(true);
	for (int c = 0; c < nummodels; ++c) {
		if (polymodels[c].vao) {
			GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
		}
		if (polymodels[c].positions) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
		}
		if (polymodels[c].colors) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
		}
		if (polymodels[c].normals) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
		}
	}
	for ( auto& polymodel : polymodelsToUpdate )
	{
		if ( polymodel.vao ) {
			GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodel.vao));
		}
		if ( polymodel.positions ) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodel.positions));
		}
		if ( polymodel.colors ) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodel.colors));
		}
		if ( polymodel.normals ) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodel.normals));
		}
	}
	polymodelsToUpdate.clear();
	generateVBOs(0, nummodels);

	// reload books
	if ( Mods::booksRequireReloadUnmodded )
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = false;
	}
	consoleCommand("/dumpcache");

	// reload music
	if (Mods::musicRequireReloadUnmodded)
	{
		gamemodsUnloadCustomThemeMusic();
		bool reloadIntroMusic = false;
		physfsReloadMusic(reloadIntroMusic, true);
		if (reloadIntroMusic)
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = false;
	}
	destroyLoadingScreen();
	loading = false;
	isLoading = false;
}

void Mods::loadMods()
{
	Mods::disableSteamAchievements = false;
	Mods::verifyAchievements(nullptr, false);

	isLoading = true;
	loading = true;
	createLoadingScreen(5);
	doLoadingScreen();

	Mods::customContentLoadedFirstTime = true;

	updateLoadingScreen(10);
	doLoadingScreen();

	// process any new model files encountered in the mod load list.
	if ( physfsSearchModelsToUpdate() || !Mods::modelsListModifiedIndexes.empty() )
	{
		int modelsIndexUpdateStart = 1;
		int modelsIndexUpdateEnd = nummodels;
		bool oldModelCache = useModelCache;
		useModelCache = false;
		physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd);
		for (int c = modelsIndexUpdateStart; c < modelsIndexUpdateEnd && c < nummodels; ++c) {
			if (polymodels[c].faces) {
				free(polymodels[c].faces);
				polymodels[c].faces = nullptr;
			}
			if (polymodels[c].vao) {
				GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
			}
			if (polymodels[c].positions) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
			}
			if (polymodels[c].colors) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
			}
			if (polymodels[c].normals) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
			}
		}

		// polymodels will get free'd if generating all models in generatePolyModels
		//free(polymodels);
		//polymodels = nullptr;
		generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, true);
		generateVBOs(modelsIndexUpdateStart, modelsIndexUpdateEnd);
		useModelCache = oldModelCache;
		Mods::modelsListRequiresReloadUnmodded = true;
	}

	updateLoadingScreen(20);
	doLoadingScreen();

	if ( physfsSearchSoundsToUpdate() || !Mods::soundsListModifiedIndexes.empty() )
	{
		physfsReloadSounds(false);
		Mods::soundListRequiresReloadUnmodded = true;
	}

	updateLoadingScreen(30);
	doLoadingScreen();

	if ( physfsSearchTilesToUpdate() )
	{
		physfsReloadTiles(false);
		Mods::tileListRequireReloadUnmodded = true;
	}
	else if ( Mods::tileListRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadTiles(true);
		Mods::tileListRequireReloadUnmodded = false;
	}

	updateLoadingScreen(40);
	doLoadingScreen();

	if ( physfsSearchSpritesToUpdate() )
	{
		physfsReloadSprites(false);
		Mods::spriteImagesRequireReloadUnmodded = true;
	}
	else if ( Mods::spriteImagesRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadSprites(true);
		Mods::spriteImagesRequireReloadUnmodded = false;
	}

	updateLoadingScreen(50);
	doLoadingScreen();

	updateLoadingScreen(60);
	doLoadingScreen();

	gamemodsUnloadCustomThemeMusic();

	if ( physfsSearchMusicToUpdate() )
	{
		bool reloadIntroMusic = false;
		physfsReloadMusic(reloadIntroMusic, false);
		if ( reloadIntroMusic )
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = true;
	}
	else if ( Mods::musicRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		// restore old music
		bool reloadIntroMusic = true;
		physfsReloadMusic(reloadIntroMusic, true);
		if ( reloadIntroMusic )
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = false;
	}

	updateLoadingScreen(70);
	doLoadingScreen();

	std::string langDirectory = PHYSFS_getRealDir("lang/en.txt");
	if ( langDirectory.compare("./") != 0 )
	{
		if ( Language::reloadLanguage() != 0 )
		{
			printlog("[PhysFS]: Error reloading modified language file in lang/ directory!");
		}
		else
		{
			printlog("[PhysFS]: Found modified language file in lang/ directory, reloading en.txt...");
		}
		Mods::langRequireReloadUnmodded = true;
	}
	else if ( Mods::langRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		Language::reloadLanguage();
		Mods::langRequireReloadUnmodded = false;
	}

	updateLoadingScreen(80);
	doLoadingScreen();

	if ( physfsSearchMonsterLimbFilesToUpdate() )
	{
		physfsReloadMonsterLimbFiles();
		Mods::monsterLimbsRequireReloadUnmodded = true;
	}
	else if ( Mods::monsterLimbsRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadMonsterLimbFiles();
		Mods::monsterLimbsRequireReloadUnmodded = false;
	}

	updateLoadingScreen(85);
	doLoadingScreen();

	if ( physfsSearchSystemImagesToUpdate() )
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = true;
	}
	else if ( Mods::systemImagesReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = false;
	}

	updateLoadingScreen(90);
	doLoadingScreen();

	initGameDatafiles(true);

	updateLoadingScreen(95);
	doLoadingScreen();

	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done]() {
		initGameDatafilesAsync(true);
		loading_done = true;
		return 0;
		});
	while ( !loading_done )
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if ( physfsSearchBooksToUpdate() )
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = true;
	}
	else if ( Mods::booksRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = false;
	}

	loadLights();

	consoleCommand("/dumpcache");

	destroyLoadingScreen();

	loading = false;
	isLoading = false;
}

void Mods::writeLevelsTxtAndPreview(std::string modFolder)
{
	std::string path = outputdir;
	path.append(PHYSFS_getDirSeparator()).append("mods/").append(modFolder);
	if ( access(path.c_str(), F_OK) == 0 )
	{
		std::string writeFile = modFolder + "/maps/levels.txt";
		PHYSFS_File* physfp = PHYSFS_openWrite(writeFile.c_str());
		if ( physfp != nullptr )
		{
			PHYSFS_writeBytes(physfp, "map: start\n", 11);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "map: minetoswamp\n", 17);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "map: swamptolabyrinth\n", 22);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "map: labyrinthtoruins\n", 22);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "map: boss\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "map: hellboss\n", 14);
			PHYSFS_writeBytes(physfp, "map: hamlet\n", 12);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "map: cavestocitadel\n", 20);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "map: sanctum", 12);
			PHYSFS_close(physfp);
		}
		else
		{
			printlog("[PhysFS]: Failed to open %s/maps/levels.txt for writing.", path.c_str());
		}

		std::string srcImage = datadir;
		srcImage.append("images/system/preview.png");
		std::string dstImage = path + "/preview.png";
		if ( access(srcImage.c_str(), F_OK) == 0 )
		{
			if ( File* fp_read = FileIO::open(srcImage.c_str(), "rb") )
			{
				if ( File* fp_write = FileIO::open(dstImage.c_str(), "wb") )
				{
					char chunk[1024];
					auto len = fp_read->read(chunk, sizeof(chunk[0]), sizeof(chunk));
					while ( len == sizeof(chunk) )
					{
						fp_write->write(chunk, sizeof(chunk[0]), len);
						len = fp_read->read(chunk, sizeof(chunk[0]), sizeof(chunk));
					}
					fp_write->write(chunk, sizeof(chunk[0]), len);
					FileIO::close(fp_write);
				}
				else
				{
					printlog("[PhysFS]: Failed to write preview.png in %s", dstImage.c_str());
				}
				FileIO::close(fp_read);
			}
			else
			{
				printlog("[PhysFS]: Failed to open %s", srcImage.c_str());
			}
		}
		else
		{
			printlog("[PhysFS]: Failed to access %s", srcImage.c_str());
		}
	}
	else
	{
		printlog("[PhysFS]: Failed to write levels.txt in %s", path.c_str());
	}
}

int Mods::createBlankModDirectory(std::string foldername)
{
	std::string baseDir = outputdir;
	baseDir.append(PHYSFS_getDirSeparator()).append("mods").append(PHYSFS_getDirSeparator()).append(foldername);

	if ( access(baseDir.c_str(), F_OK) == 0 )
	{
		// folder already exists!
		return 1;
	}
	else
	{
		if ( PHYSFS_mkdir(foldername.c_str()) )
		{
			std::string dir = foldername;
			std::string folder = "/books";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/editor";
			PHYSFS_mkdir((dir + folder).c_str());

			folder = "/images";
			PHYSFS_mkdir((dir + folder).c_str());
			std::string subfolder = "/sprites";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/system";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/tiles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/ui";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/items";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/images";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/lang";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/maps";
			PHYSFS_mkdir((dir + folder).c_str());
			writeLevelsTxtAndPreview(foldername.c_str());

			folder = "/models";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/creatures";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/decorations";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/doors";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/items";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/particles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/music";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/sound";
			PHYSFS_mkdir((dir + folder).c_str());

			folder = "/data";
			PHYSFS_mkdir((dir + folder).c_str());

			return 0;
		}
	}
	return 2;
}

