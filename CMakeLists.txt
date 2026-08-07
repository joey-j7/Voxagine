cmake_minimum_required(VERSION 3.21)

project(Voxagine
    VERSION 0.1.0
    DESCRIPTION "Voxagine engine and Bit Buster"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Options
#
# The Linux/Vulkan port is in progress. VOXAGINE_BUILD_ENGINE stays OFF until
# the vendored Windows-only dependencies below have Linux equivalents.
# ---------------------------------------------------------------------------
option(VOXAGINE_BUILD_BRINGUP "Build the Vulkan bring-up target" ON)
option(VOXAGINE_BUILD_ENGINE  "Build the engine library (needs RTTR)" OFF)
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
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKDevice.cpp
    ${VOXAGINE_SOURCE_DIR}/Core/Platform/Rendering/Vulkan/VKSwapchain.cpp
)

target_include_directories(voxagine_vulkan PUBLIC ${VOXAGINE_SOURCE_DIR})
target_link_libraries(voxagine_vulkan
    PUBLIC Vulkan::Vulkan
    PRIVATE voxagine_warnings
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
# Off by default. Blocked on:
#   RTTR    - vendored as headers only (Voxagine/Source/External/rttr, no .cpp
#             and no Linux library); 63 engine files depend on it. Fetched from
#             upstream below when the engine target is enabled.
#   FMOD    - proprietary, needs the Linux SDK downloaded by hand.
#   Optick  - vendored headers reference a Windows-only OptickCore.lib.
#   nfd     - needs the GTK backend built from upstream source.
#   teenypath - replaceable with std::filesystem.
# ---------------------------------------------------------------------------
if(VOXAGINE_BUILD_ENGINE)
    include(FetchContent)

    FetchContent_Declare(rttr
        GIT_REPOSITORY https://github.com/rttrorg/rttr.git
        GIT_TAG        v0.9.6
    )

    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
    set(BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
    set(BUILD_PACKAGE OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(rttr)

    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineSources.cmake)

    add_library(voxagine STATIC ${VOXAGINE_ENGINE_SOURCES})

    target_include_directories(voxagine PUBLIC ${VOXAGINE_SOURCE_DIR})

    target_link_libraries(voxagine
        PUBLIC voxagine_vulkan SDL3::SDL3 RTTR::Core
        PRIVATE voxagine_warnings
    )

    target_precompile_headers(voxagine PRIVATE ${VOXAGINE_SOURCE_DIR}/pch.h)

    if(VOXAGINE_ENABLE_FMOD)
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
endif()

message(STATUS "Voxagine: bring-up=${VOXAGINE_BUILD_BRINGUP} engine=${VOXAGINE_BUILD_ENGINE}")
