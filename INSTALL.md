# Dependencies

You will need the following libraries to build Barony:

 * SDL2 (https://www.libsdl.org/download-2.0.php)
 * SDL2_image (https://www.libsdl.org/projects/SDL_image/)
 * SDL2_net (https://www.libsdl.org/projects/SDL_net/)
 * SDL2_ttf (https://www.libsdl.org/projects/SDL_ttf/)
 * libpng (http://www.libpng.org/pub/png/libpng.html)
 * zlib (https://zlib.net/) used by libpng
 * PhysFS (https://icculus.org/physfs/)
 * RapidJSON (https://github.com/Tencent/rapidjson)
 * GLEW (https://glew.sourceforge.net/)

OPTIONAL dependencies:
 * FMOD Core API 2.02.14 (https://www.fmod.com/download)
 * Steamworks SDK (https://partner.steamgames.com/)
 * Epic Online Services SDK (https://dev.epicgames.com/)
 * PlayFab SDK
 * cURL
 * Opus audio codec
 * TheoraPlayer video
 * OpenAL audio

# Windows Instructions

## Acquire Dependencies

Place all dependencies in a single directory (e.g. `C:\dev\barony-deps`) with the following layout:
```
deps_root/
  include/        # headers (SDL2/, physfs.h, png.h, rapidjson/, etc.)
  lib/            # static libs (*.a) and DLL import libs (*.dll.a)
  bin/            # runtime DLLs (*.dll)
```

See `AGENTS.md` for detailed dependency build instructions using `zig cc`/`zig ar`.

## Building

```powershell
# Build ReleaseFast (working mode)
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast

# Build and run
zig build run -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast

# Build with optional features
zig build -Ddeps_root="C:\dev\barony-deps" -Ddatadir="path\to\game" -Doptimize=ReleaseFast -Dfmod -Dsteamworks
```

Runtime DLLs must be findable. Set `PATH` to include the deps bin directory:
```
$env:PATH += ";C:\dev\barony-deps\bin"
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

## build.bat / run.bat

Convenience scripts are provided at the repository root. Edit the paths at the top and run:
```
build.bat
run.bat
```
