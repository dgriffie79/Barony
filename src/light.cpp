/*-------------------------------------------------------------------------------

	BARONY
	File: light.cpp
	Desc: light spawning code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "light.hpp"
#include "draw.hpp"

/*-------------------------------------------------------------------------------

	lightSphereShadow

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and color; casts shadows against walls

-------------------------------------------------------------------------------*/

light_t* lightSphereShadow(int index, Sint32 x, Sint32 y, Sint32 radius, float r, float g, float b, float a, float exp)
{
	light_t* light = newLight(index, x, y, radius);
    r = r * 255.f;
    g = g * 255.f;
    b = b * 255.f;
    a = a * 255.f;

	for (int v = y - radius; v <= y + radius; ++v) {
		for (int u = x - radius; u <= x + radius; ++u) {
			if (u >= 0 && v >= 0 && u < map.width && v < map.height) {
				const int dx = u - x;
				const int dy = v - y;
				const int dxabs = abs(dx);
				const int dyabs = abs(dy);
				real_t a0 = dyabs * .5;
				real_t b0 = dxabs * .5;
				int u2 = u;
				int v2 = v;
                
                // check origin is okay
				bool wallhit = true;
				const int mapindex = v * MAPLAYERS + u * MAPLAYERS * map.height;
				for (int z = 0; z < MAPLAYERS; z++) {
					if ( !map.tiles[mapindex + z] || map.tiles[mapindex + z] == TRANSPARENT_TILE ) {
						wallhit = false;
						break;
					}
				}
				if (wallhit == true) {
					continue;
				}
                
                // line test
                if (dxabs >= dyabs) { // the line is more horizontal than vertical
					for (int i = 0; i < dxabs; ++i) {
						u2 -= sgn(dx);
						b0 += dyabs;
						if (b0 >= dxabs) {
							b0 -= dxabs;
							v2 -= sgn(dy);
						}
						if (u2 >= 0 && u2 < map.width && v2 >= 0 && v2 < map.height) {
							if ( map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height]
                                && map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height] != TRANSPARENT_TILE ) {
								wallhit = true;
								break;
							}
						}
					}
				}
                else { // the line is more vertical than horizontal
					for (int i = 0; i < dyabs; ++i) {
						v2 -= sgn(dy);
						a0 += dxabs;
						if (a0 >= dyabs) {
							a0 -= dyabs;
							u2 -= sgn(dx);
						}
						if (u2 >= 0 && u2 < map.width && v2 >= 0 && v2 < map.height) {
							if (map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height]
                                && map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height] != TRANSPARENT_TILE) {
								wallhit = true;
								break;
							}
						}
					}
				}
                
                // light tile if it passed line test
				if (wallhit == false || (wallhit == true && u2 == u && v2 == v)) {
                    const float dist = exp != 1.f ? powf(dx * dx + dy * dy, exp) : dx * dx + dy * dy;
                    const auto falloff = std::min<float>(dist / radius, 1.0f);
                    const auto soff = (dy + radius) + (dx + radius) * (radius * 2 + 1);
                    auto& s = light->tiles[soff];
					s.x += r - r * falloff;
                    s.y += g - g * falloff;
                    s.z += b - b * falloff;
                    s.w += a - a * falloff;
                    const auto doff = v + u * map.height;
                    if (index) {
                        auto& d = lightmaps[index][doff];
                        d.x += s.x;
                        d.y += s.y;
                        d.z += s.z;
                        d.w += s.w;
                    } else {
                        for (int c = 0; c < MAXPLAYERS + 1; ++c) {
                            auto& d = lightmaps[c][doff];
                            d.x += s.x;
                            d.y += s.y;
                            d.z += s.z;
                            d.w += s.w;
                        }
                    }
                }
			}
		}
	}
	return light;
}

/*-------------------------------------------------------------------------------

	lightSphere

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and color; casts no shadows

-------------------------------------------------------------------------------*/

light_t* lightSphere(int index, Sint32 x, Sint32 y, Sint32 radius, float r, float g, float b, float a, float exp)
{
	light_t* light = newLight(index, x, y, radius);
    r = r * 255.f;
    g = g * 255.f;
    b = b * 255.f;
    a = a * 255.f;

	for (int v = y - radius; v <= y + radius; ++v) {
		for (int u = x - radius; u <= x + radius; ++u) {
			if (u >= 0 && v >= 0 && u < map.width && v < map.height) {
				const int dx = u - x;
				const int dy = v - y;
                const float dist = exp != 1.f ? powf(dx * dx + dy * dy, exp) : dx * dx + dy * dy;
                const auto falloff = std::min<float>(dist / radius, 1.0f);
                const auto soff = (dy + radius) + (dx + radius) * (radius * 2 + 1);
                auto& s = light->tiles[soff];
                s.x += r - r * falloff;
                s.y += g - g * falloff;
                s.z += b - b * falloff;
                s.w += a - a * falloff;
                const auto doff = v + u * map.height;
                if (index) {
                    auto& d = lightmaps[index][doff];
                    d.x += s.x;
                    d.y += s.y;
                    d.z += s.z;
                    d.w += s.w;
                } else {
                    for (int c = 0; c < MAXPLAYERS + 1; ++c) {
                        auto& d = lightmaps[c][doff];
                        d.x += s.x;
                        d.y += s.y;
                        d.z += s.z;
                        d.w += s.w;
                    }
                }
			}
		}
	}
	return light;
}

#include "cJSON.h"
#include "files.hpp"

std::unordered_map<std::string, LightDef> lightDefs;
bool loadLights(bool forceLoadBaseDirectory) {
    if ( !PHYSFS_getRealDir("/data/lights.json") )
    {
        printlog("[JSON]: Error: Could not find file: data/lights.json");
        return false;
    }

    std::string inputPath = PHYSFS_getRealDir("/data/lights.json");
    if ( forceLoadBaseDirectory )
    {
        inputPath = BASE_DATA_DIR;
    }
    else
    {
        if ( inputPath != BASE_DATA_DIR )
        {
            loadLights(true); // force load the base directory first, then modded paths later.
        }
        else
        {
            forceLoadBaseDirectory = true;
        }
    }

    inputPath.append("/data/lights.json");

    File* fp = FileIO::open(inputPath.c_str(), "rb");
    if ( !fp )
    {
        printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
        return false;
    }

    if ( forceLoadBaseDirectory )
    {
        lightDefs.clear();
    }
    
    char buf[65536];
    int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf));
    buf[count] = '\0';
    FileIO::close(fp);

    cJSON* doc = cJSON_Parse(buf);
    if (!doc) {
        printlog("[JSON]: Error: Failed to parse lights.json: %s", cJSON_GetErrorPtr());
        return false;
    }

    cJSON* lights = cJSON_GetObjectItem(doc, "lights");
    if (cJSON_IsObject(lights)) {
        cJSON* lightItem = NULL;
        cJSON_ArrayForEach(lightItem, lights) {
            LightDef def;
            const char* name = lightItem->string;

            cJSON* radiusItem = cJSON_GetObjectItem(lightItem, "radius");
            def.radius = radiusItem ? radiusItem->valueint : 0;

            cJSON* rItem = cJSON_GetObjectItem(lightItem, "r");
            def.r = rItem ? (float)rItem->valuedouble : 0.0f;

            cJSON* gItem = cJSON_GetObjectItem(lightItem, "g");
            def.g = gItem ? (float)gItem->valuedouble : 0.0f;

            cJSON* bItem = cJSON_GetObjectItem(lightItem, "b");
            def.b = bItem ? (float)bItem->valuedouble : 0.0f;

            cJSON* aItem = cJSON_GetObjectItem(lightItem, "a");
            if (cJSON_IsNumber(aItem)) {
                def.a = (float)aItem->valuedouble;
            } else {
                def.a = 0.f;
            }

            cJSON* expItem = cJSON_GetObjectItem(lightItem, "falloff_exp");
            def.falloff_exp = expItem ? (float)expItem->valuedouble : 1.0f;

            cJSON* shadowsItem = cJSON_GetObjectItem(lightItem, "shadows");
            def.shadows = cJSON_IsTrue(shadowsItem);

            lightDefs[name] = def;
        }
    }

    cJSON_Delete(doc);
    return true;
}

#ifndef EDITOR
#include "interface/consolecommand.hpp"
static ConsoleCommand ccmd_reloadLights("/reloadlights", "reload light json",
    [](int argc, const char* argv[]){
    loadLights();
    });
#endif

light_t* addLight(Sint32 x, Sint32 y, const char* name, int range_bonus, int index) {
    if (!name || !name[0]) {
        return nullptr;
    }
    auto find = lightDefs.find(name);
    if (find == lightDefs.end()) {
        return nullptr;
    }
    const auto& def = find->second;
    if (def.shadows) {
        return lightSphereShadow(index, x, y, std::max(def.radius + range_bonus, 1), def.r, def.g, def.b, def.a, def.falloff_exp);
    } else {
        return lightSphere(index, x, y, std::max(def.radius + range_bonus, 1), def.r, def.g, def.b, def.a, def.falloff_exp);
    }
}
