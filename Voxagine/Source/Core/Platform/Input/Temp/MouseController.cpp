#include "pch.h"
#include "MouseController.h"

#include "Core/Platform/Window/WindowContext.h"

#ifdef _WINDOWS
#include <wrl/wrappers/corewrappers.h>
#include <External/DirectXTK12/Mouse.h>
#endif

#ifdef _ORBIS
#include <mouse.h>
#endif

// Initialization of boolean indicating libraries being initialized
bool MouseController::m_bInitLibraries = false;

MouseController::MouseController()
	: InputController()
{
}

MouseController::~MouseController()
{
}

#ifdef _WINDOWS
void MouseController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Create DirectX mouse singleton
		// Set DirectX mouse Window handle
		new DirectX::Mouse();
		DirectX::Mouse::Get().SetWindow(*(HWND*)GetWindowContext()->GetHandle());

		m_bInitLibraries = true;
	}
}

void MouseController::OnUninitialize()
{
	// Delete DirectX mouse singleton
	delete &DirectX::Mouse::Get();
}

void MouseController::OnUpdate()
{
	// Get the mouse and state
	DirectX::Mouse::State mouseState = DirectX::Mouse::Get().GetState();

	// Update mouse connected state
	SetConnected(DirectX::Mouse::Get().IsConnected());

	// Update mouse button states
	UpdateKeyState(IK_MOUSEBUTTONLEFT, mouseState.leftButton);
	UpdateKeyState(IK_MOUSEBUTTONMIDDLE, mouseState.middleButton);
	UpdateKeyState(IK_MOUSEBUTTONRIGHT, mouseState.rightButton);
	UpdateKeyState(IK_MOUSEBUTTONOPTIONAL, mouseState.xButton1);
	UpdateKeyState(IK_MOUSEBUTTONOPTIONAL2, mouseState.xButton2);

	// Update mouse states for axises in delta
	UpdateAxisValue(IK_MOUSEAXISXDELTA, mouseState.x - m_Axises[IK_MOUSEAXISX]);
	UpdateAxisValue(IK_MOUSEAXISYDELTA, mouseState.y - m_Axises[IK_MOUSEAXISY]);
	UpdateAxisValue(IK_MOUSEWHEELAXISDELTA, mouseState.scrollWheelValue - m_Axises[IK_MOUSEWHEELAXIS]);

	// Update mouse axis values
	UpdateAxisValue(IK_MOUSEAXISX, static_cast<float>(mouseState.x));
	UpdateAxisValue(IK_MOUSEAXISY, static_cast<float>(mouseState.y));
	UpdateAxisValue(IK_MOUSEWHEELAXIS, static_cast<float>(mouseState.scrollWheelValue));
}
#endif

#ifdef _ORBIS
void MouseController::OnInitialize()
{
}

void MouseController::OnUninitialize()
{
}

void MouseController::OnUpdate()
{
}
#endif

void MouseController::InitializeButtons()
{
	// Mouse input key for states initialization
	AddInputKeyStateMap(IK_MOUSEBUTTONLEFT);
	AddInputKeyStateMap(IK_MOUSEBUTTONMIDDLE);
	AddInputKeyStateMap(IK_MOUSEBUTTONRIGHT);
	AddInputKeyStateMap(IK_MOUSEBUTTONOPTIONAL);
	AddInputKeyStateMap(IK_MOUSEBUTTONOPTIONAL2);
}

void MouseController::InitializeAxises()
{
	// Mouse input key for axises values initialization
	AddInputKeyAxisMap(IK_MOUSEAXISX);
	AddInputKeyAxisMap(IK_MOUSEAXISY);
	AddInputKeyAxisMap(IK_MOUSEWHEELAXIS);

	// Mouse input key for axises deltas initialization
	AddInputKeyAxisMap(IK_MOUSEAXISXDELTA);
	AddInputKeyAxisMap(IK_MOUSEAXISYDELTA);
	AddInputKeyAxisMap(IK_MOUSEWHEELAXISDELTA);
}