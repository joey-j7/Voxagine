#include "pch.h"
#include "WINWindowContext.h"

#include "Core/Platform/Platform.h"
#include "Core/Application.h"

#include <Dbt.h>

#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/Settings.h"

#include "Editor/imgui/Platforms/W32ImPlatform.h"

#include "WndProc.h"
#include "External/optick/optick.h"

WINWindowContext::WINWindowContext(Platform* pPlatform) : WindowContext(pPlatform)
{
	HINSTANCE HInstance = GetModuleHandle(NULL);

	/* Register class */
	RegisterWindowClass(HInstance);

	/* Create window */
	InitializeWindow(HInstance);
}

WINWindowContext::~WINWindowContext()
{
	CoUninitialize();
}

void WINWindowContext::Initialize()
{
	WindowContext::Initialize();

	Settings& settings = m_pPlatform->GetApplication()->GetSettings();
	ShowWindow(m_Handle, settings.IsFullscreen() ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);

	SetWindowLongPtr(m_Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m_pPlatform->GetApplication()));
	GetClientRect(m_Handle, &m_Rect);

	m_v2Size = UVector2(m_Rect.right - m_Rect.left, m_Rect.bottom - m_Rect.top);

	RECT rect;
	GetWindowRect(m_Handle, &rect);
	m_v2Position = UVector2(rect.left, rect.top);
}

void WINWindowContext::Poll()
{
	OPTICK_CATEGORY("Input", Optick::Category::Input);
	OPTICK_EVENT();
	/* Poll window events */
	MSG msg = {};
	bool bContinue = true;

	while (bContinue) {
		bContinue = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);

		if (bContinue)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void WINWindowContext::OnMove()
{
	/* Only store non-fullscreen window position */
	if (!m_pPlatform->GetApplication()->GetSettings().IsFullscreen()) {
		RECT rect;
		GetWindowRect(m_Handle, &rect);

		if (rect.left != 0 && rect.top != 0)
		m_v2Position = UVector2(rect.left, rect.top);
	}
}

void WINWindowContext::OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 resolutionDelta)
{
	/* Only store non-fullscreen window size */
	if (!m_pPlatform->GetApplication()->GetSettings().IsFullscreen()) {
		GetClientRect(m_Handle, &m_Rect);
		m_v2Size = UVector2(m_Rect.right - m_Rect.left, m_Rect.bottom - m_Rect.top);
	}
}

void WINWindowContext::OnFullscreenChanged(bool bFullscreen)
{
	if (bFullscreen) {
		UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		::SetWindowLongW(m_Handle, GWL_STYLE, windowStyle);

		HMONITOR hMonitor = ::MonitorFromWindow(m_Handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(MONITORINFOEX);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		::SetWindowPos(m_Handle, HWND_TOPMOST,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_Handle, SW_MAXIMIZE);
	}
	else {
		// Restore all the window decorators.
		::SetWindowLong(m_Handle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		::SetWindowPos(m_Handle, HWND_NOTOPMOST,
			m_v2Position.x, m_v2Position.y, m_v2Size.x, m_v2Size.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_Handle, SW_NORMAL);
	}
}

void WINWindowContext::RegisterWindowClass(HINSTANCE HInstance)
{
	WNDCLASSEX WindowClass;
	const LPCTSTR Icon = (LPCTSTR)101;

	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = HInstance;
	WindowClass.hIcon = LoadIcon(HInstance, Icon);
	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WindowClass.lpszMenuName = nullptr;
	WindowClass.lpszClassName = m_cWindowClass;
	WindowClass.hIconSm = LoadIcon(WindowClass.hInstance, Icon);

	RegisterClassEx(&WindowClass);
}

void WINWindowContext::InitializeWindow(HINSTANCE HInstance)
{
	Settings& settings = m_pPlatform->GetApplication()->GetSettings();

	const LPCSTR& WindowTitle = settings.GetTitle().c_str();
	const UVector2& v2Size = settings.m_v2InitialWindowSize;
	bool bFullscreen = settings.IsFullscreen();

	m_Rect.top = 0;
	m_Rect.left = 0;
	m_Rect.right = static_cast<LONG>(v2Size.x);
	m_Rect.bottom = static_cast<LONG>(v2Size.y);

	AdjustWindowRect(&m_Rect, WS_OVERLAPPEDWINDOW, FALSE);

	m_Handle = CreateWindowEx(
		0, m_cWindowClass, WindowTitle, bFullscreen ? WS_EX_TOPMOST : WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, m_Rect.right - m_Rect.left, m_Rect.bottom - m_Rect.top, nullptr,
		nullptr, HInstance, nullptr
	);

	if (!m_Handle)
		__debugbreak();
}
