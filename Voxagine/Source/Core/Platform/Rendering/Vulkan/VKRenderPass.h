#pragma once

#include "Core/Platform/Rendering/RenderPass.h"

#include "Core/Platform/Rendering/Vulkan/VKDescriptorLayout.h"
#include "Core/Platform/Rendering/Vulkan/VKPassBindings.h"

#include <vulkan/vulkan.h>

class VKDevice;
class VKCommandEngine;

/* Graphics pass: pipeline, descriptor set and attachments.
 *
 * Uses dynamic rendering rather than VkRenderPass objects, so the pass does
 * not have to precompute a compatible render pass and framebuffer for every
 * attachment combination - which is closer to how the D3D12 version simply
 * set render targets on the command list.
 *
 * Not yet implemented; declared so CommandEngine's inline Begin/Draw/End can
 * resolve while the pass layer is being ported. */
class VKRenderPass : public RenderPass
{
public:
	VKRenderPass(PRenderContext* pContext);
	VKRenderPass(PRenderContext* pContext, const Data& data);
	virtual ~VKRenderPass();

	virtual void Begin(PCommandEngine* pEngine) override;
	virtual void Draw(PCommandEngine* pEngine) override;
	virtual void End(PCommandEngine* pEngine) override;

	virtual void Clear(PCommandEngine* pEngine) override;

	virtual View* GetTargetView(uint32_t i = 0) const override;
	virtual View* GetDepthView() const override;

	virtual void Resize(UVector2 uSize) override;

	virtual UVector2 GetTargetSize() const override;

protected:
	virtual void Init(const Data& data) override;

private:
	bool CreatePipeline();
	bool CreateAttachments();

	/* Fills a freshly allocated set from m_Bindings. */
	void WriteDescriptors(PCommandEngine* pEngine, VkDescriptorSet set);

	VKDevice* m_pDevice = nullptr;

	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	VKDescriptorLayout m_DescriptorLayout;
	std::vector<VKPassBinding> m_Bindings;

	UVector2 m_TargetSize = UVector2(0, 0);

	/* True between vkCmdBeginRendering and vkCmdEndRendering. */
	bool m_bIsRendering = false;
};
