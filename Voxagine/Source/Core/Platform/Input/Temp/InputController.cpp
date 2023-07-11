#include "pch.h"
#include "InputController.h"

#include "Core/Platform/Window/WindowContext.h"

InputController::InputController()
{
}

InputController::~InputController()
{
}

void InputController::Initialize(WindowContext* pWindowContext)
{
	m_pInputContext = pWindowContext;

	OnInitialize();

	InitializeButtons();
	InitializeAxises();
}

void InputController::UnInitialize()
{
	OnUninitialize();
}

void InputController::Update()
{
	OnUpdate();
}

bool InputController::IsConnected() const
{
	return m_bConnected;
}

bool InputController::IsKeyPressed(InputKey inputKey) const
{
	return GetKeyStatus(inputKey) == IKS_PRESSED;
}

bool InputController::IsKeyDown(InputKey inputKey) const
{
	return GetKeyStatus(inputKey) == IKS_HELD;
}

bool InputController::IsKeyReleased(InputKey inputKey) const
{
	return GetKeyStatus(inputKey) == IKS_RELEASED;
}

InputKeyStatus InputController::GetKeyStatus(InputKey inputKey) const
{
	std::unordered_map< InputKey, InputKeyStatus>::const_iterator found = m_Buttons.find(inputKey);
	return (found == m_Buttons.end()) ? InputKeyStatus::IKS_NONE : found->second;
}

bool InputController::HasKey(InputKey inputKey) const
{
	std::unordered_map< InputKey, InputKeyStatus>::const_iterator found = m_Buttons.find(inputKey);
	return (found != m_Buttons.end());
}

bool InputController::IsKeyAxisDown(InputKey inputKey) const
{
	return (GetAxisValue(inputKey).Value >= 0.5f);
}

InputBindingAxisValue InputController::GetAxisValue(InputKey inputKey) const
{
	std::unordered_map< InputKey, float>::const_iterator found = m_Axises.find(inputKey);
	InputBindingAxisValue axisHandled;

	if (found == m_Axises.end())
	{
		axisHandled.Proccessed = false;
		axisHandled.Value = 0.f;
	}
	else
	{
		axisHandled.Proccessed = true;
		axisHandled.Value = found->second;
	}

	return axisHandled;
}

void InputController::AddInputKeyStateMap(InputKey inputKey, InputKeyStatus inputKeyStatus)
{
	m_Buttons[inputKey] = inputKeyStatus;
}

void InputController::AddInputKeyAxisMap(InputKey inputKey, float fAxisStatus)
{
	m_Axises[inputKey] = fAxisStatus;
}

void InputController::UpdateKeyState(InputKey inputKey, bool bKeyPressed)
{
	std::unordered_map< InputKey, InputKeyStatus>::iterator found = m_Buttons.find(inputKey);

	if (found == m_Buttons.end())
		return;

	switch (m_Buttons[inputKey])
	{
	case (InputKeyStatus::IKS_NONE):
		if (bKeyPressed)
			found->second = InputKeyStatus::IKS_PRESSED;
		break;
	case (InputKeyStatus::IKS_PRESSED):
		found->second = (bKeyPressed) ? InputKeyStatus::IKS_HELD : InputKeyStatus::IKS_RELEASED;
		break;
	case (InputKeyStatus::IKS_HELD):
		if (!bKeyPressed)
			found->second = InputKeyStatus::IKS_RELEASED;
		break;
	case (InputKeyStatus::IKS_RELEASED):
		found->second = (bKeyPressed) ? InputKeyStatus::IKS_PRESSED : InputKeyStatus::IKS_NONE;
		break;
	}
}

void InputController::UpdateAxisValue(InputKey inputKey, float fAxisValue)
{
	std::unordered_map< InputKey, float>::iterator found = m_Axises.find(inputKey);

	if (found == m_Axises.end())
		return;

	m_Axises[inputKey] = fAxisValue;
}

void InputController::SetConnected(bool bConnected)
{
	m_bConnected = bConnected;
}

WindowContext * InputController::GetWindowContext()
{
	return m_pInputContext;
}
