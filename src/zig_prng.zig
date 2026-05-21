const std = @import("std");

const BaronyRNG = extern struct {
    seeded: bool,
    seed: [256]u8,
    seed_size: u8,
    buf: [256]u8,
    i1: u8,
    i2: u8,
    bytes_read: usize,
};

extern fn getTime() u32;
extern fn printlog(format: [*c]const u8, ...) void;
extern var marker: [256]u8;

fn swap_byte(a: *u8, b: *u8) void {
    const t = a.*;
    a.* = b.*;
    b.* = t;
}

fn seedImpl(rng: *BaronyRNG, key: *const anyopaque, size: usize) void {
    for (&rng.buf, 0..) |*b, i| {
        b.* = @as(u8, @intCast(i));
    }

    var b: u8 = 0;
    const bytes = @as([*]const u8, @ptrCast(key));
    for (0..256) |i| {
        b = b +% rng.buf[i] +% bytes[i % size];
        swap_byte(&rng.buf[i], &rng.buf[b]);
    }

    @memcpy(rng.seed[0..size], bytes[0..size]);
    rng.seed_size = @as(u8, @intCast(size));
    rng.i1 = 0;
    rng.i2 = 0;
    rng.bytes_read = 0;
    rng.seeded = true;
}

export fn rng_seedBytes(rng: *BaronyRNG, key: *const anyopaque, size: usize) void {
    seedImpl(rng, key, size);
}

export fn rng_seedTime(rng: *BaronyRNG) void {
    const t = getTime();
    seedImpl(rng, &t, @sizeOf(u32));
}

export fn rng_getSeed(rng: *const BaronyRNG, out_: *anyopaque, size: usize) c_int {
    if (!rng.seeded or size < rng.seed_size) {
        if (std.debug.runtime_safety) @panic("wtf are you doin");
        return -1;
    }
    const dest = @as([*]u8, @ptrCast(out_))[0..rng.seed_size];
    @memcpy(dest, rng.seed[0..rng.seed_size]);
    return @as(c_int, @intCast(rng.seed_size));
}

export fn rng_getBytes(rng: *BaronyRNG, data_: *anyopaque, size: usize) void {
    if (!rng.seeded) {
        printlog("rng not seeded, seeding by unix time");
        const t = getTime();
        seedImpl(rng, &t, @sizeOf(u32));
    }

    var data = @as([*]u8, @ptrCast(data_));
    var i: usize = 0;
    while (i < size) : (i += 1) {
        rng.i1 +%= 1;
        rng.i2 +%= rng.buf[rng.i1];
        swap_byte(&rng.buf[rng.i1], &rng.buf[rng.i2]);
        data[i] = rng.buf[rng.buf[rng.i1] +% rng.buf[rng.i2]];
        rng.bytes_read +%= 1;
    }
}

export fn rng_getU8(rng: *BaronyRNG) u8 {
    var result: u8 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(u8));
    return result;
}

export fn rng_getU16(rng: *BaronyRNG) u16 {
    var result: u16 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(u16));
    return result;
}

export fn rng_getU32(rng: *BaronyRNG) u32 {
    var result: u32 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(u32));
    return result;
}

export fn rng_getU64(rng: *BaronyRNG) u64 {
    var result: u64 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(u64));
    return result;
}

export fn rng_getI8(rng: *BaronyRNG) i8 {
    var result: i8 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(i8));
    return result;
}

export fn rng_getI16(rng: *BaronyRNG) i16 {
    var result: i16 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(i16));
    return result;
}

export fn rng_getI32(rng: *BaronyRNG) i32 {
    var result: i32 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(i32));
    return result;
}

export fn rng_getI64(rng: *BaronyRNG) i64 {
    var result: i64 = undefined;
    rng_getBytes(rng, @ptrCast(&result), @sizeOf(i64));
    return result;
}

export fn rng_getF32(rng: *BaronyRNG) f32 {
    const u32_val = rng_getU32(rng);
    const div: u64 = @as(u64, 1) << 32;
    return @as(f32, @floatCast(@as(f64, @floatFromInt(u32_val)) / @as(f64, @floatFromInt(div))));
}

export fn rng_getF64(rng: *BaronyRNG) f64 {
    const u32_val = rng_getU32(rng);
    const div: u64 = @as(u64, 1) << 32;
    return @as(f64, @floatFromInt(u32_val)) / @as(f64, @floatFromInt(div));
}

export fn rng_rand(rng: *BaronyRNG) c_int {
    var i: c_int = undefined;
    rng_getBytes(rng, @ptrCast(&i), @sizeOf(c_int));
    return i & 0x7fffffff;
}

export fn rng_uniform(rng: *BaronyRNG, a: c_int, b: c_int) c_int {
    if (a == b) return a;
    const min = if (a < b) a else b;
    const max = if (a > b) a else b;
    const diff = (max - min) + 1;
    const choice: c_int = @intFromFloat(rng_getF64(rng) * @as(f64, @floatFromInt(diff)));
    return min + choice;
}

export fn rng_discrete(rng: *BaronyRNG, chances: [*c]const c_uint, size: c_int) c_int {
    if (size <= 0) {
        if (std.debug.runtime_safety) @panic("BaronyRNG::discrete() list is less-or-equal than 0");
        return 0;
    }

    var total: c_uint = 0;
    var c: c_int = 0;
    while (c < size) : (c += 1) {
        total += chances[@as(usize, @intCast(c))];
    }
    if (total == 0) {
        if (std.debug.runtime_safety) @panic("BaronyRNG::discrete() chances of picking anything are 0");
        return 0;
    }

    var choice: c_uint = @intFromFloat(rng_getF64(rng) * @as(f64, @floatFromInt(total)));
    c = 0;
    while (c < size) : (c += 1) {
        if (chances[@as(usize, @intCast(c))] > choice) {
            return c;
        } else {
            choice -= chances[@as(usize, @intCast(c))];
        }
    }

    if (std.debug.runtime_safety) @panic("BaronyRNG::discrete() nothing was picked. this should never happen");
    return 0;
}

export fn rng_normal(rng: *BaronyRNG, mean: c_int, deviation: c_int) c_int {
    const m: f64 = @floatFromInt(mean);
    const d: f64 = @floatFromInt(deviation);
    const f1: f64 = rng_getF64(rng);
    const f2: f64 = rng_getF64(rng);
    const norm: f64 = @cos(2.0 * std.math.pi * f2) * @sqrt(-2.0 * @log(f1));
    return @intFromFloat(@round(norm * d + m));
}

export fn rng_setMarker(rng: *const BaronyRNG) void {
    if (std.debug.runtime_safety) {
        @memcpy(marker[0..], rng.buf[0..]);
    }
}

export fn rng_checkMarker(rng: *const BaronyRNG) void {
    if (std.debug.runtime_safety) {
        if (std.mem.eql(u8, marker[0..], rng.buf[0..])) {
            printlog("reached marker");
        }
    }
}

export fn rng_bytesRead(rng: *const BaronyRNG) usize {
    return rng.bytes_read;
}

export fn rng_isSeeded(rng: *const BaronyRNG) bool {
    return rng.seeded;
}

export fn rng_testSeedHealth(rng: *const BaronyRNG) void {
    var sum: f64 = 0.0;
    for (rng.buf) |byte| {
        var b: u8 = 0;
        while (b < 8) : (b += 1) {
            if (byte & (@as(u8, 1) << @as(u3, @intCast(b))) != 0) {
                sum += 1.0;
            }
        }
    }
    sum /= 2048.0;
    printlog("rng seed bits are %.2f%% on", sum * 100.0);
}
