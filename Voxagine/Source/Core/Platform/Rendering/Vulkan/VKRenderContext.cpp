#include "pch.h"
#include "VKRenderContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Window/SDL/SDLWindowContext.h"
#include "Core/Settings.h"

#include "Core/Resources/Formats/ShaderReference.h"

#include <cstdio>

VKRenderContext::VKRenderContext(Platform* pPlatform) : RenderContext(pPlatform)
{
}

VKRenderContext::~VKRenderContext()
{
	Deinitialize();
}

bool VKRenderContext::InitializeBackend()
{
	SDLWindowContext* pWindow = static_cast<SDLWindowContext*>(m_pPlatform->GetWindowContext());

	if (pWindow == nullptr)
	{
		fprintf(stderr, "[vulkan] no window context to present to\n");
		return false;
	}

	bool bValidation = false;
#ifdef _DEBUG
	bValidation = true;
#endif

	if (!m_Device.CreateInstance(pWindow->GetRequiredInstanceExtensions(), bValidation))
		return false;

	if (!pWindow->CreateSurface(m_Device.GetInstance(), &m_Surface))
		return false;

	if (!m_Device.CreateDevice(m_Surface))
		return false;

	m_Allocator.Initialize(&m_Device);

	const UVector2 size = pWindow->GetSize();

	if (!m_Swapchain.Create(&m_Device, m_Surface, size.x, size.y))
		return false;

	/* The editor shows this; it used to come from the DXGI adapter. */
	const std::string name = m_Device.GetDeviceName();
	m_pPlatform->GetApplication()->GetSettings().SetGPUName(
		std::wstring(name.begin(), name.end()).c_str());

	m_v2ScreenResolution = UVector2(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

	printf("[vulkan] %s, %ux%u, %u swapchain images\n", name.c_str(),
	       m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height,
	       m_Swapchain.GetImageCount());

	return true;
}

void VKRenderContext::Initialize()
{
	m_bBackendReady = InitializeBackend();

	if (!m_bBackendReady)
	{
		fprintf(stderr, "[vulkan] backend initialization failed; renderer is inert\n");
		return;
	}

	RenderContext::Initialize();
}

void VKRenderContext::Deinitialize()
{
	if (m_Device.Get() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_Device.Get());

	m_Swapchain.Destroy();

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_Device.GetInstance(), m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}

	m_Device.Destroy();
	m_bBackendReady = false;
}

void VKRenderContext::Clear()
{
	if (!m_bBackendReady)
		return;

	RenderContext::Clear();
}

bool VKRenderContext::Present()
{
	if (!m_bBackendReady)
		return false;

	/* Until the pass layer is ported this presents the clear colour only,
	   which is the first milestone: a window with a Vulkan clear screen. */
	const float fClear[4] = {
		m_pSettings != nullptr ? 0.1f : 0.1f,
		0.1f,
		0.12f,
		1.f
	};

	if (!m_Swapchain.ClearAndPresent(fClear))
	{
		/* Out of date: rebuild at the window's current size. */
		const UVector2 size = m_pPlatform->GetWindowContext()->GetSize();

		if (size.x == 0 || size.y == 0)
			return false;

		if (!m_Swapchain.Recreate(size.x, size.y))
			return false;
	}

	m_uiFrameIndex = (m_uiFrameIndex + 1) % m_uiFrameCount;
	++m_uiDrawnFrames;

	return true;
}

bool VKRenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
{
	if (!m_bBackendReady || uiWidth == 0 || uiHeight == 0)
		return false;

	if (!m_Swapchain.Recreate(uiWidth, uiHeight))
		return false;

	m_v2ScreenResolution = UVector2(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

	return RenderContext::OnResize(uiWidth, uiHeight);
}

void VKRenderContext::LoadShader(ShaderReference* pShaderReference)
{
	/* Shaders are compiled to SPIR-V ahead of time by cmake/Shaders.cmake.
	   Wiring ShaderReference to those modules belongs with VKShader, which
	   lands alongside the pass layer. */
	VX_UNUSED(pShaderReference);
}

void VKRenderContext::DestroyShader(const ShaderReference* pShaderReference)
{
	VX_UNUSED(pShaderReference);
}
