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
const SERVER: c_int = 1;
const SINGLE: c_int = 0;
const MAXPLAYERS: c_int = 4;
const MESSAGE_INTERACTION: c_uint = 1 << 7;

export fn actCampfire(my: *Entity) void {
    if (my.skill[4] == 0) {
        my.skill[4] = 1;
        my.skill[3] = MAXPLAYERS;
        c.entityCreateWorldUITooltip(my);
    }

    if (my.skill[3] > 0) {
        my.skill[5] -= 1;
        if (my.skill[5] <= 0) {
            my.skill[5] = 480;
            c.entityPlaySoundEntityLocal(my, 133, 128);
        }

        if (c.flickerLights) {
            var i: c_int = 0;
            while (i < 3) : (i += 1) {
                if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                    const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                    e.x += @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 30)) - 10)) / 10.0;
                    e.y += @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 30)) - 10)) / 10.0;
                    e.z -= 1;
                }
            }
            if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                e.z -= 2;
            }
        } else {
            if (my.ticks % TICKS_PER_SECOND == 0) {
                if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                    const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                    e.z -= 2;
                }
            }
        }

        if (my.skill[0] == 0) {
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "campfire",
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
                    "campfire",
                );
            } else {
                c.entityRemoveLightField(my);
                my.light = c.entityAddLight(
                    @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                    @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                    "campfire_flicker",
                );
            }
            my.skill[1] = 2 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 7));
        }
    } else {
        c.entityRemoveLightField(my);
        my.light = null;
        c.entityStopSound(my);
    }

    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    if (my.skill[3] > 0) {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(457));
                        my.skill[3] -= 1;
                        if (my.skill[3] <= 0) {
                            c.serverUpdateEntitySkillWrap(my, 3);
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(458));
                            c.entityRemoveLightField(my);
                            my.light = null;
                        }
                        const item = c.entityNewItemWrap(c.TOOL_TORCH, c.WORN, 0, 1, 0, true);
                        _ = c.entityItemPickup(i, item);
                    } else {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(458));
                    }
                }
            }
        }
    }
}

export fn actCauldron(my: *Entity) void {
    if (my.skill[4] == 0) {
        my.skill[4] = 1;
        my.skill[3] = MAXPLAYERS;
        c.entityCreateWorldUITooltip(my);
    }

    if (my.skill[3] > 0) {
        my.skill[5] -= 1;
        if (my.skill[5] <= 0) {
            my.skill[5] = 480;
            c.entityPlaySoundEntityLocal(my, 133, 128);
        }

        if (c.flickerLights) {
            var i: c_int = 0;
            while (i < 3) : (i += 1) {
                if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                    const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                    e.x += @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 30)) - 10)) / 10.0;
                    e.y += @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 30)) - 10)) / 10.0;
                    e.z -= 1;
                }
            }
            if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                e.z -= 2;
            }
        } else {
            if (my.ticks % TICKS_PER_SECOND == 0) {
                if (c.spawnFlame(my, c.SPRITE_FLAME)) |entity_raw| {
                    const e = @as(*Entity, @ptrCast(@alignCast(entity_raw)));
                    e.z -= 2;
                }
            }
        }

        if (my.skill[0] == 0) {
            my.light = c.entityAddLight(
                @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                "campfire",
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
                    "campfire",
                );
            } else {
                c.entityRemoveLightField(my);
                my.light = c.entityAddLight(
                    @as(c.Sint32, @intFromFloat(my.x / 16.0)),
                    @as(c.Sint32, @intFromFloat(my.y / 16.0)),
                    "campfire_flicker",
                );
            }
            my.skill[1] = 2 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 7));
        }
    } else {
        c.entityRemoveLightField(my);
        my.light = null;
        c.entityStopSound(my);
    }

    if (c.multiplayer == CLIENT) {
        return;
    }

    const cauldronInteracting: *c.Sint32 = &my.skill[6];
    const interacting_raw = c.entityUidToEntity(cauldronInteracting.*);

    if (cauldronInteracting.* > 0) {
        const interacting = if (interacting_raw) |e| @as(*Entity, @ptrCast(@alignCast(e))) else null;
        if (interacting == null or c.entityDist(interacting.?, my) > c.TOUCHRANGE) {
            var playernum: c_int = -1;
            if (interacting) |entity| {
                if (c.entityIsActPlayer(entity)) {
                    playernum = entity.skill[2];
                }
            } else {
                var i: c_int = 0;
                while (i < MAXPLAYERS) : (i += 1) {
                    if (c.entityAchievementObserverGetUid(i) == @as(c.Uint32, @bitCast(cauldronInteracting.*))) {
                        playernum = i;
                        break;
                    }
                }
            }
            cauldronInteracting.* = 0;
            c.serverUpdateEntitySkillWrap(my, 6);
            if (c.multiplayer == SERVER and playernum > 0) {
                c.entitySendPacketCauldronClose(my.uid, playernum);
            } else if (c.multiplayer == SINGLE or playernum == 0) {
                if (playernum >= 0 and playernum < MAXPLAYERS) {
                    c.entityCauldronCloseMenu(playernum);
                }
            }
        }
    }

    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
            if (c.inrange[idx]) {
                const playerEntity = c.entityGetPlayerEntity(i);
                if (playerEntity) |_| {
                    if (cauldronInteracting.* != 0) {
                        const interacting2_raw = c.entityUidToEntity(cauldronInteracting.*);
                        if (interacting2_raw) |e2| {
                            const interacting2 = @as(*Entity, @ptrCast(@alignCast(e2)));
                            if (interacting2 != @as(*Entity, @ptrCast(@alignCast(c.entityGetPlayerEntity(i))))) {
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6975));
                            }
                        }
                    } else {
                        const pe = c.entityGetPlayerEntity(i);
                        cauldronInteracting.* = if (pe) |p| @as(c.Sint32, @bitCast((@as(*Entity, @ptrCast(@alignCast(p)))).uid)) else 0;
                        if (c.multiplayer == SERVER) {
                            c.serverUpdateEntitySkillWrap(my, 6);
                        }
                        if (c.entityPlayerIsLocalPlayer(i)) {
                            c.entityCauldronOpenGUI(i, my);
                        } else if (c.multiplayer == SERVER and i > 0) {
                            c.entitySendPacketCauldronOpen(my.uid, i);
                        }
                    }
                    break;
                }
            }
        }
    }
}

export fn actWorkbench(my: *Entity) void {
    if (my.skill[4] == 0) {
        my.skill[4] = 1;
        my.skill[3] = MAXPLAYERS;
        c.entityCreateWorldUITooltip(my);
    }

    if (c.multiplayer == CLIENT) {
        return;
    }

    const workbenchInteracting: *c.Sint32 = &my.skill[6];
    const interacting_raw = c.entityUidToEntity(workbenchInteracting.*);

    if (workbenchInteracting.* > 0) {
        const interacting = if (interacting_raw) |e| @as(*Entity, @ptrCast(@alignCast(e))) else null;
        if (interacting == null or c.entityDist(interacting.?, my) > c.TOUCHRANGE) {
            var playernum: c_int = -1;
            if (interacting) |entity| {
                if (c.entityIsActPlayer(entity)) {
                    playernum = entity.skill[2];
                }
            } else {
                var i: c_int = 0;
                while (i < MAXPLAYERS) : (i += 1) {
                    if (c.entityAchievementObserverGetUid(i) == @as(c.Uint32, @bitCast(workbenchInteracting.*))) {
                        playernum = i;
                        break;
                    }
                }
            }
            workbenchInteracting.* = 0;
            c.serverUpdateEntitySkillWrap(my, 6);
            if (c.multiplayer == SERVER and playernum > 0) {
                c.entitySendPacketWorkbenchClose(my.uid, playernum);
            } else if (c.multiplayer == SINGLE or playernum == 0) {
                if (playernum >= 0 and playernum < MAXPLAYERS) {
                    c.entityWorkbenchCloseMenu(playernum);
                }
            }
        }
    }

    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
            if (c.inrange[idx]) {
                const playerEntity = c.entityGetPlayerEntity(i);
                if (playerEntity) |_| {
                    if (workbenchInteracting.* != 0) {
                        const interacting2_raw = c.entityUidToEntity(workbenchInteracting.*);
                        if (interacting2_raw) |e2| {
                            const interacting2 = @as(*Entity, @ptrCast(@alignCast(e2)));
                            if (interacting2 != @as(*Entity, @ptrCast(@alignCast(c.entityGetPlayerEntity(i))))) {
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6982));
                            }
                        }
                    } else {
                        const pe = c.entityGetPlayerEntity(i);
                        workbenchInteracting.* = if (pe) |p| @as(c.Sint32, @bitCast((@as(*Entity, @ptrCast(@alignCast(p)))).uid)) else 0;
                        if (c.multiplayer == SERVER) {
                            c.serverUpdateEntitySkillWrap(my, 6);
                        }
                        if (c.entityPlayerIsLocalPlayer(i)) {
                            c.entityWorkbenchOpenGUI(i, my);
                        } else if (c.multiplayer == SERVER and i > 0) {
                            c.entitySendPacketWorkbenchOpen(my.uid, i);
                        }
                    }
                    break;
                }
            }
        }
    }
}

export fn actMailbox(my: *Entity) void {
    if (my.skill[4] == 0) {
        my.skill[4] = 1;
        my.skill[3] = MAXPLAYERS;
        c.entityCreateWorldUITooltip(my);
    }

    if (c.multiplayer == CLIENT) {
        return;
    }

    const mailboxInteracting: *c.Sint32 = &my.skill[6];
    const interacting_raw = c.entityUidToEntity(mailboxInteracting.*);

    if (mailboxInteracting.* > 0) {
        const interacting = if (interacting_raw) |e| @as(*Entity, @ptrCast(@alignCast(e))) else null;
        if (interacting == null or c.entityDist(interacting.?, my) > c.TOUCHRANGE) {
            var playernum: c_int = -1;
            if (interacting) |entity| {
                if (c.entityIsActPlayer(entity)) {
                    playernum = entity.skill[2];
                }
            } else {
                var i: c_int = 0;
                while (i < MAXPLAYERS) : (i += 1) {
                    if (c.entityAchievementObserverGetUid(i) == @as(c.Uint32, @bitCast(mailboxInteracting.*))) {
                        playernum = i;
                        break;
                    }
                }
            }
            mailboxInteracting.* = 0;
            c.serverUpdateEntitySkillWrap(my, 6);
            if (c.multiplayer == SERVER and playernum > 0) {
                c.entitySendPacketMailboxClose(my.uid, playernum);
            } else if (c.multiplayer == SINGLE or playernum == 0) {
                if (playernum >= 0 and playernum < MAXPLAYERS) {
                    c.entityMailboxCloseMenu(playernum);
                }
            }
        }
    }

    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
            if (c.inrange[idx]) {
                const playerEntity = c.entityGetPlayerEntity(i);
                if (playerEntity) |_| {
                    if (mailboxInteracting.* != 0) {
                        const interacting2_raw = c.entityUidToEntity(mailboxInteracting.*);
                        if (interacting2_raw) |e2| {
                            const interacting2 = @as(*Entity, @ptrCast(@alignCast(e2)));
                            if (interacting2 != @as(*Entity, @ptrCast(@alignCast(c.entityGetPlayerEntity(i))))) {
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6987));
                            }
                        }
                    } else {
                        const pe = c.entityGetPlayerEntity(i);
                        mailboxInteracting.* = if (pe) |p| @as(c.Sint32, @bitCast((@as(*Entity, @ptrCast(@alignCast(p)))).uid)) else 0;
                        if (c.multiplayer == SERVER) {
                            c.serverUpdateEntitySkillWrap(my, 6);
                        }
                        if (c.entityPlayerIsLocalPlayer(i)) {
                            c.entityMailboxOpenGUI(i, my);
                        } else if (c.multiplayer == SERVER and i > 0) {
                            c.entitySendPacketMailboxOpen(my.uid, i);
                        }
                    }
                    break;
                }
            }
        }
    }
}
