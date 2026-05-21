const std = @import("std");
const c = @cImport({
    @cInclude("defs.h");
    @cInclude("light.h");
});

export fn defaultDeconstructor(data: ?*anyopaque) void {
    if (data) |ptr| c.free(ptr);
}

export fn stringDeconstructor(data: ?*anyopaque) void {
    const string = @as(*c.string_t, @ptrCast(@alignCast(data orelse return)));
    if (string.data) |str_data| {
        c.free(@as(?*anyopaque, @ptrCast(str_data)));
        string.data = null;
    }
    c.free(@as(?*anyopaque, @ptrCast(string)));
}

export fn emptyDeconstructor(_: ?*anyopaque) void {}

export fn listDeconstructor(data: ?*anyopaque) void {
    const list = @as(*c.list_t, @ptrCast(@alignCast(data orelse return)));
    c.list_FreeAll(list);
    c.free(@as(?*anyopaque, @ptrCast(list)));
}

export fn newButton() ?*c.button_t {
    const btn = @as(*c.button_t, @ptrCast(@alignCast(
        c.malloc(@sizeOf(c.button_t)) orelse {
            _ = c.printlog("failed to allocate memory for new button!\n");
            c.exit(1);
        },
    )));

    btn.node = c.list_AddNodeLast(&c.button_l);
    btn.node.*.element = @as(?*anyopaque, @ptrCast(btn));
    btn.node.*.deconstructor = &defaultDeconstructor;
    btn.node.*.size = @sizeOf(c.button_t);

    btn.x = 0;
    btn.y = 0;
    btn.sizex = 0;
    btn.sizey = 0;
    btn.visible = 1;
    btn.focused = 0;
    btn.key = 0;
    btn.joykey = -1;
    btn.pressed = false;
    btn.needclick = true;
    btn.action = null;
    _ = c.strcpy(&btn.label, "nodef");
    btn.outline = false;

    return btn;
}

export fn newLight(index: c_int, x: c_int, y: c_int, radius: c_int) ?*c.light_t {
    const light = @as(*c.light_t, @ptrCast(@alignCast(
        c.malloc(@sizeOf(c.light_t)) orelse {
            _ = c.printlog("failed to allocate memory for new light!\n");
            c.exit(1);
        },
    )));

    light.node = c.list_AddNodeLast(&c.light_l);
    light.node.*.element = @as(?*anyopaque, @ptrCast(light));
    light.node.*.deconstructor = &c.lightDeconstructor;
    light.node.*.size = @sizeOf(c.light_t);

    light.index = index;
    light.x = x;
    light.y = y;
    light.radius = radius;
    if (light.radius > 0) {
        const size = @sizeOf(c.vec4_t) * @as(usize, @intCast((radius * 2 + 1) * (radius * 2 + 1)));
        light.tiles = @as(*c.vec4_t, @ptrCast(@alignCast(
            c.malloc(size) orelse {
                _ = c.printlog("failed to allocate memory for new light tiles!\n");
                c.exit(1);
            },
        )));
        _ = c.memset(light.tiles, 0, size);
    } else {
        light.tiles = null;
    }

    return light;
}
