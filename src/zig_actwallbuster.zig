const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
});

const Entity = c.Entity;

export fn actWallBuster(my: *Entity) void {
    const SERVER: c_int = 1;
    const OBSTACLELAYER: usize = 1;
    const MAPLAYERS: usize = 3;

    if (my.skill[28] == 0) return;

    if (my.skill[28] == 2) {
        const mapW = c.entityMapGetWidth();
        const mapH = c.entityMapGetHeight();

        const tileX: c_int = @min(@max(@as(c_int, @intFromFloat(my.x / 16.0)), 0), @as(c_int, @intCast(mapW - 1)));
        const tileY: c_int = @min(@max(@as(c_int, @intFromFloat(my.y / 16.0)), 0), @as(c_int, @intCast(mapH - 1)));

        const idx0: c_int = @as(c_int, @intCast(OBSTACLELAYER + @as(usize, @intCast(tileY)) * MAPLAYERS + @as(usize, @intCast(tileX)) * MAPLAYERS * mapH));
        const idx1: c_int = @as(c_int, @intCast((MAPLAYERS - 1) + @as(usize, @intCast(tileY)) * MAPLAYERS + @as(usize, @intCast(tileX)) * MAPLAYERS * mapH));

        c.entityMapSetTile(idx0, 0);
        c.entityMapSetTile(idx1, 0);

        c.entitySpawnExplosionWrap(@as(c_short, @intFromFloat(my.x)), @as(c_short, @intFromFloat(my.y)), @as(c_short, @intFromFloat(my.z - 8.0)));

        if (c.multiplayer == SERVER) {
            c.entitySendWallBusterPacket(@as(c_ushort, @intCast(tileX)), @as(c_ushort, @intCast(tileY)));
        }
        c.entityGeneratePathMaps();
        c.list_RemoveNode(my.mynode);
    }
}

export fn actWallBuilder(my: *Entity) void {
    const SERVER: c_int = 1;
    const OBSTACLELAYER: usize = 1;
    const MAPLAYERS: usize = 3;

    if (my.skill[28] == 0) return;

    if (my.skill[28] == 2) {
        if (c.entityWallBuilderOccupied(my)) return;

        c.entityPlaySoundEntity(my, 182, 64);

        const mapW = c.entityMapGetWidth();
        const mapH = c.entityMapGetHeight();

        const tileX: c_int = @min(@max(@as(c_int, @intFromFloat(my.x / 16.0)), 0), @as(c_int, @intCast(mapW - 1)));
        const tileY: c_int = @min(@max(@as(c_int, @intFromFloat(my.y / 16.0)), 0), @as(c_int, @intCast(mapH - 1)));

        const idxObstacle: c_int = @as(c_int, @intCast(OBSTACLELAYER + @as(usize, @intCast(tileY)) * MAPLAYERS + @as(usize, @intCast(tileX)) * MAPLAYERS * mapH));
        const idxBase: c_int = @as(c_int, @intCast(@as(usize, @intCast(tileY)) * MAPLAYERS + @as(usize, @intCast(tileX)) * MAPLAYERS * mapH));

        c.entityMapSetTile(idxObstacle, c.entityMapGetTile(idxBase));

        const fx: f64 = @floatFromInt(tileX);
        const fy: f64 = @floatFromInt(tileY);
        const off: f64 = 2.0;

        c.entitySpawnPoofWrap(@as(c_short, @intFromFloat(fx * 16.0 - off)), @as(c_short, @intFromFloat(fy * 16.0 - off)), 8, 1.0);
        c.entitySpawnPoofWrap(@as(c_short, @intFromFloat(fx * 16.0 - off)), @as(c_short, @intFromFloat(fy * 16.0 + 16.0 + off)), 8, 1.0);
        c.entitySpawnPoofWrap(@as(c_short, @intFromFloat(fx * 16.0 + 16.0 + off)), @as(c_short, @intFromFloat(fy * 16.0 - off)), 8, 1.0);
        c.entitySpawnPoofWrap(@as(c_short, @intFromFloat(fx * 16.0 + 16.0 + off)), @as(c_short, @intFromFloat(fy * 16.0 + 16.0 + off)), 8, 1.0);

        if (c.multiplayer == SERVER) {
            c.entitySendWallBuilderPacket(@as(c_ushort, @intCast(tileX)), @as(c_ushort, @intCast(tileY)));
        }
        c.entityGeneratePathMaps();
        c.list_RemoveNode(my.mynode);
    }
}
