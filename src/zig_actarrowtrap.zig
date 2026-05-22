const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("collision.h");
    @cInclude("game.h");
    @cInclude("prng.h");
    @cInclude("items.h");
});

const std = @import("std");
const Entity = c.Entity;
const PI = std.math.pi;

const CLIENT: c_int = 2;
const SERVER: c_int = 1;

export fn actArrowTrap(my: *Entity) void {
    const checkx = @as(c_int, @intFromFloat(my.x)) >> 4;
    const checky = @as(c_int, @intFromFloat(my.y)) >> 4;
    const mapH = @as(c_int, @intCast(c.entityMapGetHeight()));
    const mapW = @as(c_int, @intCast(c.entityMapGetWidth()));
    const tileIdx = c.OBSTACLELAYER + checky * c.MAPLAYERS + checkx * c.MAPLAYERS * mapH;
    if (c.entityMapGetTile(@intCast(tileIdx)) == 0) {
        c.list_RemoveNode(my.mynode);
        return;
    }
    if (my.skill[0] >= 10) {
        c.list_RemoveNode(my.mynode);
        return;
    }
    if (my.skill[4] > 0) {
        if (c.multiplayer != CLIENT) {
            const qty: c_short = @intCast(2 + (5 - @divTrunc(my.skill[0], 2)));
            c.entityEventUpdateWorldArrowPilfered(my.skill[4] - 1, @as(c_int, @intCast(qty)));
            const item = c.entityNewItemWrap(my.skill[1], c.SERVICABLE, 0, qty, c.ITEM_GENERATED_QUIVER_APPEARANCE, false);
            const dropped = c.entityDropItemMonsterWrapCount(item, my, null, qty);
            const tileX = @divFloor(@as(c_int, @intFromFloat(my.x)), 16);
            const tileY = @divFloor(@as(c_int, @intFromFloat(my.y)), 16);
            var freeCount: c_int = 0;
            var ftx: [4]c_int = undefined;
            var fty: [4]c_int = undefined;
            inline for (.{ 1, -1, 0, 0 }, .{ 0, 0, 1, -1 }) |ox, oy| {
                const cx = tileX + ox;
                const cy = tileY + oy;
                if (cx >= 0 and cx < mapW and cy >= 0 and cy < mapH) {
                    const idx = c.OBSTACLELAYER + cy * c.MAPLAYERS + cx * c.MAPLAYERS * mapH;
                    if (c.entityMapGetTile(@intCast(idx)) == 0) {
                        ftx[@intCast(freeCount)] = cx;
                        fty[@intCast(freeCount)] = cy;
                        freeCount += 1;
                    }
                }
            }
            if (freeCount > 0) {
                const chosen = @rem(c.rng_rand(&c.local_rng), freeCount);
                const cx = ftx[@intCast(chosen)];
                const cy = fty[@intCast(chosen)];
                dropped.*.x += @as(f64, @floatFromInt(cx - tileX)) * 8.0;
                dropped.*.y += @as(f64, @floatFromInt(cy - tileY)) * 8.0;
                dropped.*.vel_x = @as(f64, @floatFromInt(cx - tileX));
                if (@abs(dropped.*.vel_x) > 0.01) {
                    dropped.*.vel_x *= @as(f64, @floatFromInt(5 + @rem(c.rng_rand(&c.local_rng), 11))) / 10.0;
                }
                dropped.*.vel_y = @as(f64, @floatFromInt(cy - tileY));
                if (@abs(dropped.*.vel_y) > 0.01) {
                    dropped.*.vel_y *= @as(f64, @floatFromInt(5 + @rem(c.rng_rand(&c.local_rng), 11))) / 10.0;
                }
            }
        }
        c.list_RemoveNode(my.mynode);
        return;
    }
    if (my.actTrapSabotaged.* == 0) {
        if (my.skill[6] == 0) {
            my.skill[6] -= 1;
            c.entityStopSound(my);
            c.entityPlaySoundEntityLocal(my, 149, 64);
        }
        if (my.entity_sound != null) {
            _ = my.entity_sound;
        }
    } else {
        c.entityStopSound(my);
    }
    if (c.multiplayer == CLIENT) {
        (@as(*[24]u8, @ptrCast(&my.flags)))[@as(usize, @intCast(c.NOUPDATE))] = @as(u8, 1);
        return;
    }
    if (my.skill[28] == 0) {
        return;
    }
    var targetToAutoHit: ?*Entity = null;
    if ((my.skill[28] == 2 or my.skill[4] == -1) and my.actTrapSabotaged.* == 0) {
        if (@rem(my.skill[0], 2) == 1) {
            if (my.skill[1] == c.QUIVER_LIGHTWEIGHT or c.entityChallengeRunStrongTrapsActive()) {
                if (my.skill[3] > 0) {
                    my.skill[3] -= 1;
                    if (my.skill[3] == 0) {
                        my.skill[0] += 1;
                    }
                }
            }
        }
        if (my.skill[4] == -1) {
            my.skill[4] = 0;
            if (@rem(my.skill[0], 2) == 1) {
                my.skill[0] += 1;
            }
            my.skill[3] = 0;
            var node: ?*c.node_t = c.entityGetMapCreatures().*.first;
            while (node) |n| {
                if (n.element) |elem| {
                    const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                    if (c.entityIsActPlayer(entity) and c.entityDist(my, entity) < c.TOUCHRANGE) {
                        targetToAutoHit = entity;
                        break;
                    }
                }
                node = n.next;
            }
        }
        if (@rem(my.skill[0], 2) == 0 and my.skill[3] <= 0) {
            my.skill[0] += 1;
            my.skill[3] = 5;
            var ci: c_int = 0;
            while (ci < 4) : (ci += 1) {
                var dx: c_int = 0;
                var dy: c_int = 0;
                switch (ci) {
                    0 => { dx = 12; dy = 0; },
                    1 => { dx = 0; dy = 12; },
                    2 => { dx = -12; dy = 0; },
                    3 => { dx = 0; dy = -12; },
                    else => {},
                }
                const ccheckx = @divFloor(@as(c_int, @intFromFloat(my.x)) + dx, 16);
                const cchecky = @divFloor(@as(c_int, @intFromFloat(my.y)) + dy, 16);
                if (!(ccheckx >= 0 and ccheckx < mapW and cchecky >= 0 and cchecky < mapH)) {
                    continue;
                }
                const cindex = cchecky * c.MAPLAYERS + ccheckx * c.MAPLAYERS * mapH;
                if (c.entityMapGetTile(@intCast(c.OBSTACLELAYER + cindex)) == 0) {
                    const arrow = c.entityNewEntity(166, 1, c.entityGetMapEntities(), null);
                    c.entityPlaySoundEntity(my, @intCast(239 + @rem(c.rng_rand(&c.local_rng), 3)), 96);
                    c.entitySetBehaviorArrow(arrow);
                    arrow.*.parent = my.uid;
                    arrow.*.x = my.x + @as(f64, @floatFromInt(dx));
                    arrow.*.y = my.y + @as(f64, @floatFromInt(dy));
                    arrow.*.z = my.z;
                    arrow.*.yaw = @as(f64, @floatFromInt(ci)) * (PI / 2.0);
                    arrow.*.sizex = 1;
                    arrow.*.sizey = 1;
                    (@as(*[24]u8, @ptrCast(&arrow.*.flags)))[@as(usize, @intCast(c.UPDATENEEDED))] = @as(u8, 1);
                    (@as(*[24]u8, @ptrCast(&arrow.*.flags)))[@as(usize, @intCast(c.PASSABLE))] = @as(u8, 1);
                    arrow.*.arrowPower.* = 17;
                    if (c.currentlevel >= 10) {
                        arrow.*.arrowPower.* += c.currentlevel - 10;
                    }
                    const stronger = c.entityChallengeRunStrongTrapsActive();
                    if (my.skill[1] == c.QUIVER_SILVER) {
                        arrow.*.sprite = 924;
                        if (stronger) my.skill[3] = 50;
                    } else if (my.skill[1] == c.QUIVER_PIERCE) {
                        arrow.*.arrowArmorPierce.* = 2;
                        arrow.*.sprite = 925;
                        if (stronger) my.skill[3] = 50;
                    } else if (my.skill[1] == c.QUIVER_LIGHTWEIGHT) {
                        arrow.*.sprite = 926;
                        my.skill[3] = 50;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_FIRE) {
                        arrow.*.sprite = 927;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_KNOCKBACK) {
                        arrow.*.sprite = 928;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_CRYSTAL) {
                        arrow.*.sprite = 929;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_HUNTING) {
                        arrow.*.sprite = 930;
                        arrow.*.arrowPoisonTime.* = 360;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_BONE) {
                        arrow.*.sprite = 2304;
                        if (stronger) my.skill[3] = 25;
                    } else if (my.skill[1] == c.QUIVER_BLACKIRON) {
                        arrow.*.sprite = 2305;
                        if (stronger) my.skill[3] = 25;
                    }
                    arrow.*.arrowQuiverType.* = my.skill[1];
                    arrow.*.arrowSpeed.* = 7.0;
                    arrow.*.vel_x = @cos(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                    arrow.*.vel_y = @sin(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                    if (c.multiplayer == SERVER) {
                        var val: c_uint = @as(c_uint, 1) << 31;
                        val |= @as(c_uint, 17);
                        val |= (@as(c_uint, @intCast(c.TOOL_SENTRYBOT)) & 0xFFF) << 8;
                        val |= @as(c_uint, 8) << 20;
                        (@as(*[60]c_int, @ptrCast(&arrow.*.skill)))[2] = @as(c_int, @bitCast(val));
                        arrow.*.arrowShotByWeapon.* = @intCast(c.TOOL_SENTRYBOT);
                    }
                    if (targetToAutoHit) |target| {
                        if (@rem(c.rng_rand(&c.local_rng), 2) == 0) {
                            const tangent = std.math.atan2(arrow.*.y - target.y, arrow.*.x - target.x);
                            arrow.*.yaw = tangent + PI;
                            arrow.*.vel_x = @cos(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                            arrow.*.vel_y = @sin(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                            targetToAutoHit = null;
                        } else if (@rem(c.rng_rand(&c.local_rng), 2) == 0) {
                            const offset = PI / 12.0;
                            const spread = 0.1 * @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), 11))) * (PI / 6.0);
                            arrow.*.yaw = arrow.*.yaw - offset + spread;
                            arrow.*.vel_x = @cos(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                            arrow.*.vel_y = @sin(arrow.*.yaw) * arrow.*.arrowSpeed.*;
                            targetToAutoHit = null;
                        }
                    }
                }
            }
        }
    } else {
        if (my.skill[3] > 0) {
            my.skill[3] -= 1;
        } else if (@rem(my.skill[0], 2) == 1) {
            my.skill[0] += 1;
        }
    }
}
