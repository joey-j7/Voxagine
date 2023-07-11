#include "pch.h"
#include "GamePadController.h"

#ifdef _WINDOWS
#include <External/DirectXTK12/gamepad.h>
#endif

#ifdef _ORBIS
#include <user_service.h>
#include <pad.h>
#endif

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

#ifdef _WINDOWS
void GamePadController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Create DirectX gamepad singleton
		new DirectX::GamePad();
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
		delete &DirectX::GamePad::Get();
	}
}

void GamePadController::OnUpdate()
{
	// Get the gamepad and state
	DirectX::GamePad::State gamepadState = DirectX::GamePad::Get().GetState(m_iGamePadHandle);

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
		DirectX::GamePad& gamepad = DirectX::GamePad::Get();
		gamepad.SetVibration(m_iUniqueGamePadID, m_fVibrateLeftMotorValue, m_fVibrateRightMotorValue);

		m_bVibrateIsDirty = false;
	}
}
#endif

#ifdef _ORBIS
void GamePadController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Initialize Orbis Controller library
		scePadInit();
		m_bInitLibraries = true;

		// Get the main user
		SceUserServiceUserId userId = 0;
		int32_t result = sceUserServiceGetInitialUser(&userId);
		if (result < 0)
		{
			// Failed to get main user
		};

		m_iGamePadHandle = scePadOpen(userId, SCE_PAD_PORT_TYPE_STANDARD, 0, NULL);
		if (m_iGamePadHandle < 0)
		{
			// Failed to get valid handle
		}
	}
}

void GamePadController::OnUninitialize()
{
	if (m_iGamePadHandle > 0)
		scePadClose(m_iGamePadHandle);
}

void GamePadController::OnUpdate()
{
	ScePadData gamepadState;
	scePadReadState(m_iGamePadHandle, &gamepadState);

	// Update gamepad connected state
	SetConnected(gamepadState.connected);

	// Update gamepad button states
	UpdateKeyState(IK_GAMEPADRIGHTPADUP, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_TRIANGLE);
	UpdateKeyState(IK_GAMEPADRIGHTPADRIGHT, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_CIRCLE);
	UpdateKeyState(IK_GAMEPADRIGHTPADDOWN, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_CROSS);
	UpdateKeyState(IK_GAMEPADRIGHTPADLEFT, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_SQUARE);
	UpdateKeyState(IK_GAMEPADLEFTPADUP, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_UP);
	UpdateKeyState(IK_GAMEPADLEFTPADRIGHT, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_RIGHT);
	UpdateKeyState(IK_GAMEPADLEFTPADDOWN, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_DOWN);
	UpdateKeyState(IK_GAMEPADLEFTPADLEFT, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_LEFT);

	UpdateKeyState(IK_GAMEPADRIGHTSTICK, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_R3);
	UpdateKeyState(IK_GAMEPADRIGHTSHOULDER, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_R1);
	UpdateKeyState(IK_GAMEPADRIGHTSHOULDER2, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_R2);
	UpdateKeyState(IK_GAMEPADLEFTSTICK, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_L3);
	UpdateKeyState(IK_GAMEPADLEFTSHOULDER, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_L1);
	UpdateKeyState(IK_GAMEPADLEFTSHOULDER2, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_L2);

	UpdateKeyState(IK_GAMEPADOPTION, gamepadState.buttons & ScePadButtonDataOffset::SCE_PAD_BUTTON_OPTIONS);
	// UpdateKeyState(IK_GAMEPADSELECT, gamepadState.touchData.touch);

	// Update gamepad states for axises in delta
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISXDELTA, NormalizeFloat(255.f, gamepadState.rightStick.x) - m_Axises[IK_GAMEPADRIGHTSTICKAXISX]);
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISYDELTA, NormalizeFloat(255.f, gamepadState.rightStick.y) - m_Axises[IK_GAMEPADRIGHTSTICKAXISY]);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISXDELTA, NormalizeFloat(255.f, gamepadState.leftStick.x) - m_Axises[IK_GAMEPADLEFTSTICKAXISX]);
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISYDELTA, NormalizeFloat(255.f, gamepadState.leftStick.y) - m_Axises[IK_GAMEPADLEFTSTICKAXISY]);
	UpdateAxisValue(IK_GAMEPADRIGHTSHOULDER2AXISDELTA, NormalizeFloat(255.f, gamepadState.analogButtons.r2) - m_Axises[IK_GAMEPADRIGHTSHOULDER2AXIS]);
	UpdateAxisValue(IK_GAMEPADLEFTSHOULDER2AXISDELTA, NormalizeFloat(255.f, gamepadState.analogButtons.l2) - m_Axises[IK_GAMEPADLEFTSHOULDER2AXIS]);

	// Update gamepad axis values
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISX, NormalizeFloat(255.f, gamepadState.rightStick.x));
	UpdateAxisValue(IK_GAMEPADRIGHTSTICKAXISY, NormalizeFloat(255.f, gamepadState.rightStick.y));
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISX, NormalizeFloat(255.f, gamepadState.leftStick.x));
	UpdateAxisValue(IK_GAMEPADLEFTSTICKAXISY, NormalizeFloat(255.f, gamepadState.leftStick.y));
	UpdateAxisValue(IK_GAMEPADRIGHTSHOULDER2AXIS, NormalizeFloat(255.f, gamepadState.analogButtons.r2));
	UpdateAxisValue(IK_GAMEPADLEFTSHOULDER2AXIS, NormalizeFloat(255.f, gamepadState.analogButtons.l2));
}
#endif


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
