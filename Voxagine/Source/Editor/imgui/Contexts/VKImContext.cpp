#include "pch.h"
#include "VKImContext.h"

#include "Core/Platform/Rendering/Vulkan/VKRenderContext.h"
#include "External/imgui/imgui.h"

VKImContext::VKImContext(VKRenderContext* pContext) : m_pContext(pContext)
{
}

VKImContext::~VKImContext()
{
	Deinitialize();
}

void VKImContext::NewFrame()
{
	/* Built on the first frame rather than in the constructor: Platform
	   constructs this before ImguiSystem::Initialize calls
	   ImGui::CreateContext, so there is no ImGuiIO to touch yet. */
	if (m_bFontsBuilt)
		return;

	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pPixels = nullptr;
	int iWidth = 0;
	int iHeight = 0;

	/* ImGui::NewFrame asserts unless the atlas has been built. */
	io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

	/* Non-null so ImGui treats the atlas as resident. The pixels are not on
	   the GPU yet - that lands with the ImGui pipeline in Draw below, and
	   until then no ImGui geometry is submitted anyway. */
	io.Fonts->TexID = reinterpret_cast<ImTextureID>(this);

	m_bFontsBuilt = true;
}

void VKImContext::Draw(ImDrawData* pDrawData)
{
	VX_UNUSED(pDrawData);

	/* Needs an ImGui pipeline, a font texture on the GPU and per-frame
	   vertex/index buffers. Until those exist the editor's geometry is not
	   submitted; everything else in the frame still renders. */
}

void VKImContext::Deinitialize()
{
}
