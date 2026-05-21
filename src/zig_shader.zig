const c = @cImport({
    @cInclude("defs.h");
});

const std = @import("std");

fn createShaderFromStrings(typ: c.GLenum, strings: [*c][*c]const u8, count: c_int) c.GLuint {
    const shader = c.glCreateShader(typ);
    if (shader == 0) return 0;

    c.glShaderSource(shader, count, strings, null);
    c.glCompileShader(shader);

    var status: c.GLint = undefined;
    c.glGetShaderiv(shader, c.GL_COMPILE_STATUS, &status);

    if (status != 0) {
        return shader;
    }

    var log_buf: [1024]u8 = undefined;
    var log_len: c.GLsizei = 0;
    c.glGetShaderInfoLog(shader, @as(c.GLsizei, @intCast(log_buf.len)), &log_len, &log_buf);

    {
        var fmt_buf: [2048]u8 = undefined;
        const msg = std.fmt.bufPrintZ(
            &fmt_buf,
            "failed to compile shader: {s}",
            .{log_buf[0..@as(usize, @intCast(log_len))]},
        ) catch "failed to compile shader";
        c.printlog(msg.ptr);
    }

    c.glDeleteShader(shader);
    return 0;
}

fn createProgramFromShaders(
    shaders: [*c]const c.GLuint,
    count: c_int,
    attribs: [*c]const c.GLuint,
    attribNames: [*c][*c]const u8,
    attribCount: c_int,
) c.GLuint {
    const program = c.glCreateProgram();
    if (program == 0) return 0;

    {
        var i: usize = 0;
        while (i < @as(usize, @intCast(count))) : (i += 1) {
            c.glAttachShader(program, shaders[i]);
        }
    }

    {
        var i: usize = 0;
        while (i < @as(usize, @intCast(attribCount))) : (i += 1) {
            c.glBindAttribLocation(program, attribs[i], attribNames[i]);
        }
    }

    c.glLinkProgram(program);

    var status: c.GLint = undefined;
    c.glGetProgramiv(program, c.GL_LINK_STATUS, &status);

    if (status != 0) {
        c.printlog("linked shader program successfully");
        return program;
    }

    var log_buf: [1024]u8 = undefined;
    var log_len: c.GLsizei = 0;
    c.glGetProgramInfoLog(program, @as(c.GLsizei, @intCast(log_buf.len)), &log_len, &log_buf);

    {
        var fmt_buf: [2048]u8 = undefined;
        const msg = std.fmt.bufPrintZ(
            &fmt_buf,
            "failed to link shader program: {s}",
            .{log_buf[0..@as(usize, @intCast(log_len))]},
        ) catch "failed to link shader program";
        c.printlog(msg.ptr);
    }

    c.glDeleteProgram(program);
    return 0;
}

fn reloadShaderFromStrings(
    program: [*c]c.GLuint,
    shaderType: c.GLenum,
    strings: [*c][*c]const u8,
    count: c_int,
    attribs: [*c]const c.GLuint,
    attribNames: [*c][*c]const u8,
    attribCount: c_int,
) void {
    const newShader = createShaderFromStrings(shaderType, strings, count);
    if (newShader == 0) {
        c.printlog("failed to reload shader: compilation failed");
        return;
    }

    if (program.* != 0) {
        c.glDeleteProgram(program.*);
    }

    const shaders_arr = [1]c.GLuint{newShader};
    const newProgram = createProgramFromShaders(
        &shaders_arr,
        1,
        attribs,
        attribNames,
        attribCount,
    );

    if (newProgram != 0) {
        program.* = newProgram;
    } else {
        c.glDeleteShader(newShader);
        program.* = 0;
    }
}
