const std = @import("std");

const IntMap = std.AutoHashMap(c_int, c_uint);
const PtrMap = std.AutoHashMap(c_int, ?*anyopaque);

export fn intmap_create() ?*anyopaque {
    const map = std.heap.c_allocator.create(IntMap) catch return null;
    map.* = IntMap.init(std.heap.c_allocator);
    return @as(?*anyopaque, @ptrCast(map));
}

export fn intmap_destroy(map_ptr: ?*anyopaque) void {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.deinit();
    std.heap.c_allocator.destroy(map);
}

export fn intmap_find(map_ptr: ?*anyopaque, key: c_int) bool {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return false)));
    return map.contains(key);
}

export fn intmap_get(map_ptr: ?*anyopaque, key: c_int) c_uint {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return 0)));
    return map.get(key) orelse 0;
}

export fn intmap_set(map_ptr: ?*anyopaque, key: c_int, value: c_uint) void {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.put(key, value) catch {};
}

export fn intmap_erase(map_ptr: ?*anyopaque, key: c_int) void {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return)));
    _ = map.remove(key);
}

export fn intmap_clear(map_ptr: ?*anyopaque) void {
    const map = @as(*IntMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.clearRetainingCapacity();
}

export fn ptrmap_create() ?*anyopaque {
    const map = std.heap.c_allocator.create(PtrMap) catch return null;
    map.* = PtrMap.init(std.heap.c_allocator);
    return @as(?*anyopaque, @ptrCast(map));
}

export fn ptrmap_destroy(map_ptr: ?*anyopaque) void {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.deinit();
    std.heap.c_allocator.destroy(map);
}

export fn ptrmap_find(map_ptr: ?*anyopaque, key: c_int) bool {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return false)));
    return map.contains(key);
}

export fn ptrmap_get(map_ptr: ?*anyopaque, key: c_int) ?*anyopaque {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return null)));
    return map.get(key) orelse null;
}

export fn ptrmap_set(map_ptr: ?*anyopaque, key: c_int, value: ?*anyopaque) void {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.put(key, value) catch {};
}

export fn ptrmap_erase(map_ptr: ?*anyopaque, key: c_int) void {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return)));
    _ = map.remove(key);
}

export fn ptrmap_clear(map_ptr: ?*anyopaque) void {
    const map = @as(*PtrMap, @ptrCast(@alignCast(map_ptr orelse return)));
    map.clearRetainingCapacity();
}
