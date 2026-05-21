const std = @import("std");

extern fn SDL_FreeSurface(surface: ?*anyopaque) void;
extern fn list_AddNodeFirst(list: ?*anyopaque) ?*anyopaque;
extern fn malloc(size: usize) ?*anyopaque;
extern fn calloc(count: usize, size: usize) ?*anyopaque;
extern fn free(ptr: ?*anyopaque) void;
extern fn strlen(str: [*c]const u8) usize;
extern fn strcmp(s1: [*c]const u8, s2: [*c]const u8) c_int;
extern fn strcpy(dest: [*c]u8, src: [*c]const u8) [*c]u8;

const HASH_SIZE = 256;

const ttfTextHash_t = extern struct {
    str: [*c]u8,
    surf: ?*anyopaque,
    font: ?*anyopaque,
    outline: bool,
};

const list_t = extern struct {
    first: ?*node_t,
    last: ?*node_t,
};

const node_t = extern struct {
    next: ?*node_t,
    prev: ?*node_t,
    element: ?*anyopaque,
    deconstructor: ?*const fn (?*anyopaque) callconv(.c) void,
    size: c_uint,
};

export fn djb2Hash(str: [*c]u8) c_ulong {
    var hash: c_ulong = 5381;
    var i: usize = 0;
    while (str[i] != 0) {
        hash = (hash << 5) +% hash +% str[i];
        i += 1;
    }
    return hash;
}

export fn ttfTextHash_deconstructor(data: ?*anyopaque) void {
    const hashedVal = @as(*ttfTextHash_t, @ptrCast(@alignCast(data orelse return)));
    SDL_FreeSurface(hashedVal.surf);
    free(@as(?*anyopaque, @ptrCast(hashedVal.str)));
    free(data);
}

export fn ttfTextHashRetrieve(buckets: [*c]list_t, str: [*c]u8, font: ?*anyopaque, outline: bool) ?*anyopaque {
    const idx = @as(usize, @intCast(djb2Hash(str) % HASH_SIZE));
    const list = &buckets[idx];
    var node: ?*node_t = list.first;
    while (node) |n| {
        if (n.element) |elem| {
            const hashedVal = @as(*ttfTextHash_t, @ptrCast(@alignCast(elem)));
            if (strcmp(hashedVal.str, str) == 0 and hashedVal.font == font and hashedVal.outline == outline) {
                return hashedVal.surf;
            }
        }
        node = n.next;
    }
    return null;
}

export fn ttfTextHashStore(buckets: [*c]list_t, str: [*c]u8, font: ?*anyopaque, outline: bool, surf: ?*anyopaque) ?*anyopaque {
    const idx = @as(usize, @intCast(djb2Hash(str) % HASH_SIZE));
    const list = &buckets[idx];
    const node_ptr = list_AddNodeFirst(@as(?*anyopaque, @ptrCast(list))) orelse return null;
    const node = @as(*node_t, @ptrCast(@alignCast(node_ptr)));
    const hashedVal = @as(*ttfTextHash_t, @ptrCast(@alignCast(
        malloc(@sizeOf(ttfTextHash_t)) orelse return null,
    )));
    const str_len = strlen(str);
    hashedVal.str = @as([*c]u8, @ptrCast(@alignCast(
        calloc(str_len + 1, 1) orelse return null,
    )));
    _ = strcpy(hashedVal.str, str);
    hashedVal.surf = surf;
    hashedVal.font = font;
    hashedVal.outline = outline;
    node.deconstructor = &ttfTextHash_deconstructor;
    node.size = @sizeOf(ttfTextHash_t);
    node.element = @as(?*anyopaque, @ptrCast(hashedVal));
    return surf;
}
