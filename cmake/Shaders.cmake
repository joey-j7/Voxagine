# HLSL -> SPIR-V via DXC.
#
# The port keeps the original HLSL rather than translating to GLSL; DXC's
# -spirv backend compiles all of it unmodified. No [[vk::binding]] annotations
# were needed.
#
# The shifts below are load-bearing, not cosmetic. HLSL's b/t/u/s register
# classes are independent namespaces but Vulkan has a single binding namespace
# per set, so without them a shader declaring b0 and u0 emits two descriptors
# at set 0 binding 0 - an invalid layout. They must stay in sync with
# VKBindings in Vulkan/VKShaderBindings.h.

find_program(DXC_EXECUTABLE NAMES dxc HINTS ENV VULKAN_SDK PATH_SUFFIXES bin)

set(VOXAGINE_DXC_SHIFTS
    -fvk-b-shift 0 0
    -fvk-t-shift 100 0
    -fvk-u-shift 200 0
    -fvk-s-shift 300 0
    # DX buffer layout, not std430: the engine memcpys tightly-packed C++
    # structs into structured buffers (StructuredVoxelBuffer is 32 bytes;
    # std430 pads its float3s to a 48-byte stride, which silently shifts
    # every element after [0]).
    -fvk-use-dx-layout
)

# voxagine_add_shaders(<target> SOURCE_DIR <dir> OUTPUT_DIR <dir>)
#
# Compiles every *.vs.hlsl / *.ps.hlsl / *.cs.hlsl under SOURCE_DIR. Plain
# *.hlsl files are include-only (Camera.hlsl, Defines.hlsl, ...) and are
# compiled as part of whatever includes them.
function(voxagine_add_shaders TARGET_NAME)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;OUTPUT_DIR" "" ${ARGN})

    if(NOT DXC_EXECUTABLE)
        message(WARNING "dxc not found - skipping shader target ${TARGET_NAME}")
        return()
    endif()

    file(GLOB SHADER_VS ${ARG_SOURCE_DIR}/*.vs.hlsl)
    file(GLOB SHADER_PS ${ARG_SOURCE_DIR}/*.ps.hlsl)
    file(GLOB SHADER_CS ${ARG_SOURCE_DIR}/*.cs.hlsl)

    # Any .hlsl may be included by any other, and dxc gives us no depfile, so
    # treat the whole directory as the dependency set. Coarse but never stale.
    file(GLOB SHADER_INCLUDES ${ARG_SOURCE_DIR}/*.hlsl)

    set(SPIRV_OUTPUTS "")

    foreach(PROFILE_PAIR "vs_6_0;${SHADER_VS}" "ps_6_0;${SHADER_PS}" "cs_6_0;${SHADER_CS}")
        list(POP_FRONT PROFILE_PAIR PROFILE)

        foreach(SHADER ${PROFILE_PAIR})
            get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
            get_filename_component(SHADER_FULL ${SHADER} NAME)
            string(REGEX REPLACE "\\.hlsl$" ".spv" SPIRV_NAME ${SHADER_FULL})

            set(SPIRV_OUT ${ARG_OUTPUT_DIR}/${SPIRV_NAME})

            add_custom_command(
                OUTPUT ${SPIRV_OUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIR}
                COMMAND ${DXC_EXECUTABLE}
                        -spirv -T ${PROFILE} -E main
                        ${VOXAGINE_DXC_SHIFTS}
                        -Fo ${SPIRV_OUT}
                        ${SHADER}
                DEPENDS ${SHADER} ${SHADER_INCLUDES}
                COMMENT "DXC ${PROFILE} ${SHADER_FULL}"
                VERBATIM
            )

            list(APPEND SPIRV_OUTPUTS ${SPIRV_OUT})
        endforeach()
    endforeach()

    add_custom_target(${TARGET_NAME} ALL DEPENDS ${SPIRV_OUTPUTS})

    list(LENGTH SPIRV_OUTPUTS SPIRV_COUNT)
    message(STATUS "Shaders: ${TARGET_NAME} -> ${SPIRV_COUNT} SPIR-V modules")
endfunction()
