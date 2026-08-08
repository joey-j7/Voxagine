#include "VKSwapchain.h"

#include "VKResource.h"

#include <algorithm>
#include <cstdio>
#include <mutex>

VKSwapchain::~VKSwapchain()
{
	Destroy();
}

bool VKSwapchain::Create(VKDevice* pDevice, VkSurfaceKHR surface, uint32_t uiWidth, uint32_t uiHeight)
{
	m_pDevice = pDevice;
	m_Surface = surface;

	if (!CreateSwapchain(uiWidth, uiHeight))
		return false;

	if (!CreateImageViews())
		return false;

	return CreateFrameResources();
}

bool VKSwapchain::CreateSwapchain(uint32_t uiWidth, uint32_t uiHeight)
{
	VkPhysicalDevice physical = m_pDevice->GetPhysicalDevice();

	VkSurfaceCapabilitiesKHR caps{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, m_Surface, &caps);

	uint32_t uiFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical, m_Surface, &uiFormatCount, nullptr);

	if (uiFormatCount == 0)
	{
		fprintf(stderr, "[vulkan] surface reports no formats\n");
		return false;
	}

	std::vector<VkSurfaceFormatKHR> formats(uiFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical, m_Surface, &uiFormatCount, formats.data());

	/* R_DEF_RESOURCE_FORMAT is E_R8G8B8A8_UNORM_SRGB; match it when offered. */
	VkSurfaceFormatKHR chosen = formats[0];
	for (const VkSurfaceFormatKHR& format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			chosen = format;
			break;
		}
	}

	uint32_t uiPresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical, m_Surface, &uiPresentModeCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes(uiPresentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical, m_Surface, &uiPresentModeCount, presentModes.data());

	/* FIFO is the only mode guaranteed present; prefer mailbox for latency. */
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (VkPresentModeKHR mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = mode;
			break;
		}
	}

	if (caps.currentExtent.width != UINT32_MAX)
	{
		m_Extent = caps.currentExtent;
	}
	else
	{
		m_Extent.width = std::clamp(uiWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
		m_Extent.height = std::clamp(uiHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
	}

	if (m_Extent.width == 0 || m_Extent.height == 0)
		return false;

	uint32_t uiImageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && uiImageCount > caps.maxImageCount)
		uiImageCount = caps.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;
	createInfo.minImageCount = uiImageCount;
	createInfo.imageFormat = chosen.format;
	createInfo.imageColorSpace = chosen.colorSpace;
	createInfo.imageExtent = m_Extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.preTransform = caps.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	const uint32_t uiFamilies[] = { m_pDevice->GetGraphicsQueueFamily(), m_pDevice->GetPresentQueueFamily() };

	if (uiFamilies[0] != uiFamilies[1])
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = uiFamilies;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(m_pDevice->Get(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateSwapchainKHR failed\n");
		return false;
	}

	m_Format = chosen.format;

	uint32_t uiActualCount = 0;
	vkGetSwapchainImagesKHR(m_pDevice->Get(), m_Swapchain, &uiActualCount, nullptr);

	m_Images.resize(uiActualCount);
	vkGetSwapchainImagesKHR(m_pDevice->Get(), m_Swapchain, &uiActualCount, m_Images.data());

	return true;
}

bool VKSwapchain::CreateImageViews()
{
	m_ImageViews.resize(m_Images.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_pDevice->Get(), &viewInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] vkCreateImageView failed\n");
			return false;
		}
	}

	/* One present semaphore per swapchain image. */
	m_RenderFinished.resize(m_Images.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < m_RenderFinished.size(); ++i)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(m_pDevice->Get(), &semaphoreInfo, nullptr, &m_RenderFinished[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] vkCreateSemaphore failed\n");
			return false;
		}
	}

	return true;
}

bool VKSwapchain::CreateFrameResources()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_pDevice->GetGraphicsQueueFamily();

	if (vkCreateCommandPool(m_pDevice->Get(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] vkCreateCommandPool failed\n");
		return false;
	}

	for (uint32_t i = 0; i < m_uiFramesInFlight; ++i)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_pDevice->Get(), &allocInfo, &m_Frames[i].m_CommandBuffer) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] vkAllocateCommandBuffers failed\n");
			return false;
		}

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		/* Signalled, so the first frame does not block on a fence nothing submitted to. */
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_pDevice->Get(), &semaphoreInfo, nullptr, &m_Frames[i].m_ImageAvailable) != VK_SUCCESS ||
			vkCreateFence(m_pDevice->Get(), &fenceInfo, nullptr, &m_Frames[i].m_InFlight) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] frame sync object creation failed\n");
			return false;
		}
	}

	return true;
}

void VKSwapchain::TransitionImage(VkCommandBuffer cmd, VkImage image,
                                  VkImageLayout oldLayout, VkImageLayout newLayout) const
{
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	/* ALL_TRANSFER rather than CLEAR: the presented image is written by a clear
	   and a blit, and a clear-only scope does not cover the blit. */
	if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	}
	else
	{
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
	}

	VkDependencyInfo dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;

	vkCmdPipelineBarrier2(cmd, &dependency);
}

bool VKSwapchain::ClearAndPresent(const float a_fColor[4])
{
	VkDevice device = m_pDevice->Get();
	FrameData& frame = m_Frames[m_uiFrameIndex];

	/* Wait only for this slot's previous submission, not for the whole GPU. */
	vkWaitForFences(device, 1, &frame.m_InFlight, VK_TRUE, UINT64_MAX);

	uint32_t uiImageIndex = 0;
	VkResult acquire = vkAcquireNextImageKHR(device, m_Swapchain, UINT64_MAX,
	                                         frame.m_ImageAvailable, VK_NULL_HANDLE, &uiImageIndex);

	if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
	{
		fprintf(stderr, "[vulkan] vkAcquireNextImageKHR failed (%d)\n", acquire);
		return false;
	}

	/* Reset only after we know we are going to submit, or the fence stays
	   unsignalled and the next wait on this slot hangs. */
	vkResetFences(device, 1, &frame.m_InFlight);
	vkResetCommandBuffer(frame.m_CommandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(frame.m_CommandBuffer, &beginInfo);

	TransitionImage(frame.m_CommandBuffer, m_Images[uiImageIndex],
	                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkClearColorValue clearColor{};
	clearColor.float32[0] = a_fColor[0];
	clearColor.float32[1] = a_fColor[1];
	clearColor.float32[2] = a_fColor[2];
	clearColor.float32[3] = a_fColor[3];

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = 1;
	range.layerCount = 1;

	vkCmdClearColorImage(frame.m_CommandBuffer, m_Images[uiImageIndex],
	                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

	TransitionImage(frame.m_CommandBuffer, m_Images[uiImageIndex],
	                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	vkEndCommandBuffer(frame.m_CommandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.m_ImageAvailable;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.m_CommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderFinished[uiImageIndex];

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinished[uiImageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &uiImageIndex;

	VkResult present = VK_SUCCESS;

	{
		std::lock_guard<std::mutex> lock(m_pDevice->GetQueueMutex());

		if (vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, frame.m_InFlight) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] vkQueueSubmit failed\n");
			return false;
		}

		present = vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);
	}

	m_uiFrameIndex = (m_uiFrameIndex + 1) % m_uiFramesInFlight;

	return present != VK_ERROR_OUT_OF_DATE_KHR && present != VK_SUBOPTIMAL_KHR;
}

bool VKSwapchain::BlitAndPresent(VKResource* pSource, VkSemaphore waitTimeline, uint64_t uiWaitValue)
{
	if (pSource == nullptr || pSource->GetKind() != VKResource::E_KIND_IMAGE)
		return false;

	VkDevice device = m_pDevice->Get();
	FrameData& frame = m_Frames[m_uiFrameIndex];

	vkWaitForFences(device, 1, &frame.m_InFlight, VK_TRUE, UINT64_MAX);

	uint32_t uiImageIndex = 0;
	VkResult acquire = vkAcquireNextImageKHR(device, m_Swapchain, UINT64_MAX,
	                                         frame.m_ImageAvailable, VK_NULL_HANDLE, &uiImageIndex);

	if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
	{
		fprintf(stderr, "[vulkan] vkAcquireNextImageKHR failed (%d)\n", acquire);
		return false;
	}

	vkResetFences(device, 1, &frame.m_InFlight);
	vkResetCommandBuffer(frame.m_CommandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBuffer cmd = frame.m_CommandBuffer;
	vkBeginCommandBuffer(cmd, &beginInfo);

	TransitionImage(cmd, m_Images[uiImageIndex],
	                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* The pass left this as a shader resource; the blit needs it readable as
	   a transfer source. VKResource tracks its own state, so this is a no-op
	   when it already matches. */
	pSource->Transition(cmd, E_STATE_COPY_SOURCE);

	const VkExtent3D srcExtent = pSource->GetExtent();

	/* Fit the source into the swapchain without distorting it; usually a 1:1
	   copy into a centred rect with black bars either side. Integer and
	   rounded, because a float ratio loses the exact fit for some window sizes
	   and a one-pixel mismatch resamples the whole frame. */
	uint32_t uiDstWidth = m_Extent.width;
	uint32_t uiDstHeight = m_Extent.height;

	if (srcExtent.width > 0 && srcExtent.height > 0)
	{
		const uint64_t uiTargetCross = static_cast<uint64_t>(m_Extent.width) * srcExtent.height;
		const uint64_t uiSourceCross = static_cast<uint64_t>(m_Extent.height) * srcExtent.width;

		if (uiTargetCross > uiSourceCross)
		{
			/* Window is wider than the image: pillarbox. */
			uiDstWidth = static_cast<uint32_t>((uiSourceCross + srcExtent.height / 2) / srcExtent.height);
		}
		else
		{
			uiDstHeight = static_cast<uint32_t>((uiTargetCross + srcExtent.width / 2) / srcExtent.width);
		}
	}

	uiDstWidth = std::min(std::max(uiDstWidth, 1u), m_Extent.width);
	uiDstHeight = std::min(std::max(uiDstHeight, 1u), m_Extent.height);

	const int32_t iOffsetX = static_cast<int32_t>((m_Extent.width - uiDstWidth) / 2);
	const int32_t iOffsetY = static_cast<int32_t>((m_Extent.height - uiDstHeight) / 2);

	/* Anything but a 1:1 blit resamples the whole frame; report it once. */
	{
		static uint32_t s_uiReportedWidth = 0;
		static uint32_t s_uiReportedHeight = 0;

		if (srcExtent.width != uiDstWidth || srcExtent.height != uiDstHeight)
		{
			if (s_uiReportedWidth != srcExtent.width || s_uiReportedHeight != srcExtent.height)
			{
				s_uiReportedWidth = srcExtent.width;
				s_uiReportedHeight = srcExtent.height;

				fprintf(stderr, "[vulkan] present rescales %ux%u to %ux%u in a %ux%u window\n",
				        srcExtent.width, srcExtent.height, uiDstWidth, uiDstHeight,
				        m_Extent.width, m_Extent.height);
			}
		}
	}

	/* The bars are never written by the blit, and a swapchain image is
	   recycled with whatever the last frame left in it. */
	if (iOffsetX != 0 || iOffsetY != 0)
	{
		VkClearColorValue black{};

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = 1;
		range.layerCount = 1;

		vkCmdClearColorImage(cmd, m_Images[uiImageIndex],
		                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1, &range);
	}

	VkImageBlit region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.srcOffsets[1] = { static_cast<int32_t>(srcExtent.width),
	                         static_cast<int32_t>(srcExtent.height), 1 };
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.dstOffsets[0] = { iOffsetX, iOffsetY, 0 };
	region.dstOffsets[1] = { iOffsetX + static_cast<int32_t>(uiDstWidth),
	                         iOffsetY + static_cast<int32_t>(uiDstHeight), 1 };

	vkCmdBlitImage(cmd,
	               pSource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               m_Images[uiImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &region, VK_FILTER_LINEAR);

	TransitionImage(cmd, m_Images[uiImageIndex],
	                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	vkEndCommandBuffer(cmd);

	/* Two waits: the acquire (binary) and the render engine's timeline, so the
	   blit cannot read a half-drawn frame. */
	VkSemaphoreSubmitInfo waits[2]{};
	waits[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waits[0].semaphore = frame.m_ImageAvailable;
	waits[0].stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

	waits[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waits[1].semaphore = waitTimeline;
	waits[1].value = uiWaitValue;
	waits[1].stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

	VkCommandBufferSubmitInfo commandInfo{};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	commandInfo.commandBuffer = cmd;

	VkSemaphoreSubmitInfo signalInfo{};
	signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo.semaphore = m_RenderFinished[uiImageIndex];
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.waitSemaphoreInfoCount = waitTimeline != VK_NULL_HANDLE ? 2 : 1;
	submitInfo.pWaitSemaphoreInfos = waits;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &commandInfo;
	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalInfo;

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinished[uiImageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &uiImageIndex;

	VkResult present = VK_SUCCESS;

	{
		/* Submit and present as one critical section: a job thread uploading a
		   texture shares this queue. */
		std::lock_guard<std::mutex> lock(m_pDevice->GetQueueMutex());

		if (vkQueueSubmit2(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, frame.m_InFlight) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] blit submit failed\n");
			return false;
		}

		present = vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);
	}

	m_uiFrameIndex = (m_uiFrameIndex + 1) % m_uiFramesInFlight;

	return present != VK_ERROR_OUT_OF_DATE_KHR && present != VK_SUBOPTIMAL_KHR;
}

bool VKSwapchain::Recreate(uint32_t uiWidth, uint32_t uiHeight)
{
	vkDeviceWaitIdle(m_pDevice->Get());

	DestroySwapchainObjects();

	if (!CreateSwapchain(uiWidth, uiHeight))
		return false;

	return CreateImageViews();
}

void VKSwapchain::DestroySwapchainObjects()
{
	VkDevice device = m_pDevice->Get();

	for (VkSemaphore semaphore : m_RenderFinished)
	{
		if (semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(device, semaphore, nullptr);
	}
	m_RenderFinished.clear();

	for (VkImageView view : m_ImageViews)
	{
		if (view != VK_NULL_HANDLE)
			vkDestroyImageView(device, view, nullptr);
	}
	m_ImageViews.clear();
	m_Images.clear();

	if (m_Swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;
	}
}

void VKSwapchain::Destroy()
{
	if (m_pDevice == nullptr || m_pDevice->Get() == VK_NULL_HANDLE)
		return;

	VkDevice device = m_pDevice->Get();
	vkDeviceWaitIdle(device);

	DestroySwapchainObjects();

	for (uint32_t i = 0; i < m_uiFramesInFlight; ++i)
	{
		if (m_Frames[i].m_ImageAvailable != VK_NULL_HANDLE)
			vkDestroySemaphore(device, m_Frames[i].m_ImageAvailable, nullptr);

		if (m_Frames[i].m_InFlight != VK_NULL_HANDLE)
			vkDestroyFence(device, m_Frames[i].m_InFlight, nullptr);

		m_Frames[i] = FrameData{};
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	m_pDevice = nullptr;
}
