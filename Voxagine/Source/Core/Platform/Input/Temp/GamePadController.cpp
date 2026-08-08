#include "pch.h"
#include "GamePadController.h"

#include "Core/Platform/Input/SDL/SDLGamePad.h"



// Initialization of boolean indicating libraries being initialized
bool GamePadController::m_bInitLibraries = false;
// Initialization of int indicating the game pad ID
int GamePadController::m_iNextUniqGamePadID = 0;
// Initialization of int indicating the game pad controller alive count
int GamePadController::m_iGamePadControllerCount = 0;

GamePadController::GamePadController()
{
}

GamePadController::~GamePadController()
{
}

void GamePadController::Initialize(WindowContext * pWindowContext)
{
	m_iUniqueGamePadID = m_iNextUniqGamePadID;
	++m_iGamePadControllerCount,
	++m_iNextUniqGamePadID;

	InputController::Initialize(pWindowContext);
}

void GamePadController::Vibrate(float fLeftMotor, float fRightMotor)
{
	m_bVibrateIsDirty = true;
	m_fVibrateLeftMotorValue = fLeftMotor;
	m_fVibrateRightMotorValue = fRightMotor;
}

int GamePadController::GetGamePadID() const
{
	return m_iUniqueGamePadID;
}

void GamePadController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Create DirectX gamepad singleton
		new GamePad();
		m_bInitLibraries = true;
	}

	m_iGamePadHandle = m_iUniqueGamePadID;
}

void GamePadController::OnUninitialize()
{
	--m_iGamePadControllerCount;

	if (m_iGamePadControllerCount == 0)
	{
		// Decrease the gamepad controller count due to destruction
		delete &GamePad::Get();
	}
}

void GamePadController::OnUpdate()
{
	// Get the gamepad and state
	GamePad::State gamepadState = GamePad::Get().GetState(m_iGamePadHandle);

	// Update gamepad connected state
	SetConnected(gamepadState.connected);

	// Update gamepad button states
	UpdateKeyState(IK_GAMEPADRIGHTPADUP,gamepadState.buttons.y);
	UpdateKeyState(IK_GAMEPADRIGHTPADRIGHT, gamepadState.buttons.b);
	UpdateKeyState(IK_GAMEPADRIGHTPADDOWN, gamepadState.buttons.a);
	UpdateKeyState(IK_GAMEPADRIGHTPADLEFT, gamepadState.buttons.x);
	UpdateKeyState(IK_GAMEPADLEFTPADUP, gamepadState.dpad.up);
	UpdateKeyState(IK_GAMEPADLEFTPADRIGHT, gamepadState.dpad.right);
	UpdateKeyState(IK_GAMEPADLEFTPADDOWN, gamepadState.dpad.down);
	UpdateKeyState(IK_GAMEPADLEFTPADLEFT, gamepadState.dpad.left);

	UpdateKeyState(IK_GAMEPADRIGHTSTICK, gamepadState.buttons.rightStick);
	UpdateKeyState(IK_GAMEPADRIGHTSHOULDER, gamepadState.buttons.rightShoulder);
	UpdateKeyState(IK_GAMEPADRIGHTSHOULDER2, gamepadState.IsRightTriggerPressed());
	UpdateKeyState(IK_GAMEPADLEFTSTICK, gamepadState.buttons.leftStick);
	UpdateKeyState(IK_GAMEPADLEFTSHOULDER, gamepadState.buttons.leftShoulder);
	UpdateKeyState(IK_GAMEPADLEFTSHOULDER2, gamepadState.IsLeftTriggerPressed());

	UpdateKeyState(IK_GAMEPADOPTION, gamepadState.buttons.start);
	UpdateKeyState(IK_GAMEPADSELECT, gamepadState.buttons.view);

	// Update gamepad states for axises in delta
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISXDELTA, gamepadState.thumbSticks.rightX - m_Axises[IK_GAMEPADRIGHTSTICKAXISX]);
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISYDELTA, gamepadState.thumbSticks.rightY - m_Axises[IK_GAMEPADRIGHTSTICKAXISY]);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISXDELTA, gamepadState.thumbSticks.leftX - m_Axises[IK_GAMEPADLEFTSTICKAXISX]);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISYDELTA, gamepadState.thumbSticks.leftY - m_Axises[IK_GAMEPADLEFTSTICKAXISY]);
	UpdateAxisValue(IK_GAMEPADRIGHTSHOULDER2AXISDELTA, gamepadState.triggers.right - m_Axises[IK_GAMEPADRIGHTSHOULDER2AXIS]);
	UpdateAxisValue(IK_GAMEPADLEFTSHOULDER2AXISDELTA, gamepadState.triggers.left - m_Axises[IK_GAMEPADLEFTSHOULDER2AXIS]);

	// Update gamepad axis values
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISX, gamepadState.thumbSticks.rightX);
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISY, gamepadState.thumbSticks.rightY);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISX, gamepadState.thumbSticks.leftX);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISY, gamepadState.thumbSticks.leftY);
	UpdateAxisValue(IK_GAMEPADRIGHTSHOULDER2AXIS, gamepadState.triggers.right);
	UpdateAxisValue(IK_GAMEPADLEFTSHOULDER2AXIS, gamepadState.triggers.left);

	if (m_bVibrateIsDirty)
	{
		GamePad& gamepad = GamePad::Get();
		gamepad.SetVibration(m_iUniqueGamePadID, m_fVibrateLeftMotorValue, m_fVibrateRightMotorValue);

		m_bVibrateIsDirty = false;
	}
}



void GamePadController::InitializeButtons()
{
	// Mouse input key for states initialization
	AddInputKeyStateMap(IK_GAMEPADRIGHTPADUP);
	AddInputKeyStateMap(IK_GAMEPADRIGHTPADRIGHT);
	AddInputKeyStateMap(IK_GAMEPADRIGHTPADDOWN);
	AddInputKeyStateMap(IK_GAMEPADRIGHTPADLEFT);
	AddInputKeyStateMap(IK_GAMEPADLEFTPADUP);
	AddInputKeyStateMap(IK_GAMEPADLEFTPADRIGHT);
	AddInputKeyStateMap(IK_GAMEPADLEFTPADDOWN);
	AddInputKeyStateMap(IK_GAMEPADLEFTPADLEFT);

	AddInputKeyStateMap(IK_GAMEPADRIGHTSTICK);
	AddInputKeyStateMap(IK_GAMEPADRIGHTSHOULDER);
	AddInputKeyStateMap(IK_GAMEPADRIGHTSHOULDER2);
	AddInputKeyStateMap(IK_GAMEPADLEFTSTICK);
	AddInputKeyStateMap(IK_GAMEPADLEFTSHOULDER);
	AddInputKeyStateMap(IK_GAMEPADLEFTSHOULDER2);

	AddInputKeyStateMap(IK_GAMEPADOPTION);
	AddInputKeyStateMap(IK_GAMEPADSELECT);
}

void GamePadController::InitializeAxises()
{
	// Gamepad input key for axises values initialization
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSTICKAXISX);
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSTICKAXISY);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSTICKAXISX);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSTICKAXISY);
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSHOULDER2AXIS);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSHOULDER2AXIS);

	// Gamepad input key for axises deltas initialization
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSTICKAXISXDELTA);
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSTICKAXISYDELTA);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSTICKAXISXDELTA);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSTICKAXISYDELTA);
	AddInputKeyAxisMap(IK_GAMEPADRIGHTSHOULDER2AXISDELTA);
	AddInputKeyAxisMap(IK_GAMEPADLEFTSHOULDER2AXISDELTA);
}

float GamePadController::NormalizeFloat(float fMax, float fValue)
{
	return fValue / fMax;
}
