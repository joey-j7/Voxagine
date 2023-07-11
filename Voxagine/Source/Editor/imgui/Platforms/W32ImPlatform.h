#pragma once

#include "ImPlatform.h"

#include <wrl/wrappers/corewrappers.h>
#include <basetsd.h>

class WINWindowContext;

typedef int ImGuiMouseCursor;

class W32ImPlatform : public ImPlatform {
public:
	W32ImPlatform(WINWindowContext* pWindow);

	virtual void Initialize() override;
	virtual void NewFrame() override;

private:
	bool UpdateCursor();
	void UpdateMouse();

	// Win32 Data
	static HWND m_HWnd;
	static INT64 m_iTime;
	static INT64 m_iTicksPerSecond;
	static ImGuiMouseCursor m_LastMouseCursor;
};