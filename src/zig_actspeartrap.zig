const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("game.h");
    @cInclude("collision.h");
    @cInclude("prng.h");
    @cInclude("monster.h");
    @cInclude("player.h");
});

const Entity = c.Entity;

const CLIENT: c_int = 2;
const TICKS_PER_SECOND: c_int = 50;
const MAXPLAYERS: c_int = 4;
const MESSAGE_STATUS: c_uint = 1 << 1;
const MESSAGE_COMBAT: c_uint = 1 << 0;

fn makeColorRGB(r: u8, g: u8, b: u8) u32 {
    return 0xff000000 | (@as(u32, b) << 16) | (@as(u32, g) << 8) | @as(u32, r);
}

export fn actSpearTrap(my: *Entity) void {
    // ambient noises
    if (my.actTrapSabotaged == 0) {
        if (my.skill[1] == 0) {
            my.skill[1] -= 1;
            c.entityStopSound(my);
            c.entityPlaySoundEntityLocal(my, 149, 64);
        }
        if (my.entity_sound != null) {
            _ = my.entity_sound;
            // NOTE: FMOD channel isPlaying check kept in C++ bridge if needed
        }
    } else {
        c.entityStopSound(my);
    }

    if (c.multiplayer != CLIENT) {
        if (my.skill[28] == 0) return;

        if (my.skill[28] == 2 and my.actTrapSabotaged == 0) {
            // shoot out the spears
            if (my.skill[3] == 0) {
                my.skill[3] = 1;
                my.skill[4] = 0;
                c.entityPlaySoundEntity(my, 82, 64);
                c.serverUpdateEntitySkillWrap(my, 3);
                c.serverUpdateEntitySkillWrap(my, 4);
            }
        } else {
            // retract the spears
            if (my.skill[3] != 0) {
                my.skill[3] = 0;
                if (my.skill[4] <= 60) {
                    c.entityPlaySoundEntity(my, 82, 64);
                }
                my.skill[4] = 0;
                c.serverUpdateEntitySkillWrap(my, 3);
                c.serverUpdateEntitySkillWrap(my, 4);
            }
        }
    } else {
        my.flags[c.NOUPDATE] = 1;
    }

    if (my.skill[0] == 0) {
        my.skill[0] = 1;
        my.fskill[0] = my.z;
    }

    if (my.skill[3] == 0 or my.skill[4] > 60) {
        // retract spears
        if (my.z < my.fskill[0]) {
            my.vel_z += 0.25;
            my.z = @min(my.fskill[0], my.z + my.vel_z);
        } else {
            my.vel_z = 0;
        }
    } else {
        // shoot out spears
        my.z = @max(my.fskill[0] - 20, my.z - 4);
        if (c.multiplayer != CLIENT) {
            my.skill[4] += 1;
            if (my.skill[4] > 60) {
                c.entityPlaySoundEntity(my, 82, 64);
                c.serverUpdateEntitySkillWrap(my, 4);
            } else if (my.skill[4] == 1) {
                // check for entities in the spear path
                var node: ?*c.node_t = c.entityGetMapCreatures().*.first;
                while (node) |n| {
                    if (n.element) |elem| {
                        const entity = @as(*Entity, @ptrCast(@alignCast(elem)));
                        if (c.entityIsActPlayer(entity) or c.entityIsActMonster(entity)) {
                            const stats_opt = c.entityGetStats(entity);
                            if (stats_opt) |st_ptr| {
                                const st = @as(*c.Stat, @ptrCast(@alignCast(st_ptr)));
                                if (st.type == c.DUCK_SMALL and c.entityInsideEntity(@as(?*Entity, @ptrCast(my)), @as(?*Entity, @ptrCast(entity)))) {
                                    if (entity.monsterAttack == 0) {
                                        const pose: c_int = if (@mod(c.rng_rand(&c.local_rng), @as(c_int, 2)) == 0) c.MONSTER_POSE_MELEE_WINDUP2 else c.MONSTER_POSE_MELEE_WINDUP3;
                                        c.entityAttack(entity, pose, 0, null);
                                    }
                                }
                                if (entity.flags[c.PASSABLE] == 0 and c.entityInsideEntity(@as(?*Entity, @ptrCast(my)), @as(?*Entity, @ptrCast(entity)))) {
                                    // do damage!
                                    if (c.entityIsActPlayer(entity)) {
                                        const color = makeColorRGB(255, 0, 0);
                                        c.entityMessagePlayerColor(entity.skill[2], MESSAGE_STATUS, color, c.languageGet(586));
                                        if (c.entityPlayerIsLocalPlayer(entity.skill[2])) {
                                            c.entityCameraShakeAdd(entity.skill[2], 0.1, 10.0);
                                        } else {
                                            if (entity.skill[2] > 0) {
                                                c.entitySendShakePacket(entity.skill[2], 10, 10);
                                            }
                                        }
                                    }

                                    var damage: c_int = 50;
                                    if (c.entityChallengeRunStrongTrapsActive()) {
                                        damage = @as(c_int, @intFromFloat(@as(f64, @floatFromInt(damage)) * 1.5));
                                    }

                                    const trapResist = c.entityGetEntityBonusTrapResist(entity);
                                    if (trapResist != 0) {
                                        const mult = @max(0.0, 1.0 - (@as(f64, @floatFromInt(trapResist)) / 100.0));
                                        damage = @as(c_int, @intFromFloat(@as(f64, @floatFromInt(damage)) * mult));
                                    }

                                    if (c.entityOnEntityTrapHitSacredPath(entity, my)) {
                                        if (c.entityIsActPlayer(entity)) {
                                            const greenColor = makeColorRGB(0, 255, 0);
                                            c.entityMessagePlayerColor(entity.skill[2], MESSAGE_COMBAT, greenColor, c.languageGet(6492));
                                        }
                                        c.entityPlaySoundEntity(entity, 166, 128);
                                    } else if (damage > 0) {
                                        c.entityPlaySoundEntity(entity, 28, 64);
                                        c.entitySpawnGib(entity);

                                        const oldHP = st.HP;
                                        c.entityModHP(entity, -damage);

                                        if (oldHP > st.HP) {
                                            if (c.entityIsActPlayer(entity)) {
                                                c.entityEventUpdateWorldTrapDamage(entity.skill[2], "spike trap", oldHP - st.HP);
                                                if (st.HP <= 0) {
                                                    c.entityEventUpdateWorldTrapKilledBy(entity.skill[2], "spike trap", 1);
                                                }
                                            } else if (c.entityIsActMonster(entity)) {
                                                const leader = c.entityMonsterAllyGetPlayerLeader(entity);
                                                if (leader != null) {
                                                    c.entityEventUpdateWorldTrapFollowersKilled(entity.monsterAllyIndex.*, "spike trap", 1);
                                                }
                                            }
                                        }

                                        if (st.HP <= 0) {
                                            if (st.type == c.AUTOMATON and c.entityIsActPlayer(entity)) {
                                                entity.playerAutomatonDeathCounter = TICKS_PER_SECOND * 5;
                                            }
                                        }
                                        // set obituary
                                        c.entitySetObituary(entity, c.languageGet(1507));
                                        st.killer = c.TRAP_SPIKE;
                                    }
                                }
                            }
                        }
                    }
                    node = n.next;
                }
            }
        }
    }
}
