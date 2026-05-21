export fn sgn(x: f64) c_int {
    if (x > 0.0) return 1;
    if (x < 0.0) return -1;
    return 0;
}
