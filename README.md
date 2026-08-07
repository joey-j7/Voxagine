# Voxagine + Bit Buster

[![Build & Test](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml/badge.svg)](https://github.com/joey-j7/Voxagine/actions/workflows/build.yml)

Voxagine is a custom C++ game engine built as a second-year project at
[IGAD](https://www.igad.nl/) (Breda University of Applied Sciences). It ships
with an ImGui-based level/entity editor and a couch co-op game, **Bit
Buster**, built on top of it.

## Engine features

- **ECS** — entity/component/system architecture (`Voxagine/Source/Core/ECS`)
- **DirectX 12 renderer** (`Voxagine/Source/Core/Platform/Rendering`)
- **FMOD audio** (`Voxagine/Source/Core/Platform/Audio`)
- **Custom memory allocators** — pool/free-list allocators (`Voxagine/Source/Core/Memory`)
- **RTTR-based reflection & JSON serialization** for save/load and the editor's
  property inspector
- **In-engine editor** — entity hierarchy, inspector, entity wizard, undo/redo,
  snapping tools (`Voxagine/Source/Editor`)
- **Optick** integration for CPU profiling

## Requirements

- Windows 10/11
- Visual Studio 2017 or newer (v141 toolset)
- One or two Xbox controllers to play Bit Buster

## Building & running

1. Open `Voxagine.sln` in Visual Studio.
2. Set the startup project to **Game**.
3. Pick a build configuration — `*Editor` configs launch into the level
   editor, `*Game` configs launch straight into Bit Buster.
4. Build and run.

## Repository layout

| Path                    | Contents                                             |
|--------------------------|-------------------------------------------------------|
| `Voxagine/`              | Engine core + editor                                  |
| `Game/`                  | Bit Buster, built on Voxagine                         |
| `SplodyMcSplodeFace/`    | An earlier game built on an earlier version of the engine |
| `UnitTesting/`           | Unit tests (allocators, reflection, physics, pathfinding, lighting) |
| `BuildScripts/`          | Windows build/packaging scripts                      |
| `PS4/`                   | PS4 dev-kit project stub (content stripped for NDA reasons, inert) |

## Status

This was completed as coursework and is no longer under active development.
It's kept here as a portfolio piece.
