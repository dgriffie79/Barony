/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_editor.cpp - Editor entity data
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
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("entities") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	colliderData.clear();
	colliderDmgTypes.clear();
	colliderRandomGenPool.clear();
	colliderNameIndexes.clear();
	auto& entityTypes = d["entities"];
	if ( entityTypes.HasMember("collider_dmg_calcs") )
	{
		for ( auto itr = entityTypes["collider_dmg_calcs"].MemberBegin(); itr != entityTypes["collider_dmg_calcs"].MemberEnd();
			++itr )
		{
			auto& colliderDmg = colliderDmgTypes[itr->name.GetString()];
			colliderDmg.burnable = itr->value["burnable"].GetBool();
			colliderDmg.minotaurPathThroughAndBreak = itr->value["minotaur_path_and_break"].GetBool();
			colliderDmg.meleeAffects = itr->value["melee"].GetBool();
			colliderDmg.magicAffects = itr->value["magic"].GetBool();
			colliderDmg.bombsAttach = itr->value["bombs_attach"].GetBool();
			colliderDmg.boulderDestroys = itr->value["boulder_destroy"].GetBool();
			colliderDmg.showAsWallOnMinimap = itr->value["minimap_appear_as_wall"].GetBool();
			if ( itr->value.HasMember("allow_npc_pathing") )
			{
				colliderDmg.allowNPCPathing = itr->value["allow_npc_pathing"].GetBool();
			}
			if ( itr->value.HasMember("bonus_damage_skills") && itr->value["bonus_damage_skills"].IsArray() )
			{
				for ( auto itr2 = itr->value["bonus_damage_skills"].Begin(); itr2 != itr->value["bonus_damage_skills"].End(); ++itr2 )
				{
					std::string s = itr2->GetString();
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
			if ( itr->value.HasMember("resist_damage_skills") && itr->value["resist_damage_skills"].IsArray() )
			{
				for ( auto itr2 = itr->value["resist_damage_skills"].Begin(); itr2 != itr->value["resist_damage_skills"].End(); ++itr2 )
				{
					std::string s = itr2->GetString();
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
	if ( entityTypes.HasMember("collider_dmg_types") )
	{
		for ( auto itr = entityTypes["collider_dmg_types"].MemberBegin(); itr != entityTypes["collider_dmg_types"].MemberEnd();
			++itr )
		{
			auto indexStr = itr->name.GetString();
			int index = std::stoi(indexStr);
			auto& collider = colliderData[index];
			collider.name = itr->value["name"].GetString();
			assert(colliderNameIndexes.find(collider.name) == colliderNameIndexes.end());
			colliderNameIndexes[collider.name] = index;
			collider.gib = itr->value["gib_model"].GetInt();
			collider.gib_hit.clear();
			if ( itr->value.HasMember("gib_hit_model") )
			{
				if ( itr->value["gib_hit_model"].IsInt() )
				{
					collider.gib_hit.push_back(itr->value["gib_hit_model"].GetInt());
				}
				else if ( itr->value["gib_hit_model"].IsArray() )
				{
					for ( auto itr2 = itr->value["gib_hit_model"].Begin();
						itr2 != itr->value["gib_hit_model"].End(); ++itr2 )
					{
						if ( itr2->IsInt() )
						{
							collider.gib_hit.push_back(itr2->GetInt());
						}
					}
				}
			}
			if ( itr->value["sfx_break"].IsInt() )
			{
				collider.sfxBreak.push_back(itr->value["sfx_break"].GetInt());
			}
			else if ( itr->value["sfx_break"].IsArray() )
			{
				for ( auto itr2 = itr->value["sfx_break"].Begin();
					itr2 != itr->value["sfx_break"].End(); ++itr2 )
				{
					if ( itr2->IsInt() )
					{
						collider.sfxBreak.push_back(itr2->GetInt());
					}
				}
			}
			collider.sfxHit = itr->value["sfx_hit"].GetInt();
			collider.damageCalculationType = itr->value["damage_calc"].GetString();
			collider.entityLangEntry = itr->value["entity_lang_entry"].GetInt();
			collider.hitMessageLangEntry = itr->value["hit_message"].GetInt();
			collider.breakMessageLangEntry = itr->value["break_message"].GetInt();
			if ( itr->value.HasMember("jump_message") )
			{
				collider.colliderJumpLangEntry = itr->value["jump_message"].GetInt();
			}
			collider.hpbarLookupName = itr->value["hp_bar_lookup_name"].GetString();
			collider.hideMonsters.clear();
			collider.spellTriggers.clear();
			collider.pathableMonsters.clear();
			if ( itr->value.HasMember("random_gen_pool") )
			{
				if ( itr->value["random_gen_pool"].IsObject() )
				{
					for ( auto itr2 = itr->value["random_gen_pool"].MemberBegin();
						itr2 != itr->value["random_gen_pool"].MemberEnd(); ++itr2 )
					{
						if ( itr2->name.IsString() )
						{
							EditorEntityData_t::colliderRandomGenPool[itr2->name.GetString()][index] =
								itr2->value.GetInt();
						}
					}
				}
			}
			if ( itr->value.HasMember("events") )
			{
				for ( auto itr2 = itr->value["events"].MemberBegin();
					itr2 != itr->value["events"].MemberEnd(); ++itr2 )
				{
					if ( !strcmp(itr2->name.GetString(), "spell_trigger") )
					{
						auto& data = collider.spellTriggers;
						if ( itr2->value.IsArray() )
						{
							for ( auto val = itr2->value.Begin(); val != itr2->value.End(); ++val )
							{
								if ( val->IsInt() )
								{
									data.push_back(val->GetInt());
								}
							}
						}
						continue;
					}
					if ( !strcmp(itr2->name.GetString(), "pathable") )
					{
						auto& data = collider.pathableMonsters;
						if ( itr2->value.IsArray() )
						{
							for ( auto val = itr2->value.Begin(); val != itr2->value.End(); ++val )
							{
								if ( val->IsString() )
								{
									for ( int i = 0; i < NUMMONSTERS; ++i )
									{
										if ( !strcmp(val->GetString(), monstertypename[i]) )
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
					std::string mapname = itr2->name.GetString();
					for ( auto itr3 = itr2->value.MemberBegin();
						itr3 != itr2->value.MemberEnd(); ++itr3 )
					{
						if ( !strcmp(itr3->name.GetString(), "summon") )
						{
							auto& data = collider.hideMonsters[mapname];
							if ( itr3->value.IsArray() )
							{
								for ( auto val = itr3->value.Begin(); val != itr3->value.End(); ++val )
								{
									if ( val->IsString() )
									{
										for ( int i = 0; i < NUMMONSTERS; ++i )
										{
											if ( !strcmp(val->GetString(), monstertypename[i]) )
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
			if ( itr->value.HasMember("override_editor_props") )
			{
				if ( itr->value["override_editor_props"].IsObject() )
				{
					for ( auto itr2 = itr->value["override_editor_props"].MemberBegin();
						itr2 != itr->value["override_editor_props"].MemberEnd(); ++itr2 )
					{
						if ( itr2->value.IsInt() )
						{
							collider.overrideProperties[itr2->name.GetString()] = itr2->value.GetInt();
						}
					}
				}
			}
		}
	}
}

