#include "pch.h"
#include "VKImContext.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"

VKImContext::VKImContext(VKRenderContext* pContext) : m_pContext(pContext)
{
}

VKImContext::~VKImContext()
{
	Deinitialize();
}

void VKImContext::NewFrame()
{
	/* Font atlas upload and per-frame vertex buffers land with the pass layer;
	   ImGui::NewFrame itself is driven by ImguiSystem. */
}

void VKImContext::Draw(ImDrawData* pDrawData)
{
	VX_UNUSED(pDrawData);

	/* Needs the ImGui pipeline, which needs VKRenderPass. Until then the
	   editor's geometry is simply not submitted - the engine still runs. */
}

void VKImContext::Deinitialize()
{
}
