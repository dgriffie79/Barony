# Barony — WASM Conversion Plan

## Goal
- Convert the Barony codebase (~487K lines) from C++ to Zig for WASM deployment
- Keep FMOD audio working at every stage (C++ API → C API → called from Zig → Web Audio for WASM)
- Skip C intermediate: convert C++→Zig directly using `@cImport` for C header access and `export fn` + `extern "C"` linkage to bridge between languages

## Phase Status

| Phase | Description | Status |
|-------|-------------|--------|
| 1-7 | Platform stripping, SDK removal, RapidJSON removal | ✅ Complete |
| 8 | cJSON integration | ✅ Complete |
| 9.0 | main.hpp → defs.h + game_types.hpp + main.h | ✅ Complete |
| 9.1-9.6 | ccontainers, FMOD C API, headers conversion, extern "C" wrappers | ✅ Complete |
| 9.7 | extern "C" wrappers in all core headers | ✅ Complete |
| 10 | Language extraction from game_types.hpp | ✅ Complete |
| 11 | C++→Zig bridge proof (7 Zig files, 21 functions) | ✅ Complete |
| 12a-c | first wave Zig files (utils, entity, collision, mechanisms, item, objects, scores) | ✅ Complete |
| 12d | Remaining C file conversion (hash, savepng, shader, cursors, prng, defines) | ❌ **Blocked** — @cImport can't handle SDL/libpng/OpenGL macro-heavy headers; translate-c produces too much noise (109 @compileError lines). These 6 C files are kept as system-API glue layer. |
| 13 | STL container replacement (std::set, std::map → Zig AutoHashMap/IntHashSet) | ✅ **map_t fully C-compatible** — liquidSfxPlayedTiles, tileAttributes, entities_map converted |
| 14 | Header conversion (.hpp → .h) | ⏳ Pending |
| 15 | Full .cpp → .zig conversion | ⏳ Pending |
| 16 | WASM target | ⏳ Pending |

## Current Project State

| Language | File Count | Status |
|----------|-----------|--------|
| **Zig** 🦎 | 10 files (8 active + 2 empty) | ✅ Working (utils, entity, item, objects, scores, set, map) |
| **C** 🔧 | 6 files (hash, savepng, shader, cursors, prng, defines) | ✅ Working (system-API glue layer) |
| **C++** 📦 | 148 files | ✅ Working (map_t now STL-free; Entity/Stat/Item STL containers opaque-ized) |
| **Headers** | 25 dual-mode .h + ~30 .hpp | ✅ Working |

## Build Commands

```powershell
# Debug (may crash due to safety checks)
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game"

# ReleaseFast (working)
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast

# Run
zig build run -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast
```

DLLs must be in PATH: `$env:PATH += ";C:\dev\barony-deps\bin"`

## Key Decisions

1. **C++→Zig directly** — Skip C intermediate. Zig's @cImport reads C-compatible headers natively.
2. **extern "C" linkage** — Every converted function declared `export fn` in Zig, matching `extern "C"` in headers.
3. **6 C files as glue layer** — SDL, libpng, OpenGL/GLEW headers are too complex for @cImport. translate-c produces 3803 lines with 109 @compileError markers. These 6 files stay as C.
4. **Per-file Zig modules** — Each module is a separate .zig file, imported via zig_root.zig.

## Known Issues

1. **Debug mode crashes** — Zig safety checks fire on benign null-pointer-offset arithmetic.
2. **@cImport limitations** — Works for our C-compatible headers (entity.h, defs.h) but fails on libpng/SDL/GLEW macro-heavy headers.
3. **Zig 0.16 quirks** — `callconv(.C)` must be lowercase `.c`. `[*c]` pointer semantics differ from `*`.
4. **Entity struct layout mismatch** — The dual-mode `Entity` in `entity.h` has `void* dithering` (8 bytes) in the C struct vs `std::unordered_map<view_t*, Dither>` (~48 bytes) in the C++ class, causing a ~40-byte offset shift. **Fixed**: `dithering` is now `void*` in both C++ and C structs (heap-allocated map). Similarly `bodyparts` (`std::vector<Entity*>` → `void*`) and `collisionIgnoreTargets` (`std::set<Uint32>` → `void*`) have been heap-allocated. The C++ and C structs now have identical layouts throughout. Any Zig function reading Entity fields via @cImport will get correct data.
