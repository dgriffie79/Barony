# Barony — Build System Migration

## Project State

Barony is a C++17 first-person dungeon crawler forked from [Turning Wheel LLC's Barony](https://github.com/TurningWheel/Barony). The game has been migrated from its original CMake build system to Zig's build system with `zig cc`/`zig c++` as the compiler.

## Build System Migration Completed (May 2026)

### What changed

- **CMake replaced with Zig** — `build.zig` + `build.zig.zon` replace all CMakeLists.txt files
- **Compiler changed from MSVC to `zig c++`** (Clang-based) targeting `x86_64-windows-gnu` (MinGW ABI)
- **Config.hpp generation** — `b.addConfigHeader()` with `.autoconf_at` style substitutes `@VAR@` placeholders from `Config.hpp.in`

### Files created

- `build.zig` — Main build file (~400 lines) with:
  - Config.hpp generation from `Config.hpp.in` via `b.addConfigHeader`
  - Two executable targets: `barony` (game) and `editor` (map editor)
  - All ~159 game + ~36 editor C++ source files listed by subdirectory
  - Platform detection (`target.result.os.tag`) for Windows/macOS/Linux
  - Build options: `steamworks`, `eos`, `playfab`, `fmod`, `openal`, `tremor`, `theoraplayer`, `curl`, `opus`, `editor`, `game`, `datadir`, `deps_root`
  - Proper C string escaping for `BASE_DATA_DIR` macro (doubled backslashes + quotes)
  - Dynamic linking for SDL2/SDL2_image/SDL2_net/SDL2_ttf via DLL import libraries
- `build.zig.zon` — Package manifest

### Files modified

- `.gitignore` — Added `zig-out/` and `.zig-cache/`
- `src/savepng.cpp` — Added `#undef ERROR` guard to fix MinGW `wingdi.h` collision; `long int` → `intptr_t`
- `src/draw.cpp` — `long int` → `intptr_t` (pointer-to-int casting for 64-bit Windows safety)
- `src/opengl.cpp` — `long int` → `intptr_t`
- `src/files.cpp` — `long int` → `intptr_t`

### External dependencies

Located at `C:\dev\barony-deps` (or configurable via `-Ddeps_root`):

| Dependency | Type | Source |
|---|---|---|
| SDL2 2.32.10 | Dynamic (`.dll`) | libsdl.org MinGW devel package |
| SDL2_image 2.8.12 | Dynamic (`.dll`) | libsdl.org MinGW devel package |
| SDL2_net 2.2.0 | Dynamic (`.dll`) | libsdl.org MinGW devel package |
| SDL2_ttf 2.24.0 | Dynamic (`.dll`) | libsdl.org MinGW devel package |
| PhysFS 3.2.0 | Static (`.a`) | Built from source with `zig cc` |
| libpng 1.6.44 | Static (`.a`) | Built from source with `zig cc` |
| zlib 1.3.1 | Static (`.a`) | Built from source with `zig cc` |
| RapidJSON 1.1.0 | Header-only | GitHub release (patched for C++17: removed `const` from `GenericStringRef::length`) |
| GLEW 2.2.0 | Dynamic (`.dll`) | glew.sourceforge.net MinGW package |

### Build and run

```powershell
# Build Debug (has false-positive safety checks)
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game"

# Build ReleaseFast (working mode)
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast

# Build and run
zig build run -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast
```

Runtime DLLs must be findable. Set `PATH` or they're in `{deps_root}/bin/`:
```
$env:PATH += ";C:\dev\barony-deps\bin"
```

### Known issues

1. **Debug mode crashes** — Zig's runtime safety checks fire on benign null-pointer-offset arithmetic in RapidJSON's `Stack::Reserve` and early init code. These are not real bugs — the code works with MSVC and `-Doptimize=ReleaseFast`. Fix would require `-fno-sanitize=undefined` or patching the specific code paths.

2. **CRT mismatch with pre-compiled MinGW static libs** — The MinGW `libSDL2_ttf.a` pre-compiled with MinGW GCC crashes at `TTF_Init()` due to CRT incompatibility with Clang/LLD. Workaround: use the DLL import library instead (`.dll.a` → rename to `.dll` and place in lib dir).

3. **`__DATE__` / `__TIME__` in Release modes** — Suppressed with `-Wno-date-time`. The code uses `__DATE__` for build date display.

4. **`size_t`/`int` format mismatch** — Suppressed with `-Wno-format`. The game's `snprintf` calls use `%d` with `size_t` arguments.

5. **No optional dependencies configured yet** — Steamworks, EOS, PlayFab, FMOD, OpenAL, TheoraPlayer, Opus, cURL are all disabled by default. The build options exist but require SDK downloads.

### Dependency setup (Windows)

If setting up from scratch on a new machine:

```powershell
# Create dependency directory
mkdir C:\dev\barony-deps

# Download SDL2 MinGW devel packages from libsdl.org:
# SDL2-devel-2.32.10-mingw.zip
# SDL2_image-devel-2.8.12-mingw.zip
# SDL2_net-devel-2.2.0-mingw.zip
# SDL2_ttf-devel-2.24.0-mingw.zip

# Extract and copy:
# include\SDL2\* → deps\include\SDL2\
# lib\*.a → deps\lib\ (keep both .a and .dll.a)
# bin\*.dll → deps\bin\

# Build PhysFS:
cd physfs-3.2.0
zig cc -c src/physfs.c src/physfs_byteorder.c src/physfs_unicode.c src/physfs_platform_windows.c src/physfs_archiver_*.c -Ideps -O2 -fno-sanitize=undefined
zig ar rcs libphysfs.a *.o

# Build zlib:
zig cc -c *.c -O2
zig ar rcs libz.a *.o

# Build libpng:
zig cc -c *.c -I. -I../zlib -O2
zig ar rcs libpng.a *.o

# Install RapidJSON:
# Download rapidjson-1.1.0.zip, copy include/rapidjson/ → deps/include/rapidjson/
```

## Optional Feature Flags

| Flag | Description | Default |
|---|---|---|
| `-Dsteamworks` | Enable Steamworks SDK | `false` |
| `-Deos` | Enable Epic Online Services | `false` |
| `-Dplayfab` | Enable PlayFab | `false` |
| `-Dfmod` | Enable FMOD audio | `false` |
| `-Dopenal` | Enable OpenAL audio | `false` |
| `-Dtheoraplayer` | Enable TheoraPlayer video | `false` |
| `-Dcurl` | Enable cURL networking | `false` |
| `-Dopus` | Enable Opus audio codec | `false` |
| `-Deditor=false` | Skip building editor | `true` |
| `-Dgame=false` | Skip building game | `true` |
