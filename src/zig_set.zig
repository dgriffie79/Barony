const std = @import("std");

const Set = std.AutoHashMap(c_int, void);

export fn set_create() callconv(.c) ?*anyopaque {
    const allocator = std.heap.c_allocator;
    const set = allocator.create(Set) catch return null;
    set.* = Set.init(allocator);
    return @as(?*anyopaque, @ptrCast(set));
}

export fn set_destroy(set_ptr: ?*anyopaque) callconv(.c) void {
    const set = @as(*Set, @ptrCast(@alignCast(set_ptr orelse return)));
    set.deinit();
    std.heap.c_allocator.destroy(set);
}

export fn set_find(set_ptr: ?*anyopaque, key: c_int) callconv(.c) bool {
    const set = @as(*Set, @ptrCast(@alignCast(set_ptr orelse return false)));
    return set.contains(key);
}

export fn set_insert(set_ptr: ?*anyopaque, key: c_int) callconv(.c) void {
    const set = @as(*Set, @ptrCast(@alignCast(set_ptr orelse return)));
    set.put(key, {}) catch {};
}

export fn set_clear(set_ptr: ?*anyopaque) callconv(.c) void {
    const set = @as(*Set, @ptrCast(@alignCast(set_ptr orelse return)));
    set.clearRetainingCapacity();
}
