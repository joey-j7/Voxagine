#include "VKCommandEngine.h"

#include "VKAllocator.h"
#include "VKDevice.h"
#include "VKTranslate.h"

#include <cstdio>
#include <mutex>

VKCommandEngine::VKCommandEngine(VKDevice* pDevice, const VKAllocator* pAllocator, const Info& info)
	: CommandEngine(info)
	, m_pDevice(pDevice)
	, m_pAllocator(pAllocator)
{
}

VKCommandEngine::~VKCommandEngine()
{
	if (m_pDevice == nullptr || m_pDevice->Get() == VK_NULL_HANDLE)
		return;

	WaitForGPU();

	m_pUploadBuffer.reset();

	for (uint32_t i = 0; i < m_uiFrameCount; ++i)
	{
		if (m_Frames[i].m_CommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(m_pDevice->Get(), m_Frames[i].m_CommandPool, nullptr);
	}

	if (m_Timeline != VK_NULL_HANDLE)
		vkDestroySemaphore(m_pDevice->Get(), m_Timeline, nullptr);
}

bool VKCommandEngine::Initialize()
{
	/* One queue family for now. VKDevice only surfaces graphics and present;
	   E_COPY and E_COMPUTE would benefit from dedicated transfer/compute
	   families, which is a VKDevice change rather than one here. */
	m_Queue = m_pDevice->GetGraphicsQueue();
	m_uiQueueFamily = m_pDevice->GetGraphicsQueueFamily();

	VkSemaphoreTypeCreateInfo typeInfo{};
	typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	typeInfo.initialValue = 0;

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = &typeInfo;

	if (vkCreateSemaphore(m_pDevice->Get(), &semaphoreInfo, nullptr, &m_Timeline) != VK_SUCCESS)
	{
		fprintf(stderr, "[vulkan] timeline semaphore creation failed for '%s'\n",
		        m_Info.m_Name.c_str());
		return false;
	}

	for (uint32_t i = 0; i < m_uiFrameCount; ++i)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_uiQueueFamily;

		/* One pool per frame slot, reset wholesale. That is cheaper than
		   resetting buffers individually and matches how the D3D12 backend
		   recycled its allocators. */
		if (vkCreateCommandPool(m_pDevice->Get(), &poolInfo, nullptr, &m_Frames[i].m_CommandPool) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] command pool creation failed for '%s'\n",
			        m_Info.m_Name.c_str());
			return false;
		}

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_Frames[i].m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_pDevice->Get(), &allocInfo, &m_Frames[i].m_CommandBuffer) != VK_SUCCESS)
		{
			fprintf(stderr, "[vulkan] command buffer allocation failed for '%s'\n",
			        m_Info.m_Name.c_str());
			return false;
		}
	}

	m_pUploadBuffer = std::make_unique<VKUploadBuffer>(m_pDevice, m_pAllocator);

	return true;
}

VkCommandBuffer VKCommandEngine::GetCommandBuffer() const
{
	return m_Frames[m_uiFrameIndex].m_CommandBuffer;
}

void VKCommandEngine::Reset()
{
	FrameData& frame = m_Frames[m_uiFrameIndex];

	/* Wait only until this slot's own previous submission has retired. The
	   other slot may still be in flight, which is the entire point of double
	   buffering. */
	if (frame.m_uiSubmitValue > 0)
	{
		uint64_t uiCompleted = 0;
		vkGetSemaphoreCounterValue(m_pDevice->Get(), m_Timeline, &uiCompleted);

		if (uiCompleted < frame.m_uiSubmitValue)
		{
			VkSemaphoreWaitInfo waitInfo{};
			waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
			waitInfo.semaphoreCount = 1;
			waitInfo.pSemaphores = &m_Timeline;
			waitInfo.pValues = &frame.m_uiSubmitValue;

			vkWaitSemaphores(m_pDevice->Get(), &waitInfo, UINT64_MAX);
		}
	}

	vkResetCommandPool(m_pDevice->Get(), frame.m_CommandPool, 0);

	/* Safe now that the GPU is done with this slot's upload pages. */
	if (m_pUploadBuffer != nullptr)
		m_pUploadBuffer->Reset();

	m_PendingBarriers.clear();
	m_bIsStarted = false;
	m_bRenderingOpen = false;
}

void VKCommandEngine::Start()
{
	if (m_bIsStarted)
		return;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(GetCommandBuffer(), &beginInfo);

	m_bIsStarted = true;
}

void VKCommandEngine::QueueBarrier(VKResource* pResource, PEResourceState newState)
{
	if (pResource == nullptr)
		return;

	/* An image that has never been transitioned is in VK_IMAGE_LAYOUT_UNDEFINED
	   regardless of what its tracked state says, so it still needs a barrier. */
	if (pResource->GetState() == newState && !pResource->IsLayoutUndefined())
		return;

	m_PendingBarriers.push_back({ pResource, newState });
}

void VKCommandEngine::ApplyBarriers()
{
	if (m_PendingBarriers.empty())
		return;

	if (!m_bIsStarted)
		Start();

	VkCommandBuffer cmd = GetCommandBuffer();

	/* VKResource::Transition records one barrier each. Batching them into a
	   single vkCmdPipelineBarrier2 would be better and is worth doing once the
	   passes exist to show which combinations actually occur. */
	for (const PendingBarrier& barrier : m_PendingBarriers)
		barrier.m_pResource->Transition(cmd, barrier.m_NewState);

	m_PendingBarriers.clear();
}

void VKCommandEngine::Wait(PCommandEngine* pEngine, uint64_t uiValue)
{
	if (pEngine == nullptr || pEngine == this)
		return;

	/* Recorded now, applied at the next Execute(). D3D12's
	   ID3D12CommandQueue::Wait was immediate; Vulkan attaches waits to a
	   submission, so they have to be deferred. */
	VkSemaphoreSubmitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitInfo.semaphore = pEngine->GetTimeline();
	waitInfo.value = uiValue;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	m_WaitSemaphores.push_back(waitInfo);
}

void VKCommandEngine::Execute()
{
	if (!m_bIsStarted)
		return;

	ApplyBarriers();

	VkCommandBuffer cmd = GetCommandBuffer();
	vkEndCommandBuffer(cmd);

	Signal();

	VkCommandBufferSubmitInfo commandInfo{};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	commandInfo.commandBuffer = cmd;

	VkSemaphoreSubmitInfo signalInfo{};
	signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo.semaphore = m_Timeline;
	signalInfo.value = m_uiFenceValue;
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(m_WaitSemaphores.size());
	submitInfo.pWaitSemaphoreInfos = m_WaitSemaphores.data();
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &commandInfo;
	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalInfo;

	{
		/* Every engine shares one graphics queue, and resource loading submits
		   uploads from job threads while the main thread submits the frame. */
		std::lock_guard<std::mutex> lock(m_pDevice->GetQueueMutex());

		if (vkQueueSubmit2(m_Queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			fprintf(stderr, "[vulkan] vkQueueSubmit2 failed for '%s'\n", m_Info.m_Name.c_str());
	}

	m_Frames[m_uiFrameIndex].m_uiSubmitValue = m_uiFenceValue;

	m_WaitSemaphores.clear();
	m_bIsStarted = false;

	/* No AdvanceFrame here. DXCommandEngine::Execute did not advance either,
	   and RenderContext::Present calls AdvanceFrame itself; doing both rotated
	   twice per frame. With two slots that lands back on the same one every
	   frame, so Reset always waited on the immediately preceding submission
	   and CPU and GPU ran fully serialised. */
}

void VKCommandEngine::AdvanceFrame()
{
	m_uiFrameIndex = (m_uiFrameIndex + 1) % m_uiFrameCount;
}

void VKCommandEngine::WaitForGPU()
{
	if (m_uiFenceValue == 0)
		return;

	VkSemaphoreWaitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &m_Timeline;
	waitInfo.pValues = &m_uiFenceValue;

	vkWaitSemaphores(m_pDevice->Get(), &waitInfo, UINT64_MAX);
}

uint64_t VKCommandEngine::GetCompletedValue() const
{
	uint64_t uiValue = 0;
	vkGetSemaphoreCounterValue(m_pDevice->Get(), m_Timeline, &uiValue);

	return uiValue;
}

void VKCommandEngine::CopyResource(VKResource* pDest, VKResource* pSource)
{
	if (pDest == nullptr || pSource == nullptr)
		return;

	if (!m_bIsStarted)
		Start();

	/* D3D12 required the caller to have transitioned both resources already.
	   Doing it here means the copy cannot be issued against a wrong layout,
	   and QueueBarrier is a no-op when the state already matches. */
	QueueBarrier(pSource, E_STATE_COPY_SOURCE);
	QueueBarrier(pDest, E_STATE_COPY_DEST);
	ApplyBarriers();

	VkCommandBuffer cmd = GetCommandBuffer();

	if (pDest->GetKind() == VKResource::E_KIND_IMAGE &&
		pSource->GetKind() == VKResource::E_KIND_IMAGE)
	{
		VkImageCopy region{};
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.layerCount = 1;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.layerCount = 1;
		region.extent = pSource->GetExtent();

		vkCmdCopyImage(cmd,
		               pSource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               pDest->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &region);
	}
	else if (pDest->GetKind() == VKResource::E_KIND_BUFFER &&
	         pSource->GetKind() == VKResource::E_KIND_BUFFER)
	{
		VkBufferCopy region{};
		region.size = pSource->GetSize() < pDest->GetSize() ? pSource->GetSize() : pDest->GetSize();

		vkCmdCopyBuffer(cmd, pSource->GetBuffer(), pDest->GetBuffer(), 1, &region);
	}
	else
	{
		fprintf(stderr, "[vulkan] CopyResource between an image and a buffer needs an explicit region\n");
	}
}
