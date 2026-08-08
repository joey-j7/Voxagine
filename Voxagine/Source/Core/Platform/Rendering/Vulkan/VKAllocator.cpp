#include "VKAllocator.h"

#include "VKDevice.h"

#include <cstdio>

void VKAllocator::Initialize(VKDevice* pDevice)
{
	m_pDevice = pDevice;
	vkGetPhysicalDeviceMemoryProperties(pDevice->GetPhysicalDevice(), &m_MemoryProperties);
}

uint32_t VKAllocator::FindMemoryType(uint32_t uiTypeFilter, VkMemoryPropertyFlags properties) const
{
	for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; ++i)
	{
		const bool bTypeAllowed = (uiTypeFilter & (1u << i)) != 0;
		const bool bHasProperties =
			(m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

		if (bTypeAllowed && bHasProperties)
			return i;
	}

	return UINT32_MAX;
}

bool VKAllocator::Allocate(const VkMemoryRequirements& requirements,
                           VkMemoryPropertyFlags properties,
                           Allocation& outAllocation) const
{
	const uint32_t uiType = FindMemoryType(requirements.memoryTypeBits, properties);

	if (uiType == UINT32_MAX)
	{
		fprintf(stderr, "[vulkan] no memory type for filter 0x%x properties 0x%x\n",
		        requirements.memoryTypeBits, properties);
		return false;
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = uiType;

	if (vkAllocateMemory(m_pDevice->Get(), &allocInfo, nullptr, &outAllocation.m_Memory) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkAllocateMemory failed for %llu bytes\n",
		        static_cast<unsigned long long>(requirements.size));
		return false;
	}

	outAllocation.m_uiSize = requirements.size;
	outAllocation.m_uiMemoryType = uiType;

	return true;
}

void VKAllocator::Free(Allocation& allocation) const
{
	if (allocation.m_Memory == VK_NULL_HANDLE)
		return;

	vkFreeMemory(m_pDevice->Get(), allocation.m_Memory, nullptr);

	allocation = Allocation{};
}
