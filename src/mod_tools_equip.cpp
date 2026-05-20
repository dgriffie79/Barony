/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_equip.cpp - Equipment model offsets
Desc: Extracted from mod_tools.cpp for modularity

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "mod_tools_private.hpp"

EquipmentModelOffsets_t EquipmentModelOffsets;

int EquipmentModelOffsets_t::modelOffsetExists(int monster, int sprite, int monsterSprite)
{
	if ( monsterSprite >= NUMMONSTERS )
	{
		auto find = monsterModelsMap.find(monsterSprite);
		if ( find != monsterModelsMap.end() )
		{
			auto find2 = find->second.find(sprite);
			if ( find2 != find->second.end() )
			{
				return monsterSprite;
			}
		}
	}
	{
		auto find = monsterModelsMap.find(monster);
		if ( find != monsterModelsMap.end() )
		{
			auto find2 = find->second.find(sprite);
			if ( find2 != find->second.end() )
			{
				return monster;
			}
		}
	}
	return 0;
}

EquipmentModelOffsets_t::ModelOffset_t& EquipmentModelOffsets_t::getModelOffset(int monster, int sprite)
{
	return monsterModelsMap[monster][sprite];
}

int EquipmentModelOffsets_t::expandHelmToFitMask(int monster, int helmSprite, int maskSprite, int monsterSprite)
{
	if ( int resultMonsterSprite = modelOffsetExists(monster, maskSprite, monsterSprite) )
	{
		auto& maskOffset = getModelOffset(resultMonsterSprite, maskSprite);
		if ( maskOffset.oversizedMask )
		{
			if ( modelOffsetExists(resultMonsterSprite, helmSprite, 0) )
			{
				auto& helmOffset = getModelOffset(resultMonsterSprite, helmSprite);
				if ( helmOffset.expandToFitMask )
				{
					return resultMonsterSprite;
				}
			}
		}
	}
	return 0;
}

int EquipmentModelOffsets_t::maskHasAdjustmentForExpandedHelm(int monster, int helmSprite, int maskSprite, int monsterSprite)
{
	if ( int resultMonsterSprite = modelOffsetExists(monster, maskSprite, monsterSprite) )
	{
		auto& maskOffset = getModelOffset(resultMonsterSprite, maskSprite);
		if ( maskOffset.adjustToExpandedHelm.find(helmSprite) != maskOffset.adjustToExpandedHelm.end() )
		{
			return resultMonsterSprite;
		}
		else if ( maskOffset.adjustToExpandedHelm.find(-1) != maskOffset.adjustToExpandedHelm.end() )
		{
			return resultMonsterSprite;
		}
	}
	return 0;
}

EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t EquipmentModelOffsets_t::getExpandHelmOffset(int monster, 
	int helmSprite, int maskSprite)
{
	if ( int resultMonsterSprite = modelOffsetExists(monster, helmSprite, 0) )
	{
		auto& helmOffset = getModelOffset(resultMonsterSprite, helmSprite);
		if ( helmOffset.adjustToOversizeMask.find(maskSprite) != helmOffset.adjustToOversizeMask.end() )
		{
			return helmOffset.adjustToOversizeMask[maskSprite];
		}
		else if ( helmOffset.adjustToOversizeMask.find(-1) != helmOffset.adjustToOversizeMask.end() )
		{
			return helmOffset.adjustToOversizeMask[-1];
		}
	}
	return EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t();
}

EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t EquipmentModelOffsets_t::getMaskOffsetForExpandHelm(int monster, 
	int helmSprite, int maskSprite)
{
	if ( int resultMonsterSprite = modelOffsetExists(monster, maskSprite, 0) )
	{
		auto& maskOffset = getModelOffset(resultMonsterSprite, maskSprite);
		if ( maskOffset.adjustToExpandedHelm.find(helmSprite) != maskOffset.adjustToExpandedHelm.end() )
		{
			return maskOffset.adjustToExpandedHelm[helmSprite];
		}
		else if ( maskOffset.adjustToExpandedHelm.find(-1) != maskOffset.adjustToExpandedHelm.end() )
		{
			return maskOffset.adjustToExpandedHelm[-1];
		}
	}
	return EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t();
}

void EquipmentModelOffsets_t::readBaseItemsFromFile()
{
	std::string filename = "models/creatures/";
	filename += "item_model_positions.json";

	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		//printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
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

	static char buf[32000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		return;
	}
	if ( !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	int version = d["version"].GetInt();

	miscItemsBaseOffsets.clear();

	auto& itemsArr = d["items"];
	for ( auto it = itemsArr.Begin(); it != itemsArr.End(); ++it )
	{
		for ( auto it2 = it->MemberBegin(); it2 != it->MemberEnd(); ++it2 )
		{
			std::string itemName = it2->name.GetString();
			if ( ItemTooltips.itemNameStringToItemID.find(itemName) == ItemTooltips.itemNameStringToItemID.end() )
			{
				continue;
			}
			ItemType itemType = (ItemType)ItemTooltips.itemNameStringToItemID[itemName];
			std::vector<int> models;
			if ( it2->value.HasMember("models") )
			{
				if ( it2->value["models"].IsArray() )
				{
					if ( it2->value["models"].Size() == 0 )
					{
						for ( int i = items[itemType].index; i < items[itemType].index + items[itemType].variations; ++i )
						{
							models.push_back(i);
						}
					}
					else
					{
						for ( auto itArr = it2->value["models"].Begin(); itArr != it2->value["models"].End(); ++itArr )
						{
							if ( itArr->IsInt() )
							{
								models.push_back(itArr->GetInt());
							}
						}
					}
				}
			}

			real_t focalx = it2->value.HasMember("focalx") ? it2->value["focalx"].GetDouble() : 0.0;
			real_t focaly = it2->value.HasMember("focaly") ? it2->value["focaly"].GetDouble() : 0.0;
			real_t focalz = it2->value.HasMember("focalz") ? it2->value["focalz"].GetDouble() : 0.0;
			real_t scalex = 0.0;
			if ( it2->value.HasMember("scalex") )
			{
				scalex = it2->value["scalex"].GetDouble();
			}
			real_t scaley = 0.0;
			if ( it2->value.HasMember("scaley") )
			{
				scaley = it2->value["scaley"].GetDouble();
			}
			real_t scalez = 0.0;
			if ( it2->value.HasMember("scalez") )
			{
				scalez = it2->value["scalez"].GetDouble();
			}
			for ( auto index : models )
			{
				auto& entry = miscItemsBaseOffsets[index];
				entry.focalx = focalx;
				entry.focaly = focaly;
				entry.focalz = focalz;
				entry.scalex = scalex;
				entry.scaley = scaley;
				entry.scalez = scalez;
			}
		}
	}
}

void EquipmentModelOffsets_t::readFromFile(std::string monsterName, int monsterType)
{
	if ( monsterType == NOTHING )
	{
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			if ( monstertypename[i] == monsterName )
			{
				monsterType = i;
				break;
			}
		}
	}

	if ( monsterType == NOTHING )
	{
		return;
	}

	std::string filename = "models/creatures/";
	filename += monstertypename[monsterType];
	filename += "/model_positions.json";

	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		//printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
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

	static char buf[32000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		return;
	}
	if ( !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	int version = d["version"].GetInt();
	monsterModelsMap[monsterType].clear();

	real_t baseFocalX = 0.0;
	real_t baseFocalY = 0.0;
	real_t baseFocalZ = 0.0;
	real_t baseFocalX_rot1 = 0.0;
	real_t baseFocalY_rot1 = 0.0;
	real_t baseFocalZ_rot1 = 0.0;
	if ( d.HasMember("base_offsets") )
	{
		if ( d["base_offsets"].HasMember("focalx") )
		{
			baseFocalX = d["base_offsets"]["focalx"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focaly") )
		{
			baseFocalY = d["base_offsets"]["focaly"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focalz") )
		{
			baseFocalZ = d["base_offsets"]["focalz"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focalx_rot1") )
		{
			baseFocalX_rot1 = d["base_offsets"]["focalx_rot1"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focaly_rot1") )
		{
			baseFocalY_rot1 = d["base_offsets"]["focaly_rot1"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focalz_rot1") )
		{
			baseFocalZ_rot1 = d["base_offsets"]["focalz_rot1"].GetDouble();
		}
	}

	auto& itemsArr = d["items"];
	for ( auto it = itemsArr.Begin(); it != itemsArr.End(); ++it )
	{
		for ( auto it2 = it->MemberBegin(); it2 != it->MemberEnd(); ++it2 )
		{
			std::string itemName = it2->name.GetString();
			if ( ItemTooltips.itemNameStringToItemID.find(itemName) == ItemTooltips.itemNameStringToItemID.end() )
			{
				continue;
			}
			ItemType itemType = (ItemType)ItemTooltips.itemNameStringToItemID[itemName];
			std::vector<int> models;
			if ( it2->value.HasMember("models") )
			{
				if ( it2->value["models"].IsArray() )
				{
					if ( it2->value["models"].Size() == 0 )
					{
						for ( int i = items[itemType].index; i < items[itemType].index + items[itemType].variations; ++i )
						{
							models.push_back(i);
						}
					}
					else
					{
						for ( auto itArr = it2->value["models"].Begin(); itArr != it2->value["models"].End(); ++itArr )
						{
							if ( itArr->IsInt() )
							{
								models.push_back(itArr->GetInt());
							}
						}
					}
				}
			}

			real_t focalx = it2->value["focalx"].GetDouble();
			real_t focaly = it2->value["focaly"].GetDouble();
			real_t focalz = it2->value["focalz"].GetDouble();
			real_t scalex = 0.0;
			if ( it2->value.HasMember("scalex") )
			{
				scalex = it2->value["scalex"].GetDouble();
			}
			real_t scaley = 0.0;
			if ( it2->value.HasMember("scaley") )
			{
				scaley = it2->value["scaley"].GetDouble();
			}
			real_t scalez = 0.0;
			if ( it2->value.HasMember("scalez") )
			{
				scalez = it2->value["scalez"].GetDouble();
			}
			real_t rotation = it2->value["rotation"].GetDouble();
			real_t pitch = it2->value.HasMember("pitch") ?
				it2->value["pitch"].GetDouble() : 0.0;
			int limbsIndex = it2->value["limbs_index"].GetInt();
			bool oversizedMask = it2->value.HasMember("oversize_mask") ? 
				it2->value["oversize_mask"].GetBool() : false;
			bool expandToFitMask = it2->value.HasMember("expand_to_fit_oversize_mask") ?
				it2->value["expand_to_fit_oversize_mask"].GetBool() : false;

			for ( auto index : models )
			{
				auto& entry = monsterModelsMap[monsterType][index];
				entry.rotation = rotation * (PI / 2);
				entry.focalx = focalx;
				entry.focaly = focaly;
				entry.focalz = focalz;
				if ( items[itemType].item_slot == EQUIPPABLE_IN_SLOT_BREASTPLATE )
				{
				}
				else if ( static_cast<int>(entry.rotation) == 1 && version >= 2 )
				{
					entry.focalx += baseFocalX_rot1;
					entry.focaly += baseFocalY_rot1;
					entry.focalz += baseFocalZ_rot1;
				}
				else
				{
					entry.focalx += baseFocalX;
					entry.focaly += baseFocalY;
					entry.focalz += baseFocalZ;
				}
				entry.scalex = scalex;
				entry.scaley = scaley;
				entry.scalez = scalez;
				entry.pitch = pitch * (PI / 2);
				entry.limbsIndex = limbsIndex;
				entry.expandToFitMask = expandToFitMask;
				entry.oversizedMask = oversizedMask;

				if ( it2->value.HasMember("adjust_on_oversize_mask") )
				{
					auto& itr = it2->value["adjust_on_oversize_mask"];
					for ( auto adjItr = itr.Begin(); adjItr != itr.End(); ++adjItr )
					{
						std::vector<int> models;
						if ( (*adjItr)["mask_sprite"].Size() == 0 )
						{
							models.push_back(-1);
						}
						else
						{
							for ( auto itr2 = (*adjItr)["mask_sprite"].Begin(); itr2 != (*adjItr)["mask_sprite"].End(); ++itr2 )
							{
								models.push_back(itr2->GetInt());
							}
						}
						for ( auto model : models )
						{
							if ( (*adjItr).HasMember("focalx") )
							{
								entry.adjustToOversizeMask[model].focalx = (*adjItr)["focalx"].GetDouble();
							}
							if ( (*adjItr).HasMember("focaly") )
							{
								entry.adjustToOversizeMask[model].focaly = (*adjItr)["focaly"].GetDouble();
							}
							if ( (*adjItr).HasMember("focalz") )
							{
								entry.adjustToOversizeMask[model].focalz = (*adjItr)["focalz"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalex") )
							{
								entry.adjustToOversizeMask[model].scalex = (*adjItr)["scalex"].GetDouble();
							}
							if ( (*adjItr).HasMember("scaley") )
							{
								entry.adjustToOversizeMask[model].scaley = (*adjItr)["scaley"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalez") )
							{
								entry.adjustToOversizeMask[model].scalez = (*adjItr)["scalez"].GetDouble();
							}
						}
					}
				}

				if ( it2->value.HasMember("adjust_on_expand_helm") )
				{
					auto& itr = it2->value["adjust_on_expand_helm"];
					for ( auto adjItr = itr.Begin(); adjItr != itr.End(); ++adjItr )
					{
						std::vector<int> models;
						if ( (*adjItr)["helm_sprite"].Size() == 0 )
						{
							models.push_back(-1);
						}
						else
						{
							for ( auto itr2 = (*adjItr)["helm_sprite"].Begin(); itr2 != (*adjItr)["helm_sprite"].End(); ++itr2 )
							{
								models.push_back(itr2->GetInt());
							}
						}
						for ( auto model : models )
						{
							if ( (*adjItr).HasMember("focalx") )
							{
								entry.adjustToExpandedHelm[model].focalx = (*adjItr)["focalx"].GetDouble();
							}
							if ( (*adjItr).HasMember("focaly") )
							{
								entry.adjustToExpandedHelm[model].focaly = (*adjItr)["focaly"].GetDouble();
							}
							if ( (*adjItr).HasMember("focalz") )
							{
								entry.adjustToExpandedHelm[model].focalz = (*adjItr)["focalz"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalex") )
							{
								entry.adjustToExpandedHelm[model].scalex = (*adjItr)["scalex"].GetDouble();
							}
							if ( (*adjItr).HasMember("scaley") )
							{
								entry.adjustToExpandedHelm[model].scaley = (*adjItr)["scaley"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalez") )
							{
								entry.adjustToExpandedHelm[model].scalez = (*adjItr)["scalez"].GetDouble();
							}
						}
					}
				}
			}
		}
	}

	if ( d.HasMember("base_offsets") && d["base_offsets"].HasMember("sprite_adjust") && d["base_offsets"]["sprite_adjust"].IsArray() )
	{
		for ( auto itr = d["base_offsets"]["sprite_adjust"].Begin(); itr != d["base_offsets"]["sprite_adjust"].End(); ++itr )
		{
			if ( (*itr).HasMember("sprite") )
			{
				int customSprite = (*itr)["sprite"].GetInt();
				monsterModelsMap[customSprite] = monsterModelsMap[monsterType];

				real_t baseFocalX = 0.0;
				real_t baseFocalY = 0.0;
				real_t baseFocalZ = 0.0;
				real_t baseFocalX_rot1 = 0.0;
				real_t baseFocalY_rot1 = 0.0;
				real_t baseFocalZ_rot1 = 0.0;
				if ( (*itr).HasMember("focalx") )
				{
					baseFocalX = (*itr)["focalx"].GetDouble();
				}
				if ( (*itr).HasMember("focaly") )
				{
					baseFocalY = (*itr)["focaly"].GetDouble();
				}
				if ( (*itr).HasMember("focalz") )
				{
					baseFocalZ = (*itr)["focalz"].GetDouble();
				}
				if ( (*itr).HasMember("focalx_rot1") )
				{
					baseFocalX_rot1 = (*itr)["focalx_rot1"].GetDouble();
				}
				if ( (*itr).HasMember("focaly_rot1") )
				{
					baseFocalY_rot1 = (*itr)["focaly_rot1"].GetDouble();
				}
				if ( (*itr).HasMember("focalz_rot1") )
				{
					baseFocalZ_rot1 = (*itr)["focalz_rot1"].GetDouble();
				}

				for ( auto& model : monsterModelsMap[customSprite] )
				{
					auto& entry = model.second;
					if ( static_cast<int>(entry.rotation) == 1 && version >= 2 )
					{
						entry.focalx += baseFocalX_rot1;
						entry.focaly += baseFocalY_rot1;
						entry.focalz += baseFocalZ_rot1;
					}
					else
					{
						entry.focalx += baseFocalX;
						entry.focaly += baseFocalY;
						entry.focalz += baseFocalZ;
					}
				}
			}
		}
	}

	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
}

