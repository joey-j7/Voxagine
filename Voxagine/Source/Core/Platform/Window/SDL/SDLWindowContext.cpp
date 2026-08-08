#include "pch.h"
#include "SDLWindowContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/Settings.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

SDLWindowContext::SDLWindowContext(Platform* pPlatform) : WindowContext(pPlatform)
{
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
		fprintf(stderr, "[sdl] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
		return;
	}

	CreateWindow();
}

SDLWindowContext::~SDLWindowContext()
{
	if (m_pWindow != nullptr)
	{
		SDL_DestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void SDLWindowContext::CreateWindow()
{
	Settings& settings = m_pPlatform->GetApplication()->GetSettings();

	SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
	if (settings.IsFullscreen())
		flags |= SDL_WINDOW_FULLSCREEN;

	const UVector2 resolution = settings.m_v2InitialWindowSize;

	m_pWindow = SDL_CreateWindow(settings.GetTitle().c_str(),
	                             static_cast<int>(resolution.x),
	                             static_cast<int>(resolution.y),
	                             flags);

	if (m_pWindow == nullptr)
	{
		fprintf(stderr, "[sdl] SDL_CreateWindow failed: %s\n", SDL_GetError());
		return;
	}

	m_v2Size = resolution;
}

void SDLWindowContext::Initialize()
{
	WindowContext::Initialize();

	if (m_pWindow == nullptr)
		return;

	int iWidth = 0;
	int iHeight = 0;
	SDL_GetWindowSizeInPixels(m_pWindow, &iWidth, &iHeight);
	m_v2Size = UVector2(static_cast<uint32_t>(iWidth), static_cast<uint32_t>(iHeight));

	int iX = 0;
	int iY = 0;
	SDL_GetWindowPosition(m_pWindow, &iX, &iY);
	m_v2Position = UVector2(static_cast<uint32_t>(iX), static_cast<uint32_t>(iY));
}

std::vector<const char*> SDLWindowContext::GetRequiredInstanceExtensions() const
{
	uint32_t uiCount = 0;
	const char* const* ppNames = SDL_Vulkan_GetInstanceExtensions(&uiCount);

	if (ppNames == nullptr)
	{
		fprintf(stderr, "[sdl] SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
		return {};
	}

	return std::vector<const char*>(ppNames, ppNames + uiCount);
}

bool SDLWindowContext::CreateSurface(VkInstance instance, VkSurfaceKHR* pSurface) const
{
	if (m_pWindow == nullptr)
		return false;

	if (!SDL_Vulkan_CreateSurface(m_pWindow, instance, nullptr, pSurface))
	{
		fprintf(stderr, "[sdl] SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
		return false;
	}

	return true;
}

void SDLWindowContext::Poll()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			m_bShouldClose = true;
			m_pPlatform->GetApplication()->Exit();
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		{
			const uint32_t uiWidth = static_cast<uint32_t>(event.window.data1);
			const uint32_t uiHeight = static_cast<uint32_t>(event.window.data2);


			const IVector2 delta(static_cast<int32_t>(uiWidth) - static_cast<int32_t>(m_v2Size.x),
			                     static_cast<int32_t>(uiHeight) - static_cast<int32_t>(m_v2Size.y));

			OnResize(uiWidth, uiHeight, delta);

			/* Drives the swapchain and render target recreation; under Win32
			   this went through the message pump instead. */
			if (m_pPlatform->GetRenderContext() != nullptr)
				m_pPlatform->GetRenderContext()->OnResize(uiWidth, uiHeight);

			break;
		}

		case SDL_EVENT_WINDOW_MOVED:
			OnMove();
			break;

		default:
			break;
		}
	}
}

void SDLWindowContext::OnMove()
{
	/* Only store non-fullscreen window position */
	if (m_pPlatform->GetApplication()->GetSettings().IsFullscreen())
		return;

	int iX = 0;
	int iY = 0;
	SDL_GetWindowPosition(m_pWindow, &iX, &iY);

	if (iX > 0 && iY > 0)
		m_v2Position = UVector2(static_cast<uint32_t>(iX), static_cast<uint32_t>(iY));
}

void SDLWindowContext::OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 /*resolutionDelta*/)
{
	m_v2Size = UVector2(a_uiWidth, a_uiHeight);
	m_bResizeRequested = true;
}

void SDLWindowContext::OnFullscreenChanged(bool bFullscreen)
{
	if (m_pWindow == nullptr)
		return;

	SDL_SetWindowFullscreen(m_pWindow, bFullscreen);
	m_bResizeRequested = true;
}

bool SDLWindowContext::ConsumeResizeRequest()
{
	const bool bRequested = m_bResizeRequested;
	m_bResizeRequested = false;

	return bRequested;
}
