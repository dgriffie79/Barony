const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("player.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
    @cInclude("item_types.h");
    @cInclude("items.h");
    @cInclude("light.h");
});

const Entity = c.Entity;

const TICKS_PER_SECOND: c_int = 50;
const CLIENT: c_int = 2;
const MAXPLAYERS: c_int = 4;
const MAPLAYERS: c_int = 3;
const OBSTACLELAYER: c_int = 1;
const MESSAGE_INTERACTION: c_uint = 1 << 7;
const MESSAGE_INVENTORY: c_uint = 1 << 2;

export fn actTorch(my: *Entity) void {
    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    // ambient noises
    if (my.skill[3] <= 0) {
        my.skill[3] = 480;
        c.entityPlaySoundEntityLocal(my, 133, 32);
    } else {
        my.skill[3] -= 1;
    }

    if (c.flickerLights or my.ticks % TICKS_PER_SECOND == 1) {
        if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity| {
            const e = @as(*Entity, @ptrCast(@alignCast(entity)));
            e.x += 0.25 * @cos(my.yaw);
            e.y += 0.25 * @sin(my.yaw);
            e.z -= 2.5;
            e.flags[c.GENIUS] = 0;
            c.entitySetUID(e, @as(c.Uint32, @bitCast(@as(c_int, -3))));
        }
    }

    // check wall behind me
    var checkx: c_int = @intFromFloat(my.x - @cos(my.yaw) * 8.0);
    checkx >>= 4;
    var checky: c_int = @intFromFloat(my.y - @sin(my.yaw) * 8.0);
    checky >>= 4;
    const tileIndex = OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * @as(c_int, @intCast(c.entityMapGetHeight()));
    if (c.entityMapGetTile(tileIndex) == 0) {
        c.entityRemoveLightField(my);
        c.list_RemoveNode(my.mynode);
        return;
    }

    // lighting
    if (my.skill[0] == 0) {
        my.light = c.entityAddLight(
            @as(c.Sint32, @intFromFloat(my.x / 16.0)),
            @as(c.Sint32, @intFromFloat(my.y / 16.0)),
            "torch_wall",
        );
        my.skill[0] = 1;
    }

    if (c.flickerLights) {
        my.skill[1] -= 1;
    }

    if (my.skill[1] <= 0) {
        my.skill[0] = @as(c.Sint32, @intFromBool(my.skill[0] == 1)) + 1;
        if (my.skill[0] == 1) {
            c.entityRemoveLightField(my);
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "torch_wall",
            );
        } else {
            c.entityRemoveLightField(my);
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "torch_wall_flicker",
            );
        }
        my.skill[1] = 2 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 7));
    }

    // using
    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    var trySalvage = false;
                    const playerEntity = c.entityGetPlayerEntity(i);
                    if (playerEntity) |pe_raw| {
                        const pe = @as(*Entity, @ptrCast(@alignCast(pe_raw)));
                        const salvageUid: c.Uint32 = @intCast(my.itemAutoSalvageByPlayer.*);
                        if (salvageUid == pe.uid) {
                            trySalvage = true;
                            my.itemAutoSalvageByPlayer.* = 0;
                        }
                    }

                    const item = c.entityNewItemWrap(
                        c.TOOL_TORCH,
                        c.WORN,
                        0, 1, 0, true,
                    );

                    if (trySalvage) {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, c.languageGet(589));
                        const salvaged = c.entityTorchTrySalvage(i, item);
                        if (salvaged) {
                            const pe2 = c.entityGetPlayerEntity(i);
                            if (pe2) |p_raw| {
                                const p = @as(*Entity, @ptrCast(@alignCast(p_raw)));
                                const snd: c_ushort = @intCast(35 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 3)));
                                c.entityPlaySoundEntity(p, snd, 64);
                            }
                            c.entityRemoveLightNode(my);
                            c.list_RemoveNode(my.mynode);
                            return;
                        } else {
                            return;
                        }
                    } else {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, c.languageGet(589));
                        c.entityCompendiumTorchEvent(i, c.TOOL_TORCH);
                        c.entityRemoveLightNode(my);
                        c.list_RemoveNode(my.mynode);
                        _ = c.entityItemPickup(i, item);
                    }
                    return;
                }
            }
        }
        if (c.entityIsInteractWithMonster(my)) {
            c.entityRemoveLightNode(my);
            const monster = c.entityUidToEntity(@as(c.Sint32, my.interactedByMonster.*));
            c.entityClearMonsterInteract(my);
            if (monster) |m_raw| {
                const m = @as(*Entity, @ptrCast(@alignCast(m_raw)));
                const mStats = c.entityGetStats(m);
                if (mStats) |ms| {
                    const item2 = c.entityNewItemWrap(
                        c.TOOL_TORCH,
                        c.WORN,
                        0, 1, 0, true,
                    );
                    _ = c.entityDropItemMonsterWrap(item2, m, ms);
                }
            }
            c.list_RemoveNode(my.mynode);
        }
    }
}

export fn actCrystalShard(my: *Entity) void {
    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    // ambient noises
    if (my.skill[3] <= 0) {
        my.skill[3] = 480;
        c.entityPlaySoundEntityLocal(my, 133, 32);
    } else {
        my.skill[3] -= 1;
    }

    // wall check
    var checkx: c_int = @intFromFloat(my.x - @cos(my.yaw) * 8.0);
    checkx >>= 4;
    var checky: c_int = @intFromFloat(my.y - @sin(my.yaw) * 8.0);
    checky >>= 4;
    const tileIndex = OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * @as(c_int, @intCast(c.entityMapGetHeight()));
    if (c.entityMapGetTile(tileIndex) == 0) {
        c.entityRemoveLightField(my);
        c.list_RemoveNode(my.mynode);
        return;
    }

    // lighting
    if (my.skill[0] == 0) {
        my.light = c.entityAddLight(
            @as(c.Sint32, @intFromFloat(my.x / 16.0)),
            @as(c.Sint32, @intFromFloat(my.y / 16.0)),
            "crystal_shard_wall",
        );
        my.skill[0] = 1;
    }

    if (c.flickerLights) {
        my.skill[1] -= 1;
    }

    if (my.skill[1] <= 0) {
        my.skill[0] = @as(c.Sint32, @intFromBool(my.skill[0] == 1)) + 1;
        if (my.skill[0] == 1) {
            c.entityRemoveLightField(my);
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "crystal_shard_wall",
            );
        } else {
            c.entityRemoveLightField(my);
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "crystal_shard_wall_flicker",
            );
        }
        my.skill[1] = 2 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 7));
    }

    // using
    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    var trySalvage = false;
                    const playerEntity = c.entityGetPlayerEntity(i);
                    if (playerEntity) |pe_raw| {
                        const pe = @as(*Entity, @ptrCast(@alignCast(pe_raw)));
                        const salvageUid: c.Uint32 = @intCast(my.itemAutoSalvageByPlayer.*);
                        if (salvageUid == pe.uid) {
                            trySalvage = true;
                            my.itemAutoSalvageByPlayer.* = 0;
                        }
                    }

                    const item = c.entityNewItemWrap(
                        c.TOOL_CRYSTALSHARD,
                        c.WORN,
                        0, 1, 0, true,
                    );

                    if (trySalvage) {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, c.languageGet(589));
                        const salvaged = c.entityTorchTrySalvage(i, item);
                        if (salvaged) {
                            const pe2 = c.entityGetPlayerEntity(i);
                            if (pe2) |p_raw| {
                                const p = @as(*Entity, @ptrCast(@alignCast(p_raw)));
                                const snd: c_ushort = @intCast(35 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 3)));
                                c.entityPlaySoundEntity(p, snd, 64);
                            }
                            c.entityRemoveLightNode(my);
                            c.list_RemoveNode(my.mynode);
                            return;
                        } else {
                            return;
                        }
                    } else {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, c.languageGet(589));
                        c.entityCompendiumTorchEvent(i, c.TOOL_CRYSTALSHARD);
                        c.entityRemoveLightNode(my);
                        c.list_RemoveNode(my.mynode);
                        _ = c.entityItemPickup(i, item);
                    }
                    return;
                }
            }
        }
    }
}

export fn actLightSource(my: *Entity) void {
    const CIRCUIT_ON: c_int = 2;
    const CIRCUIT_OFF: c_int = 1;

    if (c.multiplayer != CLIENT) {
        if (my.lightSourceDelay.* > 0 and my.lightSourceDelayCounter.* == 0) {
            my.lightSourceDelayCounter.* = my.lightSourceDelay.*;
        }
    }

    if (my.skill[10] != 0) { // LIGHTSOURCE_ENABLED
        // lighting
        if (my.skill[8] == 0) { // LIGHTSOURCE_LIGHT
            const brightness: f32 = @floatFromInt(my.lightSourceBrightness.*);
            const color = brightness / 255.0;
            const rgb_val: u32 = @bitCast(my.lightSourceRGB.*);
            const r: f32 = (color / 255.0) * @as(f32, @floatFromInt(rgb_val & 0xFF));
            const g: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 8) & 0xFF));
            const b: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 16) & 0xFF));
            my.light = c.entityLightSphereShadowWrap(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                my.lightSourceRadius.*,
                r, g, b,
            );
            my.skill[8] = 1;
        }

        if (my.lightSourceFlicker.* != 0 and c.flickerLights) {
            my.skill[9] -= 1; // LIGHTSOURCE_FLICKER
        } else {
            my.skill[8] = 1;
            if (my.light == null) {
                const brightness: f32 = @floatFromInt(my.lightSourceBrightness.*);
                const color = brightness / 255.0;
                const rgb_val: u32 = @bitCast(my.lightSourceRGB.*);
                const r: f32 = (color / 255.0) * @as(f32, @floatFromInt(rgb_val & 0xFF));
                const g: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 8) & 0xFF));
                const b: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 16) & 0xFF));
                my.light = c.entityLightSphereShadowWrap(
                    @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                    @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                    my.lightSourceRadius.*,
                    r, g, b,
                );
            }
        }

        if (my.skill[9] <= 0) { // LIGHTSOURCE_FLICKER
            my.skill[8] = @as(c.Sint32, @intFromBool(my.skill[8] == 1)) + 1;

            if (my.skill[8] == 1) {
                c.entityRemoveLightField(my);
                const brightness: f32 = @floatFromInt(my.lightSourceBrightness.*);
                const color = brightness / 255.0;
                const rgb_val: u32 = @bitCast(my.lightSourceRGB.*);
                const r: f32 = (color / 255.0) * @as(f32, @floatFromInt(rgb_val & 0xFF));
                const g: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 8) & 0xFF));
                const b: f32 = (color / 255.0) * @as(f32, @floatFromInt((rgb_val >> 16) & 0xFF));
                my.light = c.entityLightSphereShadowWrap(
                    @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                    @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                    my.lightSourceRadius.*,
                    r, g, b,
                );
            } else {
                c.entityRemoveLightField(my);
                const dimBrightness = @max(my.lightSourceBrightness.* - 16, 0);
                const dimColor = @as(f32, @floatFromInt(dimBrightness)) / 255.0;
                const rgb_val: u32 = @bitCast(my.lightSourceRGB.*);
                const r: f32 = (dimColor / 255.0) * @as(f32, @floatFromInt(rgb_val & 0xFF));
                const g: f32 = (dimColor / 255.0) * @as(f32, @floatFromInt((rgb_val >> 8) & 0xFF));
                const b: f32 = (dimColor / 255.0) * @as(f32, @floatFromInt((rgb_val >> 16) & 0xFF));
                my.light = c.entityLightSphereShadowWrap(
                    @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                    @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                    my.lightSourceRadius.*,
                    r, g, b,
                );
            }
            my.skill[9] = 2 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 7));
        }

        if (c.multiplayer != CLIENT) {
            const alwaysOn = my.lightSourceAlwaysOn.*;
            const invertPower = my.lightSourceInvertPower.*;
            const delay = my.lightSourceDelay.*;
            const delayCounter = my.lightSourceDelayCounter.*;
            if ((alwaysOn == 0 and my.circuit_status.* == CIRCUIT_OFF and invertPower == 0) or
                (my.circuit_status.* == CIRCUIT_ON and invertPower == 1))
            {
                if (my.skill[10] == 1 and my.lightSourceLatchOn.* < 2 + invertPower) {
                    if (invertPower == 1 and delayCounter > 0) {
                        my.lightSourceDelayCounter.* -= 1;
                        if (my.lightSourceDelayCounter.* != 0) {
                            return;
                        }
                    } else if (invertPower == 0 and delay > 0) {
                        my.lightSourceDelayCounter.* = delay;
                    }
                    my.skill[10] = 0;
                    c.serverUpdateEntitySkillWrap(my, 10);
                    if (my.lightSourceLatchOn.* > 0) {
                        my.lightSourceLatchOn.* += 1;
                    }
                }
            }
        }
    } else {
        c.entityRemoveLightField(my);
        if (c.multiplayer == CLIENT) {
            return;
        }

        const alwaysOn = my.lightSourceAlwaysOn.*;
        const invertPower = my.lightSourceInvertPower.*;
        const delay = my.lightSourceDelay.*;
        const delayCounter = my.lightSourceDelayCounter.*;
        if (alwaysOn == 1 or
            (my.circuit_status.* == CIRCUIT_ON and invertPower == 0) or
            (my.circuit_status.* == CIRCUIT_OFF and invertPower == 1))
        {
            if (my.skill[10] == 0 and my.lightSourceLatchOn.* < 2 + invertPower) {
                if (invertPower == 0 and delayCounter > 0) {
                    my.lightSourceDelayCounter.* -= 1;
                    if (my.lightSourceDelayCounter.* != 0) {
                        return;
                    }
                } else if (invertPower == 1 and delay > 0) {
                    my.lightSourceDelayCounter.* = delay;
                }
                my.skill[10] = 1;
                c.serverUpdateEntitySkillWrap(my, 10);
                if (my.lightSourceLatchOn.* > 0) {
                    my.lightSourceLatchOn.* += 1;
                }
            }
        }
    }
}
