#include "VKResource.h"

#include "VKDevice.h"
#include "VKTranslate.h"

#include <cstdio>

namespace
{
	bool IsDepthFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT ||
		       format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		       format == VK_FORMAT_D24_UNORM_S8_UINT ||
		       format == VK_FORMAT_D16_UNORM;
	}
}

VKResource::~VKResource()
{
	Destroy();
}

bool VKResource::CreateImage(VKDevice* pDevice, const VKAllocator* pAllocator,
                             VkImageType type, VkFormat format,
                             uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth,
                             uint32_t uiMipLevels, VkImageUsageFlags usage)
{
	Destroy();

	m_pDevice = pDevice;
	m_pAllocator = pAllocator;
	m_Format = format;
	m_bIsDepth = IsDepthFormat(format);

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.format = format;
	imageInfo.extent = { uiWidth, uiHeight, uiDepth };
	imageInfo.mipLevels = uiMipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(pDevice->Get(), &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateImage failed\n");
		return false;
	}

	VkMemoryRequirements requirements{};
	vkGetImageMemoryRequirements(pDevice->Get(), m_Image, &requirements);

	if (!pAllocator->Allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Allocation))
	{
		vkDestroyImage(pDevice->Get(), m_Image, nullptr);
		m_Image = VK_NULL_HANDLE;
		return false;
	}

	vkBindImageMemory(pDevice->Get(), m_Image, m_Allocation.m_Memory, 0);

	m_Kind = E_KIND_IMAGE;
	m_Extent = { uiWidth, uiHeight, uiDepth };
	m_State = E_STATE_COMMON_RESOURCE;
	m_bLayoutUndefined = true;

	return true;
}

bool VKResource::CreateBuffer(VKDevice* pDevice, const VKAllocator* pAllocator,
                              VkDeviceSize uiSize, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties)
{
	Destroy();

	m_pDevice = pDevice;
	m_pAllocator = pAllocator;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = uiSize;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(pDevice->Get(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateBuffer failed for %llu bytes\n",
		        static_cast<unsigned long long>(uiSize));
		return false;
	}

	VkMemoryRequirements requirements{};
	vkGetBufferMemoryRequirements(pDevice->Get(), m_Buffer, &requirements);

	/* Derived from usage rather than asked of the caller: forgetting it is a
	   spec violation most drivers silently accept. */
	const bool bDeviceAddress = (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0;

	if (!pAllocator->Allocate(requirements, properties, m_Allocation, bDeviceAddress))
	{
		vkDestroyBuffer(pDevice->Get(), m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
		return false;
	}

	vkBindBufferMemory(pDevice->Get(), m_Buffer, m_Allocation.m_Memory, 0);

	m_Kind = E_KIND_BUFFER;
	m_State = E_STATE_COMMON_RESOURCE;

	return true;
}

void* VKResource::Map()
{
	if (m_pMapped != nullptr)
		return m_pMapped;

	if (!m_Allocation.IsValid())
		return nullptr;

	if (vkMapMemory(m_pDevice->Get(), m_Allocation.m_Memory, 0,
	                m_Allocation.m_uiSize, 0, &m_pMapped) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkMapMemory failed for '%s'\n", m_Name.c_str());
		m_pMapped = nullptr;
	}

	return m_pMapped;
}

void VKResource::Unmap()
{
	if (m_pMapped == nullptr)
		return;

	vkUnmapMemory(m_pDevice->Get(), m_Allocation.m_Memory);
	m_pMapped = nullptr;
}

void VKResource::Transition(VkCommandBuffer cmd, PEResourceState newState)
{
	/* Still worth transitioning out of an undefined layout even if the engine
	   state happens to match. */
	if (newState == m_State && !m_bLayoutUndefined)
		return;

	VKResourceState from = VKStateOf(m_State);
	const VKResourceState to = VKStateOf(newState);

	if (m_bLayoutUndefined)
	{
		/* Discards any existing contents, which is correct for an image that
		   has never been written. */
		from.m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		from.m_Access = VK_ACCESS_2_NONE;
		from.m_Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

		m_bLayoutUndefined = false;
	}

	if (m_Kind == E_KIND_IMAGE)
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = from.m_Stage;
		barrier.srcAccessMask = from.m_Access;
		barrier.dstStageMask = to.m_Stage;
		barrier.dstAccessMask = to.m_Access;
		barrier.oldLayout = from.m_Layout;
		barrier.newLayout = to.m_Layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask =
			m_bIsDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependency);
	}
	else if (m_Kind == E_KIND_BUFFER)
	{
		VkBufferMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		barrier.srcStageMask = from.m_Stage;
		barrier.srcAccessMask = from.m_Access;
		barrier.dstStageMask = to.m_Stage;
		barrier.dstAccessMask = to.m_Access;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = m_Buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.bufferMemoryBarrierCount = 1;
		dependency.pBufferMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependency);
	}

	m_State = newState;
}

void VKResource::Destroy()
{
	if (m_pDevice == nullptr)
		return;

	Unmap();

	if (m_Image != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_pDevice->Get(), m_Image, nullptr);
		m_Image = VK_NULL_HANDLE;
	}

	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_pDevice->Get(), m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
	}

	if (m_pAllocator != nullptr)
		m_pAllocator->Free(m_Allocation);

	m_Kind = E_KIND_NONE;
	m_pDevice = nullptr;
	m_pAllocator = nullptr;
}

VKShaderBlob::~VKShaderBlob()
{
	Destroy();
}

bool VKShaderBlob::Create(VKDevice* pDevice, const std::vector<uint32_t>& a_SpirV)
{
	Destroy();

	if (a_SpirV.empty())
		return false;

	m_pDevice = pDevice;
	m_SpirV = a_SpirV;

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = m_SpirV.size() * sizeof(uint32_t);
	createInfo.pCode = m_SpirV.data();

	if (vkCreateShaderModule(pDevice->Get(), &createInfo, nullptr, &m_Module) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateShaderModule failed\n");
		m_Module = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void VKShaderBlob::Destroy()
{
	if (m_Module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_pDevice->Get(), m_Module, nullptr);
		m_Module = VK_NULL_HANDLE;
	}

	m_SpirV.clear();
	m_pDevice = nullptr;
}
