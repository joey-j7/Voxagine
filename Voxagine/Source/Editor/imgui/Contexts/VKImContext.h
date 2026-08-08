#pragma once

#include "Editor/imgui/Contexts/ImContext.h"

class VKRenderContext;

/* ImGui rendering for the Vulkan backend.
 *
 * The editor uses this small custom ImContext/ImPlatform pair rather than
 * upstream imgui_impl_vulkan, so this is a handful of methods rather than a
 * vendored backend.
 *
 * Draw() takes only the draw data: unlike the DX12 version it does not need
 * the command list passed in, because it reads the current command buffer off
 * the render context's engine. */
class VKImContext : public ImContext
{
public:
	VKImContext(VKRenderContext* pContext);
	virtual ~VKImContext();

	virtual void NewFrame() override;
	virtual void Draw(ImDrawData* pDrawData) override;

	virtual void Deinitialize() override;

private:
	VKRenderContext* m_pContext = nullptr;
};
