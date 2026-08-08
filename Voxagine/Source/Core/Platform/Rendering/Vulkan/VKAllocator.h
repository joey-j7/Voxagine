#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

class VKDevice;

/* Device memory allocation.
 *
 * D3D12 committed resources bundled the resource and its heap, so the old
 * backend never allocated memory explicitly. Vulkan splits them, so every
 * image and buffer needs a VkDeviceMemory chosen from a memory type that
 * satisfies the resource's requirements and the intended CPU access.
 *
 * This is a straight one-allocation-per-resource implementation. That is
 * enough for bring-up and it is honest about what it is: real content will
 * want suballocation (or VMA) before the voxel renderer starts creating
 * resources per frame. */
class VKAllocator
{
public:
	struct Allocation
	{
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkDeviceSize m_uiSize = 0;
		uint32_t m_uiMemoryType = UINT32_MAX;

		bool IsValid() const { return m_Memory != VK_NULL_HANDLE; }
	};

	VKAllocator() = default;

	void Initialize(VKDevice* pDevice);

	/* properties is a mask of VkMemoryPropertyFlagBits: DEVICE_LOCAL for GPU
	   resources, HOST_VISIBLE | HOST_COHERENT for anything a Mapper writes.

	   bDeviceAddress must be set for any memory backing a buffer created with
	   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT. Omitting it is a spec
	   violation that NVIDIA's driver happens to accept, so only validation
	   catches it. */
	bool Allocate(const VkMemoryRequirements& requirements,
	              VkMemoryPropertyFlags properties,
	              Allocation& outAllocation,
	              bool bDeviceAddress = false) const;

	void Free(Allocation& allocation) const;

	/* Returns UINT32_MAX when no memory type satisfies the request. */
	uint32_t FindMemoryType(uint32_t uiTypeFilter, VkMemoryPropertyFlags properties) const;

private:
	VKDevice* m_pDevice = nullptr;
	VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
};
