const std = @import("std");
const c = @cImport({
    @cInclude("entity.h");
    @cInclude("defs.h");
});

export fn getPowerablesOnTile(x: c_int, y: c_int, list: *?*c.list_t) void {
    const entities = c.checkTileForEntity(x, y) orelse return;

    var node: ?*c.node_t = entities.*.first;
    while (node) |n| {
        if (n.element) |elem| {
            const entity = @as(*c.Entity, @ptrCast(@alignCast(elem)));
            if (entity.skill[28] != 0) {
                if (list.* == null) {
                    const new_list = @as(*c.list_t, @ptrCast(@alignCast(std.c.malloc(@sizeOf(c.list_t)))));
                    new_list.first = null;
                    new_list.last = null;
                    list.* = new_list;
                }
                const node2 = c.list_AddNodeLast(list.*.?) orelse unreachable;
                node2.*.element = @as(?*anyopaque, @ptrCast(entity));
                node2.*.deconstructor = &c.emptyDeconstructor;
            }
        }
        node = n.next;
    }
}
