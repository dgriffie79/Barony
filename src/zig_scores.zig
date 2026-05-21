const std = @import("std");
const c = @cImport({
    @cInclude("cJSON.h");
    @cInclude("defs.h");
    @cInclude("string.h");
});

export fn jsonGetInt(d: ?*c.cJSON, key: [*c]const u8) c_int {
    const val = c.cJSON_GetObjectItem(d, key);
    if (val) |v| {
        return @intFromFloat(v.*.valuedouble);
    }
    return 0;
}

export fn jsonGetBool(d: ?*c.cJSON, key: [*c]const u8) bool {
    const val = c.cJSON_GetObjectItem(d, key);
    if (val) |v| {
        return c.cJSON_IsTrue(v) != 0;
    }
    return false;
}

export fn jsonGetStr(d: ?*c.cJSON, key: [*c]const u8) [*c]const u8 {
    const val = c.cJSON_GetObjectItem(d, key);
    if (val) |v| {
        return v.*.valuestring;
    }
    return "";
}

fn strlen_zig(s: [*c]const u8) usize {
    var len: usize = 0;
    while (s[len] != 0) { len += 1; }
    return len;
}

export fn getSavegameVersion(checkstr: [*c]const u8) c_int {
    const maxlen = strlen_zig(checkstr);
    var versionStr: [4]u8 = [_]u8{ '0', '0', '0', 0 };
    var i: usize = 0;
    var j: usize = 0;
    while (j < maxlen and checkstr[j] != 0) {
        if (checkstr[j] >= '0' and checkstr[j] <= '9') {
            versionStr[i] = checkstr[j];
            i += 1;
            if (i == 3) break;
        }
        j += 1;
    }
    const versionNumber = std.fmt.parseInt(i32, versionStr[0..i], 10) catch return -1;
    if (versionNumber < 200 or versionNumber > 999) return -1;
    return versionNumber;
}
