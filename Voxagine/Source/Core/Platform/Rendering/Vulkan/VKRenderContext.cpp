#include "pch.h"
#include "VKRenderContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Window/SDL/SDLWindowContext.h"
#include "Core/Settings.h"

#include "Core/Resources/Formats/ShaderReference.h"

#include "Core/Platform/Rendering/FrameProfiler.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Vulkan/VKCommandEngine.h"
#include "Core/Platform/Rendering/Vulkan/Managers/VKModelManager.h"
#include "Core/Platform/Rendering/Vulkan/Managers/VKTextureManager.h"

#include "External/imgui/imgui.h"

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

	/* EnableVSync used to be stored and read by nothing; the swapchain took
	   mailbox unconditionally. See the present-mode choice in VKSwapchain. */
	if (!m_Swapchain.Create(&m_Device, m_Surface, size.x, size.y,
	                        m_pPlatform->GetApplication()->GetSettings().IsVSyncEnabled()))
		return false;

	/* The editor shows this; it used to come from the DXGI adapter. */
	const std::string name = m_Device.GetDeviceName();
	m_pPlatform->GetApplication()->GetSettings().SetGPUName(
		std::wstring(name.begin(), name.end()).c_str());

	m_v2ScreenResolution = UVector2(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

	printf("[vulkan] %s, %ux%u, %u swapchain images, %s\n", name.c_str(),
	       m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height,
	       m_Swapchain.GetImageCount(),
	       m_Swapchain.IsVSyncEnabled() ? "FIFO (vsync)" : "mailbox where offered");

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

	/* Decided before any command engine is created: their Initialize()
	   reads this to decide whether to allocate query pools at all.

	   VOXAGINE_PROFILE forces it either way, because the costs worth
	   measuring - the bake, the chunk load - are the ones that behave
	   differently in Release, which is exactly where the default is off. */
	bool bProfiling = m_pPlatform->GetApplication()->GetSettings().IsGPUProfilingEnabled();

	if (const char* pEnv = std::getenv("VOXAGINE_PROFILE"))
		bProfiling = (pEnv[0] != '0');

	FrameProfiler::Get().SetEnabled(bProfiling);

	/* Same set of engines DX12RenderContext created, under the same names -
	   RenderContext::Present looks them up by string. */
	const CommandEngine::Info engines[] = {
		{ CommandEngine::E_COPY,    "Copy" },
		{ CommandEngine::E_DIRECT,  "Direct" },
		{ CommandEngine::E_DIRECT,  "Texture" },
		{ CommandEngine::E_DIRECT,  "VDirect" },
		{ CommandEngine::E_COMPUTE, "Compute" },
	};

	for (const CommandEngine::Info& info : engines)
	{
		std::unique_ptr<PCommandEngine> pEngine =
			std::make_unique<PCommandEngine>(&m_Device, &m_Allocator, info);

		if (!pEngine->Initialize())
		{
			fprintf(stderr, "[vulkan] command engine '%s' failed\n", info.m_Name.c_str());
			return;
		}

		m_pCommandEngines.emplace(info.m_Name, std::move(pEngine));
	}

	m_pTextureManager = std::make_unique<PTextureManager>(this);
	m_pModelManager = std::make_unique<PModelManager>(this);

	RenderContext::Initialize();

	/* Builds the buffers, samplers, mappers and the six passes. It had no
	   caller once DX12RenderContext was deleted, which is why the renderer
	   only ever cleared. */
	InitializeRenderLoop();
}

void VKRenderContext::Deinitialize()
{
	if (m_Device.Get() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_Device.Get());

	/* These belong to RenderContext, whose members are destroyed after this
	   derived body runs - too late, they hold live device objects. Dependency
	   order: passes reference views, engines own the upload pages. */
	m_pRenderPasses.clear();
	m_pComputePasses.clear();
	m_pCommandEngines.clear();

	m_pTextureManager.reset();
	m_pModelManager.reset();

	m_mBuffers.clear();
	m_pMappers.clear();
	m_pSamplers.clear();
	m_pShaders.clear();
	m_pViews.clear();

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

	if (!RenderContext::Present())
		return false;

	/* DX12 pointed the pass whose target type is E_STATE_PRESENT straight at
	   the swapchain buffers, so presenting was a flip. Here the pass owns its
	   own image and it is blitted across, which also rescales when the render
	   resolution differs from the window. */
	PRenderPass* pScreenPass = nullptr;

	for (auto& entry : m_pRenderPasses)
	{
		if (entry.second != nullptr && entry.second->GetData().m_TargetType == E_STATE_PRESENT)
		{
			pScreenPass = entry.second.get();
			break;
		}
	}

	if (pScreenPass == nullptr)
		return false;

	View* pSourceView = pScreenPass->GetTargetView(0);

	if (pSourceView == nullptr || pSourceView->GetNative() == nullptr)
		return false;

	/* Wait on the engine that drew the frame, not on the whole device. */
	PCommandEngine* pDirectEngine = m_pCommandEngines["Direct"].get();

	const VkSemaphore timeline = pDirectEngine != nullptr ? pDirectEngine->GetTimeline() : VK_NULL_HANDLE;
	const uint64_t uiValue = pDirectEngine != nullptr ? pDirectEngine->GetValue() : 0;

	if (!m_Swapchain.BlitAndPresent(pSourceView->GetNative(), timeline, uiValue))
	{
		const UVector2 size = m_pPlatform->GetWindowContext()->GetSize();

		if (size.x == 0 || size.y == 0)
			return false;

		if (!m_Swapchain.Recreate(size.x, size.y))
			return false;
	}

	m_uiFrameIndex = (m_uiFrameIndex + 1) % m_uiFrameCount;
	m_bWorldUpdated = false;

	return true;
}

void VKRenderContext::SetVSyncEnabled(bool bEnabled)
{
	if (!m_bBackendReady || bEnabled == m_Swapchain.IsVSyncEnabled())
		return;

	/* Kept on Settings as well as on the swapchain: the swapchain is the only
	   thing that acts on it, but Settings is what is serialized and what the
	   next run reads. Letting the two disagree is what made this a dead
	   setting in the first place. */
	m_pPlatform->GetApplication()->GetSettings().SetVSync(bEnabled);

	if (!m_Swapchain.SetVSync(bEnabled))
		fprintf(stderr, "[vulkan] swapchain rebuild for vsync=%d failed\n", static_cast<int>(bEnabled));
}

bool VKRenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
{
	if (!m_bBackendReady || uiWidth == 0 || uiHeight == 0)
		return false;

	if (!m_Swapchain.Recreate(uiWidth, uiHeight))
		return false;

	m_v2ScreenResolution = UVector2(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

	if (!RenderContext::OnResize(uiWidth, uiHeight))
		return false;

	/* The base class only notifies SizeChanged listeners (camera, window);
	   the render targets themselves are resized here. Sized from the render
	   resolution rather than the window: with an aspect ratio locked those
	   differ, and a target larger than the viewport the shaders divide by
	   pushes sample coordinates past 1, which a repeating sampler tiles. */
	const UVector2 resolution = GetRenderResolution();

	for (auto& entry : m_pRenderPasses)
	{
		PRenderPass* pPass = entry.second.get();

		if (pPass == nullptr || !pPass->GetData().m_bUseScreenResolution)
			continue;

		const float fScale = pPass->GetData().m_fRenderScale;

		pPass->Resize(UVector2(static_cast<uint32_t>(resolution.x * fScale),
		                       static_cast<uint32_t>(resolution.y * fScale)));
	}

	return true;
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

void RenderContext::Report()
{
	/* DX12 called DXGI's ReportLiveObjects here. Vulkan has no equivalent
	   built in - leaked handles are found with the validation layers'
	   object-lifetime tracking, which reports at instance destruction, so
	   there is nothing to trigger from here. Kept because Platform calls it
	   under _DEBUG. */
	printf("[vulkan] leak reporting is handled by the validation layers\n");
}
