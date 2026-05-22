const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("player.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
    @cInclude("light.h");
});

const Entity = c.Entity;

const PI: f64 = 3.14159265358979323846;
const CLIENT: c_int = 2;
const MAXPLAYERS: c_int = 4;
const MESSAGE_INTERACTION: c_uint = 1 << 7;

const CRYSTAL_HOVER_UP: c_int = 0;
const CRYSTAL_HOVER_UP_WAIT: c_int = 1;
const CRYSTAL_HOVER_DOWN: c_int = 2;
const CRYSTAL_HOVER_DOWN_WAIT: c_int = 3;

export fn actPowerCrystalBase(my: *Entity) void {
    if (my.flags[c.PASSABLE] != 0) {
        my.flags[c.PASSABLE] = 0;
    }
}

export fn actPowerCrystal(my: *Entity) void {
    actPowerCrystalImpl(my);
}

fn actPowerCrystalImpl(my: *Entity) void {
    const upper_z = my.crystalStartZ.* - 0.4;
    const lower_z = my.crystalStartZ.* + 0.4;
    const acceleration: f64 = 0.95;

    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    if (my.crystalInitialised.* == 0 and my.crystalSpellToActivate.* == 0) {
        if (my.z > my.crystalStartZ.*) {
            my.z -= my.vel_z * (1.0 / acceleration);
        } else {
            my.z = my.crystalStartZ.*;
            powerCrystalCreateElectricityNodes(my);
            my.crystalInitialised.* = 1;
        }
    }

    if (my.crystalInitialised.* != 0) {
        if (my.crystalHoverDirection.* == CRYSTAL_HOVER_UP) {
            my.z -= my.vel_z;

            if (my.z < upper_z) {
                my.z = upper_z;
                my.crystalHoverDirection.* = CRYSTAL_HOVER_UP_WAIT;
            }

            if (my.z < my.crystalStartZ.*) {
                my.vel_z = @max(my.vel_z * acceleration, my.crystalMinZVelocity.*);
            } else if (my.z > my.crystalStartZ.*) {
                my.vel_z = @min(my.vel_z * (1.0 / acceleration), my.crystalMaxZVelocity.*);
            }
        } else if (my.crystalHoverDirection.* == CRYSTAL_HOVER_UP_WAIT) {
            my.crystalHoverWaitTimer.* += 1;
            if (my.crystalHoverWaitTimer.* >= 1) {
                my.crystalHoverDirection.* = CRYSTAL_HOVER_DOWN;
                my.crystalHoverWaitTimer.* = 0;
            }
        } else if (my.crystalHoverDirection.* == CRYSTAL_HOVER_DOWN) {
            my.z += my.vel_z;

            if (my.z > lower_z) {
                my.z = lower_z;
                my.crystalHoverDirection.* = CRYSTAL_HOVER_DOWN_WAIT;
            }

            if (my.z < my.crystalStartZ.*) {
                my.vel_z = @min(my.vel_z * (1.0 / acceleration), my.crystalMaxZVelocity.*);
            } else if (my.z > my.crystalStartZ.*) {
                my.vel_z = @max(my.vel_z * acceleration, my.crystalMinZVelocity.*);
            }
        } else if (my.crystalHoverDirection.* == CRYSTAL_HOVER_DOWN_WAIT) {
            my.crystalHoverWaitTimer.* += 1;
            if (my.crystalHoverWaitTimer.* >= 1) {
                my.crystalHoverDirection.* = CRYSTAL_HOVER_UP;
                my.crystalHoverWaitTimer.* = 0;
            }
        }

        if (my.z <= my.crystalStartZ.* + my.crystalMaxZVelocity.* and my.z >= my.crystalStartZ.* - my.crystalMaxZVelocity.*) {
            my.vel_z = my.fskill[1];
        }

        _ = c.entitySpawnAmbientParticles(my, 80, 579, 10 + @rem(c.rng_rand(&c.local_rng), @as(c_int, 40)), 1.0, false);

        if (my.crystalTurning.* == 1) {
            if (my.crystalTurnReverse.* == 0) {
                my.yaw += my.crystalTurnVelocity.*;

                if (my.yaw >= @as(f64, @floatFromInt(my.crystalTurnStartDir.*)) * (PI / 2.0) + (PI / 2.0)) {
                    my.yaw = @as(f64, @floatFromInt(my.crystalTurnStartDir.*)) * (PI / 2.0) + (PI / 2.0);
                    my.crystalTurning.* = 0;

                    if (my.yaw >= 2.0 * PI) {
                        my.yaw = 0.0;
                    }
                    powerCrystalCreateElectricityNodes(my);
                }
            } else {
                my.yaw -= my.crystalTurnVelocity.*;

                if (my.yaw <= @as(f64, @floatFromInt(my.crystalTurnStartDir.*)) * (PI / 2.0) - (PI / 2.0)) {
                    my.yaw = @as(f64, @floatFromInt(my.crystalTurnStartDir.*)) * (PI / 2.0) - (PI / 2.0);
                    my.crystalTurning.* = 0;

                    if (my.yaw < 0.0) {
                        my.yaw += 2.0 * PI;
                    }
                    powerCrystalCreateElectricityNodes(my);
                }
            }
        }
    }

    if (c.multiplayer == CLIENT) {
        return;
    }

    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if ((c.client_selected[idx] == my or c.selectedEntity[idx] == my) and my.crystalTurning.* == 0) {
            if (c.inrange[idx]) {
                const interactEntity = c.entityGetPlayerInteractEntity(i);
                if (interactEntity != null and my.crystalInitialised.* != 0) {
                    c.entityPlaySoundEntity(my, 151, 128);
                    my.crystalTurning.* = 1;
                    my.crystalTurnStartDir.* = @as(c.Sint32, @intFromFloat(my.yaw / (PI / 2.0)));
                    c.serverUpdateEntitySkillWrap(my, 3);
                    c.serverUpdateEntitySkillWrap(my, 4);
                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(2356));
                } else if (my.crystalInitialised.* == 0) {
                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(2357));
                }
            }
        }
    }
}

export fn actPowerCrystalParticleIdle(my: *Entity) void {
    if (my.skill[0] < 0) {
        c.list_RemoveNode(my.mynode);
        return;
    } else {
        my.skill[0] -= 1;
        my.z += my.vel_z;
    }
}

fn powerCrystalCreateElectricityNodes(my: *Entity) void {
    var xtest: f64 = 0.0;
    var ytest: f64 = 0.0;

    if (my.crystalGeneratedElectricityNodes.* != 0) {
        my.circuit_status.* = 1; // CIRCUIT_OFF
        c.entityUpdateCircuitNeighbors(my);

        if (c.multiplayer != CLIENT) {
            var node: ?*c.node_t = my.children.first;
            var nextnode: ?*c.node_t = undefined;
            while (node) |n| {
                nextnode = n.next;
                if (n.element) |elem| {
                    const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                    c.entityRemoveLightNode(entity);
                    entity.light = null;
                    c.list_RemoveNode(entity.mynode);
                }
                c.list_RemoveNode(n);
                node = nextnode;
            }
        }
    }

    var i: c_int = 1;
    while (i <= my.crystalNumElectricityNodes.*) : (i += 1) {
        const entity_raw = c.newEntity(-1, 0, c.entityGetMapEntities(), null);
        if (entity_raw == null) break;
        const entity = @as(*Entity, @ptrCast(@alignCast(entity_raw.?)));

        xtest = my.x + @as(f64, @floatFromInt(i)) * 16.0 *
            (@as(f64, @floatFromInt(@intFromBool(my.yaw == 0.0))) - @as(f64, @floatFromInt(@intFromBool(my.yaw == PI))));
        ytest = my.y + @as(f64, @floatFromInt(i)) * 16.0 *
            (@as(f64, @floatFromInt(@intFromBool(my.yaw == PI / 2.0))) - @as(f64, @floatFromInt(@intFromBool(my.yaw == 3.0 * PI / 2.0))));

        const mapX: c_int = @as(c_int, @intFromFloat(xtest)) >> 4;
        const mapY: c_int = @as(c_int, @intFromFloat(ytest)) >> 4;
        if (mapX < 0 or mapX >= @as(c_int, @intCast(c.entityMapGetWidth())) or
            mapY < 0 or mapY >= @as(c_int, @intCast(c.entityMapGetHeight())))
        {
            break;
        }

        entity.x = xtest;
        entity.y = ytest;
        entity.z = 5.0;
        entity.behavior = @ptrCast(&c.entityActCircuit);
        entity.flags[c.PASSABLE] = 1;
        entity.flags[c.INVISIBLE] = 1;
        entity.flags[c.NOUPDATE] = 1;
        entity.circuit_status.* = 1; // CIRCUIT_OFF

        const node = c.list_AddNodeLast(&my.children) orelse unreachable;
        node.*.element = @as(?*anyopaque, @ptrCast(entity));
        node.*.deconstructor = &c.emptyDeconstructor;
        node.*.size = @sizeOf(*Entity);

        c.tileEntityListAddEntity(entity);

        my.crystalGeneratedElectricityNodes.* = 1;
    }

    my.circuit_status.* = 2; // CIRCUIT_ON
    c.entityUpdateCircuitNeighbors(my);
}
