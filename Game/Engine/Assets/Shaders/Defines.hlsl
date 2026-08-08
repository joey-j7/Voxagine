/* Global */
#define AMBIENT_VALUE 0.5
#define OPTIMIZED 1
// #define DIRECT_LIGHTING 2

/* Shadow ray distance-to-fade scale. Retuned for MarchLight's 64-step,
   double-stride walk (Phase 1 of RENDERING_PLAN.md) - the original 0.0125
   assumed 128 full-stride steps. Tune by eye. */
#define SHADOW_FADE_K 0.0125

/* Sky colour for rays that leave the world without hitting anything or the
   ground plane. Becomes fog input in RENDERING_PLAN.md phase 6.1. */
#define SKY_COLOR float4(150.0 / 255.0, 230.0 / 255.0, 255.0 / 255.0, 1.0)

#define DEG2RAD 0.0174532925
#define RAD2DEG 57.2957795

/* Platform specific */
#ifdef __PSSL__

#define FORCE_DEPTH_TEST [FORCE_EARLY_DEPTH_STENCIL]

#define STRUCTURED_BUFFER(x) RegularBuffer<x>
#define BUFFER(x) RegularBuffer<x>
#define RW_BUFFER(x) RW_RegularBuffer<x>

#define VOXEL_FORMAT uint

#define VOXEL_BUFFER BUFFER(VOXEL_FORMAT)
#define VOXEL_RW_BUFFER RW_BUFFER(VOXEL_FORMAT)
#define VOXEL_BUFFER_LOCAL BUFFER(VOXEL_FORMAT)

#define EMPTY_VOXEL 0

#define CONSTANT_BUFFER ConstantBuffer

#define VERT_ID S_VERTEX_ID
#define INST_ID S_INSTANCE_ID
#define POS_OUT S_POSITION

#define TAR_OUT S_TARGET_OUTPUT
#define TAR_OUT0 S_TARGET_OUTPUT0
#define TAR_OUT1 S_TARGET_OUTPUT1
#define TAR_OUT2 S_TARGET_OUTPUT2
#define TAR_OUT3 S_TARGET_OUTPUT3
#define TAR_OUT4 S_TARGET_OUTPUT4
#define TAR_OUT5 S_TARGET_OUTPUT5
#define TAR_OUT6 S_TARGET_OUTPUT6
#define TAR_OUT7 S_TARGET_OUTPUT7
#define TAR_OUT8 S_TARGET_OUTPUT8

#else

#define FORCE_DEPTH_TEST [earlydepthstencil]

#define STRUCTURED_BUFFER(x) StructuredBuffer<x>
#define BUFFER(x) Buffer<x>
#define RW_BUFFER(x) RWBuffer<x>

#define VOXEL_FORMAT float4

/* The voxel buffers are float4 in the shader but the engine stores one 32-bit
   RGBA8 texel per voxel and relies on the view converting on read. D3D12 took
   that from the UAV format; SPIR-V carries the format on the declaration, and
   without this DXC assumes Rgba32f and the view no longer matches. */
#ifdef __spirv__
#define VOXEL_IMAGE_FORMAT [[vk::image_format("rgba8")]]
#else
#define VOXEL_IMAGE_FORMAT
#endif

#define VOXEL_BUFFER VOXEL_IMAGE_FORMAT BUFFER(VOXEL_FORMAT)
#define VOXEL_RW_BUFFER VOXEL_IMAGE_FORMAT RW_BUFFER(VOXEL_FORMAT)

/* Same type without the annotation, for local aliases - the attribute is only
   valid on a resource declaration. */
#define VOXEL_BUFFER_LOCAL BUFFER(VOXEL_FORMAT)

#define EMPTY_VOXEL float4(0.0, 0.0, 0.0, 0.0)

#define CONSTANT_BUFFER cbuffer

#define VERT_ID SV_VERTEXID
#define INST_ID SV_INSTANCEID
#define POS_OUT SV_POSITION

#define TAR_OUT SV_TARGET
#define TAR_OUT0 SV_TARGET0
#define TAR_OUT1 SV_TARGET1
#define TAR_OUT2 SV_TARGET2
#define TAR_OUT3 SV_TARGET3
#define TAR_OUT4 SV_TARGET4
#define TAR_OUT5 SV_TARGET5
#define TAR_OUT6 SV_TARGET6
#define TAR_OUT7 SV_TARGET7
#define TAR_OUT8 SV_TARGET8

#endif