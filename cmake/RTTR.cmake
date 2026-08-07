# Provides the RTTR::Core target.
#
# Prefers an installed RTTR so offline and packaged builds work; otherwise
# fetches the v0.9.6 tag, which is the version the engine's headers were
# vendored from and the one verified against GCC 16 / Clang 22.

set(VOXAGINE_RTTR_TAG v0.9.6)

if(NOT VOXAGINE_FETCH_RTTR)
    find_package(rttr CONFIG REQUIRED)
    return()
endif()

find_package(rttr CONFIG QUIET)

if(rttr_FOUND AND TARGET RTTR::Core)
    message(STATUS "RTTR: using installed ${rttr_VERSION}")
    return()
endif()

message(STATUS "RTTR: fetching ${VOXAGINE_RTTR_TAG} from upstream")

include(FetchContent)

FetchContent_Declare(rttr
    GIT_REPOSITORY https://github.com/rttrorg/rttr.git
    GIT_TAG        ${VOXAGINE_RTTR_TAG}
    GIT_SHALLOW    TRUE
)

# RTTR 0.9.6 predates CMake 3.5 being dropped as a compatibility floor, so
# configuring it under CMake 4 fails outright without this.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
set(BUILD_PACKAGE OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_STATIC_RUNTIME_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(rttr)

# RTTR's own warnings are not ours to fix.
if(TARGET rttr_core)
    target_compile_options(rttr_core PRIVATE -w)

    # Its headers are reached as <rttr/...>; the generated version.h lands in
    # the build tree, so both directories have to be on the search path.
    target_include_directories(rttr_core SYSTEM INTERFACE
        $<BUILD_INTERFACE:${rttr_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${rttr_BINARY_DIR}/src>
    )
endif()
