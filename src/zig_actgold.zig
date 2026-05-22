const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
    @cInclude("player.h");
    @cInclude("stat.h");
});

const Entity = c.Entity;

const CLIENT: c_int = 2;
const SERVER: c_int = 1;
const MAXPLAYERS: c_int = 4;
const TICKS_PER_SECOND: c_int = 50;
const MAPLAYERS: c_int = 3;

const MESSAGE_MISC: c_uint = 1 << 31;
const MESSAGE_INTERACTION: c_uint = 1 << 7;
const MESSAGE_INVENTORY: c_uint = 1 << 2;
const MESSAGE_HINT: c_uint = 1 << 9;

const COLOR_YELLOW: c_uint = 0xff00ffff;

const GOLD_SPRITE_BOUNCING: c.Sint32 = 1379;

export fn actGoldBag(my: *Entity) void {
    my.goldTelepathy.* = 0;
    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    if (my.flags[c.INVISIBLE] != 0 and my.goldSokoban.* == 1) {
        if (c.multiplayer != CLIENT) {
            var node: ?*c.node_t = c.entityGetMapEntities().*.first;
            while (node) |n| {
                if (n.element) |elem| {
                    const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                    if (c.entityIsBoulderSprite(entity)) {
                        return;
                    }
                }
                node = n.next;
            }
            my.flags[c.INVISIBLE] = 0;
            c.entityServerUpdateEntityFlag(my, c.INVISIBLE);
            if (c.strcmp(c.entityGetMapName(), "Sokoban") == 0) {
                var i: c_int = 0;
                while (i < MAXPLAYERS) : (i += 1) {
                    c.entitySteamAchievementClient(i, "BARONY_ACH_PUZZLE_MASTER");
                }
            }
        } else {
            return;
        }
    }

    my.goldAmbience.* -= 1;
    if (my.goldAmbience.* <= 0) {
        my.goldAmbience.* = TICKS_PER_SECOND * 30;
        c.entityPlaySoundEntityLocal(my, 149, 16);
    }

    {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            if (c.entityPlayerIsLocalPlayer(i)) {
                const playerEntity = c.entityGetPlayerEntity(i);
                if (playerEntity != null and c.entityIsBlind(playerEntity) and c.entityIsPlayerGnome(i)) {
                    my.goldTelepathy.* |= @as(c.Sint32, 1) << @as(u5, @intCast(i));
                }
            }
        }
    }

    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    if (c.entityGoldPickupForPlayer(my, i)) {
                        return;
                    }
                    return;
                }
            }
        }
    } else {
        my.flags[c.NOUPDATE] = 1;
    }

    var onground = false;
    const groundheight: c.real_t = if (my.sprite == GOLD_SPRITE_BOUNCING) 7.75 else 6.25;

    my.flags[c.BURNING] = 0;
    my.flags[c.NOCLIP_CREATURES] = 1;

    if (my.goldBouncing.* == 0) {
        if (my.z < groundheight) {
            my.vel_z += 0.04;
            my.z += my.vel_z;
            my.roll += 0.08;
        } else {
            if (my.x >= 0 and my.y >= 0 and my.x < @as(c.real_t, @floatFromInt(c.entityMapGetWidth() << 4)) and my.y < @as(c.real_t, @floatFromInt(c.entityGetMapHeight() << 4))) {
                const tileY = @as(c_int, @intFromFloat(my.y / 16.0));
                const tileX = @as(c_int, @intFromFloat(my.x / 16.0));
                const tileIndex = tileY * MAPLAYERS + tileX * MAPLAYERS * c.entityGetMapHeight();
                const tile = c.entityMapGetTile(tileIndex);
                if (tile != 0) {
                    onground = true;
                    my.vel_z *= -0.7;
                    if (my.vel_z > -0.35) {
                        my.roll = 0.0;
                        my.z = groundheight;
                        my.vel_z = 0.0;
                    } else {
                        my.z = groundheight - 0.0001;
                    }
                } else {
                    my.vel_z += 0.04;
                    my.z += my.vel_z;
                    my.roll += 0.08;
                }
            } else {
                my.vel_z += 0.04;
                my.z += my.vel_z;
                my.roll += 0.08;
            }
        }

        if (my.z > 128) {
            c.list_RemoveNode(my.mynode);
            return;
        }

        if (onground and
            my.z > groundheight - 0.0001 and my.z < groundheight + 0.0001 and
            c.fabs(my.vel_x) < 0.02 and c.fabs(my.vel_y) < 0.02)
        {
            my.goldBouncing.* = 1;
            return;
        }

        const result = c.clipMove(&my.x, &my.y, my.vel_x, my.vel_y, my);
        my.yaw += result * 0.05;
        if (result != @sqrt(my.vel_x * my.vel_x + my.vel_y * my.vel_y)) {
            if (c.hit.side == 0) {
                my.vel_x *= -0.5;
                my.vel_y *= -0.5;
            } else if (c.hit.side == c.HORIZONTAL) {
                my.vel_x *= -0.5;
            } else {
                my.vel_y *= -0.5;
            }
        }

        my.vel_x *= 0.925;
        my.vel_y *= 0.925;
    }
}
