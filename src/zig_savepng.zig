const c = @cImport({
    @cInclude("SDL.h");
    @cInclude("png.h");
    @cInclude("stdio.h");
});

const SUCCESS: c_int = 0;
const ERROR: c_int = -1;

const rmask: u32 = if (c.SDL_BYTEORDER == c.SDL_BIG_ENDIAN) 0xFF000000 else 0x000000FF;
const gmask: u32 = if (c.SDL_BYTEORDER == c.SDL_BIG_ENDIAN) 0x00FF0000 else 0x0000FF00;
const bmask: u32 = if (c.SDL_BYTEORDER == c.SDL_BIG_ENDIAN) 0x0000FF00 else 0x00FF0000;
const amask: u32 = if (c.SDL_BYTEORDER == c.SDL_BIG_ENDIAN) 0x000000FF else 0xFF000000;

pub export fn SDL_SavePNG(surface: ?*c.SDL_Surface, filename: [*c]const u8) c_int {
    const file = c.SDL_RWFromFile(filename, "wb");
    if (file != null) {
        return SDL_SavePNG_RW(surface, file, 1);
    }
    return -1;
}

fn png_error_SDL(_png_ptr: c.png_structp, str: c.png_const_charp) callconv(.c) void {
    _ = _png_ptr;
    _ = c.SDL_SetError("libpng: %s\n", str);
}

fn png_write_SDL(png_ptr: c.png_structp, data: c.png_bytep, length: c.png_size_t) callconv(.c) void {
    const rw = @as(?*c.SDL_RWops, @ptrCast(c.png_get_io_ptr(png_ptr)));
    _ = c.SDL_RWwrite(rw, @as(?*const anyopaque, @ptrCast(data)), @sizeOf(c.png_byte), length);
}

pub export fn SDL_PNGFormatAlpha(src: ?*c.SDL_Surface) ?*c.SDL_Surface {
    if (src.?.format.*.BitsPerPixel <= 24 or src.?.format.*.Amask != 0) {
        const ud = if (src.?.userdata) |u| @intFromPtr(u) else @as(usize, 0);
        src.?.userdata = @ptrFromInt(ud + 1);
        return src;
    }

    var rect: c.SDL_Rect = .{ .x = 0, .y = 0, .w = src.?.w, .h = src.?.h };
    const surf = c.SDL_CreateRGBSurface(
        src.?.flags, src.?.w, src.?.h, 24,
        src.?.format.*.Rmask, src.?.format.*.Gmask, src.?.format.*.Bmask, 0,
    );
    _ = c.SDL_LowerBlit(src, &rect, surf, &rect);
    return surf;
}

pub export fn SDL_SavePNG_RW(surface: ?*c.SDL_Surface, dst: ?*c.SDL_RWops, freedst: c_int) c_int {
    if (dst == null) {
        _ = c.SDL_SetError("Argument 2 to SDL_SavePNG_RW can't be NULL, expecting SDL_RWops*\n");
        if (freedst != 0) _ = c.SDL_RWclose(dst);
        return ERROR;
    }
    if (surface == null) {
        _ = c.SDL_SetError("Argument 1 to SDL_SavePNG_RW can't be NULL, expecting SDL_Surface*\n");
        if (freedst != 0) _ = c.SDL_RWclose(dst);
        return ERROR;
    }

    const png_ptr = c.png_create_write_struct(c.PNG_LIBPNG_VER_STRING, null, png_error_SDL, null);
    if (png_ptr == null) {
        _ = c.SDL_SetError("Unable to png_create_write_struct on %s\n", c.PNG_LIBPNG_VER_STRING);
        if (freedst != 0) _ = c.SDL_RWclose(dst);
        return ERROR;
    }

    const info_ptr = c.png_create_info_struct(png_ptr);
    if (info_ptr == null) {
        _ = c.SDL_SetError("Unable to png_create_info_struct\n");
        c.png_destroy_write_struct(@as([*c]c.png_structp, @ptrCast(@constCast(&png_ptr))), null);
        if (freedst != 0) _ = c.SDL_RWclose(dst);
        return ERROR;
    }

    if (c.setjmp(c.png_jmpbuf(png_ptr)) != 0) {
        c.png_destroy_write_struct(&png_ptr, &info_ptr);
        if (freedst != 0) _ = c.SDL_RWclose(dst);
        return ERROR;
    }

    c.png_set_write_fn(png_ptr, @as(?*anyopaque, @ptrCast(dst)), png_write_SDL, null);

    var colortype: c_int = c.PNG_COLOR_MASK_COLOR;
    if (surface.?.format.*.BytesPerPixel > 0 and
        surface.?.format.*.BytesPerPixel <= 8 and
        surface.?.format.*.palette) |pal|
    {
        colortype |= c.PNG_COLOR_MASK_PALETTE;
        const pal_size = @as(usize, @intCast(pal.ncolors)) * @sizeOf(c.png_color);
        const pal_ptr = @as([*c]c.png_color, @ptrCast(c.malloc(pal_size).?));
        for (0..@as(usize, @intCast(pal.ncolors))) |i| {
            pal_ptr[i].red = pal.colors[i].r;
            pal_ptr[i].green = pal.colors[i].g;
            pal_ptr[i].blue = pal.colors[i].b;
        }
        c.png_set_PLTE(png_ptr, info_ptr, pal_ptr, pal.ncolors);
        c.free(pal_ptr);
    } else if (surface.?.format.*.BytesPerPixel > 3 or surface.?.format.*.Amask != 0) {
        colortype |= c.PNG_COLOR_MASK_ALPHA;
    }

    c.png_set_IHDR(
        png_ptr, info_ptr, surface.?.w, surface.?.h, 8, colortype,
        c.PNG_INTERLACE_NONE, c.PNG_COMPRESSION_TYPE_DEFAULT, c.PNG_FILTER_TYPE_DEFAULT,
    );

    if (surface.?.format.*.Rmask == bmask and
        surface.?.format.*.Gmask == gmask and
        surface.?.format.*.Bmask == rmask)
    {
        c.png_set_bgr(png_ptr);
    }

    c.png_write_info(png_ptr, info_ptr);

    const row_bytes = @as(usize, @intCast(surface.?.h)) * @sizeOf(c.png_bytep);
    const row_pointers = @as([*c]c.png_bytep, @ptrCast(c.malloc(row_bytes).?));
    for (0..@as(usize, @intCast(surface.?.h))) |i| {
        const offset = i * @as(usize, @intCast(surface.?.pitch));
        row_pointers[i] = @as(c.png_bytep, @ptrCast(@as([*c]u8, @ptrCast(surface.?.pixels)) + offset));
    }
    c.png_write_image(png_ptr, row_pointers);
    c.free(row_pointers);

    c.png_write_end(png_ptr, info_ptr);
    c.png_destroy_write_struct(&png_ptr, &info_ptr);
    if (freedst != 0) _ = c.SDL_RWclose(dst);
    return SUCCESS;
}
