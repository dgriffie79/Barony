const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("collision.h");
    @cInclude("game.h");
    @cInclude("player.h");
});

const Entity = c.Entity;

export fn actGate(my: *Entity) void {
    const CLIENT = 2;
    const CIRCUIT_ON = 2;
    const CIRCUIT_OFF = 1;
    const NOUPDATE = 3;
    const PASSABLE = 12;
    const MAXPLAYERS = 4;
    const MESSAGE_INTERACTION = 1 << 7;

    // Server-side gate logic
    if (c.multiplayer != CLIENT) {
        if (my.skill[28] == 0) return;

        if (my.skill[5] == 0) {
            if (my.skill[28] == CIRCUIT_ON) {
                if (my.skill[3] == 0) {
                    my.skill[3] = 1;
                    c.entityPlaySoundEntity(my, 81, 64);
                    c.serverUpdateEntitySkillWrap(my, 3);
                }
            } else {
                if (my.skill[3] != 0) {
                    my.skill[3] = 0;
                    c.entityPlaySoundEntity(my, 82, 64);
                    c.serverUpdateEntitySkillWrap(my, 3);
                }
            }
        } else {
            if (my.skill[28] == CIRCUIT_OFF) {
                if (my.skill[3] == 0) {
                    my.skill[3] = 1;
                    c.entityPlaySoundEntity(my, 81, 64);
                    c.serverUpdateEntitySkillWrap(my, 3);
                }
            } else {
                if (my.skill[3] != 0) {
                    my.skill[3] = 0;
                    c.entityPlaySoundEntity(my, 82, 64);
                    c.serverUpdateEntitySkillWrap(my, 3);
                }
            }
        }
    } else {
        my.flags[NOUPDATE] = 1;
    }

    // Initialization
    if (my.skill[1] == 0) {
        my.skill[1] = 1;
        my.fskill[0] = my.z;
        if (my.skill[5] != 0) {
            my.z = my.fskill[0] - 12;
        }
        my.scalex = 1.01;
        my.scaley = 1.01;
        my.scalez = 1.01;
        c.entityCreateWorldUITooltip(my);
    }

    // Right-click message
    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(475));
                }
            }
        }
    }

    // Gate movement
    if (my.skill[3] == 0) {
        if (my.z < my.fskill[0]) {
            my.vel_z += 0.25;
            my.z = @min(my.fskill[0], my.z + my.vel_z);
        } else {
            my.vel_z = 0;
        }
    } else {
        if (my.z > my.fskill[0] - 12) {
            my.z = @max(my.fskill[0] - 12, my.z - 0.25);
            my.skill[4] = if (my.skill[4] == 0) 1 else 0;
            if (my.skill[4] != 0) {
                my.x += 0.05;
                my.y += 0.05;
            } else {
                my.x -= 0.05;
                my.y -= 0.05;
            }
        } else {
            if (my.skill[4] != 0) {
                my.skill[4] = 0;
                my.x -= 0.05;
                my.y -= 0.05;
            }
        }
    }

    // Collision
    var somebodyinside = false;
    if (my.z > my.fskill[0] - 6 and my.flags[PASSABLE] != 0) {
        var node: ?*c.node_t = c.entityGetMapEntities().*.first;
        while (node) |n| {
            if (n.element) |elem| {
                const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                if (!c.entityIsValidColliderForGate(my, entity)) {
                    node = n.next;
                    continue;
                }
                if (c.entityInsideEntity(@as(?*Entity, @ptrCast(my)), @as(?*Entity, @ptrCast(entity)))) {
                    somebodyinside = true;
                    break;
                }
            }
            node = n.next;
        }
        if (!somebodyinside) {
            my.flags[PASSABLE] = 0;
        }
    } else if (my.z < my.fskill[0] - 9 and my.flags[PASSABLE] == 0) {
        my.flags[PASSABLE] = 1;
    }
}
