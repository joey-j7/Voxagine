#include "pch.h"

#include "PlayerController.h"
#include "Core/Platform/Input/Temp/GamePadController.h"

int PlayerController::UNIQUE_PLAYER_ID = 1;

PlayerController::PlayerController()
{
	m_PlayerID = UNIQUE_PLAYER_ID;
	++UNIQUE_PLAYER_ID;
}

PlayerController::~PlayerController()
{
}

void PlayerController::Update()
{
	ProcessInputBindings();
}

void PlayerController::VibrateGamePad(float fLeftMotor, float fRightMotor)
{
	if (GetGamePadController() != nullptr)
		GetGamePadController()->Vibrate(fLeftMotor, fRightMotor);
}

bool PlayerController::CreateBindingMap(const std::string & bindingMapName, bool bSetActiveMap)
{
	return m_InputBindingMap.CreateBindingMap(bindingMapName, bSetActiveMap);
}

bool PlayerController::DestroyBindingMap(const std::string & bindingMapName)
{
	return m_InputBindingMap.DestroyBindingMap(bindingMapName);
}

const InputBindingMapInformation * PlayerController::GetBindingMap(const std::string & bindingMapName)
{
	return m_InputBindingMap.GetBindingMap(bindingMapName);
}

bool PlayerController::SetActiveBindingMap(const std::string & bindingMapName)
{
	return m_InputBindingMap.SetActiveBindingMap(bindingMapName);
}

const InputBindingMapInformation * PlayerController::GetActiveBindingMap() const
{
	return m_InputBindingMap.GetActiveBindingMap();
}

void PlayerController::SetDefaultMapName(const std::string & defaultMapName)
{
	m_InputBindingMap.SetDefaultMapName(defaultMapName);
}

std::string PlayerController::GetDefaultMapName() const
{
	return m_InputBindingMap.GetDefaultMapName();
}

InputBindingAxisValue PlayerController::GetAxisValue(const std::string & axisName)
{
	const InputBindingAxis* inputBindingAxis = InputBindingMapInterface::GetInputBindingAxis(axisName);
	return (inputBindingAxis != nullptr) ? ProcessBindingAxis(*inputBindingAxis) : InputBindingAxisValue({ false, 0.f });
}

InputBindingAxisValue PlayerController::GetAxisValue(const std::string & bindingMapName, const std::string & axisName)
{
	const InputBindingAxis* inputBindingAxis = InputBindingMapInterface::GetInputBindingAxis(bindingMapName, axisName);
	return (inputBindingAxis != nullptr) ? ProcessBindingAxis(*inputBindingAxis) : InputBindingAxisValue({ false, 0.f });
}

void PlayerController::ProcessInputBindings()
{
	const InputBindingMapInformation* inputBindingMapInformation = m_InputBindingMap.GetActiveBindingMap();
	if (inputBindingMapInformation == nullptr)
		return;

	ProcessBindingMapAction(*inputBindingMapInformation);
}

InputBindingMap* PlayerController::GetInputBindingMap()
{
	return &m_InputBindingMap;
}

MouseController * PlayerController::GetMouseController()
{
	return m_pPlayerMouse;
}

KeyboardController * PlayerController::GetKeyBoardController()
{
	return m_pPlayerKeyboard;
}

GamePadController * PlayerController::GetGamePadController()
{
	return m_pPlayerGamePad;
}
