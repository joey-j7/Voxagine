#include "pch.h"
#include "SDLGamePad.h"

#include <SDL3/SDL.h>

#include <cstdio>

GamePad* GamePad::s_pInstance = nullptr;

struct GamePad::Pad
{
	SDL_Gamepad* m_pGamepad = nullptr;
	SDL_JoystickID m_Id = 0;
};

namespace
{
	/* SDL reports sticks as int16 and triggers as 0..32767. DirectXTK used
	   -1..1 and 0..1, and the engine's dead zones are tuned for that. */
	float NormalizeStick(int16_t iValue)
	{
		return iValue < 0 ? static_cast<float>(iValue) / 32768.f
		                  : static_cast<float>(iValue) / 32767.f;
	}

	float NormalizeTrigger(int16_t iValue)
	{
		return static_cast<float>(iValue) / 32767.f;
	}
}

GamePad::GamePad()
{
	m_pPads = new Pad[m_iMaxPlayers];

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
		fprintf(stderr, "[sdl] SDL_InitSubSystem(GAMEPAD) failed: %s\n", SDL_GetError());

	s_pInstance = this;

	Refresh();
}

GamePad::~GamePad()
{
	for (int i = 0; i < m_iMaxPlayers; ++i)
	{
		if (m_pPads[i].m_pGamepad != nullptr)
			SDL_CloseGamepad(m_pPads[i].m_pGamepad);
	}

	delete[] m_pPads;
	m_pPads = nullptr;

	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);

	if (s_pInstance == this)
		s_pInstance = nullptr;
}

GamePad& GamePad::Get()
{
	/* DirectXTK asserted here. Constructing on demand is friendlier and keeps
	   GetState() from dereferencing null if the order ever changes. */
	if (s_pInstance == nullptr)
		new GamePad();

	return *s_pInstance;
}

void GamePad::Refresh()
{
	int iCount = 0;
	SDL_JoystickID* pIds = SDL_GetGamepads(&iCount);

	if (pIds == nullptr)
		return;

	/* Drop pads that went away. */
	for (int i = 0; i < m_iMaxPlayers; ++i)
	{
		if (m_pPads[i].m_pGamepad == nullptr)
			continue;

		bool bStillPresent = false;
		for (int j = 0; j < iCount; ++j)
		{
			if (pIds[j] == m_pPads[i].m_Id)
			{
				bStillPresent = true;
				break;
			}
		}

		if (!bStillPresent)
		{
			SDL_CloseGamepad(m_pPads[i].m_pGamepad);
			m_pPads[i] = Pad{};
		}
	}

	/* Take newly attached pads into the first free slot, so player indices
	   stay stable across a hot-unplug of a different pad. */
	for (int j = 0; j < iCount; ++j)
	{
		bool bKnown = false;
		for (int i = 0; i < m_iMaxPlayers; ++i)
		{
			if (m_pPads[i].m_pGamepad != nullptr && m_pPads[i].m_Id == pIds[j])
			{
				bKnown = true;
				break;
			}
		}

		if (bKnown)
			continue;

		for (int i = 0; i < m_iMaxPlayers; ++i)
		{
			if (m_pPads[i].m_pGamepad != nullptr)
				continue;

			SDL_Gamepad* pGamepad = SDL_OpenGamepad(pIds[j]);

			if (pGamepad != nullptr)
			{
				m_pPads[i].m_pGamepad = pGamepad;
				m_pPads[i].m_Id = pIds[j];
			}

			break;
		}
	}

	SDL_free(pIds);
}

GamePad::State GamePad::GetState(int iPlayer)
{
	State state;

	if (iPlayer < 0 || iPlayer >= m_iMaxPlayers)
		return state;

	Refresh();

	SDL_Gamepad* pGamepad = m_pPads[iPlayer].m_pGamepad;

	if (pGamepad == nullptr)
		return state;

	state.connected = true;

	state.buttons.a = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_SOUTH);
	state.buttons.b = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_EAST);
	state.buttons.x = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_WEST);
	state.buttons.y = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_NORTH);

	state.buttons.leftStick = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK);
	state.buttons.rightStick = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
	state.buttons.leftShoulder = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
	state.buttons.rightShoulder = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

	state.buttons.start = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_START);
	state.buttons.view = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_BACK);

	state.dpad.up = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
	state.dpad.down = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
	state.dpad.left = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
	state.dpad.right = SDL_GetGamepadButton(pGamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

	state.thumbSticks.leftX = NormalizeStick(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_LEFTX));
	state.thumbSticks.rightX = NormalizeStick(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_RIGHTX));

	/* SDL's Y axis points down, DirectXTK's points up. */
	state.thumbSticks.leftY = -NormalizeStick(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_LEFTY));
	state.thumbSticks.rightY = -NormalizeStick(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_RIGHTY));

	state.triggers.left = NormalizeTrigger(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
	state.triggers.right = NormalizeTrigger(SDL_GetGamepadAxis(pGamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

	return state;
}

void GamePad::SetVibration(int iPlayer, float fLeftMotor, float fRightMotor)
{
	if (iPlayer < 0 || iPlayer >= m_iMaxPlayers)
		return;

	SDL_Gamepad* pGamepad = m_pPads[iPlayer].m_pGamepad;

	if (pGamepad == nullptr)
		return;

	const Uint16 uiLeft = static_cast<Uint16>(SDL_clamp(fLeftMotor, 0.f, 1.f) * 65535.f);
	const Uint16 uiRight = static_cast<Uint16>(SDL_clamp(fRightMotor, 0.f, 1.f) * 65535.f);

	/* DirectXTK's vibration was level-triggered and stayed until changed; SDL
	   wants a duration, so use one long enough to outlast a frame and rely on
	   the next SetVibration to replace it. */
	SDL_RumbleGamepad(pGamepad, uiLeft, uiRight, 1000);
}
