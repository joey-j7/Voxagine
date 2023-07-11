#pragma once

#include "Core/Platform/Window/WindowContext.h"
#include <wrl/wrappers/corewrappers.h>

class ImguiSystem;

class WINWindowContext : public WindowContext
{
public:
	WINWindowContext(Platform* pPlatform);
	virtual ~WINWindowContext();

	virtual void Initialize() override;

	virtual void* GetHandle() override {
		return &m_Handle;
	}

	virtual void Poll() override;

	/* Events */
	virtual void OnMove() override;

private:
	/* Events */
	virtual void OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 resolutionDelta) override;
	virtual void OnFullscreenChanged(bool bFullscreen) override;

	void RegisterWindowClass(HINSTANCE HInstance);
	void InitializeWindow(HINSTANCE HInstance);

	HWND m_Handle;
	RECT m_Rect;

	const LPCSTR m_cWindowClass = "VoxagineWindow";
};