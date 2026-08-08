#include "pch.h"

#include "Core/Platform/Input/SDL/SDLMouse.h"
#include "MouseController.h"

#include "Core/Platform/Window/WindowContext.h"



// Initialization of boolean indicating libraries being initialized
bool MouseController::m_bInitLibraries = false;

MouseController::MouseController()
	: InputController()
{
}

MouseController::~MouseController()
{
}

void MouseController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Create DirectX mouse singleton
		// Set DirectX mouse Window handle
		new Mouse();
		Mouse::Get().SetWindow(GetWindowContext()->GetHandle());

		m_bInitLibraries = true;
	}
}

void MouseController::OnUninitialize()
{
	// Delete DirectX mouse singleton
	delete &Mouse::Get();
}

void MouseController::OnUpdate()
{
	// Get the mouse and state
	Mouse::State mouseState = Mouse::Get().GetState();

	// Update mouse connected state
	SetConnected(Mouse::Get().IsConnected());

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