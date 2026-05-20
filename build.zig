const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // -----------------------------------------------------------------------
    // Build options
    // -----------------------------------------------------------------------
    const deps_root = b.option([]const u8, "deps_root", "Path to dependency root (e.g. C:\\dev\\barony-deps)") orelse "C:\\dev\\barony-deps";
    const fmod_enabled = b.option(bool, "fmod", "Enable FMOD audio") orelse false;
    const editor_enabled = b.option(bool, "editor", "Build editor executable") orelse true;
    const game_enabled = b.option(bool, "game", "Build game executable") orelse true;
    const datadir = b.option([]const u8, "datadir", "Base data directory path");

    // -----------------------------------------------------------------------
    // Config.hpp generation
    // -----------------------------------------------------------------------
    const config = b.addConfigHeader(.{
        .style = .{ .autoconf_at = b.path("src/Config.hpp.in") },
        .include_path = "Config.hpp",
    }, .{
        .FMOD = fmod_enabled,
        .NOT_DWORD_DEFINED = target.result.os.tag != .windows,
    });

    // -----------------------------------------------------------------------
    // Compiler flags
    // -----------------------------------------------------------------------
    const cxx_flags: []const []const u8 = &.{
        "-std=c++17",
        "-ffast-math",
        "-funroll-loops",
        "-fstrict-aliasing",
        "-Wno-pedantic",
        "-Wno-empty-body",
        "-Wno-string-plus-int",
        "-Wno-parentheses",
        "-Wno-format-security",
        "-Wno-multichar",
        "-Wno-inconsistent-missing-override",
        "-Wno-nontrivial-memcall",
        "-Wno-int-to-void-pointer-cast",
        "-Wno-void-pointer-to-int-cast",
        "-Wno-pointer-to-int-cast",
        "-Wno-date-time",
        "-Wno-format",

    };

    // -----------------------------------------------------------------------
    // Source file lists
    // -----------------------------------------------------------------------
    const game_sources = &.{ "src/main.cpp", "src/init.cpp", "src/collision.cpp", "src/light.cpp",
        "src/maps.cpp", "src/cursors.cpp", "src/draw.cpp", "src/opengl.cpp",
        "src/objects.cpp", "src/list.cpp", "src/files.cpp", "src/items.cpp",
        "src/paths.cpp", "src/charclass.cpp", "src/net.cpp", "src/game.cpp",
        "src/stat.cpp", "src/acttorch.cpp", "src/actplayer.cpp", "src/actmonster.cpp",
        "src/actitem.cpp", "src/acthudweapon.cpp", "src/actgold.cpp", "src/actgib.cpp",
        "src/actgeneral.cpp", "src/actdoor.cpp", "src/actladder.cpp", "src/input.cpp",
        "src/item_usage_funcs.cpp", "src/entity.cpp", "src/messages.cpp",
        "src/actflame.cpp", "src/actcampfire.cpp", "src/actfountain.cpp", "src/actsink.cpp",
        "src/monster_rat.cpp", "src/monster_goblin.cpp", "src/monster_slime.cpp",
        "src/monster_human.cpp", "src/monster_scorpion.cpp", "src/monster_succubus.cpp",
        "src/monster_troll.cpp", "src/monster_shopkeeper.cpp", "src/monster_skeleton.cpp",
        "src/monster_minotaur.cpp", "src/monster_ghoul.cpp", "src/monster_demon.cpp",
        "src/monster_spider.cpp", "src/monster_lich.cpp", "src/monster_imp.cpp",
        "src/monster_gnome.cpp", "src/monster_devil.cpp", "src/monster_automaton.cpp",
        "src/monster_cockatrice.cpp", "src/monster_crystalgolem.cpp", "src/monster_goatman.cpp",
        "src/monster_incubus.cpp", "src/monster_insectoid.cpp", "src/monster_kobold.cpp",
        "src/monster_lichfire.cpp", "src/monster_lichice.cpp", "src/monster_scarab.cpp",
        "src/monster_sentrybot.cpp", "src/monster_shadow.cpp", "src/monster_vampire.cpp",
        "src/monster_mimic.cpp", "src/monster_bugbear.cpp", "src/monster_bat.cpp",
        "src/shops.cpp", "src/mechanisms.cpp", "src/actgate.cpp", "src/actchest.cpp",
        "src/actsprite.cpp", "src/menu.cpp", "src/savepng.cpp", "src/actarrow.cpp",
        "src/actarrowtrap.cpp", "src/actboulder.cpp", "src/actheadstone.cpp",
        "src/actthrown.cpp", "src/actbeartrap.cpp", "src/actspeartrap.cpp",
        "src/actwallbuster.cpp", "src/book.cpp", "src/init_game.cpp", "src/prng.cpp",
        "src/scores.cpp", "src/achievements.cpp", "src/hash.cpp", "src/player.cpp",
        "src/entity_shared.cpp", "src/stat_shared.cpp", "src/item_tool.cpp",
        "src/actsummontrap.cpp", "src/actpowercrystal.cpp", "src/monster_shared.cpp",
        "src/actpedestal.cpp", "src/actteleporter.cpp", "src/actmagictrap.cpp",
        "src/mod_tools.cpp", "src/json.cpp",
        "src/lobbies.cpp", "src/shader.cpp",
        "src/monster_d.cpp", "src/monster_m.cpp", "src/monster_s.cpp", "src/monster_g.cpp",
        "src/monster_summons.cpp", "src/monster_moth.cpp", "src/monster_duck.cpp",
        "src/magic/magic.cpp", "src/magic/actmagic.cpp", "src/magic/spell.cpp",
        "src/magic/setupSpells.cpp", "src/magic/act_HandMagic.cpp", "src/magic/castSpell.cpp",
        "src/interface/interface.cpp", "src/interface/screenshot.cpp",
        "src/interface/clickdescription.cpp", "src/interface/consolecommand.cpp",
        "src/interface/drawminimap.cpp", "src/interface/drawstatus.cpp",
        "src/interface/magicgui.cpp", "src/interface/updatecharactersheet.cpp",
        "src/interface/updaterightsidebar.cpp", "src/interface/updatechestinventory.cpp",
        "src/interface/identify_and_appraise.cpp", "src/interface/playerinventory.cpp",
        "src/interface/bookgui.cpp", "src/interface/shopgui.cpp",
        "src/interface/removecurse.cpp", "src/interface/ui_general.cpp",
        "src/ui/Button.cpp", "src/ui/Field.cpp", "src/ui/Font.cpp",
        "src/ui/Frame.cpp", "src/ui/Image.cpp", "src/ui/Slider.cpp",
        "src/ui/Text.cpp", "src/ui/Widget.cpp", "src/ui/GameUI.cpp",
        "src/ui/LoadingScreen.cpp", "src/ui/MainMenu.cpp",
        "src/engine/audio/defines.cpp", "src/engine/audio/init_audio.cpp",
        "src/engine/audio/sound.cpp", "src/engine/audio/sound_game.cpp",
        "src/engine/audio/music.cpp",

    };

    const editor_sources = &.{
        "src/main.cpp", "src/init.cpp", "src/light.cpp", "src/buttons.cpp",
        "src/cursors.cpp", "src/draw.cpp", "src/opengl.cpp", "src/objects.cpp",
        "src/entity_editor.cpp", "src/input.cpp", "src/list.cpp", "src/hash.cpp",
        "src/files.cpp", "src/editor.cpp", "src/entity_shared.cpp",
        "src/stat_editor.cpp", "src/stat_shared.cpp", "src/items_editor.cpp",
        "src/json.cpp",
        "src/mod_tools.cpp", "src/prng.cpp", "src/shader.cpp",
        "src/ui/Button.cpp", "src/ui/Field.cpp", "src/ui/Font.cpp",
        "src/ui/Frame.cpp", "src/ui/Image.cpp", "src/ui/Slider.cpp",
        "src/ui/Text.cpp", "src/ui/Widget.cpp", "src/ui/LoadingScreen.cpp",
        "src/engine/audio/defines.cpp", "src/engine/audio/init_audio.cpp",
        "src/engine/audio/sound.cpp",
    };

    // -----------------------------------------------------------------------
    // Module setup
    // -----------------------------------------------------------------------
    const mod_options = std.Build.Module.CreateOptions{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    };

    const game_mod = b.createModule(mod_options);
    const editor_mod = b.createModule(mod_options);

    // Config.hpp include path (generated header)
    game_mod.addConfigHeader(config);
    editor_mod.addConfigHeader(config);

    // Local include paths
    game_mod.addIncludePath(b.path("src"));
    editor_mod.addIncludePath(b.path("src"));

    // Dependency include paths (absolute path via cwd_relative)
    {
        const inc_path = b.fmt("{s}/include", .{deps_root});
        game_mod.addSystemIncludePath(.{ .cwd_relative = inc_path });
        editor_mod.addSystemIncludePath(.{ .cwd_relative = inc_path });
        // SDL2 headers use both <SDL.h> and <SDL2/SDL.h> patterns
        const sdl_inc = b.fmt("{s}/include/SDL2", .{deps_root});
        game_mod.addSystemIncludePath(.{ .cwd_relative = sdl_inc });
        editor_mod.addSystemIncludePath(.{ .cwd_relative = sdl_inc });
    }

    // Platform defines
    game_mod.addCMacro("WINDOWS", "");
    editor_mod.addCMacro("WINDOWS", "");
    game_mod.addCMacro("_CRT_SECURE_NO_WARNINGS", "");
    editor_mod.addCMacro("_CRT_SECURE_NO_WARNINGS", "");
    game_mod.addCMacro("WIN32_LEAN_AND_MEAN", "");
    editor_mod.addCMacro("WIN32_LEAN_AND_MEAN", "");
    game_mod.addCMacro("GLEW_STATIC", "");
    editor_mod.addCMacro("GLEW_STATIC", "");

    // Add source files
    game_mod.addCSourceFiles(.{ .files = game_sources, .flags = cxx_flags });
    game_mod.addCSourceFile(.{ .file = b.path("src/cJSON.c"), .flags = &.{} });
    editor_mod.addCSourceFile(.{ .file = b.path("src/cJSON.c"), .flags = &.{} });

    const editor_flags = b.allocator.alloc([]const u8, cxx_flags.len + 1) catch @panic("OOM");
    for (cxx_flags, 0..) |flag, i| editor_flags[i] = flag;
    editor_flags[cxx_flags.len] = "-DEDITOR";
    editor_mod.addCSourceFiles(.{ .files = editor_sources, .flags = editor_flags });

    // -----------------------------------------------------------------------
    // Library linking helper
    // -----------------------------------------------------------------------
    const link_required = struct {
        fn apply(mod: *std.Build.Module, root: []const u8, os: std.Target.Os.Tag) void {
            const b2 = mod.owner;
            mod.addLibraryPath(.{ .cwd_relative = b2.fmt("{s}/lib", .{root}) });

            mod.linkSystemLibrary("SDL2", .{});
            mod.linkSystemLibrary("SDL2main", .{});
            mod.linkSystemLibrary("SDL2_image", .{});
            mod.linkSystemLibrary("SDL2_net", .{});
            mod.linkSystemLibrary("SDL2_ttf", .{});

            mod.linkSystemLibrary("physfs", .{});
            mod.linkSystemLibrary("png", .{});
            mod.linkSystemLibrary("z", .{});

            if (os == .windows) {
                mod.linkSystemLibrary("opengl32", .{});
                mod.linkSystemLibrary("ws2_32", .{});
                mod.linkSystemLibrary("wsock32", .{});
                mod.linkSystemLibrary("mingw32", .{});
            mod.linkSystemLibrary("glew32", .{});
            mod.linkSystemLibrary("gdi32", .{});
            mod.linkSystemLibrary("winmm", .{});
            mod.linkSystemLibrary("setupapi", .{});
            mod.linkSystemLibrary("ole32", .{});
            mod.linkSystemLibrary("oleaut32", .{});
            mod.linkSystemLibrary("imm32", .{});
            mod.linkSystemLibrary("version", .{});
            mod.linkSystemLibrary("rpcrt4", .{});
            mod.linkSystemLibrary("usp10", .{});
            mod.linkSystemLibrary("iphlpapi", .{});
            }
            mod.linkSystemLibrary("m", .{});
        }
    }.apply;

    // -----------------------------------------------------------------------
    // Game executable
    // -----------------------------------------------------------------------
    if (game_enabled) {
        const barony = b.addExecutable(.{
            .name = "barony",
            .root_module = game_mod,
        });

        barony.root_module.addWin32ResourceFile(.{ .file = b.path("barony.rc") });

        link_required(barony.root_module, deps_root, target.result.os.tag);
        // Optional libs
        if (fmod_enabled) barony.root_module.linkSystemLibrary("fmod", .{});

        if (datadir) |dd| {
            const escaped = std.mem.replaceOwned(u8, b.allocator, dd, "\\", "\\\\") catch @panic("OOM");
            barony.root_module.addCMacro("BASE_DATA_DIR", b.fmt("\"{s}\"", .{escaped}));
        } else {
            barony.root_module.addCMacro("BASE_DATA_DIR", "\"./\"");
        }

        b.installArtifact(barony);

        const run_step = b.step("run", "Run Barony");
        const run_barony = b.addRunArtifact(barony);
        if (datadir) |dd| run_barony.setCwd(.{ .cwd_relative = dd });
        // Add DLL search path for SDL2/GLEW/Png
        run_barony.addPathDir(b.fmt("{s}/bin", .{deps_root}));
        run_step.dependOn(&run_barony.step);
    }

    // -----------------------------------------------------------------------
    // Editor executable
    // -----------------------------------------------------------------------
    if (editor_enabled) {
        const editor = b.addExecutable(.{
            .name = "editor",
            .root_module = editor_mod,
        });

        editor.root_module.addWin32ResourceFile(.{ .file = b.path("editor.rc") });

        link_required(editor.root_module, deps_root, target.result.os.tag);
        // Optional libs
        if (fmod_enabled) editor.root_module.linkSystemLibrary("fmod", .{});

        if (datadir) |dd| {
            const escaped = std.mem.replaceOwned(u8, b.allocator, dd, "\\", "\\\\") catch @panic("OOM");
            editor.root_module.addCMacro("BASE_DATA_DIR", b.fmt("\"{s}\"", .{escaped}));
        } else {
            editor.root_module.addCMacro("BASE_DATA_DIR", "\"./\"");
        }

        b.installArtifact(editor);

        const run_editor_step = b.step("run-editor", "Run Barony Editor");
        const run_editor = b.addRunArtifact(editor);
        if (datadir) |dd| run_editor.setCwd(.{ .cwd_relative = dd });
        run_editor_step.dependOn(&run_editor.step);
    }
}
 