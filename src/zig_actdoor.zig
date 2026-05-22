const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("player.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
});

const Entity = c.Entity;

const CLIENT: c_int = 2;
const SERVER: c_int = 1;
const MAXPLAYERS: c_int = 4;
const MESSAGE_INTERACTION: c_uint = 1 << 7;
const MESSAGE_COMBAT: c_uint = 1 << 0;
const MESSAGE_COMBAT_BASIC: c_uint = 1 << 9;
const PASSABLE: c_int = 12;
const BURNABLE: c_int = 10;
const BURNING: c_int = 9;
const INVISIBLE: c_int = 2;
const CIRCUIT_ON: c_int = 2;
const CIRCUIT_OFF: c_int = 1;
const CPDM_DOOR_OPENED: c_int = 68;
const CPDM_DOOR_CLOSED: c_int = 114;
const CPDM_DOOR_BROKEN: c_int = 67;
const PI: f64 = 3.14159265358979323846;

fn doorSwing(yaw: *f64, doorStartAng: f64, doorStatus: c_int) void {
    if (doorStatus == 0) {
        if (yaw.* > doorStartAng) {
            yaw.* = @max(doorStartAng, yaw.* - 0.15);
        } else if (yaw.* < doorStartAng) {
            yaw.* = @min(doorStartAng, yaw.* + 0.15);
        }
    } else if (doorStatus == 1) {
        const target = doorStartAng + PI / 2.0;
        if (yaw.* > target) {
            yaw.* = @max(target, yaw.* - 0.15);
        } else if (yaw.* < target) {
            yaw.* = @min(target, yaw.* + 0.15);
        }
    } else if (doorStatus == 2) {
        const target = doorStartAng - PI / 2.0;
        if (yaw.* > target) {
            yaw.* = @max(target, yaw.* - 0.15);
        } else if (yaw.* < target) {
            yaw.* = @min(target, yaw.* + 0.15);
        }
    }
}

fn doorCollisionCheck(my: *Entity) bool {
    const oldmyx = my.x;
    const oldmyy = my.y;
    my.x = @as(f64, @floatFromInt(@as(c_int, @intFromFloat(my.x)) >> 4)) * 16.0 + 8.0;
    my.y = @as(f64, @floatFromInt(@as(c_int, @intFromFloat(my.y)) >> 4)) * 16.0 + 8.0;

    var somebodyinside = false;
    var node: ?*c.node_t = c.entityGetMapEntities().*.first;
    while (node) |n| : (node = n.next) {
        if (n.element) |elem| {
            const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
            if (entity == my) continue;
            if (entity.flags[PASSABLE] != 0 and !c.entityIsActDeathGhost(entity)) continue;
            if (c.entityIsActDoorFrame(entity)) continue;

            var insideEntity = false;
            if (c.entityIsActDoor(entity) or c.entityIsActIronDoor(entity)) {
                const oldx = entity.x;
                const oldy = entity.y;
                entity.x = @as(f64, @floatFromInt(@as(c_int, @intFromFloat(entity.x)) >> 4)) * 16.0 + 8.0;
                entity.y = @as(f64, @floatFromInt(@as(c_int, @intFromFloat(entity.y)) >> 4)) * 16.0 + 8.0;
                insideEntity = c.entityInsideEntity(@as(?*Entity, @ptrCast(my)), @as(?*Entity, @ptrCast(entity)));
                entity.x = oldx;
                entity.y = oldy;
            } else {
                insideEntity = c.entityInsideEntity(@as(?*Entity, @ptrCast(my)), @as(?*Entity, @ptrCast(entity)));
            }

            if (insideEntity) {
                somebodyinside = true;
                break;
            }
        }
    }

    my.x = oldmyx;
    my.y = oldmyy;
    return somebodyinside;
}

fn spawnGibs(my: *Entity) void {
    var c_idx: c_int = 0;
    while (c_idx < 5) : (c_idx += 1) {
        const raw = c.entitySpawnGibReturn(my);
        if (raw == null) continue;
        const entity = @as(*Entity, @ptrCast(@alignCast(raw)));
        entity.flags[INVISIBLE] = 0;
        entity.sprite = 187;
        entity.x = @floor(my.x / 16.0) * 16.0 + 8.0;
        entity.y = @floor(my.y / 16.0) * 16.0 + 8.0;
        entity.z = 0.0;
        entity.z += @as(f64, @floatFromInt(-7 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 14))));
        if (my.doorDir.* == 0) {
            entity.y += @as(f64, @floatFromInt(-4 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 8))));
            entity.yaw = if (my.doorSmacked.* != 0) PI else 0.0;
        } else {
            entity.x += @as(f64, @floatFromInt(-4 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 8))));
            entity.yaw = if (my.doorSmacked.* != 0) PI / 2.0 else 3.0 * PI / 2.0;
        }
        entity.pitch = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 360)))) * PI / 180.0;
        entity.roll = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 360)))) * PI / 180.0;
        entity.vel_x = @cos(entity.yaw) * (1.2 + @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 10)))) / 50.0);
        entity.vel_y = @sin(entity.yaw) * (1.2 + @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 10)))) / 50.0);
        entity.vel_z = -0.25;
        entity.fskill[3] = 0.04;
        c.entityServerSpawnGibForClient(entity);
    }
}

fn doorInitCommon(my: *Entity) void {
    my.doorInit.* = 1;
    my.doorStartAng.* = my.yaw;
    my.doorOldStatus.* = my.doorStatus.*;
    my.scalex = 1.01;
    my.scaley = 1.01;
    my.scalez = 1.01;
    c.entityCreateWorldUITooltip(my);
}

fn doorPlayerInteract(my: *Entity, isIron: bool) void {
    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
            if (c.inrange[idx]) {
                const rawPlayer = c.entityGetPlayerInteractEntity(i);
                if (rawPlayer == null) continue;
                const pe = @as(*Entity, @ptrCast(@alignCast(rawPlayer)));
                if (my.doorLocked.* == 0) {
                    if (my.doorDir.* == 0 and my.doorStatus.* == 0) {
                        my.doorStatus.* = 1 + @as(c_int, @intFromBool(pe.x > my.x));
                        c.entityPlaySoundEntity(my, 21, 96);
                        if (!isIron) {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(464));
                            c.entityEventUpdateWorld(i, CPDM_DOOR_OPENED, "door", 1);
                        } else {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6404));
                            c.entityEventUpdateWorld(i, CPDM_DOOR_OPENED, "iron door", 1);
                        }
                    } else if (my.doorDir.* != 0 and my.doorStatus.* == 0) {
                        my.doorStatus.* = 1 + @as(c_int, @intFromBool(pe.y < my.y));
                        c.entityPlaySoundEntity(my, 21, 96);
                        if (!isIron) {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(464));
                            c.entityEventUpdateWorld(i, CPDM_DOOR_OPENED, "door", 1);
                        } else {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6404));
                            c.entityEventUpdateWorld(i, CPDM_DOOR_OPENED, "iron door", 1);
                        }
                    } else {
                        my.doorStatus.* = 0;
                        c.entityPlaySoundEntity(my, 22, 96);
                        if (!isIron) {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(465));
                            c.entityEventUpdateWorld(i, CPDM_DOOR_CLOSED, "door", 1);
                        } else {
                            if (my.doorUnlockWhenPowered.* == 1 and my.circuit_status.* == CIRCUIT_OFF) {
                                my.doorLocked.* = 1;
                                c.entityPlaySoundEntity(my, 57, 64);
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6406));
                            } else {
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6405));
                            }
                            c.entityEventUpdateWorld(i, CPDM_DOOR_CLOSED, "iron door", 1);
                        }
                    }
                } else {
                    if (!isIron) {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(466));
                    } else {
                        _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(6407));
                    }
                    c.entityPlaySoundEntity(my, 152, 64);
                }
            }
        }
    }
}

export fn actDoor(my: *Entity) void {
    if (my.doorInit.* == 0) {
        doorInitCommon(my);

        const rng = if (my.entity_rng != null) my.entity_rng else &c.local_rng;
        my.doorHealth.* = 15 + @rem(c.rng_rand(rng), @as(c_int, 5));
        my.doorMaxHealth.* = my.doorHealth.*;
        my.doorOldHealth.* = my.doorHealth.*;
        my.doorPreventLockpickExploit.* = 1;
        my.doorLockpickHealth.* = 20;

        if (my.doorForceLockedUnlocked.* == 2) {
            my.doorLocked.* = 0;
        } else if (@rem(c.rng_rand(rng), @as(c_int, 20)) == 0 or
            (c.strncmp(c.entityGetMapName(), "The Great Castle", 16) == 0 and @rem(c.rng_rand(rng), @as(c_int, 2)) == 0) or
            my.doorForceLockedUnlocked.* == 1)
        {
            my.doorLocked.* = 1;
            my.doorPreventLockpickExploit.* = 0;
        }
        my.flags[BURNABLE] = 1;
        return;
    }

    if (c.multiplayer != CLIENT) {
        if (my.flags[BURNING] != 0) {
            if (@rem(c.ticks, @as(c_uint, 30)) == 0) {
                my.doorHealth.* -= 1;
            }
        }

        my.doorOldHealth.* = my.doorHealth.*;
        if (my.doorHealth.* <= 0) {
            spawnGibs(my);
            c.entityPlaySoundEntity(my, 177, 64);
            c.list_RemoveNode(my.mynode);
            return;
        }

        doorPlayerInteract(my, false);
    }

    doorSwing(&my.yaw, my.doorStartAng.*, my.doorStatus.*);

    if (my.yaw == my.doorStartAng.* and my.flags[PASSABLE] != 0) {
        if (!doorCollisionCheck(my)) {
            my.focaly = 0;
            if (my.doorStartAng.* == 0) {
                my.y -= 5;
            } else {
                my.x -= 5;
            }
            my.flags[PASSABLE] = 0;
        }
    } else if (my.yaw != my.doorStartAng.* and my.flags[PASSABLE] == 0) {
        my.focaly = -5;
        if (my.doorStartAng.* == 0) {
            my.y += 5;
        } else {
            my.x += 5;
        }
        my.flags[PASSABLE] = 1;
    }

    if (c.multiplayer == SERVER) {
        if (my.doorOldStatus.* != my.doorStatus.*) {
            my.doorOldStatus.* = my.doorStatus.*;
            c.serverUpdateEntitySkillWrap(my, 3);
        }
    }
}

export fn actDoorFrame(my: *Entity) void {
    if (my.flags[INVISIBLE] == 0) {
        my.flags[PASSABLE] = 1;
    }
}

export fn actIronDoor(my: *Entity) void {
    if (my.doorInit.* == 0) {
        doorInitCommon(my);

        my.doorHealth.* = 100;
        my.doorMaxHealth.* = my.doorHealth.*;
        my.doorOldHealth.* = my.doorHealth.*;
        my.doorPreventLockpickExploit.* = 1;
        my.doorLockpickHealth.* = 50;

        if (my.doorForceLockedUnlocked.* == 2) {
            my.doorLocked.* = 0;
        } else if (my.doorForceLockedUnlocked.* <= 1) {
            my.doorLocked.* = 1;
            my.doorPreventLockpickExploit.* = 0;
        }
        my.flags[BURNABLE] = 0;
        return;
    }

    if (c.multiplayer != CLIENT) {
        my.doorOldHealth.* = my.doorHealth.*;

        if (my.doorHealth.* <= 0) {
            c.entityPlaySoundEntity(my, 76, 64);
            c.list_RemoveNode(my.mynode);
            return;
        }

        if (my.doorUnlockWhenPowered.* == 1) {
            if (my.circuit_status.* == CIRCUIT_ON) {
                if (my.doorLocked.* == 1) {
                    my.doorLocked.* = 0;
                    c.entityPlaySoundEntity(my, 91, 64);
                }
            } else if (my.circuit_status.* == CIRCUIT_OFF) {
                if (my.doorStatus.* == 0 and my.doorLocked.* == 0) {
                    my.doorLocked.* = 1;
                    c.entityPlaySoundEntity(my, 57, 64);
                }
            }
        }

        doorPlayerInteract(my, true);
    }

    doorSwing(&my.yaw, my.doorStartAng.*, my.doorStatus.*);

    if (my.yaw == my.doorStartAng.* and my.flags[PASSABLE] != 0) {
        if (!doorCollisionCheck(my)) {
            my.focaly = 0;
            if (my.doorStartAng.* == 0) {
                my.y -= 5;
            } else {
                my.x -= 5;
            }
            my.flags[PASSABLE] = 0;
        }
    } else if (my.yaw != my.doorStartAng.* and my.flags[PASSABLE] == 0) {
        my.focaly = -5;
        if (my.doorStartAng.* == 0) {
            my.y += 5;
        } else {
            my.x += 5;
        }
        my.flags[PASSABLE] = 1;
    }

    if (c.multiplayer == SERVER) {
        if (my.doorOldStatus.* != my.doorStatus.*) {
            my.doorOldStatus.* = my.doorStatus.*;
            c.serverUpdateEntitySkillWrap(my, 3);
        }
    }
}
