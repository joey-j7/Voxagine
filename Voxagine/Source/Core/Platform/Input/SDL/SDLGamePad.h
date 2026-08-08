#pragma once

#include <cstdint>

/* SDL3-backed replacement for DirectXTK12's DirectX::GamePad.
 *
 * The State struct keeps DirectXTK's shape and field names deliberately:
 * GamePadController reads about thirty of them, and matching the layout means
 * the controller logic ports without being rewritten and without any chance of
 * silently reordering a button mapping.
 *
 * Ranges match DirectXTK too - sticks are -1..1 with Y up, triggers 0..1 -
 * so the axis and dead-zone handling above this stays correct. */
class GamePad
{
public:
	static const int m_iMaxPlayers = 4;

	struct Buttons
	{
		bool a = false;
		bool b = false;
		bool x = false;
		bool y = false;
		bool leftStick = false;
		bool rightStick = false;
		bool leftShoulder = false;
		bool rightShoulder = false;
		bool start = false;
		bool view = false;
	};

	struct DPad
	{
		bool up = false;
		bool down = false;
		bool right = false;
		bool left = false;
	};

	struct ThumbSticks
	{
		float leftX = 0.f;
		float leftY = 0.f;
		float rightX = 0.f;
		float rightY = 0.f;
	};

	struct Triggers
	{
		float left = 0.f;
		float right = 0.f;
	};

	struct State
	{
		bool connected = false;

		Buttons buttons;
		DPad dpad;
		ThumbSticks thumbSticks;
		Triggers triggers;

		/* DirectXTK's threshold; kept so trigger-as-button behaviour is
		   unchanged. */
		bool IsLeftTriggerPressed() const { return triggers.left > 0.5f; }
		bool IsRightTriggerPressed() const { return triggers.right > 0.5f; }
	};

	GamePad();
	~GamePad();

	GamePad(const GamePad&) = delete;
	GamePad& operator=(const GamePad&) = delete;

	/* Matches DirectXTK's singleton, which GamePadController constructs with
	   `new GamePad()` and releases with `delete &GamePad::Get()`. */
	static GamePad& Get();

	State GetState(int iPlayer);

	void SetVibration(int iPlayer, float fLeftMotor, float fRightMotor);

	/* Opens newly attached pads and closes removed ones. Cheap to call every
	   frame; SDL only reports actual changes. */
	void Refresh();

private:
	struct Pad;

	Pad* m_pPads = nullptr;
	static GamePad* s_pInstance;
};
