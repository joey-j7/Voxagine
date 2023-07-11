#pragma once

enum GPUAccessType
{
	E_READ_ONLY,
	E_READ_WRITE
};

#ifdef _WINDOWS

// #define _OPENGL

#ifdef _OPENGL

class GLRenderContext;
class GLUploadBuffer;
class GLCommandEngine;
class GLRenderPass;

typedef GLCommandEngine PCommandEngine;
typedef GLRenderContext PRenderContext;
typedef GLUploadBuffer PUploadBuffer;
typedef GLRenderPass PRenderPass;

#else
#include <d3d12.h>
#include "External/DirectX12/d3dx12.h"

#include "Core/Platform/Rendering/DX12/DXUploadBuffer.h"

#define R_PTR_TYPE(x) Microsoft::WRL::ComPtr<x>

class DX12RenderContext;
class DXUploadBuffer;
class DXCommandEngine;
class DXTextureManager;
class DXModelManager;
class DXRenderPass;
class DXComputePass;

typedef DXCommandEngine PCommandEngine;
typedef DX12RenderContext PRenderContext;
typedef DXTextureManager PTextureManager;
typedef DXModelManager PModelManager;
typedef DXUploadBuffer PUploadBuffer;
typedef DXRenderPass PRenderPass;
typedef DXComputePass PComputePass;

enum PEResourceDimension
{
	E_TEXTURE_1D = D3D12_RESOURCE_DIMENSION_TEXTURE1D,
	E_TEXTURE_2D = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	E_TEXTURE_3D = D3D12_RESOURCE_DIMENSION_TEXTURE3D
};

enum PESRVDimension
{
	E_SRV_TEXTURE_1D = D3D12_SRV_DIMENSION_TEXTURE1D,
	E_SRV_TEXTURE_2D = D3D12_SRV_DIMENSION_TEXTURE2D,
	E_SRV_TEXTURE_3D = D3D12_SRV_DIMENSION_TEXTURE3D
};

enum PEResourceFormat
{
	E_UNKNOWN = DXGI_FORMAT_UNKNOWN,
	E_R8G8B8A8_UNORM = DXGI_FORMAT_R8G8B8A8_UNORM,
	E_R8G8B8A8_UNORM_SRGB = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	E_R32_FLOAT = DXGI_FORMAT_R32_FLOAT,
	E_D32_FLOAT = DXGI_FORMAT_D32_FLOAT
};

enum PECullMode
{
	E_CULL_NONE = D3D12_CULL_MODE_NONE,
	E_CULL_FRONT = D3D12_CULL_MODE_FRONT,
	E_CULL_BACK = D3D12_CULL_MODE_BACK
};

enum PEFilterMode
{
	E_LINEAR = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
	E_POINT = D3D12_FILTER_MIN_MAG_MIP_POINT
};

enum PEWrapMode
{
	E_CLAMP = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
	E_WRAP = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	E_BORDER = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
	E_MIRROR = D3D12_TEXTURE_ADDRESS_MODE_MIRROR
};

enum PEPrimitiveTopology
{
	E_TOPOLOGY_TRIANGLELIST = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	E_TOPOLOGY_TRIANGLESTRIP = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	E_TOPOLOGY_LINELIST = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
	E_TOPOLOGY_POINTLIST = D3D_PRIMITIVE_TOPOLOGY_POINTLIST
};

enum PEPrimitiveTopologyType
{
	E_PRIMITIVE_TOPOLOGY_POINT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
	E_PRIMITIVE_TOPOLOGY_LINE = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
	E_PRIMITIVE_TOPOLOGY_TRIANGLE = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
};

enum PEResourceState
{
	E_STATE_COMMON_RESOURCE = D3D12_RESOURCE_STATE_COMMON,
	E_STATE_VERTEX_BUFFER = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
	E_STATE_CONSTANT_BUFFER = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
	E_STATE_PIXEL_SHADER_RESOURCE = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	E_STATE_NON_PIXEL_SHADER_RESOURCE = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	E_STATE_RENDER_TARGET = D3D12_RESOURCE_STATE_RENDER_TARGET,
	E_STATE_DEPTH_WRITE = D3D12_RESOURCE_STATE_DEPTH_WRITE,
	E_STATE_DEPTH_READ = D3D12_RESOURCE_STATE_DEPTH_READ,
	E_STATE_COPY_SOURCE = D3D12_RESOURCE_STATE_COPY_SOURCE,
	E_STATE_COPY_DEST = D3D12_RESOURCE_STATE_COPY_DEST,
	E_STATE_GENERAL_READ = D3D12_RESOURCE_STATE_GENERIC_READ,
	E_STATE_PRESENT = D3D12_RESOURCE_STATE_PRESENT
};

typedef ID3D12Resource PResource;
typedef ID3D12Resource PTexture;
typedef PTexture PTextureType;
typedef D3D12_RESOURCE_DESC PResourceDesc;

typedef ID3D12GraphicsCommandList PCommandList;

typedef ID3DBlob PVShader;
typedef ID3DBlob PFShader;
typedef ID3DBlob PShader;

typedef void* PShaderCache;

typedef std::vector<D3D12_INPUT_ELEMENT_DESC> PVertexLayout;

#endif

#elif _ORBIS

#include <gnm.h>
#include <gnm/dataformats.h>
#include <gnm/texture.h>
#include <gnm/constants.h>

#include <gnmx.h>
#include <gnmx/context.h>
#include <gnmx/shaderbinary.h>

class ORBRenderContext;
class ORBCommandEngine;
class ORBRenderPass;
class ORBComputePass;
class ORBModelManager;
class ORBTextureManager;

class Buffer;

typedef ORBCommandEngine PCommandEngine;
typedef ORBRenderContext PRenderContext;
typedef ORBTextureManager PTextureManager;
typedef ORBModelManager PModelManager;
typedef sce::Gnm::Buffer PUploadBuffer;
typedef ORBRenderPass PRenderPass;
typedef ORBComputePass PComputePass;

#define R_PTR_TYPE(x) std::unique_ptr<x>

static sce::Gnm::TextureType PEResourceDimensions[] = {
	sce::Gnm::kTextureType1d, // E_TEXTURE_1D
	sce::Gnm::kTextureType2d, //E_TEXTURE_2D
	sce::Gnm::kTextureType3d //E_TEXTURE_3D
};

enum PEResourceDimension
{
	E_TEXTURE_1D = 0,
	E_TEXTURE_2D = 1,
	E_TEXTURE_3D = 2
};

static sce::Gnm::TextureType PESRVDimensions[] = {
	sce::Gnm::kTextureType1d, // E_TEXTURE_1D
	sce::Gnm::kTextureType2d, //E_TEXTURE_2D
	sce::Gnm::kTextureType3d //E_TEXTURE_3D
};

enum PESRVDimension
{
	E_SRV_TEXTURE_1D = 0,
	E_SRV_TEXTURE_2D = 1,
	E_SRV_TEXTURE_3D = 2
};

static sce::Gnm::DataFormat PEResourceFormats[] = {
	sce::Gnm::kDataFormatInvalid, // E_UNKNOWN
	sce::Gnm::kDataFormatR8G8B8A8Unorm, // E_R8G8B8A8_UNORM
	sce::Gnm::kDataFormatR8G8B8A8UnormSrgb, // E_R8G8B8A8_UNORM
	sce::Gnm::kDataFormatR32Float, // E_R32_FLOAT
	sce::Gnm::kDataFormatR32Float // E_D32_FLOAT
};

static sce::Gnm::ZFormat PEDepthFormats[] = {
	sce::Gnm::kZFormat16, // E_Z_16
	sce::Gnm::kZFormat32Float // E_Z_32FLOAT
};

enum PEResourceFormat
{
	E_UNKNOWN = 0,
	E_R8G8B8A8_UNORM = 1,
	E_R8G8B8A8_UNORM_SRGB = 2,
	E_R32_FLOAT = 3,
	E_D32_FLOAT = 4,
	E_Z_16 = 0,
	E_Z_32FLOAT = 1
};

static sce::Gnm::PrimitiveSetupCullFaceMode PECullModes[] = {
	sce::Gnm::kPrimitiveSetupCullFaceNone, // E_CULL_NONE
	sce::Gnm::kPrimitiveSetupCullFaceFront, // E_CULL_FRONT
	sce::Gnm::kPrimitiveSetupCullFaceBack // E_CULL_BACK
};

enum PECullMode
{
	E_CULL_NONE = 0,
	E_CULL_FRONT = 1,
	E_CULL_BACK = 2
};

static sce::Gnm::FilterMode PEFilterModes[] = {
	sce::Gnm::kFilterModeBilinear, // E_LINEAR
	sce::Gnm::kFilterModePoint // E_POINT
};

enum PEFilterMode
{
	E_LINEAR = 0,
	E_POINT = 1
};

static sce::Gnm::WrapMode PEWrapModes[] = {
	sce::Gnm::kWrapModeClampBorder, // E_CLAMP
	sce::Gnm::kWrapModeWrap, // E_WRAP
	sce::Gnm::kWrapModeClampBorder, // E_BORDER
	sce::Gnm::kWrapModeMirror // E_MIRROR
};

enum PEWrapMode
{
	E_CLAMP = 0,
	E_WRAP = 1,
	E_BORDER = 2,
	E_MIRROR = 3
};

static sce::Gnm::PrimitiveType PEPrimitiveTopologies[] = {
	sce::Gnm::kPrimitiveTypeTriList, // E_TOPOLOGY_TRIANGLELIST
	sce::Gnm::kPrimitiveTypeTriStrip, // E_TOPOLOGY_TRIANGLESTRIP,
	sce::Gnm::kPrimitiveTypeLineList, // E_TOPOLOGY_LINELIST,
	sce::Gnm::kPrimitiveTypePointList // E_TOPOLOGY_POINTLIST
};

enum PEPrimitiveTopology
{
	E_TOPOLOGY_TRIANGLELIST = 0,
	E_TOPOLOGY_TRIANGLESTRIP = 1,
	E_TOPOLOGY_LINELIST = 2,
	E_TOPOLOGY_POINTLIST = 3
};

static sce::Gnm::PrimitiveType PEPrimitiveTopologyTypes[] = {
	sce::Gnm::kPrimitiveTypePointList, // E_PRIMITIVE_TOPOLOGY_POINT
	sce::Gnm::kPrimitiveTypeLineList, // E_PRIMITIVE_TOPOLOGY_LINE
	sce::Gnm::kPrimitiveTypeTriList // E_PRIMITIVE_TOPOLOGY_TRIANGLE
};

enum PEPrimitiveTopologyType
{
	E_PRIMITIVE_TOPOLOGY_POINT = 0,
	E_PRIMITIVE_TOPOLOGY_LINE = 1,
	E_PRIMITIVE_TOPOLOGY_TRIANGLE = 2
};

static sce::Gnmx::ResourceBarrier::Usage PEResourceStates[] = {
	sce::Gnmx::ResourceBarrier::Usage::kUsageRwBuffer, // E_STATE_COMMON_RESOURCE
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoBuffer, // E_STATE_VERTEX_BUFFER
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoBuffer, // E_STATE_CONSTANT_BUFFER
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoTexture, // E_STATE_PIXEL_SHADER_RESOURCE
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoTexture, // E_STATE_NON_PIXEL_SHADER_RESOURCE
	sce::Gnmx::ResourceBarrier::Usage::kUsageRenderTarget, // E_STATE_RENDER_TARGET
	sce::Gnmx::ResourceBarrier::Usage::kUsageDepthSurface, // E_STATE_DEPTH_WRITE
	sce::Gnmx::ResourceBarrier::Usage::kUsageDepthSurface, // E_STATE_DEPTH_READ
	sce::Gnmx::ResourceBarrier::Usage::kUsageRwTexture, // E_STATE_COPY_SOURCE
	sce::Gnmx::ResourceBarrier::Usage::kUsageRwTexture, // E_STATE_COPY_DEST
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoTexture, // E_STATE_GENERAL_READ
	sce::Gnmx::ResourceBarrier::Usage::kUsageRoTexture, // E_STATE_PRESENT
};

enum PEResourceState
{
	E_STATE_COMMON_RESOURCE = 0,
	E_STATE_VERTEX_BUFFER = 1,
	E_STATE_CONSTANT_BUFFER = 2,
	E_STATE_PIXEL_SHADER_RESOURCE = 3,
	E_STATE_NON_PIXEL_SHADER_RESOURCE = 4,
	E_STATE_RENDER_TARGET = 5,
	E_STATE_DEPTH_WRITE = 6,
	E_STATE_DEPTH_READ = 7,
	E_STATE_COPY_SOURCE = 8,
	E_STATE_COPY_DEST = 9,
	E_STATE_GENERAL_READ = 10,
	E_STATE_PRESENT = 11
};

typedef sce::Gnm::Texture PResource;
typedef sce::Gnm::Texture PTexture;
typedef void PTextureType;
typedef sce::Gnm::TextureSpec PResourceDesc;

typedef sce::Gnmx::GnmxGfxContext PCommandList;

typedef sce::Gnmx::VsShader PVShader;
typedef sce::Gnmx::PsShader PPShader;
typedef PVShader PShader;

typedef sce::Gnmx::InputOffsetsCache PShaderCache;

typedef std::vector<Buffer*> PVertexLayout;

#endif

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