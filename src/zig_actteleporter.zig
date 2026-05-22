const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("player.h");
    @cInclude("light.h");
});

const Entity = c.Entity;

export fn actTeleporter(my: *Entity) void {
    const CLIENT = 2;
    const MAXPLAYERS = 4;
    const MESSAGE_INTERACTION = 1 << 7;
    const TICKS_PER_SECOND = 50;

    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    if (my.skill[8] > 0 and c.multiplayer != CLIENT) {
        my.skill[8] -= 1;
        if (my.skill[8] <= 0) {
            c.entityRemoveLightField(my);
            c.list_RemoveNode(my.mynode);
            return;
        }
    }

    my.skill[4] -= 1;
    if (my.skill[4] <= 0) {
        my.skill[4] = TICKS_PER_SECOND * 30;
        c.entityPlaySoundEntityLocal(my, 149, 64);
    }

    if (c.multiplayer != CLIENT) {
        if (c.entityIsInteractWithMonster(my)) {
            const monsterInteracting = c.entityUidToEntity(my.skill[47]);
            if (monsterInteracting != null) {
                if (my.skill[3] == 3) {
                    if (c.entityTeleport(monsterInteracting, my.skill[0], my.skill[1])) {
                        c.entityTeleporterMagicEvent(my);
                    }
                } else {
                    _ = c.entityTeleporterMove(monsterInteracting, my.skill[0], my.skill[1], my.skill[3]);
                }
                c.entityClearMonsterInteract(my);
                return;
            }
            c.entityClearMonsterInteract(my);
        }
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                const interactEntity = c.entityGetPlayerInteractEntity(i);
                if (c.inrange[idx] and interactEntity != null) {
                    switch (my.skill[3]) {
                        0 => {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(2378));
                        },
                        1 => {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(506));
                        },
                        2 => {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(510));
                        },
                        else => {},
                    }
                    if (my.skill[3] == 3) {
                        if (c.entityTeleport(interactEntity, my.skill[0], my.skill[1])) {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6696));
                            c.entityTeleporterMagicEvent(my);
                        }
                    } else {
                        _ = c.entityTeleporterMove(interactEntity, my.skill[0], my.skill[1], my.skill[3]);
                    }
                    return;
                }
            }
        }
    }

    if (my.skill[3] == 2) {
        if (my.light == null) {
            my.light = c.entityAddLight(@as(c_int, @intFromFloat(my.x / 16.0)), @as(c_int, @intFromFloat(my.y / 16.0)), "portal_purple");
        }
        my.yaw += 0.01;
        my.sprite = @as(c_int, 620);
        const tickAnim = @as(c_int, @intCast(my.ticks / 20 % 4));
        if (tickAnim > 0) {
            my.sprite = @as(c_int, 992) + tickAnim - 1;
        }
    } else if (my.skill[3] == 3) {
        if (my.light == null) {
            my.light = c.entityAddLight(@as(c_int, @intFromFloat(my.x / 16.0)), @as(c_int, @intFromFloat(my.y / 16.0)), "portal_purple");
        }
        if (c.ticks % 4 == 0) {
            my.sprite = my.skill[5] + my.skill[6];
            my.skill[6] += 1;
            if (my.skill[6] >= my.skill[7]) {
                my.skill[6] = 0;
            }
        }
        const increment = @max(@as(f64, 0.05), @as(f64, 1.0) - my.scalex) / 3.0;
        my.scalex = @min(@as(f64, 1.0), my.scalex + increment);
        my.scaley = @min(@as(f64, 1.0), my.scaley + increment);
        my.scalez = @min(@as(f64, 1.0), my.scalez + increment);
    }
}
