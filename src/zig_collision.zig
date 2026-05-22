const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
});

const Entity = c.Entity;

export fn entityDist(my: *Entity, your: *Entity) f64 {
    const dx = my.x - your.x;
    const dy = my.y - your.y;
    return @sqrt(dx * dx + dy * dy);
}

export fn entityInsideEntity(entity1: ?*Entity, entity2: ?*Entity) bool {
    const e1 = entity1 orelse return false;
    const e2 = entity2 orelse return false;
    const e1x = e1.x;
    const e2x = e2.x;
    const e1y = e1.y;
    const e2y = e2.y;
    const e1sx = @as(f64, @floatFromInt(e1.sizex));
    const e2sx = @as(f64, @floatFromInt(e2.sizex));
    const e1sy = @as(f64, @floatFromInt(e1.sizey));
    const e2sy = @as(f64, @floatFromInt(e2.sizey));
    if (e1x + e1sx > e2x - e2sx) {
        if (e1x - e1sx < e2x + e2sx) {
            if (e1y + e1sy > e2y - e2sy) {
                if (e1y - e1sy < e2y + e2sy) {
                    return true;
                }
            }
        }
    }
    return false;
}
