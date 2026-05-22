const c = @cImport({
    @cInclude("defs.h");
});
const std = @import("std");

export fn list_AddNodeFirst(list: ?*c.list_t) ?*c.node_t {
    const l = list orelse return null;
    const mem = c.malloc(@sizeOf(c.node_t)) orelse {
        c.printlog("failed to allocate memory for new node!\n");
        std.process.exit(1);
    };
    const n: *c.node_t = @ptrCast(@alignCast(mem));
    n.*.element = null;
    n.*.deconstructor = null;
    n.*.size = 0;
    n.*.prev = null;
    n.*.next = l.*.first;
    n.*.list = l;
    if (l.*.first) |first| {
        first.*.prev = n;
    } else {
        l.*.last = n;
    }
    l.*.first = n;
    return n;
}

export fn list_AddNodeLast(list: ?*c.list_t) ?*c.node_t {
    const l = list orelse return null;
    const mem = c.malloc(@sizeOf(c.node_t)) orelse {
        c.printlog("failed to allocate memory for new node!\n");
        std.process.exit(1);
    };
    const n: *c.node_t = @ptrCast(@alignCast(mem));
    n.*.element = null;
    n.*.deconstructor = null;
    n.*.size = 0;
    n.*.next = null;
    n.*.prev = l.*.last;
    n.*.list = l;
    if (l.*.last) |last| {
        last.*.next = n;
    } else {
        l.*.first = n;
    }
    l.*.last = n;
    return n;
}

export fn list_AddNode(list: ?*c.list_t, index: c_int) ?*c.node_t {
    const l = list orelse return null;
    if (index < 0 or index > @as(c_int, @intCast(list_Size(l)))) return null;
    const mem = c.malloc(@sizeOf(c.node_t)) orelse {
        c.printlog("failed to allocate memory for new node!\n");
        std.process.exit(1);
    };
    const n: *c.node_t = @ptrCast(@alignCast(mem));
    n.*.element = null;
    n.*.deconstructor = null;
    n.*.size = 0;
    n.*.prev = null;
    n.*.next = null;
    n.*.list = l;
    const oldnode = list_Node(l, index);
    if (oldnode) |on| {
        n.*.prev = on.*.prev;
        n.*.next = on;
        if (l.*.first == on) {
            l.*.first = n;
        } else {
            if (on.*.prev) |prev| {
                prev.*.next = n;
            }
        }
        on.*.prev = n;
    } else {
        if (list_Size(l) > 0) {
            n.*.prev = l.*.last;
            n.*.next = null;
            if (l.*.last) |last| {
                last.*.next = n;
            }
            l.*.last = n;
        } else {
            l.*.first = n;
            l.*.last = n;
        }
    }
    return n;
}

export fn list_RemoveNode(node: ?*c.node_t) void {
    const n = node orelse return;
    if (n.*.list) |lst| {
        if (lst.*.first) |first| {
            if (first == n) {
                if (lst.*.last) |last| {
                    if (last == n) {
                        lst.*.first = null;
                        lst.*.last = null;
                    } else {
                        if (n.*.next) |next| {
                            next.*.prev = null;
                        }
                        lst.*.first = n.*.next;
                    }
                } else {
                    lst.*.first = null;
                    lst.*.last = null;
                }
            } else if (lst.*.last) |last| {
                if (last == n) {
                    if (n.*.prev) |prev| {
                        prev.*.next = null;
                    }
                    lst.*.last = n.*.prev;
                } else {
                    if (n.*.prev) |prev| {
                        prev.*.next = n.*.next;
                    }
                    if (n.*.next) |next| {
                        next.*.prev = n.*.prev;
                    }
                }
            }
        }
    }
    if (n.*.deconstructor) |decon| {
        decon(n.*.element);
    } else {
        if (n.*.element) |el| {
            c.free(el);
        }
    }
    c.free(@as(?*anyopaque, @ptrCast(@alignCast(n))));
}

export fn list_FreeAll(list: ?*c.list_t) void {
    const l = list orelse return;
    var node: ?*c.node_t = l.*.first;
    while (node) |n| {
        const nextnode = n.*.next;
        list_RemoveNode(n);
        node = nextnode;
    }
    l.*.first = null;
    l.*.last = null;
}

export fn list_Size(list: ?*c.list_t) c_uint {
    const l = list orelse return 0;
    var count: c_uint = 0;
    var node: ?*c.node_t = l.*.first;
    while (node) |n| {
        count += 1;
        node = n.*.next;
    }
    return count;
}

export fn list_Copy(destlist: ?*c.list_t, srclist: ?*c.list_t) ?*c.list_t {
    const dest = destlist orelse return null;
    const src = srclist orelse return null;
    var node: ?*c.node_t = src.*.first;
    while (node) |n| {
        if (n.*.size == 0) {
            c.printlog("error: attempted copy of node with size 0! Node not copied\n");
            node = n.*.next;
            continue;
        }
        const nn = list_AddNodeLast(dest) orelse unreachable;
        nn.*.deconstructor = n.*.deconstructor;
        const elem = c.malloc(n.*.size) orelse {
            c.printlog("critical error: list_Copy() failed to allocate memory for element!\n");
            return null;
        };
        nn.*.element = elem;
        _ = c.memcpy(elem, n.*.element, n.*.size);
        nn.*.size = n.*.size;
        node = n.*.next;
    }
    return dest;
}

export fn list_CopyNew(srclist: ?*c.list_t) ?*c.list_t {
    const src = srclist orelse return null;
    const mem = c.malloc(@sizeOf(c.list_t)) orelse {
        c.printlog("critical error: list_CopyNew() failed to allocate memory for new list!\n");
        return null;
    };
    const dest: *c.list_t = @ptrCast(@alignCast(mem));
    dest.*.first = null;
    dest.*.last = null;
    var node: ?*c.node_t = src.*.first;
    while (node) |n| {
        if (n.*.size == 0) {
            c.printlog("error: attempted copy of node with size 0! Node not copied\n");
            node = n.*.next;
            continue;
        }
        const nn2 = list_AddNodeLast(dest) orelse unreachable;
        nn2.*.deconstructor = n.*.deconstructor;
        const elem = c.malloc(n.*.size) orelse {
            c.printlog("critical error: list_CopyNew() failed to allocate memory for element!\n");
            return null;
        };
        nn2.*.element = elem;
        _ = c.memcpy(elem, n.*.element, n.*.size);
        nn2.*.size = n.*.size;
        node = n.*.next;
    }
    return dest;
}

export fn list_Index(node: ?*c.node_t) c_uint {
    const n = node orelse return std.math.maxInt(c_uint);
    if (n.*.list) |lst| {
        var i: c_uint = 0;
        var tempnode: ?*c.node_t = lst.*.first;
        while (tempnode) |tn| {
            if (tn == n) return i;
            i += 1;
            tempnode = tn.*.next;
        }
    }
    return 0;
}

export fn list_Node(list: ?*c.list_t, index: c_int) ?*c.node_t {
    const l = list orelse return null;
    if (index < 0) return null;
    var i: c_int = 0;
    var node: ?*c.node_t = l.*.first;
    while (node) |n| {
        if (i == index) return n;
        i += 1;
        node = n.*.next;
    }
    return null;
}
