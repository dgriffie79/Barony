/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_editor.cpp - Editor entity data (converted from rapidjson to cJSON)
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"


EditorEntityData_t editorEntityData;
std::map<int, EditorEntityData_t::EntityColliderData_t> EditorEntityData_t::colliderData;
std::map<std::string, EditorEntityData_t::ColliderDmgProperties_t> EditorEntityData_t::colliderDmgTypes;
std::map<std::string, std::map<int, int>> EditorEntityData_t::colliderRandomGenPool;
std::map<std::string, int> EditorEntityData_t::colliderNameIndexes;
void EditorEntityData_t::readFromFile()
{
	const std::string filename = "data/entity_data.json";
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

	if ( !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "entities") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		cJSON_Delete(d);
		return;
	}

	colliderData.clear();
	colliderDmgTypes.clear();
	colliderRandomGenPool.clear();
	colliderNameIndexes.clear();
	cJSON* entityTypes = cJSON_GetObjectItem(d, "entities");
	if ( cJSON_HasObjectItem(entityTypes, "collider_dmg_calcs") )
	{
		cJSON* calcsObj = cJSON_GetObjectItem(entityTypes, "collider_dmg_calcs");
		for ( cJSON* itr = calcsObj->child; itr; itr = itr->next )
		{
			auto& colliderDmg = colliderDmgTypes[itr->string];
			cJSON* burnable = cJSON_GetObjectItem(itr, "burnable");
			if ( burnable ) colliderDmg.burnable = cJSON_IsTrue(burnable);
			cJSON* mino = cJSON_GetObjectItem(itr, "minotaur_path_and_break");
			if ( mino ) colliderDmg.minotaurPathThroughAndBreak = cJSON_IsTrue(mino);
			cJSON* melee = cJSON_GetObjectItem(itr, "melee");
			if ( melee ) colliderDmg.meleeAffects = cJSON_IsTrue(melee);
			cJSON* magic = cJSON_GetObjectItem(itr, "magic");
			if ( magic ) colliderDmg.magicAffects = cJSON_IsTrue(magic);
			cJSON* bombs = cJSON_GetObjectItem(itr, "bombs_attach");
			if ( bombs ) colliderDmg.bombsAttach = cJSON_IsTrue(bombs);
			cJSON* boulder = cJSON_GetObjectItem(itr, "boulder_destroy");
			if ( boulder ) colliderDmg.boulderDestroys = cJSON_IsTrue(boulder);
			cJSON* minimap = cJSON_GetObjectItem(itr, "minimap_appear_as_wall");
			if ( minimap ) colliderDmg.showAsWallOnMinimap = cJSON_IsTrue(minimap);
			cJSON* npcPathing = cJSON_GetObjectItem(itr, "allow_npc_pathing");
			if ( npcPathing )
			{
				colliderDmg.allowNPCPathing = cJSON_IsTrue(npcPathing);
			}
			cJSON* bonusSkills = cJSON_GetObjectItem(itr, "bonus_damage_skills");
			if ( bonusSkills && cJSON_IsArray(bonusSkills) )
			{
				cJSON* itr2 = NULL;
				cJSON_ArrayForEach(itr2, bonusSkills)
				{
					std::string s = itr2->valuestring;
					if ( s == "PRO_AXE" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_AXE);
					}
					else if ( s == "PRO_SWORD" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_SWORD);
					}
					else if ( s == "PRO_MACE" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_MACE);
					}
					else if ( s == "PRO_POLEARM" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_POLEARM);
					}
					else if ( s == "PRO_UNARMED" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_UNARMED);
					}
					else if ( s == "PRO_MAGIC" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_SORCERY);
						colliderDmg.proficiencyBonusDamage.insert(PRO_MYSTICISM);
						colliderDmg.proficiencyBonusDamage.insert(PRO_THAUMATURGY);
					}
					else if ( s == "PRO_RANGED" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_RANGED);
					}
				}
			}
			cJSON* resistSkills = cJSON_GetObjectItem(itr, "resist_damage_skills");
			if ( resistSkills && cJSON_IsArray(resistSkills) )
			{
				cJSON* itr2 = NULL;
				cJSON_ArrayForEach(itr2, resistSkills)
				{
					std::string s = itr2->valuestring;
					if ( s == "PRO_AXE" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_AXE);
					}
					else if ( s == "PRO_SWORD" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_SWORD);
					}
					else if ( s == "PRO_MACE" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_MACE);
					}
					else if ( s == "PRO_POLEARM" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_POLEARM);
					}
					else if ( s == "PRO_UNARMED" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_UNARMED);
					}
					else if ( s == "PRO_MAGIC" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_SORCERY);
						colliderDmg.proficiencyResistDamage.insert(PRO_MYSTICISM);
						colliderDmg.proficiencyResistDamage.insert(PRO_THAUMATURGY);
					}
					else if ( s == "PRO_RANGED" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_RANGED);
					}
				}
			}
		}
	}
	if ( cJSON_HasObjectItem(entityTypes, "collider_dmg_types") )
	{
		cJSON* dmgTypesObj = cJSON_GetObjectItem(entityTypes, "collider_dmg_types");
		for ( cJSON* itr = dmgTypesObj->child; itr; itr = itr->next )
		{
			int index = std::stoi(itr->string);
			auto& collider = colliderData[index];
			cJSON* nameItem = cJSON_GetObjectItem(itr, "name");
			if ( nameItem ) collider.name = nameItem->valuestring;
			assert(colliderNameIndexes.find(collider.name) == colliderNameIndexes.end());
			colliderNameIndexes[collider.name] = index;
			cJSON* gib = cJSON_GetObjectItem(itr, "gib_model");
			if ( gib ) collider.gib = gib->valueint;
			collider.gib_hit.clear();
			cJSON* gibHit = cJSON_GetObjectItem(itr, "gib_hit_model");
			if ( gibHit )
			{
				if ( cJSON_IsNumber(gibHit) )
				{
					collider.gib_hit.push_back(gibHit->valueint);
				}
				else if ( cJSON_IsArray(gibHit) )
				{
					cJSON* itr2 = NULL;
					cJSON_ArrayForEach(itr2, gibHit)
					{
						if ( cJSON_IsNumber(itr2) )
						{
							collider.gib_hit.push_back(itr2->valueint);
						}
					}
				}
			}
			cJSON* sfxBreak = cJSON_GetObjectItem(itr, "sfx_break");
			if ( sfxBreak )
			{
				if ( cJSON_IsNumber(sfxBreak) )
				{
					collider.sfxBreak.push_back(sfxBreak->valueint);
				}
				else if ( cJSON_IsArray(sfxBreak) )
				{
					cJSON* itr2 = NULL;
					cJSON_ArrayForEach(itr2, sfxBreak)
					{
						if ( cJSON_IsNumber(itr2) )
						{
							collider.sfxBreak.push_back(itr2->valueint);
						}
					}
				}
			}
			cJSON* sfxHit = cJSON_GetObjectItem(itr, "sfx_hit");
			if ( sfxHit ) collider.sfxHit = sfxHit->valueint;
			cJSON* dmgCalc = cJSON_GetObjectItem(itr, "damage_calc");
			if ( dmgCalc ) collider.damageCalculationType = dmgCalc->valuestring;
			cJSON* entityLang = cJSON_GetObjectItem(itr, "entity_lang_entry");
			if ( entityLang ) collider.entityLangEntry = entityLang->valueint;
			cJSON* hitMsg = cJSON_GetObjectItem(itr, "hit_message");
			if ( hitMsg ) collider.hitMessageLangEntry = hitMsg->valueint;
			cJSON* breakMsg = cJSON_GetObjectItem(itr, "break_message");
			if ( breakMsg ) collider.breakMessageLangEntry = breakMsg->valueint;
			cJSON* jumpMsg = cJSON_GetObjectItem(itr, "jump_message");
			if ( jumpMsg )
			{
				collider.colliderJumpLangEntry = jumpMsg->valueint;
			}
			cJSON* hpbarName = cJSON_GetObjectItem(itr, "hp_bar_lookup_name");
			if ( hpbarName ) collider.hpbarLookupName = hpbarName->valuestring;
			collider.hideMonsters.clear();
			collider.spellTriggers.clear();
			collider.pathableMonsters.clear();
			cJSON* randGenPool = cJSON_GetObjectItem(itr, "random_gen_pool");
			if ( randGenPool )
			{
				if ( cJSON_IsObject(randGenPool) )
				{
					for ( cJSON* itr2 = randGenPool->child; itr2; itr2 = itr2->next )
					{
						if ( itr2->string )
						{
							EditorEntityData_t::colliderRandomGenPool[itr2->string][index] =
								itr2->valueint;
						}
					}
				}
			}
			cJSON* events = cJSON_GetObjectItem(itr, "events");
			if ( events )
			{
				for ( cJSON* itr2 = events->child; itr2; itr2 = itr2->next )
				{
					if ( !strcmp(itr2->string, "spell_trigger") )
					{
						auto& data = collider.spellTriggers;
						if ( cJSON_IsArray(itr2) )
						{
							cJSON* val = NULL;
							cJSON_ArrayForEach(val, itr2)
							{
								if ( cJSON_IsNumber(val) )
								{
									data.push_back(val->valueint);
								}
							}
						}
						continue;
					}
					if ( !strcmp(itr2->string, "pathable") )
					{
						auto& data = collider.pathableMonsters;
						if ( cJSON_IsArray(itr2) )
						{
							cJSON* val = NULL;
							cJSON_ArrayForEach(val, itr2)
							{
								if ( cJSON_IsString(val) && val->valuestring )
								{
									for ( int i = 0; i < NUMMONSTERS; ++i )
									{
										if ( !strcmp(val->valuestring, monstertypename[i]) )
										{
											data.insert(i);
											break;
										}
									}
								}
							}
						}
						continue;
					}
					std::string mapname = itr2->string;
					for ( cJSON* itr3 = itr2->child; itr3; itr3 = itr3->next )
					{
						if ( !strcmp(itr3->string, "summon") )
						{
							auto& data = collider.hideMonsters[mapname];
							if ( cJSON_IsArray(itr3) )
							{
								cJSON* val = NULL;
								cJSON_ArrayForEach(val, itr3)
								{
									if ( cJSON_IsString(val) && val->valuestring )
									{
										for ( int i = 0; i < NUMMONSTERS; ++i )
										{
											if ( !strcmp(val->valuestring, monstertypename[i]) )
											{
												data.push_back(i);
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			collider.overrideProperties.clear();
			cJSON* overrideProps = cJSON_GetObjectItem(itr, "override_editor_props");
			if ( overrideProps )
			{
				if ( cJSON_IsObject(overrideProps) )
				{
					for ( cJSON* itr2 = overrideProps->child; itr2; itr2 = itr2->next )
					{
						if ( cJSON_IsNumber(itr2) )
						{
							collider.overrideProperties[itr2->string] = itr2->valueint;
						}
					}
				}
			}
		}
	}
	cJSON_Delete(d);
}
