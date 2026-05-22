const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("prng.h");
    @cInclude("game.h");
    @cInclude("player.h");
    @cInclude("collision.h");
    @cInclude("monster.h");
});

const Entity = c.Entity;

export fn actHeadstone(my: *Entity) void {
    const CLIENT: c_int = 2;
    const MAXPLAYERS = 4;
    const MESSAGE_INTERACTION: c_uint = 1 << 7;
    const TICKS_PER_SECOND = 50;

    // skill aliases:
    //   0 HEADSTONE_INIT
    //   1 HEADSTONE_GHOUL
    //   3 HEADSTONE_FIRED
    //   4 HEADSTONE_MESSAGE
    //   5 HEADSTONE_AMBIENCE
    //   6 HEADSTONE_WAS_INVISIBLE

    if (my.flags[c.INVISIBLE] != 0) {
        my.skill[6] = 1;
        if (c.multiplayer != CLIENT) {
            var goldbags: c_int = 0;
            var artifact = false;
            var node: ?*c.node_t = c.entityGetMapEntities().*.first;
            while (node) |n| {
                if (n.element) |elem| {
                    const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                    if (entity.sprite == 130) {
                        goldbags += 1;
                    }
                    if (entity.sprite == 508) {
                        artifact = true;
                    }
                }
                node = n.next;
            }
            if (goldbags >= 9 and artifact) {
                return;
            }
            my.flags[c.INVISIBLE] = 0;
            my.flags[c.PASSABLE] = 0;
            c.entityServerUpdateEntityFlag(my, c.INVISIBLE);
            c.entityServerUpdateEntityFlag(my, c.PASSABLE);
        } else {
            return;
        }
    }

    my.skill[5] -= 1;
    if (my.skill[5] <= 0) {
        my.skill[5] = TICKS_PER_SECOND * 30;
        c.entityPlaySoundEntityLocal(my, 149, 32);
    }

    if (c.multiplayer == CLIENT) {
        if (my.skill[0] == 0) {
            my.skill[0] = 1;
            c.entityCreateWorldUITooltip(my);
        }
        return;
    }

    if (my.skill[0] == 0) {
        my.skill[0] = 1;
        c.entityCreateWorldUITooltip(my);

        const rng: *c.BaronyRNG = if (my.entity_rng) |er| er else &c.local_rng;
        my.skill[4] = @as(c_int, @intCast(c.rng_rand(rng)));
        my.skill[1] = if (@rem(c.rng_rand(rng), @as(c_int, 4)) == 0) 1 else 0;
    }

    var shouldspawn = false;
    var triggeredPlayer: c_int = -1;

    if (c.multiplayer != CLIENT) {
        var i: c_int = 0;
        while (i < MAXPLAYERS) : (i += 1) {
            const idx = @as(usize, @intCast(i));
            if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
                if (c.inrange[idx]) {
                    c.entityHeadstoneCreateDialogue(i, my, my.skill[4]);
                    triggeredPlayer = i;

                    if (my.skill[1] != 0 and my.skill[3] == 0) {
                        shouldspawn = true;
                        const color: c_uint = 0xff0080ff;
                        _ = c.entityHeadstoneMessageColor(i, MESSAGE_INTERACTION, color, c.languageGet(502));
                    }
                }
            }
        }

        if (my.skill[6] != 0) {
            const somebodyinside = c.entityHeadstoneCheckInside(my);
            if (somebodyinside) {
                if (my.flags[c.PASSABLE] == 0) {
                    my.flags[c.PASSABLE] = 1;
                    c.entityServerUpdateEntityFlag(my, c.PASSABLE);
                }
            } else {
                if (my.flags[c.PASSABLE] != 0) {
                    my.flags[c.PASSABLE] = 0;
                    c.entityServerUpdateEntityFlag(my, c.PASSABLE);
                }
            }
        }
    }

    if (my.skill[28] == 2 or shouldspawn) {
        if (my.skill[3] == 0) {
            my.skill[3] = 1;

            const monster = c.entitySummonMonsterNoSmoke(
                @as(c_int, c.GHOUL),
                @as(c_long, @intFromFloat(my.x)),
                @as(c_long, @intFromFloat(my.y)),
            );
            if (monster) |m_raw| {
                const m = @as(*Entity, @ptrCast(@alignCast(m_raw)));
                const rng = if (my.entity_rng) |er| er else &c.local_rng;
                c.entitySeedEntityRNG(m, c.rng_getU32(rng));
                m.z = 13;
                if (c.currentlevel >= 15 or c.entityMapIsHauntedCastle()) {
                    c.entityHeadstoneGhoulEnslaved(m, triggeredPlayer);
                } else {
                    c.entityHeadstoneGhoulDefault(triggeredPlayer);
                }
            }
        }
    }
}
