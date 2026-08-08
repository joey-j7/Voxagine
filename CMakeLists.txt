cmake_minimum_required(VERSION 3.21)

project(Voxagine
    VERSION 0.1.0
    DESCRIPTION "Voxagine engine and Bit Buster"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Options
#
# VOXAGINE_BUILD_ENGINE defaults off so a plain configure stays offline-capable
# (no RTTR fetch). Use the game/editor presets in CMakePresets.json to build
# BitBuster.
# ---------------------------------------------------------------------------
option(VOXAGINE_BUILD_BRINGUP "Build the Vulkan bring-up target" ON)
option(VOXAGINE_BUILD_ENGINE  "Build the engine library" OFF)
option(VOXAGINE_BUILD_EDITOR  "Build the game with the editor (EDITOR define)" ON)
option(VOXAGINE_FETCH_RTTR    "Fetch RTTR from upstream instead of find_package" ON)
option(VOXAGINE_CHECK_RTTR    "Compile the reflection TU to prove RTTR works" OFF)
option(VOXAGINE_ENABLE_FMOD   "Enable FMOD audio (needs the FMOD Linux SDK)" OFF)
option(VOXAGINE_ENABLE_OPTICK "Enable Optick profiling" OFF)
option(VOXAGINE_ENABLE_NFD    "Enable native file dialogs" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(VOXAGINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Voxagine/Source)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

# RTTR 0.9.6 is what this engine was written against, and its reflection macros
# are load-bearing for save/load and the editor's property inspector. It used
# to live in External/rttr as headers only, with no sources and only a Windows
# .lib to link - which is why nothing here could build on Linux. That copy is
# gone; includes are plain <rttr/...> and resolve to whatever this provides.
#
# Only pulled in when something actually needs it, so the default bring-up
# build stays offline-capable.
if(VOXAGINE_BUILD_ENGINE OR VOXAGINE_CHECK_RTTR)
    include(cmake/RTTR.cmake)
endif()

find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    add_library(SDL3::SDL3 ALIAS PkgConfig::SDL3)
endif()

# ---------------------------------------------------------------------------
# Warning set shared by our own targets. Vendored code is included as SYSTEM
# so its warnings stay out of the way.
# ---------------------------------------------------------------------------
# MSVC defines _DEBUG automatically in debug configurations and the engine
# leans on it in ~90 places (debug draw, validation, asserts). GCC does not, so
# a Debug build here would silently behave like a release one.
add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)


add_library(voxagine_warnings INTERFACE)
target_compile_options(voxagine_warnings INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall;-Wextra;-Wno-unused-parameter>
)

# ---------------------------------------------------------------------------
# Vulkan backend
#
# No engine dependencies by design, so it can be built and run before the rest
# of Core compiles on Linux.
# ---------------------------------------------------------------------------
add_library(voxagine_vulkan STATIC
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKAllocator.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKCommandEngine.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKDescriptorLayout.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKDevice.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKPassBindings.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKResource.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKSwapchain.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKUploadBuffer.cpp
)

target_include_directories(voxagine_vulkan PUBLIC ${VOXAGINE_SOURCE_DIR})
target_link_libraries(voxagine_vulkan
    PUBLIC Vulkan::Vulkan
    PRIVATE voxagine_warnings
)

# ---------------------------------------------------------------------------
# Shaders
# ---------------------------------------------------------------------------
include(cmake/Shaders.cmake)

# SPIR-V lands next to the HLSL it came from, because the engine asks for
# shaders by asset path - "Engine/Assets/Shaders/Particles.vs" - and appends
# .spv. Keeping them in the build tree would mean the runtime could not find
# them. They are build output, so .gitignore excludes *.spv.
voxagine_add_shaders(voxagine_shaders_game
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Game/Engine/Assets/Shaders
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Game/Engine/Assets/Shaders
)

# ---------------------------------------------------------------------------
# Bring-up target: SDL3 window + Vulkan clear screen
# ---------------------------------------------------------------------------
if(VOXAGINE_BUILD_BRINGUP)
    add_executable(voxagine_bringup
        ${VOXAGINE_SOURCE_DIR}/Bringup/VulkanBringup.cpp
    )

    target_link_libraries(voxagine_bringup
        PRIVATE voxagine_vulkan SDL3::SDL3 voxagine_warnings
    )
endif()

# ---------------------------------------------------------------------------
# Engine library
#
# Off by default. Still blocked on:
#   FMOD    - proprietary, needs the Linux SDK downloaded by hand.
#   Optick  - vendored headers reference a Windows-only OptickCore.lib.
#   nfd     - needs the GTK backend built from upstream source.
#   teenypath - replaceable with std::filesystem.
# and on the Vulkan render context, managers and passes not existing yet.
# ---------------------------------------------------------------------------
if(VOXAGINE_BUILD_ENGINE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineSources.cmake)

    add_library(voxagine STATIC ${VOXAGINE_ENGINE_SOURCES})

    # The vendored STB .c files open with #include "pch.h", so they are only
    # valid as C++. The old build compiled every file in the project as C++
    # with a forced PCH; CMake picks the language from the extension.
    set_source_files_properties(
        ${VOXAGINE_SOURCE_DIR}/External/STB/image_DXT.c
        ${VOXAGINE_SOURCE_DIR}/External/STB/image_helper.c
        ${VOXAGINE_SOURCE_DIR}/External/STB/stb_image_aug.c
        PROPERTIES LANGUAGE CXX
    )

    target_include_directories(voxagine PUBLIC ${VOXAGINE_SOURCE_DIR})

    target_link_libraries(voxagine
        PUBLIC voxagine_vulkan SDL3::SDL3 RTTR::Core
        PRIVATE voxagine_warnings
    )

    # GetOpenFileName, for the editor's file dialogs.
    if(WIN32)
        target_link_libraries(voxagine PRIVATE comdlg32)
    endif()

    # No target_precompile_headers here on purpose.
    #
    # Every engine TU already starts with #include "pch.h". CMake's PCH support
    # force-includes its own cmake_pch.h on top of that, so headers reached
    # both ways get pulled in twice and #pragma once does not save them -
    # moodycamel's concurrentqueue.h then reports every declaration as a
    # redefinition of itself. Adopting CMake's PCH means removing the explicit
    # includes from all 147 TUs first; not worth doing mid-port.

    if(VOXAGINE_ENABLE_FMOD)
        # The real FMOD implementations replace the silent stubs, which define
        # the same symbols.
        list(REMOVE_ITEM VOXAGINE_ENGINE_SOURCES
            ${VOXAGINE_SOURCE_DIR}/Core/Platform/Audio/NullAudioContext.cpp
            ${VOXAGINE_SOURCE_DIR}/Core/Resources/Formats/NullSoundReference.cpp
        )
        target_sources(voxagine PRIVATE ${VOXAGINE_FMOD_SOURCES})
        target_compile_definitions(voxagine PUBLIC VOXAGINE_FMOD)
    endif()

    if(VOXAGINE_ENABLE_OPTICK)
        target_compile_definitions(voxagine PUBLIC USE_OPTICK=1)
    else()
        target_compile_definitions(voxagine PUBLIC USE_OPTICK=0)
    endif()

    if(VOXAGINE_ENABLE_NFD)
        target_compile_definitions(voxagine PUBLIC VOXAGINE_NFD)
    endif()

    # PUBLIC, and on the library: Editor.cpp and the #ifdef EDITOR branch of
    # Application::Run are both library sources, so defining this only on the
    # executable compiled the whole editor out.
    if(VOXAGINE_BUILD_EDITOR)
        target_compile_definitions(voxagine PUBLIC EDITOR)
    endif()
endif()

# ---------------------------------------------------------------------------
# Bit Buster
#
# EDITOR selects the editor build; the old .sln expressed this as separate
# *Editor and *Game configurations.
# ---------------------------------------------------------------------------
if(VOXAGINE_BUILD_ENGINE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GameSources.cmake)

    add_executable(BitBuster ${VOXAGINE_GAME_SOURCES})

    target_include_directories(BitBuster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source)
    target_link_libraries(BitBuster PRIVATE voxagine voxagine_warnings)

    # EDITOR itself is defined on the voxagine library above, PUBLIC, so both
    # the library and this executable see it.

    add_dependencies(BitBuster voxagine_shaders_game)
endif()

# ---------------------------------------------------------------------------
# Reflection check
#
# Core/Settings.cpp is the smallest real engine TU that exercises RTTR the way
# the rest of the engine does: an enum registration plus member-pointer and
# getter/setter properties. Compiling it proves the RTTR dependency is usable
# on this platform while the full engine target is still blocked on FMOD and
# the Vulkan render context. Off by default because it needs the network.
# ---------------------------------------------------------------------------
if(VOXAGINE_CHECK_RTTR AND NOT VOXAGINE_BUILD_ENGINE)
    add_library(voxagine_reflection_check OBJECT
        ${VOXAGINE_SOURCE_DIR}/Core/Settings.cpp
    )

    target_include_directories(voxagine_reflection_check PRIVATE ${VOXAGINE_SOURCE_DIR})
    target_link_libraries(voxagine_reflection_check PRIVATE RTTR::Core)
endif()

message(STATUS "Voxagine: bring-up=${VOXAGINE_BUILD_BRINGUP} engine=${VOXAGINE_BUILD_ENGINE} rttr-check=${VOXAGINE_CHECK_RTTR}")
