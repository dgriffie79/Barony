const c = @cImport({
    @cInclude("defs.h");
    @cInclude("SDL.h");
});

export var cursor_pencil = [_][*c]const u8{
    "    32    32        3            1",
    "X c #000000",
    ". c #ffffff",
    "  c None",
    "XXXXXX                          ",
    "X....XX                         ",
    "X...X..X                        ",
    "X..X....X                       ",
    "X.X......X                      ",
    "XX........X                     ",
    " X.........X                    ",
    "  X.........X                   ",
    "   X.........X                  ",
    "    X.........X                 ",
    "     X.......X.X                ",
    "      X.....X..XX               ",
    "       X...X..X..X              ",
    "        X.X..X...X              ",
    "         X..X....X              ",
    "          XX....X               ",
    "           X...X                ",
    "            XXX                 ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "0,0",
};

export var cursor_point = [_][*c]const u8{
    "    32    32        3            1",
    "X c #000000",
    ". c #ffffff",
    "  c None",
    "     XX                         ",
    "    X..X                        ",
    "    X..X                        ",
    "    X..X                        ",
    "    X..X                        ",
    "    X..XXX                      ",
    "    X..X..XXX                   ",
    "    X..X..X..XX                 ",
    "    X..X..X..X.X                ",
    "XXX X..X..X..X..X               ",
    "X..XX........X..X               ",
    "X...X...........X               ",
    "X...X...........X               ",
    " X..X...........X               ",
    "  X.X...........X               ",
    "  X.............X               ",
    "   X............X               ",
    "   X...........X                ",
    "    X..........X                ",
    "    X..........X                ",
    "     X........X                 ",
    "     X........X                 ",
    "     XXXXXXXXXX                 ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "5,0",
};

export var cursor_brush = [_][*c]const u8{
    "    32    32        3            1",
    "X c #000000",
    ". c #ffffff",
    "  c None",
    " XX                             ",
    " XX                             ",
    "X.X                             ",
    "X..X                            ",
    "X...XX                          ",
    "X.....XX                        ",
    "X......X                        ",
    " X....X.X                       ",
    " X...X..XX                      ",
    "  XXX..X..X                     ",
    "    XXX....X                    ",
    "     X......X                   ",
    "      X......X                  ",
    "       X......X                 ",
    "        X......X                ",
    "         X.....X                ",
    "          X.....X               ",
    "           XX....X              ",
    "             X....X             ",
    "              X....X            ",
    "               X...X            ",
    "                X..X            ",
    "                 XX             ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "1,0",
};

export var cursor_fill = [_][*c]const u8{
    "    32    32        3            1",
    "X c #000000",
    ". c #ffffff",
    "  c None",
    "          XXX                   ",
    "         XXXXXXXXXXX            ",
    "        XXXXXXXXXXXXXX          ",
    "       XXXX..XXX..XXXX          ",
    "      XXXX....XXXXXXX           ",
    "  XXXXXXXXXXXXXXXX              ",
    "XXXXXXXXXXXX....XXX             ",
    "XXXXXXX..........XXX            ",
    "XXXXXX...XXX......XXX           ",
    "XXXXXXX.XXXXX.....XXX           ",
    "XXXXXXXXXXXXXX....XX            ",
    "XXXXX XXXXXXX....XXX            ",
    " XXXX  XXXXX....XXX             ",
    "  XXX   XXX...XXXX              ",
    "  XXX    XXX.XXXX               ",
    "   XX     XXXXXX                ",
    "   XX      XXXX                 ",
    "   X        X                   ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "3,17",
};

pub export fn newCursor(image: [*c]const [*c]const u8) ?*c.SDL_Cursor {
    var data: [128]u8 = undefined;
    var mask: [128]u8 = undefined;
    var i: c_int = -1;
    for (0..32) |row| {
        for (0..32) |col| {
            if (col % 8 != 0) {
                const idx = @as(usize, @intCast(i));
                data[idx] <<= 1;
                mask[idx] <<= 1;
            } else {
                i += 1;
                data[@as(usize, @intCast(i))] = 0;
                mask[@as(usize, @intCast(i))] = 0;
            }
            const idx = @as(usize, @intCast(i));
            switch (image[4 + row][col]) {
                '.' => {
                    data[idx] |= 0x01;
                    mask[idx] |= 0x01;
                },
                'X' => mask[idx] |= 0x01,
                else => {},
            }
        }
    }
    var hot_x: c_int = 0;
    var hot_y: c_int = 0;
    _ = c.sscanf(image[4 + 32], "%d,%d", &hot_x, &hot_y);
    return c.SDL_CreateCursor(&data, &mask, 32, 32, hot_x, hot_y);
}
