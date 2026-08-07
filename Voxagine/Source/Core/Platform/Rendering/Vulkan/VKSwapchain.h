#pragma once

#include "Core/Platform/Rendering/Vulkan/VKDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

/* Swapchain, per-frame sync objects and the command buffers that write to the
   acquired image. Frames in flight matches RenderContext::m_uiFrameCount.

   Nothing here waits for the GPU on the main thread mid-frame: acquire and
   submit are fenced per frame slot, which is what the DX12 path failed to do
   (DXCommandEngine::Reset called WaitForGPU every frame and serialised
   CPU and GPU, defeating double buffering). */
class VKSwapchain
{
public:
	static const uint32_t m_uiFramesInFlight = 2;

	VKSwapchain() = default;
	~VKSwapchain();

	VKSwapchain(const VKSwapchain&) = delete;
	VKSwapchain& operator=(const VKSwapchain&) = delete;

	bool Create(VKDevice* pDevice, VkSurfaceKHR surface, uint32_t uiWidth, uint32_t uiHeight);

	/* Tears down and rebuilds the swapchain for a new window size. */
	bool Recreate(uint32_t uiWidth, uint32_t uiHeight);

	void Destroy();

	/* Records and submits a frame that clears the acquired image, then
	   presents it. Returns false when the swapchain is out of date and the
	   caller should recreate it. */
	bool ClearAndPresent(const float a_fColor[4]);

	VkFormat GetFormat() const { return m_Format; }
	VkExtent2D GetExtent() const { return m_Extent; }
	uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

private:
	bool CreateSwapchain(uint32_t uiWidth, uint32_t uiHeight);
	bool CreateImageViews();
	bool CreateFrameResources();

	void DestroySwapchainObjects();

	void TransitionImage(VkCommandBuffer cmd, VkImage image,
	                     VkImageLayout oldLayout, VkImageLayout newLayout) const;

	VKDevice* m_pDevice = nullptr;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	VkFormat m_Format = VK_FORMAT_UNDEFINED;
	VkExtent2D m_Extent = { 0, 0 };

	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;

	VkCommandPool m_CommandPool = VK_NULL_HANDLE;

	struct FrameData
	{
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		VkSemaphore m_ImageAvailable = VK_NULL_HANDLE;
		VkFence m_InFlight = VK_NULL_HANDLE;
	};

	FrameData m_Frames[m_uiFramesInFlight];

	/* Present semaphores are per swapchain image, not per frame in flight:
	   a frame slot may present an image whose previous present is still
	   pending, and reusing one semaphore for both is a validation error. */
	std::vector<VkSemaphore> m_RenderFinished;

	uint32_t m_uiFrameIndex = 0;
};
