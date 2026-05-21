comptime {
    _ = @import("zig_entity.zig");
}

export fn sgn(x: f64) c_int {
    if (x > 0.0) return 1;
    if (x < 0.0) return -1;
    return 0;
}

export fn longestline(str: [*c]const u8) c_int {
    var c: usize = 0;
    var x: c_int = 0;
    var result: c_int = 0;
    while (str[c] != 0) {
        if (str[c] == 10) { x = 0; c += 1; continue; }
        x += 1;
        if (x > result) result = x;
        c += 1;
    }
    return result;
}

export fn numdigits_sint16(x: i16) c_int {
    var n: i32 = @as(i32, x);
    if (n < 0) n = -n;
    if (n == 0) return 1;
    var count: c_int = 0;
    while (n > 0) { n = @divTrunc(n, 10); count += 1; }
    if (x < 0) count += 1;
    return count;
}

export fn stringCmp(str1: [*c]const u8, str2: [*c]const u8, str1_size: usize, str2_size: usize) c_int {
    if (str1 == null or str2 == null) return 0;
    var c: usize = 0;
    while (c < str1_size and c < str2_size and str1[c] != 0 and str2[c] != 0 and str1[c] == str2[c]) {
        c += 1;
    }
    const end1 = (c == str1_size or str1[c] == 0);
    const end2 = (c == str2_size or str2[c] == 0);
    if (end1 and end2) return 0;
    if (end1 and !end2) return -@as(c_int, str2[c]);
    if (end2 and !end1) return @as(c_int, str1[c]);
    return @as(c_int, str1[c]) - @as(c_int, str2[c]);
}

export fn stringLen(str: [*c]const u8, size: usize) usize {
    if (str == null or size == 0) return 0;
    var len: usize = 0;
    while (len < size and str[len] != 0) { len += 1; }
    return len;
}

export fn stringStr(str1: [*c]const u8, str2: [*c]const u8, str1_size: usize, str2_size: usize) [*c]const u8 {
    if (str1 == null or str2 == null) return null;
    var s: usize = 0;
    while (s < str1_size and str1[s] != 0) {
        const ptr = @as([*c]const u8, @ptrCast(&str1[s]));
        if (stringCmp(ptr, str2, str1_size - s, str2_size) == 0) return ptr;
        s += 1;
    }
    return null;
}
