#pragma once

#include "Core/Platform/Rendering/Vulkan/VKResource.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <deque>
#include <memory>

class VKDevice;
class VKAllocator;

/* Linear page allocator for per-frame upload data.
 *
 * A port of the DX12 UploadBuffer (originally Jeremiah van Oosten's, MIT).
 * Same shape: allocate out of a mapped page, hand back a CPU pointer and a GPU
 * address, and recycle whole pages at frame boundaries rather than freeing
 * individual allocations.
 *
 * Pages are host-visible buffers with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS,
 * so the GPU address is a real VkDeviceAddress - the direct equivalent of the
 * D3D12_GPU_VIRTUAL_ADDRESS the engine's Buffer already stores in a uint64. */
class VKUploadBuffer
{
public:
	struct Allocation
	{
		void* CPU = nullptr;
		VkDeviceAddress GPU = 0;

		/* Vulkan descriptors bind a buffer plus an offset rather than a raw
		   address, so both are needed for uniform/storage buffer bindings. */
		VkBuffer Buffer = VK_NULL_HANDLE;
		VkDeviceSize Offset = 0;
	};

	/* Default page size matches the DX12 backend's 20 MB. */
	explicit VKUploadBuffer(VKDevice* pDevice, const VKAllocator* pAllocator,
	                        size_t uiPageSize = 2097152 * 10);

	~VKUploadBuffer();

	VKUploadBuffer(const VKUploadBuffer&) = delete;
	VKUploadBuffer& operator=(const VKUploadBuffer&) = delete;

	size_t GetPageSize() const { return m_uiPageSize; }

	/* An allocation may not exceed one page. Returns an empty Allocation if it
	   does, rather than throwing the way the DX12 version did. */
	Allocation Allocate(size_t uiSizeInBytes, size_t uiAlignment);

	Allocation AllocateConstant(size_t uiSizeInBytes);

	/* Recycles every page. Only safe once the GPU is done with the frame that
	   used them. */
	void Reset();

private:
	class Page
	{
	public:
		Page(VKDevice* pDevice, const VKAllocator* pAllocator, size_t uiSizeInBytes);
		~Page();

		bool IsValid() const { return m_pCPU != nullptr; }

		bool HasSpace(size_t uiSizeInBytes, size_t uiAlignment) const;
		Allocation Allocate(size_t uiSizeInBytes, size_t uiAlignment);

		void Reset() { m_uiOffset = 0; }

	private:
		VKResource m_Resource;

		void* m_pCPU = nullptr;
		VkDeviceAddress m_GPU = 0;

		size_t m_uiPageSize = 0;
		size_t m_uiOffset = 0;
	};

	std::shared_ptr<Page> RequestPage();

	VKDevice* m_pDevice = nullptr;
	const VKAllocator* m_pAllocator = nullptr;

	typedef std::deque<std::shared_ptr<Page>> PagePool;

	PagePool m_PagePool;
	PagePool m_AvailablePages;

	std::shared_ptr<Page> m_pCurrentPage;

	size_t m_uiPageSize = 0;
	size_t m_uiConstantAlignment = 256;
};
