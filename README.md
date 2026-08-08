# Voxagine + Bit Buster

[![Build](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml/badge.svg)](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml)

Voxagine is a custom C++ game engine built as a second-year project at
[IGAD](https://www.igad.nl/) (Breda University of Applied Sciences). It ships
with an ImGui-based level/entity editor and a couch co-op game, **Bit
Buster**, built on top of it.

It was originally a Windows/DirectX 12 engine. It now targets **Linux and
Windows** on a single Vulkan renderer; see [Port status](#port-status) for what
currently builds and what does not.

## Engine features

- **ECS** — entity/component/system architecture (`Voxagine/Source/Core/ECS`)
- **Vulkan renderer** (`Voxagine/Source/Core/Platform/Rendering`) — dynamic
  rendering, bindless textures, HLSL shaders compiled to SPIR-V with DXC
- **FMOD audio** (`Voxagine/Source/Core/Platform/Audio`)
- **Custom memory allocators** — pool/free-list allocators (`Voxagine/Source/Core/Memory`)
- **RTTR-based reflection & JSON serialization** for save/load and the editor's
  property inspector
- **In-engine editor** — entity hierarchy, inspector, entity wizard, undo/redo,
  snapping tools (`Voxagine/Source/Editor`)
- **Optick** integration for CPU profiling

## Requirements

- A C++17 compiler, CMake 3.21+ and Ninja
- Vulkan 1.3 headers, loader and drivers
- SDL3
- DXC, to compile the HLSL shaders to SPIR-V
- One or two gamepads to play Bit Buster

On Arch: `pacman -S cmake ninja vulkan-devel sdl3 directx-shader-compiler`.
Add `vulkan-validation-layers` for validation output.

On Windows: the Vulkan SDK supplies the loader, headers, validation layers and
DXC; SDL3 comes from vcpkg or a binary release. MSVC 2022 is the expected
compiler. The editor's file dialogs use Win32 `GetOpenFileName` there and
zenity or kdialog on Linux.

## Building & running

The game:

```bash
cmake -S . -B build-game -G Ninja -DVOXAGINE_BUILD_ENGINE=ON -DVOXAGINE_BUILD_EDITOR=OFF
cmake --build build-game
cd Game && ../build-game/bin/BitBuster    # run from Game/, asset paths are relative
```

The editor is the same executable built with `-DVOXAGINE_BUILD_EDITOR=ON`.

There is also `voxagine_bringup`, a standalone SDL3 + Vulkan target that clears
the screen without any of the engine. It is the default build and needs no
assets, which makes it the quickest check that a toolchain is set up:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/voxagine_bringup --frames 120
```

## Port status

**The game runs on Linux.** Bit Buster boots, renders the voxel world, sprites
and text, plays at ~200 fps, resizes, switches levels and exits cleanly, with
zero Vulkan validation errors.

**The editor builds and starts but hangs part-way through loading assets.**

**Windows is a supported target but is currently unverified.** DirectX 12 is
gone and the renderer is Vulkan-only, which runs on both platforms; SDL3 covers
the window, input and gamepads; the filesystem layer is plain C stdio; and the
one genuinely per-platform piece, the editor's file dialogs, has a Win32
implementation selected by CMake. What is missing is that nobody has actually
built it on Windows since the port began, so expect small breakages rather than
none. The MSBuild files are deleted for good — CMake is the only build system.

Off by default, and unchanged by the port:

| Dependency | State |
|------------|-------|
| FMOD | Proprietary; needs the SDK downloaded by hand. Off behind `VOXAGINE_ENABLE_FMOD`, and audio runs silent without it. |
| Optick | Vendored headers reference a Windows-only `OptickCore.lib`. Off behind `VOXAGINE_ENABLE_OPTICK`. |

RTTR was a third: it was vendored as 126 headers with no sources and only a
Windows `rttr_core.lib`. That copy is gone; CMake uses an installed RTTR if
there is one and otherwise fetches upstream v0.9.6, so the default build still
needs no network. `nativefiledialog` and `teenypath` were a fourth and fifth —
both shipped as a header plus a Windows `.lib` — and are now implemented in
this repository, on `std::filesystem` and the platform dialog APIs.

## Repository layout

| Path                    | Contents                                             |
|--------------------------|-------------------------------------------------------|
| `Voxagine/`              | Engine core + editor                                  |
| `Game/`                  | Bit Buster, built on Voxagine                         |
| `SplodyMcSplodeFace/`    | An earlier game built on an earlier version of the engine |
| `UnitTesting/`           | Unit tests (allocators, reflection, physics, pathfinding, lighting) |
| `cmake/`                 | Build helper scripts                                  |

## Status

This was completed as coursework and is no longer under active development,
apart from the port to Vulkan and Linux. It's kept here as a portfolio piece.

Contributions should stay portable: the engine targets Linux and Windows from
one source tree, so platform-specific code belongs behind `_WIN32` or a CMake
`if(WIN32)`, not in the shared path. `_WINDOWS` is not defined by this build —
it came from the deleted `.vcxproj` files.
