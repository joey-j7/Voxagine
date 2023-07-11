#include "pch.h"
#include "W32ImPlatform.h"

#include "External/imgui/imgui.h"

#include <wrl/wrappers/corewrappers.h>

#include <tchar.h>
#include <profileapi.h>

#include "Core/Platform/Window/Windows/WINWindowContext.h"

// Win32 Data
HWND W32ImPlatform::m_HWnd = 0;
INT64 W32ImPlatform::m_iTime = 0;
INT64 W32ImPlatform::m_iTicksPerSecond = 0;
ImGuiMouseCursor W32ImPlatform::m_LastMouseCursor = ImGuiMouseCursor_COUNT;

W32ImPlatform::W32ImPlatform(WINWindowContext* pWindow)
{
	if (!::QueryPerformanceFrequency((LARGE_INTEGER *)&m_iTicksPerSecond))
		return;

	if (!::QueryPerformanceCounter((LARGE_INTEGER *)&m_iTime))
		return;

	// Setup back-end capabilities flags
	m_HWnd = *(HWND*)pWindow->GetHandle();
}

void W32ImPlatform::Initialize()
{
	ImGuiIO& Io = ImGui::GetIO();

	Io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	Io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
	Io.ImeWindowHandle = m_HWnd;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
	Io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	Io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	Io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	Io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	Io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	Io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	Io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	Io.KeyMap[ImGuiKey_Home] = VK_HOME;
	Io.KeyMap[ImGuiKey_End] = VK_END;
	Io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
	Io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	Io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	Io.KeyMap[ImGuiKey_Space] = VK_SPACE;
	Io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	Io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;

	Io.KeyMap[ImGuiKey_A] = 'A';
	Io.KeyMap[ImGuiKey_C] = 'C';
	Io.KeyMap[ImGuiKey_V] = 'V';
	Io.KeyMap[ImGuiKey_X] = 'X';
	Io.KeyMap[ImGuiKey_Y] = 'Y';
	Io.KeyMap[ImGuiKey_Z] = 'Z';
}

void W32ImPlatform::NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect;
	::GetClientRect(m_HWnd, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	INT64 currentTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
	io.DeltaTime = (float)(currentTime - m_iTime) / m_iTicksPerSecond;
	m_iTime = currentTime;

	// Read keyboard modifiers inputs
	io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;
	// io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

	// Update OS mouse position
	UpdateMouse();

	// Update OS mouse cursor with the cursor requested by imgui
	ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (m_LastMouseCursor != mouse_cursor)
	{
		m_LastMouseCursor = mouse_cursor;
		UpdateCursor();
	}
}

bool W32ImPlatform::UpdateCursor()
{
	ImGuiIO& Io = ImGui::GetIO();

	if (Io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return false;

	ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
	if (ImguiCursor == ImGuiMouseCursor_None || Io.MouseDrawCursor)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		::SetCursor(NULL);
	}
	else
	{
		// Show OS mouse cursor
		LPSTR Win32Cursor = IDC_ARROW;

		switch (ImguiCursor)
		{
		case ImGuiMouseCursor_Arrow: Win32Cursor = IDC_ARROW; break;
		case ImGuiMouseCursor_TextInput: Win32Cursor = IDC_IBEAM; break;
		case ImGuiMouseCursor_ResizeAll: Win32Cursor = IDC_SIZEALL; break;
		case ImGuiMouseCursor_ResizeEW: Win32Cursor = IDC_SIZEWE; break;
		case ImGuiMouseCursor_ResizeNS: Win32Cursor = IDC_SIZENS; break;
		case ImGuiMouseCursor_ResizeNESW: Win32Cursor = IDC_SIZENESW; break;
		case ImGuiMouseCursor_ResizeNWSE: Win32Cursor = IDC_SIZENWSE; break;
		case ImGuiMouseCursor_Hand: Win32Cursor = IDC_HAND; break;
		}
		
		::SetCursor(::LoadCursor(NULL, Win32Cursor));
	}

	return true;
}

void W32ImPlatform::UpdateMouse()
{
	ImGuiIO& io = ImGui::GetIO();

	// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
	if (io.WantSetMousePos)
	{
		POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
		::ClientToScreen(m_HWnd, &pos);
		::SetCursorPos(pos.x, pos.y);
	}

	// Set mouse position
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	POINT pos;

	if (::GetActiveWindow() == m_HWnd && ::GetCursorPos(&pos)) {
		if (::ScreenToClient(m_HWnd, &pos))
			io.MousePos = ImVec2((float)pos.x, (float)pos.y);
	}
}
