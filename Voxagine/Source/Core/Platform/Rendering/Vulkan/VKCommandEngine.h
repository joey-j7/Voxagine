#pragma once

#include "Core/Platform/Rendering/CommandEngine.h"

#include "Core/Platform/Rendering/Vulkan/VKResource.h"
#include "Core/Platform/Rendering/Vulkan/VKUploadBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

class VKDevice;
class VKAllocator;

/* Command recording and submission.
 *
 * The engine's CommandEngine models a D3D12 command queue plus allocator plus
 * list, with an ID3D12Fence carrying a monotonically increasing value that
 * other engines can wait on. Vulkan timeline semaphores are the direct
 * equivalent, so GetValue()/Wait() keep their meaning.
 *
 * Reset() deliberately does NOT wait for the GPU. The DX12 version called
 * WaitForGPU() unconditionally every frame, which serialised CPU and GPU and
 * defeated the double-buffered swapchain; here each frame slot waits only on
 * its own previous submission. */
class VKCommandEngine : public CommandEngine
{
	/* RenderContext drives frame advance directly; CommandEngine declares
	   AdvanceFrame protected and friendship does not inherit. */
	friend class RenderContext;

public:
	static const uint32_t m_uiFrameCount = 2;

	/* Timestamps per frame slot, i.e. up to 32 Begin/End pass pairs. Every
	   pass this engine records in one frame shares this budget; running out
	   drops (and warns once) rather than crashing. */
	static const uint32_t m_uiMaxTimestampsPerFrame = 64;

	VKCommandEngine(VKDevice* pDevice, const VKAllocator* pAllocator, const Info& info);
	virtual ~VKCommandEngine();

	bool Initialize();

	/* Recycles this frame slot's allocator once its previous submission has
	   retired. Waits on one fence value, not on the whole device. */
	virtual void Reset() override;

	virtual void Start() override;
	virtual void Execute() override;

	virtual void Wait(PCommandEngine* pEngine, uint64_t uiValue) override;

	virtual void ApplyBarriers() override;

	virtual void WaitForGPU() override;

	/* Queues a barrier to be flushed by the next ApplyBarriers(). Batching
	   matters: one vkCmdPipelineBarrier2 with N barriers is materially cheaper
	   than N calls. */
	void QueueBarrier(VKResource* pResource, PEResourceState newState);

	VkCommandBuffer GetCommandBuffer() const;

	VkSemaphore GetTimeline() const { return m_Timeline; }

	/* Timeline value the GPU has actually reached. Replaces
	   ID3D12Fence::GetCompletedValue(). */
	uint64_t GetCompletedValue() const;

	/* Whole-resource copy, as ID3D12GraphicsCommandList::CopyResource was.
	   Transitions both sides into the required layouts first. */
	void CopyResource(VKResource* pDest, VKResource* pSource);

	VKUploadBuffer* GetUploadBuffer() { return m_pUploadBuffer.get(); }

	/* Dynamic rendering is a property of the command buffer, not of a pass, so
	   whether a render pass instance is open has to be tracked here. Each pass
	   tracking its own flag meant one pass could open rendering and a second
	   could open another inside it. */
	uint32_t GetFrameIndex() const { return m_uiFrameIndex; }

	bool IsRenderingOpen() const { return m_bRenderingOpen; }
	void SetRenderingOpen(bool bOpen) { m_bRenderingOpen = bOpen; }

	/* RENDERING_PLAN.md Phase 0. Writes a GPU timestamp into this frame
	   slot's query pool and returns its index, or UINT32_MAX when profiling
	   is disabled or the device has no timestamp support - callers do not
	   need to check that themselves. Pair with WriteTimestampEnd(). */
	uint32_t WriteTimestampBegin();

	/* Pairs with the index from WriteTimestampBegin() under `name`; a
	   UINT32_MAX index (profiling disabled, or the frame ran out of query
	   slots) is a no-op. Results are read back non-stalling in Reset(),
	   once this slot's submission that recorded them has retired. */
	void WriteTimestampEnd(const std::string& name, uint32_t uiBeginIndex);

protected:
	virtual void AdvanceFrame() override;

private:
	/* One frame's worth of paired GPU timestamps awaiting readback. */
	struct PendingPassQuery
	{
		std::string m_Name;
		uint32_t m_uiBeginIndex;
		uint32_t m_uiEndIndex;
	};

	struct FrameData
	{
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

		/* Timeline value this slot's last submission signals. */
		uint64_t m_uiSubmitValue = 0;

		/* One query pool per frame slot, exactly like the command pool
		   above: reading back slot i's queries only ever happens after
		   waiting for slot i's own previous submission, so a pool being
		   read never races a new recording into it. */
		VkQueryPool m_QueryPool = VK_NULL_HANDLE;
		uint32_t m_uiQueryCursor = 0;
		std::vector<PendingPassQuery> m_PendingQueries;
	};

	struct PendingBarrier
	{
		VKResource* m_pResource;
		PEResourceState m_NewState;
	};

	VKDevice* m_pDevice = nullptr;
	const VKAllocator* m_pAllocator = nullptr;

	VkQueue m_Queue = VK_NULL_HANDLE;
	uint32_t m_uiQueueFamily = 0;

	VkSemaphore m_Timeline = VK_NULL_HANDLE;

	FrameData m_Frames[m_uiFrameCount];
	uint32_t m_uiFrameIndex = 0;

	std::vector<PendingBarrier> m_PendingBarriers;

	bool m_bRenderingOpen = false;

	/* Cached once in Initialize(): profiling asked for and the device can
	   actually do it. Checked on every WriteTimestamp* call so passes never
	   have to know why timing might be unavailable. */
	bool m_bTimingEnabled = false;

	/* Engines this one must wait on before its next submit. */
	std::vector<VkSemaphoreSubmitInfo> m_WaitSemaphores;

	std::unique_ptr<VKUploadBuffer> m_pUploadBuffer;
};
