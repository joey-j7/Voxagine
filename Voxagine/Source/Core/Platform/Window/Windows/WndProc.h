#pragma once

#include "External/DirectXTK12/Keyboard.h"
#include "External/DirectXTK12/Mouse.h"

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	Application* Engine = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (!Engine) return DefWindowProc(hWnd, message, wParam, lParam);

	RenderContext* pRenderContext = Engine->GetPlatform().GetRenderContext();
	WindowContext* pWindowContext = Engine->GetPlatform().GetWindowContext();

	Settings& settings = Engine->GetSettings();

	static bool bInSizemove = false;
	static bool bMinimized = false;
	static bool bIsFullscreen = settings.IsFullscreen();

	switch (message)
	{
	case WM_CREATE:
		/* TODO: Create audio device */
		break;

	case WM_QUIT:
		Engine->Exit();
		return TRUE;

	case WM_CLOSE:
		Engine->Exit();
		return TRUE;

	case WM_DEVICECHANGE:
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);

			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					/*auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (Engine)
							Engine->NewAudioDevice();
					}*/
				}
			}
		}
		break;

		case DBT_DEVICEREMOVECOMPLETE:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				/*if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (Engine)
							Engine->NewAudioDevice();
					}
				}*/
			}
		}
		break;
		}
		return 0;

	case WM_PAINT:
		/*	if (bInSizemove && Engine)
				Engine->Update();*/
		if (!bInSizemove)
		{
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_MOVE:
		if (!bIsFullscreen && pWindowContext)
			pWindowContext->OnMove();

		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!bMinimized)
			{
				bMinimized = true;
				Engine->SetSuspended(true);
			}
		}
		else if (bMinimized)
		{
			bMinimized = false;
			Engine->SetSuspended(false);
		}
		else if (!bInSizemove && Engine)
		{
			pRenderContext->OnResize(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		bInSizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		bInSizemove = false;

		if (!bIsFullscreen && Engine)
		{
			RECT Rect;
			GetClientRect(hWnd, &Rect);

			pRenderContext->OnResize(Rect.right - Rect.left, Rect.bottom - Rect.top);
		}
		break;

	case WM_GETMINMAXINFO:
	{
		auto info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
	}
	break;

	case WM_ACTIVATEAPP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);

		/*if (Engine)
		{
			if (wParam) Engine->OnActivate();
			else Engine->OnDeactivate();
		}*/

		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			Engine->SetSuspended(true);
			return TRUE;

		case PBT_APMRESUMESUSPEND:
			if (!bMinimized)
				Engine->SetSuspended(false);

			return TRUE;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SYSKEYDOWN:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);

		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
			settings.SetFullscreen(!settings.IsFullscreen());

		break;

	case WM_CHAR:
#if EDITOR == 1
		if (wParam > 0 && wParam < 0x10000) {
			ImGuiIO& io = ImGui::GetIO();
			io.AddInputCharacter((unsigned short)wParam);
		}
#endif
		break;

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		break;
	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}