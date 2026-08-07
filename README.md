# Voxagine + Bit Buster

[![Build](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml/badge.svg)](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml)

Voxagine is a custom C++ game engine built as a second-year project at
[IGAD](https://www.igad.nl/) (Breda University of Applied Sciences). It ships
with an ImGui-based level/entity editor and a couch co-op game, **Bit
Buster**, built on top of it.

It was originally a Windows/DirectX 12 engine. It is being ported to Linux on
Vulkan; see [Port status](#port-status) for what currently builds.

## Engine features

- **ECS** — entity/component/system architecture (`Voxagine/Source/Core/ECS`)
- **Vulkan renderer** (`Voxagine/Source/Core/Platform/Rendering`) — in progress
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
- One or two gamepads to play Bit Buster

On Arch: `pacman -S cmake ninja vulkan-devel sdl3`. Add
`vulkan-validation-layers` for validation output.

## Building & running

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/voxagine_bringup
```

`voxagine_bringup` opens an SDL3 window and drives the Vulkan backend to a
clear screen. Pass `--frames N` to exit after N frames, `--no-validation` to
skip the validation layer.

## Port status

Building and running today:

- `voxagine_vulkan` — instance/device/swapchain, per-frame sync, clear + present
- `voxagine_bringup` — SDL3 window driving the above

Not yet building (`-DVOXAGINE_BUILD_ENGINE=ON` is off by default). The engine
library is blocked on vendored dependencies that only exist here as Windows
binaries:

| Dependency | State |
|------------|-------|
| RTTR | Headers only, no sources and no Linux library. 63 engine files use it. CMake fetches upstream v0.9.6 when the engine target is enabled. |
| FMOD | Proprietary; needs the Linux SDK downloaded by hand. Off behind `VOXAGINE_ENABLE_FMOD`. |
| Optick | Vendored headers reference a Windows-only `OptickCore.lib`. Off behind `VOXAGINE_ENABLE_OPTICK`. |
| nativefiledialog | Needs the GTK backend from upstream. Off behind `VOXAGINE_ENABLE_NFD`. |
| teenypath | Replaceable with `std::filesystem`. |

The render passes, resource managers and `RenderContext` itself still need
Vulkan implementations; `RenderDefines.h` no longer names any graphics API, so
that work is now a matter of adding `VK*` classes behind the existing aliases.

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
apart from the Linux/Vulkan port. It's kept here as a portfolio piece.
