const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
    @cInclude("stat.h");
    @cInclude("monster.h");
    @cInclude("items.h");
    @cInclude("player.h");
    @cInclude("net.h");
    @cInclude("prng.h");
});

const Entity = c.Entity;
const Stat = c.Stat;

const CLIENT = 2;
const SERVER = 1;
const MAXPLAYERS = 4;
const TICKS_PER_SECOND = 50;
const PI = 3.14159265358979323846;

const INVISIBLE: c_uint = 2;
const NOUPDATE: c_uint = 3;
const UPDATENEEDED: c_uint = 4;
const SPRITE: c_uint = 7;
const BURNING: c_uint = 9;

const MESSAGE_INTERACTION: c_uint = 1 << 7;
const MESSAGE_INVENTORY: c_uint = 1 << 2;
const MESSAGE_STATUS: c_uint = 1 << 1;
const MESSAGE_HINT: c_uint = 1 << 9;

const EFF_POLYMORPH: c_int = 25;
const EFF_SHAPESHIFT: c_int = 29;
const EFF_GROWTH: c_int = 110;

const SLIME: c_int = 4;
const AUTOMATON: c_int = 30;
const VAMPIRE: c_int = 25;
const SKELETON: c_int = 9;
const DRYAD: c_int = 38;
const MYCONID: c_int = 39;
const NOTHING: c_int = 0;

const KILLEDBY_SINK: c_int = 24;

const STATUS_DECREPIT: c_int = 0;
const STATUS_WORN: c_int = 1;
const STATUS_SERVICABLE: c_int = 2;
const STATUS_EXCELLENT: c_int = 3;

const PARTICLE_EFFECT_RISING_DROP: c_int = 9;

const CPDM_SINKS_USED: c_int = 74;
const CPDM_SINKS_RINGS: c_int = 75;
const CPDM_SINKS_SLIMES: c_int = 76;
const CPDM_SINKS_HEALTH_RESTORED: c_int = 113;

const FEMALE: c_int = 1;
const RACE_INSECTOID: c_int = 2;
const SV_FLAG_HUNGER: c_uint = 1 << 3;
const RNG_ROLL_GROWTH: c_int = 2;
const RING_ADORNMENT: c_int = 99;

export fn actSink(my: *Entity) void {
    my.skill[7] -= 1;
    if (my.skill[7] <= 0) {
        my.skill[7] = TICKS_PER_SECOND * 30;
        c.entityPlaySoundEntityLocal(my, 149, 32);
    }

    if (my.ticks == 1) {
        c.entityCreateWorldUITooltip(my);
    }

    if (my.skill[2] > 0) {
        const entity = c.entitySpawnGibReturn(my);
        if (entity) {
            entity.flags[@as(usize, @intCast(INVISIBLE))] = false;
            entity.x += 0.5;
            entity.z = my.z - 3;
            entity.flags[@as(usize, @intCast(SPRITE))] = false;
            entity.flags[@as(usize, @intCast(NOUPDATE))] = true;
            entity.flags[@as(usize, @intCast(UPDATENEEDED))] = false;
            entity.skill[4] = 6;
            entity.sprite = 4;
            entity.yaw = @as(f64, @floatFromInt(c.rng_rand(&c.local_rng) % 360)) * PI / 180.0;
            entity.pitch = @as(f64, @floatFromInt(c.rng_rand(&c.local_rng) % 360)) * PI / 180.0;
            entity.roll = @as(f64, @floatFromInt(c.rng_rand(&c.local_rng) % 360)) * PI / 180.0;
            entity.vel_x = 0;
            entity.vel_y = 0;
            entity.vel_z = 0.25;
            entity.fskill[3] = 0.03;
        }

        if (c.multiplayer != CLIENT) {
            my.skill[2] -= 1;
        }
    }

    if (c.multiplayer == CLIENT) {
        return;
    }

    var i: c_int = 0;
    while (i < MAXPLAYERS) : (i += 1) {
        const idx = @as(usize, @intCast(i));
        if (c.selectedEntity[idx] == my or c.client_selected[idx] == my) {
            if (c.inrange[idx]) {
                const rng: *c.BaronyRNG = if (my.entity_rng) |r| r else &c.local_rng;
                const playerEntity = c.entityGetPlayerEntity(i);
                const st = c.stats[idx];

                if (my.skill[0] == 0) {
                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(580));
                    c.entityPlaySoundEntity(my, 140 + @as(c_uint, @intCast(c.rng_rand(&c.local_rng))) % 2, 64);
                } else {
                    if (playerEntity) {
                        if (playerEntity.flags[@as(usize, @intCast(BURNING))] != 0) {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(468));
                            playerEntity.flags[@as(usize, @intCast(BURNING))] = false;
                            c.entityServerUpdateEntityFlag(playerEntity, @as(c_int, @intCast(BURNING)));
                            c.entitySteamAchievementClient(i, "BARONY_ACH_HOT_SHOWER");
                        }
                    }

                    if (st != null) {
                        if (c.stat_getEffectActive(st, EFF_POLYMORPH) != 0 and my.skill[8] == 0) {
                            if (playerEntity) {
                                _ = c.entitySetEffectBool(playerEntity, EFF_POLYMORPH, false, 0, true);
                                playerEntity.skill[50] = 0;
                                c.serverUpdateEntitySkillWrap(playerEntity, 50);
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(3192));
                                if (c.stat_getEffectActive(st, EFF_SHAPESHIFT) == 0) {
                                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(3185));
                                } else {
                                    _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(4303));
                                }
                            }
                            if (playerEntity) {
                                c.entityPlaySoundEntity(playerEntity, 400, 92);
                                c.entityCreateParticleDropRising(playerEntity, 593, 1.0);
                                c.entityServerSpawnMiscParticles(playerEntity, PARTICLE_EFFECT_RISING_DROP, 593);
                            }
                        }
                    }

                    switch (my.skill[3]) {
                        0 => {
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(581));
                            c.entityEventUpdateWorld(i, CPDM_SINKS_USED, "sink", 1);
                            const ring_offset = c.rng_rand(rng) % 12;
                            const ring = RING_ADORNMENT + ring_offset;
                            var status: c_int = STATUS_SERVICABLE;
                            const status_rand = c.rng_rand(rng) % 4;
                            status = switch (status_rand) {
                                0 => STATUS_DECREPIT,
                                1 => STATUS_WORN,
                                2 => STATUS_SERVICABLE,
                                3 => STATUS_EXCELLENT,
                                else => STATUS_SERVICABLE,
                            };
                            const beatitude = c.rng_rand(rng) % 5 - 2;
                            const item = c.entityNewItemWrap(ring, status, @as(c_short, @intCast(beatitude)), 1, @as(c_uint, @intCast(c.rng_rand(rng))), false);
                            if (item) {
                                _ = c.entityItemPickup(i, item);
                                const desc = c.entityItemDescription(item);
                                c.entityMessagePlayerFormatted(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, c.languageGet(504), desc);
                                c.free(item);
                                c.entityEventUpdateWorld(i, CPDM_SINKS_RINGS, "sink", 1);
                            }
                        },
                        1 => {
                            c.entityEventUpdateWorld(i, CPDM_SINKS_USED, "sink", 1);
                            const monster = c.entitySummonMonsterNoSmoke(SLIME, @as(c_long, @intFromFloat(my.x)), @as(c_long, @intFromFloat(my.y)));
                            if (monster) {
                                c.entitySeedEntityRNG(monster, c.rng_getU32(rng));
                                c.entitySlimeSetType(monster, c.entityGetStats(monster), true, rng);
                                const color: c_uint = @as(c_uint, 255) | (@as(c_uint, 128) << 8) | (@as(c_uint, 0) << 16);
                                c.entityMessagePlayerColor(i, MESSAGE_HINT, color, c.languageGet(582));
                                c.entityEventUpdateWorld(i, CPDM_SINKS_SLIMES, "sink", 1);
                            }
                        },
                        2 => {
                            if (st == null) break;
                            if (playerEntity == null) break;
                            c.entityEventUpdateWorld(i, CPDM_SINKS_USED, "sink", 1);
                            if (st.type == AUTOMATON) {
                                const color: c_uint = @as(c_uint, 255) | (@as(c_uint, 128) << 8);
                                c.entityMessagePlayerColor(i, MESSAGE_STATUS, color, c.languageGet(3700));
                                c.entityPlaySoundEntity(playerEntity, 52, 64);
                                st.HUNGER -= 200;
                                _ = c.entityModMP(playerEntity, 5 + c.rng_rand(&c.local_rng) % 6);
                                c.entityServerUpdateHunger(i);
                            } else if (st.type != VAMPIRE) {
                                _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(583));
                                c.entityPlaySoundEntity(playerEntity, 52, 64);
                                if (st.type != SKELETON) {
                                    if (!((c.svFlags & SV_FLAG_HUNGER) != 0 and st.playerRace.* == RACE_INSECTOID and st.stat_appearance == 0)) {
                                        st.HUNGER += 50;
                                        c.entityServerUpdateHunger(i);
                                    }
                                }
                                c.entityModHP(playerEntity, 2 + c.rng_rand(&c.local_rng) % 2);
                                const mpAmount = c.entityModMP(playerEntity, 1 + c.rng_rand(&c.local_rng) % 2);
                                c.entityPlayerInsectoidIncrementHungerToMP(playerEntity, mpAmount);
                                c.entityEventUpdateWorld(i, CPDM_SINKS_HEALTH_RESTORED, "sink", 1);

                                if (st.type == DRYAD) {
                                    const effectStrength = c.stat_getEffectActive(st, EFF_GROWTH);
                                    if (effectStrength != 0) {
                                        var chance: c_int = 5;
                                        if (st.sex == FEMALE) {
                                            chance = 10;
                                        }
                                        if (c.player_mechanics_rollRngProc(c.players[idx], RNG_ROLL_GROWTH, chance, -1)) {
                                            if (c.stat_getEffectActive(st, EFF_GROWTH) < 4) {
                                                const newStrength: u8 = if (effectStrength + 1 > 4) @as(u8, 4) else @as(u8, effectStrength + 1);
                                                _ = c.entitySetEffectU8(playerEntity, EFF_GROWTH, newStrength, 15 * TICKS_PER_SECOND, false);
                                                const green_color: c_uint = @as(c_uint, 0) | (@as(c_uint, 255) << 8);
                                                c.entityMessagePlayerColor(i, MESSAGE_STATUS, green_color, c.languageGet(6924));
                                            }
                                        }
                                    }
                                }
                            } else {
                                c.entityModHP(playerEntity, -2);
                                c.entityEventUpdateWorld(i, CPDM_SINKS_HEALTH_RESTORED, "sink", -2);
                                c.entityPlaySoundEntity(playerEntity, 28, 64);
                                c.entityPlaySoundEntity(playerEntity, 249, 128);
                                c.entitySetObituary(playerEntity, c.languageGet(1533));
                                st.killer = KILLEDBY_SINK;

                                const red_color: c_uint = @as(c_uint, 255) | (@as(c_uint, 0) << 8);
                                c.entityMessagePlayerColor(i, MESSAGE_STATUS, red_color, c.languageGet(3183));
                                if (i >= 0 and c.entityPlayerIsLocalPlayer(i)) {
                                    c.entityCameraShakeAdd(i, 0.1, 10.0);
                                } else if (c.multiplayer == SERVER and i > 0 and !c.entityPlayerIsLocalPlayer(i)) {
                                    c.entitySendShakePacket(i, 10, 10);
                                }
                            }
                        },
                        3 => {
                            if (st == null) break;
                            if (playerEntity == null) break;
                            c.entityEventUpdateWorld(i, CPDM_SINKS_USED, "sink", 1);
                            if (st.type == AUTOMATON) {
                                const orange_color: c_uint = @as(c_uint, 255) | (@as(c_uint, 128) << 8);
                                c.entityMessagePlayerColor(i, MESSAGE_STATUS, orange_color, c.languageGet(3701));
                                c.entityPlaySoundEntity(playerEntity, 52, 64);
                                st.HUNGER += 200;
                                _ = c.entityModMP(playerEntity, 4);
                                c.entityServerUpdateHunger(i);
                            } else {
                                if (st.type != VAMPIRE) {
                                    c.entityModHP(playerEntity, -2);
                                } else {
                                    c.entityModHP(playerEntity, -2);
                                    c.entityPlaySoundEntity(playerEntity, 249, 128);
                                }
                                c.entityEventUpdateWorld(i, CPDM_SINKS_HEALTH_RESTORED, "sink", -2);
                                c.entityPlaySoundEntity(playerEntity, 28, 64);
                                c.entitySetObituary(playerEntity, c.languageGet(1533));
                                st.killer = KILLEDBY_SINK;

                                const red_color: c_uint = @as(c_uint, 255) | (@as(c_uint, 0) << 8);
                                c.entityMessagePlayerColor(i, MESSAGE_STATUS, red_color, c.languageGet(584));

                                if (i >= 0 and c.entityPlayerIsLocalPlayer(i)) {
                                    c.entityCameraShakeAdd(i, 0.1, 10.0);
                                } else if (c.multiplayer == SERVER and i > 0 and !c.entityPlayerIsLocalPlayer(i)) {
                                    c.entitySendShakePacket(i, 10, 10);
                                }
                            }
                        },
                        else => {},
                    }

                    if (st != null) {
                        my.skill[2] = TICKS_PER_SECOND / 2;

                        if (my.skill[0] > 1) {
                            my.skill[0] -= 1;
                            const effect = c.rng_rand(rng) % 10;
                            my.skill[3] = switch (effect) {
                                0 => 0,
                                1 => 1,
                                2...7 => 2,
                                8, 9 => 3,
                                else => my.skill[3],
                            };
                        } else {
                            my.skill[0] -= 1;
                            _ = c.messagePlayerSimple(i, MESSAGE_INTERACTION, c.languageGet(585));
                            c.entityPlaySoundEntity(my, 132, 64);
                        }
                        c.serverUpdateEntitySkillWrap(my, 0);
                    }
                }
            }
        }
    }
}
