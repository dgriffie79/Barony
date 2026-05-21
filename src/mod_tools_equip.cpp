/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_equip.cpp - Equipment model offsets (converted from rapidjson to cJSON)
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d )
	{
		return;
	}
	if ( !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		cJSON_Delete(d);
		return;
	}

	int version = cJSON_GetObjectItem(d, "version")->valueint;

	miscItemsBaseOffsets.clear();

	cJSON* itemsArr = cJSON_GetObjectItem(d, "items");
	if ( itemsArr )
	{
		cJSON* it = NULL;
		cJSON_ArrayForEach(it, itemsArr)
		{
			for ( cJSON* it2 = it->child; it2; it2 = it2->next )
			{
				std::string itemName = it2->string;
				if ( ItemTooltips.itemNameStringToItemID.find(itemName) == ItemTooltips.itemNameStringToItemID.end() )
				{
					continue;
				}
				ItemType itemType = (ItemType)ItemTooltips.itemNameStringToItemID[itemName];
				std::vector<int> models;
				cJSON* modelsItem = cJSON_GetObjectItem(it2, "models");
				if ( modelsItem )
				{
					if ( cJSON_IsArray(modelsItem) )
					{
						if ( cJSON_GetArraySize(modelsItem) == 0 )
						{
							for ( int i = items[itemType].index; i < items[itemType].index + items[itemType].variations; ++i )
							{
								models.push_back(i);
							}
						}
						else
						{
							cJSON* itArr = NULL;
							cJSON_ArrayForEach(itArr, modelsItem)
							{
								if ( cJSON_IsNumber(itArr) )
								{
									models.push_back(itArr->valueint);
								}
							}
						}
					}
				}

				cJSON* focalxItem = cJSON_GetObjectItem(it2, "focalx");
				real_t focalx = focalxItem ? focalxItem->valuedouble : 0.0;
				cJSON* focalyItem = cJSON_GetObjectItem(it2, "focaly");
				real_t focaly = focalyItem ? focalyItem->valuedouble : 0.0;
				cJSON* focalzItem = cJSON_GetObjectItem(it2, "focalz");
				real_t focalz = focalzItem ? focalzItem->valuedouble : 0.0;
				real_t scalex = 0.0;
				cJSON* scalexItem = cJSON_GetObjectItem(it2, "scalex");
				if ( scalexItem )
				{
					scalex = scalexItem->valuedouble;
				}
				real_t scaley = 0.0;
				cJSON* scaleyItem = cJSON_GetObjectItem(it2, "scaley");
				if ( scaleyItem )
				{
					scaley = scaleyItem->valuedouble;
				}
				real_t scalez = 0.0;
				cJSON* scalezItem = cJSON_GetObjectItem(it2, "scalez");
				if ( scalezItem )
				{
					scalez = scalezItem->valuedouble;
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
	cJSON_Delete(d);
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
	FileIO::close(fp);

	cJSON* d = cJSON_Parse(buf);
	if ( !d )
	{
		return;
	}
	if ( !cJSON_IsObject(d) || !cJSON_HasObjectItem(d, "version") || !cJSON_HasObjectItem(d, "items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		cJSON_Delete(d);
		return;
	}

	int version = cJSON_GetObjectItem(d, "version")->valueint;
	monsterModelsMap[monsterType].clear();

	real_t baseFocalX = 0.0;
	real_t baseFocalY = 0.0;
	real_t baseFocalZ = 0.0;
	real_t baseFocalX_rot1 = 0.0;
	real_t baseFocalY_rot1 = 0.0;
	real_t baseFocalZ_rot1 = 0.0;
	cJSON* baseOffsets = cJSON_GetObjectItem(d, "base_offsets");
	if ( baseOffsets )
	{
		cJSON* item = cJSON_GetObjectItem(baseOffsets, "focalx");
		if ( item ) baseFocalX = item->valuedouble;
		item = cJSON_GetObjectItem(baseOffsets, "focaly");
		if ( item ) baseFocalY = item->valuedouble;
		item = cJSON_GetObjectItem(baseOffsets, "focalz");
		if ( item ) baseFocalZ = item->valuedouble;
		item = cJSON_GetObjectItem(baseOffsets, "focalx_rot1");
		if ( item ) baseFocalX_rot1 = item->valuedouble;
		item = cJSON_GetObjectItem(baseOffsets, "focaly_rot1");
		if ( item ) baseFocalY_rot1 = item->valuedouble;
		item = cJSON_GetObjectItem(baseOffsets, "focalz_rot1");
		if ( item ) baseFocalZ_rot1 = item->valuedouble;
	}

	cJSON* itemsArr = cJSON_GetObjectItem(d, "items");
	if ( itemsArr )
	{
		cJSON* it = NULL;
		cJSON_ArrayForEach(it, itemsArr)
		{
			for ( cJSON* it2 = it->child; it2; it2 = it2->next )
			{
				std::string itemName = it2->string;
				if ( ItemTooltips.itemNameStringToItemID.find(itemName) == ItemTooltips.itemNameStringToItemID.end() )
				{
					continue;
				}
				ItemType itemType = (ItemType)ItemTooltips.itemNameStringToItemID[itemName];
				std::vector<int> models;
				cJSON* modelsItem = cJSON_GetObjectItem(it2, "models");
				if ( modelsItem )
				{
					if ( cJSON_IsArray(modelsItem) )
					{
						if ( cJSON_GetArraySize(modelsItem) == 0 )
						{
							for ( int i = items[itemType].index; i < items[itemType].index + items[itemType].variations; ++i )
							{
								models.push_back(i);
							}
						}
						else
						{
							cJSON* itArr = NULL;
							cJSON_ArrayForEach(itArr, modelsItem)
							{
								if ( cJSON_IsNumber(itArr) )
								{
									models.push_back(itArr->valueint);
								}
							}
						}
					}
				}

				cJSON* focalxItem = cJSON_GetObjectItem(it2, "focalx");
				real_t focalx = focalxItem ? focalxItem->valuedouble : 0.0;
				cJSON* focalyItem = cJSON_GetObjectItem(it2, "focaly");
				real_t focaly = focalyItem ? focalyItem->valuedouble : 0.0;
				cJSON* focalzItem = cJSON_GetObjectItem(it2, "focalz");
				real_t focalz = focalzItem ? focalzItem->valuedouble : 0.0;
				real_t scalex = 0.0;
				cJSON* scalexItem = cJSON_GetObjectItem(it2, "scalex");
				if ( scalexItem )
				{
					scalex = scalexItem->valuedouble;
				}
				real_t scaley = 0.0;
				cJSON* scaleyItem = cJSON_GetObjectItem(it2, "scaley");
				if ( scaleyItem )
				{
					scaley = scaleyItem->valuedouble;
				}
				real_t scalez = 0.0;
				cJSON* scalezItem = cJSON_GetObjectItem(it2, "scalez");
				if ( scalezItem )
				{
					scalez = scalezItem->valuedouble;
				}
				cJSON* rotItem = cJSON_GetObjectItem(it2, "rotation");
				real_t rotation = rotItem ? rotItem->valuedouble : 0.0;
				cJSON* pitchItem = cJSON_GetObjectItem(it2, "pitch");
				real_t pitch = pitchItem ? pitchItem->valuedouble : 0.0;
				cJSON* limbsItem = cJSON_GetObjectItem(it2, "limbs_index");
				int limbsIndex = limbsItem ? limbsItem->valueint : 0;
				cJSON* oversizeItem = cJSON_GetObjectItem(it2, "oversize_mask");
				bool oversizedMask = oversizeItem ? cJSON_IsTrue(oversizeItem) : false;
				cJSON* expandItem = cJSON_GetObjectItem(it2, "expand_to_fit_oversize_mask");
				bool expandToFitMask = expandItem ? cJSON_IsTrue(expandItem) : false;

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

					cJSON* adjustOversize = cJSON_GetObjectItem(it2, "adjust_on_oversize_mask");
					if ( adjustOversize && cJSON_IsArray(adjustOversize) )
					{
						cJSON* adjItr = NULL;
						cJSON_ArrayForEach(adjItr, adjustOversize)
						{
							std::vector<int> models;
							cJSON* maskSprite = cJSON_GetObjectItem(adjItr, "mask_sprite");
							if ( cJSON_IsArray(maskSprite) )
							{
								if ( cJSON_GetArraySize(maskSprite) == 0 )
								{
									models.push_back(-1);
								}
								else
								{
									cJSON* itr2 = NULL;
									cJSON_ArrayForEach(itr2, maskSprite)
									{
										if ( cJSON_IsNumber(itr2) )
										{
											models.push_back(itr2->valueint);
										}
									}
								}
							}
							for ( auto model : models )
							{
								cJSON* val = cJSON_GetObjectItem(adjItr, "focalx");
								if ( val ) entry.adjustToOversizeMask[model].focalx = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "focaly");
								if ( val ) entry.adjustToOversizeMask[model].focaly = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "focalz");
								if ( val ) entry.adjustToOversizeMask[model].focalz = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scalex");
								if ( val ) entry.adjustToOversizeMask[model].scalex = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scaley");
								if ( val ) entry.adjustToOversizeMask[model].scaley = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scalez");
								if ( val ) entry.adjustToOversizeMask[model].scalez = val->valuedouble;
							}
						}
					}

					cJSON* adjustExpand = cJSON_GetObjectItem(it2, "adjust_on_expand_helm");
					if ( adjustExpand && cJSON_IsArray(adjustExpand) )
					{
						cJSON* adjItr = NULL;
						cJSON_ArrayForEach(adjItr, adjustExpand)
						{
							std::vector<int> models;
							cJSON* helmSprite = cJSON_GetObjectItem(adjItr, "helm_sprite");
							if ( cJSON_IsArray(helmSprite) )
							{
								if ( cJSON_GetArraySize(helmSprite) == 0 )
								{
									models.push_back(-1);
								}
								else
								{
									cJSON* itr2 = NULL;
									cJSON_ArrayForEach(itr2, helmSprite)
									{
										if ( cJSON_IsNumber(itr2) )
										{
											models.push_back(itr2->valueint);
										}
									}
								}
							}
							for ( auto model : models )
							{
								cJSON* val = cJSON_GetObjectItem(adjItr, "focalx");
								if ( val ) entry.adjustToExpandedHelm[model].focalx = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "focaly");
								if ( val ) entry.adjustToExpandedHelm[model].focaly = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "focalz");
								if ( val ) entry.adjustToExpandedHelm[model].focalz = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scalex");
								if ( val ) entry.adjustToExpandedHelm[model].scalex = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scaley");
								if ( val ) entry.adjustToExpandedHelm[model].scaley = val->valuedouble;
								val = cJSON_GetObjectItem(adjItr, "scalez");
								if ( val ) entry.adjustToExpandedHelm[model].scalez = val->valuedouble;
							}
						}
					}
				}
			}
		}
	}

	cJSON* spriteAdjust = NULL;
	if ( baseOffsets )
	{
		spriteAdjust = cJSON_GetObjectItem(baseOffsets, "sprite_adjust");
	}
	if ( spriteAdjust && cJSON_IsArray(spriteAdjust) )
	{
		cJSON* itr = NULL;
		cJSON_ArrayForEach(itr, spriteAdjust)
		{
			cJSON* spriteItem = cJSON_GetObjectItem(itr, "sprite");
			if ( spriteItem )
			{
				int customSprite = spriteItem->valueint;
				monsterModelsMap[customSprite] = monsterModelsMap[monsterType];

				real_t baseFocalX = 0.0;
				real_t baseFocalY = 0.0;
				real_t baseFocalZ = 0.0;
				real_t baseFocalX_rot1 = 0.0;
				real_t baseFocalY_rot1 = 0.0;
				real_t baseFocalZ_rot1 = 0.0;
				cJSON* val = cJSON_GetObjectItem(itr, "focalx");
				if ( val ) baseFocalX = val->valuedouble;
				val = cJSON_GetObjectItem(itr, "focaly");
				if ( val ) baseFocalY = val->valuedouble;
				val = cJSON_GetObjectItem(itr, "focalz");
				if ( val ) baseFocalZ = val->valuedouble;
				val = cJSON_GetObjectItem(itr, "focalx_rot1");
				if ( val ) baseFocalX_rot1 = val->valuedouble;
				val = cJSON_GetObjectItem(itr, "focaly_rot1");
				if ( val ) baseFocalY_rot1 = val->valuedouble;
				val = cJSON_GetObjectItem(itr, "focalz_rot1");
				if ( val ) baseFocalZ_rot1 = val->valuedouble;

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

	cJSON_Delete(d);
	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
}
