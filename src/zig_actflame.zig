const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
});

const Entity = c.Entity;

const PI: f64 = 3.14159265358979323846;
const TICKS_PER_SECOND: c_int = 50;
const CLIENT: c_int = 2;
const SPRITE_FLAME: c.Sint32 = 13;
const SPRITE_CRYSTALFLAME: c.Sint32 = 96;

export fn actFlame(my: *Entity) void {
    if (my.skill[0] > 0) {
        my.skill[0] -= 1;
        if (my.skill[0] <= 0) {
            c.list_RemoveNode(my.mynode);
            return;
        }
    }

    if (c.flickerLights == false and
        (my.sprite == SPRITE_FLAME or my.sprite == SPRITE_CRYSTALFLAME))
    {
        my.fskill[0] += PI / @as(f64, TICKS_PER_SECOND) * 2.0;
        if (my.fskill[0] > PI * 2.0) {
            my.fskill[0] -= PI * 2.0;
        }
        my.vel_z = -@sin(my.fskill[0]) * 0.02;

        const parent_raw = c.entityUidToEntity(@as(c.Sint32, @intCast(my.parent)));
        if (parent_raw) |pr| {
            const p = @as(*Entity, @ptrCast(@alignCast(pr)));
            my.x += p.x - my.fskill[1];
            my.y += p.y - my.fskill[2];
            my.z += p.z - my.fskill[3];
            my.fskill[1] = p.x;
            my.fskill[2] = p.y;
            my.fskill[3] = p.z;
            my.flags[c.INVISIBLE] = p.flags[c.INVISIBLE];
        }
    }

    my.x += my.vel_x;
    my.y += my.vel_y;
    my.z += my.vel_z;
}

export fn spawnFlameSprites(parentent: ?*Entity, sprite: c.Sint32) ?*Entity {
    if (parentent == null) return null;

    const fx_raw = c.entityCreateParticleFlameOrbit(parentent, sprite);
    if (fx_raw) |fr| {
        const e = @as(*Entity, @ptrCast(@alignCast(fr)));
        e.flags[c.SPRITE] = 1;
        e.flags[c.INVISIBLE] = 1;
        e.x = parentent.?.x;
        e.y = parentent.?.y;
        e.z = parentent.?.z;
        e.scalex = 1.0;
        e.scaley = e.scalex;
        e.scalez = e.scalex;
        e.fskill[0] = e.x;
        e.fskill[1] = e.y;
        e.vel_z = -0.25;
        e.skill[8] = 0; // actmagicOrbitDist
        e.fskill[2] = parentent.?.yaw + @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 8)))) * PI / 4.0;
        e.yaw = e.fskill[2];
        e.skill[33] = 1; // actmagicNoLight
        return e;
    }
    return null;
}

export fn spawnFlame(parentent: ?*Entity, sprite: c.Sint32) ?*Entity {
    if (parentent == null) return null;

    if (c.entitySpawnFlameVismapCheck(parentent) == false) return null;

    const entity_raw = c.newEntity(sprite, 1, c.entityGetMapEntities(), null);
    if (entity_raw == null) return null;
    const e = @as(*Entity, @ptrCast(@alignCast(entity_raw.?)));

    if (c.intro) {
        c.entitySetUID(e, 0);
    }
    e.parent = parentent.?.uid;
    e.x = parentent.?.x;
    e.y = parentent.?.y;
    e.z = parentent.?.z;
    e.fskill[1] = parentent.?.x;
    e.fskill[2] = parentent.?.y;
    e.fskill[3] = parentent.?.z;
    e.sizex = 6;
    e.sizey = 6;
    e.yaw = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 360)))) * PI / 180.0;
    e.pitch = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 360)))) * PI / 180.0;
    e.roll = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 360)))) * PI / 180.0;
    const vel_val = @as(f64, @floatFromInt(@rem(c.rng_rand(&c.local_rng), @as(c_int, 10)))) / 10.0;
    if (c.flickerLights) {
        e.skill[0] = 5; // life-span
        e.vel_x = vel_val * @cos(e.yaw) * 0.1;
        e.vel_y = vel_val * @sin(e.yaw) * 0.1;
        e.vel_z = -0.25;
    } else {
        e.skill[0] = TICKS_PER_SECOND + 1;
        e.vel_x = 0.0;
        e.vel_y = 0.0;
        e.vel_z = 0.0;
        e.z -= 0.5;
    }
    e.flags[c.NOUPDATE] = 1;
    e.flags[c.PASSABLE] = 1;
    e.flags[c.SPRITE] = 1;
    e.flags[c.UNCLICKABLE] = 1;
    const bonus = c.entityGetFlameLightBonus();
    e.lightBonus = c.vec4_t{ .x = bonus, .y = bonus, .z = bonus, .w = 0.0 };
    e.ditheringDisabled = true;
    e.behavior = @ptrCast(&actFlame);
    if (c.multiplayer != CLIENT) {
        c.entity_uids -= 1;
    }
    c.entitySetUID(e, @as(c.Uint32, @bitCast(@as(c_int, -3))));

    return e;
}
