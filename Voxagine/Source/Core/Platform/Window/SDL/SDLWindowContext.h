#pragma once

#include "Core/Platform/Window/WindowContext.h"

#include <vulkan/vulkan.h>

#include <vector>

struct SDL_Window;

/* SDL3-backed window. Replaces WINWindowContext and its WndProc; SDL owns the
   event loop, so Poll() drains SDL's queue instead of PeekMessage. */
class SDLWindowContext : public WindowContext
{
public:
	SDLWindowContext(Platform* pPlatform);
	virtual ~SDLWindowContext();

	virtual void Initialize() override;

	/* The SDL_Window itself. Vulkan surface creation goes through
	   CreateSurface below rather than through a raw platform handle. */
	virtual void* GetHandle() override { return m_pWindow; }

	virtual void Poll() override;

	/* Events */
	virtual void OnMove() override;

	/* Instance extensions Vulkan needs to present to this window. */
	std::vector<const char*> GetRequiredInstanceExtensions() const;

	bool CreateSurface(VkInstance instance, VkSurfaceKHR* pSurface) const;

	bool ShouldClose() const { return m_bShouldClose; }

	/* Cursor in framebuffer pixels. SDL reports logical units, which differ
	   from pixels on a high-density window, and everything downstream wants
	   pixels. */
	static void GetMousePositionInPixels(float* pfX, float* pfY);

	/* True from the moment SDL reports a resize until the renderer clears it
	   by rebuilding the swapchain. */
	bool ConsumeResizeRequest();

private:
	/* Events */
	virtual void OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 resolutionDelta) override;
	virtual void OnFullscreenChanged(bool bFullscreen) override;

	void CreateWindow();

	SDL_Window* m_pWindow = nullptr;

	bool m_bShouldClose = false;
	bool m_bResizeRequested = false;
};
