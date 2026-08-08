#pragma once

#include "Editor/imgui/Contexts/ImContext.h"

#include "Core/Platform/Rendering/Vulkan/VKDescriptorLayout.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

class VKRenderContext;
class View;

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
 * pass - so it never begins one of its own.
 *
 * ImTextureID is a View*, the same one TextureReference hands out; the font
 * atlas is a View too, so there is only one case to resolve. */
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

	/* Number of frames the swapchain can have in flight; a set written this
	   frame must not be one the GPU is still reading. */
	static const uint32_t m_uiFrameSlots = 3;

	/* Per frame slot: a ceiling on distinct textures in one frame, not on
	   draw commands. */
	static const uint32_t m_uiTexturesPerFrame = 64;

	/* Set for pTexture in the current frame slot, with the projection and
	   sampler alongside. VK_NULL_HANDLE once the frame runs out. */
	VkDescriptorSet GetTextureSet(View* pTexture, const VkDescriptorBufferInfo& constants);

	VKRenderContext* m_pContext = nullptr;
	bool m_bFontsBuilt = false;
	bool m_bInitialised = false;
	bool m_bInitFailed = false;

	/* Owned by the texture manager; this is just the pointer draw commands
	   are matched against. */
	View* m_pFontTexture = nullptr;
	VkSampler m_Sampler = VK_NULL_HANDLE;

	VKDescriptorLayout m_DescriptorLayout;

	/* Grown to the high-water mark rather than allocated up front - a frame
	   that only draws text needs one set, not 64. */
	std::array<std::vector<VkDescriptorSet>, m_uiFrameSlots> m_DescriptorSets;
	std::vector<View*> m_FrameTextures;
	uint32_t m_uiFrameSlot = 0;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
