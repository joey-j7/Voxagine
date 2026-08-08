#pragma once

#include "Editor/imgui/Contexts/ImContext.h"

#include "Core/Platform/Rendering/Vulkan/VKDescriptorLayout.h"
#include "Core/Platform/Rendering/Vulkan/VKResource.h"

#include <vulkan/vulkan.h>

#include <array>

class VKRenderContext;

/* ImGui rendering for the Vulkan backend.
 *
 * The editor uses this small custom ImContext/ImPlatform pair rather than
 * upstream imgui_impl_vulkan, so this is a handful of methods rather than a
 * vendored backend.
 *
 * Draw() takes only the draw data: unlike the DX12 version it does not need
 * the command list passed in, because it reads the current command buffer off
 * the render context's engine. It records into whichever render pass instance
 * is already open - RenderContext::Present calls it inside the post processing
 * pass - so it never begins one of its own. */
class VKImContext : public ImContext
{
public:
	VKImContext(VKRenderContext* pContext);
	virtual ~VKImContext();

	virtual void NewFrame() override;
	virtual void Draw(ImDrawData* pDrawData) override;

	virtual void Deinitialize() override;

private:
	bool BuildFontTexture();
	bool BuildPipeline();

	VKRenderContext* m_pContext = nullptr;
	bool m_bFontsBuilt = false;
	bool m_bInitialised = false;
	bool m_bInitFailed = false;

	VKResource m_FontTexture;
	VkSampler m_Sampler = VK_NULL_HANDLE;

	VKDescriptorLayout m_DescriptorLayout;

	/* One set per frame in flight, so a set written this frame is never one
	   the GPU is still reading. */
	std::array<VkDescriptorSet, 3> m_DescriptorSets{};
	uint32_t m_uiSetIndex = 0;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
