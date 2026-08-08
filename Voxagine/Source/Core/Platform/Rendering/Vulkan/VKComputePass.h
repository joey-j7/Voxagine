#pragma once

#include "Core/Platform/Rendering/ComputePass.h"

#include "Core/Platform/Rendering/Vulkan/VKDescriptorLayout.h"

#include <vulkan/vulkan.h>

class VKDevice;

/* Compute pass: pipeline plus descriptor set, dispatched over Data::m_ThreadGroup.
 *
 * Not yet implemented; declared so CommandEngine::Compute can resolve while
 * the pass layer is being ported. */
class VKComputePass : public ComputePass
{
public:
	VKComputePass(PRenderContext* pContext, const Data& data);
	virtual ~VKComputePass();

	virtual void Compute(PCommandEngine* pEngine) override;

protected:
	virtual void Init(const Data& data) override;

private:
	bool CreatePipeline();

	VKDevice* m_pDevice = nullptr;

	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	VKDescriptorLayout m_DescriptorLayout;
};
