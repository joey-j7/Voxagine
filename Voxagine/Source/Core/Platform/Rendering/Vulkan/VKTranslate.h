#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"

#include <vulkan/vulkan.h>

/* The only place engine rendering vocabulary is allowed to meet Vulkan's.
   Everything here is a pure function of a RenderDefines.h value. */

inline VkFormat VKFormat(PEResourceFormat format)
{
	switch (format)
	{
	case E_R8G8B8A8_UNORM:      return VK_FORMAT_R8G8B8A8_UNORM;
	case E_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
	case E_R32_FLOAT:           return VK_FORMAT_R32_SFLOAT;
	case E_D32_FLOAT:           return VK_FORMAT_D32_SFLOAT;
	case E_UNKNOWN:             break;
	}

	return VK_FORMAT_UNDEFINED;
}

inline VkImageType VKImageType(PEResourceDimension dimension)
{
	switch (dimension)
	{
	case E_TEXTURE_1D: return VK_IMAGE_TYPE_1D;
	case E_TEXTURE_2D: return VK_IMAGE_TYPE_2D;
	case E_TEXTURE_3D: return VK_IMAGE_TYPE_3D;
	}

	return VK_IMAGE_TYPE_2D;
}

inline VkImageViewType VKImageViewType(PESRVDimension dimension)
{
	switch (dimension)
	{
	case E_SRV_TEXTURE_1D: return VK_IMAGE_VIEW_TYPE_1D;
	case E_SRV_TEXTURE_2D: return VK_IMAGE_VIEW_TYPE_2D;
	case E_SRV_TEXTURE_3D: return VK_IMAGE_VIEW_TYPE_3D;
	}

	return VK_IMAGE_VIEW_TYPE_2D;
}

inline VkCullModeFlags VKCullMode(PECullMode mode)
{
	switch (mode)
	{
	case E_CULL_NONE:  return VK_CULL_MODE_NONE;
	case E_CULL_FRONT: return VK_CULL_MODE_FRONT_BIT;
	case E_CULL_BACK:  return VK_CULL_MODE_BACK_BIT;
	}

	return VK_CULL_MODE_NONE;
}

inline VkFilter VKFilter(PEFilterMode mode)
{
	return mode == E_POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

inline VkSamplerMipmapMode VKMipmapMode(PEFilterMode mode)
{
	return mode == E_POINT ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

inline VkSamplerAddressMode VKAddressMode(PEWrapMode mode)
{
	switch (mode)
	{
	case E_CLAMP:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case E_WRAP:   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case E_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case E_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}

	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

inline VkPrimitiveTopology VKTopology(PEPrimitiveTopology topology)
{
	switch (topology)
	{
	case E_TOPOLOGY_TRIANGLELIST:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case E_TOPOLOGY_TRIANGLESTRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case E_TOPOLOGY_LINELIST:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case E_TOPOLOGY_POINTLIST:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}

	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline VkPrimitiveTopology VKTopology(PEPrimitiveTopologyType type)
{
	switch (type)
	{
	case E_PRIMITIVE_TOPOLOGY_POINT:    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case E_PRIMITIVE_TOPOLOGY_LINE:     return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case E_PRIMITIVE_TOPOLOGY_TRIANGLE: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

/* A DX12 resource state carries layout, access and stage all at once. Vulkan
   splits them, so each engine state expands into all three. */
struct VKResourceState
{
	VkImageLayout m_Layout;
	VkAccessFlags2 m_Access;
	VkPipelineStageFlags2 m_Stage;
};

inline VKResourceState VKStateOf(PEResourceState state)
{
	switch (state)
	{
	case E_STATE_COMMON_RESOURCE:
		return { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT };

	case E_STATE_VERTEX_BUFFER:
		/* Also covers constant buffers; the two shared a D3D12 state. */
		return { VK_IMAGE_LAYOUT_UNDEFINED,
		         VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT,
		         VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT };

	case E_STATE_CONSTANT_BUFFER:
		return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_2_UNIFORM_READ_BIT,
		         VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT };

	case E_STATE_PIXEL_SHADER_RESOURCE:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_SHADER_READ_BIT,
		         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT };

	case E_STATE_NON_PIXEL_SHADER_RESOURCE:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_SHADER_READ_BIT,
		         VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT };

	case E_STATE_RENDER_TARGET:
		return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		         VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT };

	case E_STATE_DEPTH_WRITE:
		return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		         VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT };

	case E_STATE_DEPTH_READ:
		return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		         VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT };

	case E_STATE_COPY_SOURCE:
		return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_2_TRANSFER_READ_BIT,
		         VK_PIPELINE_STAGE_2_COPY_BIT };

	case E_STATE_COPY_DEST:
		return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		         VK_PIPELINE_STAGE_2_COPY_BIT };

	case E_STATE_GENERAL_READ:
		return { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_MEMORY_READ_BIT,
		         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT };

	case E_STATE_PRESENT:
		return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_NONE,
		         VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT };
	}

	return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT };
}
