#pragma once

#include <cstdint>
#include <memory>
#include <vector>

/* Engine-owned rendering vocabulary.
 *
 * Every value below is defined by the engine, not by a graphics API. This file
 * used to typedef D3D12 types straight into these names and give the enums
 * D3D12_* values, which meant Buffer/Shader/View/Mapper and every render pass
 * were written against DX12 concrete types rather than against the abstract
 * RenderContext interface. Backends now translate these values into their own
 * (see Vulkan/VKTranslate.h); nothing outside a backend directory may name an
 * API type. */

enum GPUAccessType
{
	E_READ_ONLY,
	E_READ_WRITE
};

enum PEResourceDimension
{
	E_TEXTURE_1D,
	E_TEXTURE_2D,
	E_TEXTURE_3D
};

enum PESRVDimension
{
	E_SRV_TEXTURE_1D,
	E_SRV_TEXTURE_2D,
	E_SRV_TEXTURE_3D
};

enum PEResourceFormat
{
	E_UNKNOWN,
	E_R8G8B8A8_UNORM,
	E_R8G8B8A8_UNORM_SRGB,
	E_R32_FLOAT,
	E_D32_FLOAT
};

enum PECullMode
{
	E_CULL_NONE,
	E_CULL_FRONT,
	E_CULL_BACK
};

enum PEFilterMode
{
	E_LINEAR,
	E_POINT
};

enum PEWrapMode
{
	E_CLAMP,
	E_WRAP,
	E_BORDER,
	E_MIRROR
};

enum PEPrimitiveTopology
{
	E_TOPOLOGY_TRIANGLELIST,
	E_TOPOLOGY_TRIANGLESTRIP,
	E_TOPOLOGY_LINELIST,
	E_TOPOLOGY_POINTLIST
};

enum PEPrimitiveTopologyType
{
	E_PRIMITIVE_TOPOLOGY_POINT,
	E_PRIMITIVE_TOPOLOGY_LINE,
	E_PRIMITIVE_TOPOLOGY_TRIANGLE
};

/* Logical resource state. Vulkan expresses these as an image layout plus an
   access mask and pipeline stage; VKTranslate.h does that expansion. */
enum PEResourceState
{
	E_STATE_COMMON_RESOURCE,
	E_STATE_VERTEX_BUFFER,
	E_STATE_CONSTANT_BUFFER,
	E_STATE_PIXEL_SHADER_RESOURCE,
	E_STATE_NON_PIXEL_SHADER_RESOURCE,
	E_STATE_RENDER_TARGET,
	E_STATE_DEPTH_WRITE,
	E_STATE_DEPTH_READ,
	E_STATE_COPY_SOURCE,
	E_STATE_COPY_DEST,
	E_STATE_GENERAL_READ,
	E_STATE_PRESENT
};

/* One vertex attribute, in engine terms. Backends turn a PVertexLayout into
   whatever their pipeline creation wants. */
struct VertexElement
{
	const char* m_pSemanticName = nullptr;
	uint32_t m_uiSemanticIndex = 0;
	PEResourceFormat m_Format = E_UNKNOWN;
	uint32_t m_uiInputSlot = 0;
	uint32_t m_uiByteOffset = 0;
	bool m_bPerInstance = false;
};

typedef std::vector<VertexElement> PVertexLayout;

/* Backend types. These stay as aliases so call sites keep their current
   spelling, but they now resolve to the Vulkan backend rather than to DX12. */
class VKRenderContext;
class VKCommandEngine;
class VKUploadBuffer;
class VKRenderPass;
class VKComputePass;
class VKTextureManager;
class VKModelManager;

class VKResource;
class VKShaderBlob;

typedef VKCommandEngine PCommandEngine;
typedef VKRenderContext PRenderContext;
typedef VKUploadBuffer PUploadBuffer;
typedef VKRenderPass PRenderPass;
typedef VKComputePass PComputePass;
typedef VKTextureManager PTextureManager;
typedef VKModelManager PModelManager;

/* Owning handle for backend resources. DX12 used Microsoft::WRL::ComPtr here;
   Vulkan handles are refcounted by the engine instead. */
#define R_PTR_TYPE(x) std::shared_ptr<x>

typedef VKResource PResource;
typedef VKResource PTexture;
typedef PTexture PTextureType;

/* Describes a resource independently of the backend. */
struct PResourceDesc
{
	PEResourceDimension m_Dimension = E_TEXTURE_2D;
	PEResourceFormat m_Format = E_UNKNOWN;

	uint64_t m_uiWidth = 0;
	uint32_t m_uiHeight = 0;
	uint16_t m_uiDepthOrArraySize = 1;
	uint16_t m_uiMipLevels = 1;

	uint32_t m_uiSampleCount = 1;
};

class VKCommandList;
typedef VKCommandList PCommandList;

/* Compiled shader bytecode (SPIR-V). */
typedef VKShaderBlob PVShader;
typedef VKShaderBlob PFShader;
typedef VKShaderBlob PShader;

typedef void* PShaderCache;

typedef PEResourceDimension PResourceDimension;
typedef PESRVDimension PSRVDimension;

typedef PEResourceFormat PResourceFormat;
typedef PECullMode PCullMode;
typedef PEFilterMode PFilterMode;
typedef PEWrapMode PWrapMode;

typedef PEPrimitiveTopology PPrimitiveTopology;
typedef PEPrimitiveTopologyType PPrimitiveTopologyType;
typedef PEResourceState PResourceStates;

#define R_DEF_RESOURCE_DIMENSION_TYPE E_TEXTURE_2D
#define R_DEF_VIEW_DIMENSION_TYPE E_SRV_TEXTURE_2D
#define R_DEF_RESOURCE_FORMAT E_R8G8B8A8_UNORM
#define R_DEF_DEPTH_FORMAT E_D32_FLOAT
#define R_DEF_FILTER_MODE E_LINEAR
#define R_DEF_WRAP_MODE E_WRAP
#define R_DEF_CULL_TYPE E_CULL_NONE
#define R_DEF_PRIMITIVE_TOPOLOGY E_TOPOLOGY_TRIANGLELIST
#define R_DEF_PRIMITIVE_TOPOLOGY_TYPE E_PRIMITIVE_TOPOLOGY_TRIANGLE
#define R_DEF_RESOURCE_STATE_TYPE E_STATE_PIXEL_SHADER_RESOURCE
