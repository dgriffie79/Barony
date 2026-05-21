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
