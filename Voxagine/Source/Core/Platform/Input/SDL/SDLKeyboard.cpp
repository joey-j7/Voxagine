#include "pch.h"
#include "SDLKeyboard.h"

#include <SDL3/SDL.h>

Keyboard* Keyboard::s_pInstance = nullptr;

Keyboard::Keyboard()
{
	s_pInstance = this;
}

Keyboard::~Keyboard()
{
	if (s_pInstance == this)
		s_pInstance = nullptr;
}

Keyboard& Keyboard::Get()
{
	if (s_pInstance == nullptr)
		new Keyboard();

	return *s_pInstance;
}

Keyboard::State Keyboard::GetState() const
{
	State state;

	int iCount = 0;
	const bool* pKeys = SDL_GetKeyboardState(&iCount);

	if (pKeys == nullptr)
		return state;

		state.A = pKeys[SDL_SCANCODE_A];
		state.Add = pKeys[SDL_SCANCODE_KP_PLUS];
		state.Apps = pKeys[SDL_SCANCODE_APPLICATION];
		state.B = pKeys[SDL_SCANCODE_B];
		state.Back = pKeys[SDL_SCANCODE_BACKSPACE];
		state.C = pKeys[SDL_SCANCODE_C];
		state.CapsLock = pKeys[SDL_SCANCODE_CAPSLOCK];
		state.D = pKeys[SDL_SCANCODE_D];
		state.D0 = pKeys[SDL_SCANCODE_0];
		state.D1 = pKeys[SDL_SCANCODE_1];
		state.D2 = pKeys[SDL_SCANCODE_2];
		state.D3 = pKeys[SDL_SCANCODE_3];
		state.D4 = pKeys[SDL_SCANCODE_4];
		state.D5 = pKeys[SDL_SCANCODE_5];
		state.D6 = pKeys[SDL_SCANCODE_6];
		state.D7 = pKeys[SDL_SCANCODE_7];
		state.D8 = pKeys[SDL_SCANCODE_8];
		state.D9 = pKeys[SDL_SCANCODE_9];
		state.Decimal = pKeys[SDL_SCANCODE_KP_PERIOD];
		state.Delete = pKeys[SDL_SCANCODE_DELETE];
		state.Divide = pKeys[SDL_SCANCODE_KP_DIVIDE];
		state.Down = pKeys[SDL_SCANCODE_DOWN];
		state.E = pKeys[SDL_SCANCODE_E];
		state.End = pKeys[SDL_SCANCODE_END];
		state.Enter = pKeys[SDL_SCANCODE_RETURN];
		state.Escape = pKeys[SDL_SCANCODE_ESCAPE];
		state.Execute = pKeys[SDL_SCANCODE_EXECUTE];
		state.F = pKeys[SDL_SCANCODE_F];
		state.F1 = pKeys[SDL_SCANCODE_F1];
		state.F10 = pKeys[SDL_SCANCODE_F10];
		state.F11 = pKeys[SDL_SCANCODE_F11];
		state.F12 = pKeys[SDL_SCANCODE_F12];
		state.F13 = pKeys[SDL_SCANCODE_F13];
		state.F14 = pKeys[SDL_SCANCODE_F14];
		state.F15 = pKeys[SDL_SCANCODE_F15];
		state.F16 = pKeys[SDL_SCANCODE_F16];
		state.F17 = pKeys[SDL_SCANCODE_F17];
		state.F18 = pKeys[SDL_SCANCODE_F18];
		state.F19 = pKeys[SDL_SCANCODE_F19];
		state.F2 = pKeys[SDL_SCANCODE_F2];
		state.F20 = pKeys[SDL_SCANCODE_F20];
		state.F21 = pKeys[SDL_SCANCODE_F21];
		state.F22 = pKeys[SDL_SCANCODE_F22];
		state.F23 = pKeys[SDL_SCANCODE_F23];
		state.F24 = pKeys[SDL_SCANCODE_F24];
		state.F3 = pKeys[SDL_SCANCODE_F3];
		state.F4 = pKeys[SDL_SCANCODE_F4];
		state.F5 = pKeys[SDL_SCANCODE_F5];
		state.F6 = pKeys[SDL_SCANCODE_F6];
		state.F7 = pKeys[SDL_SCANCODE_F7];
		state.F8 = pKeys[SDL_SCANCODE_F8];
		state.F9 = pKeys[SDL_SCANCODE_F9];
		state.G = pKeys[SDL_SCANCODE_G];
		state.H = pKeys[SDL_SCANCODE_H];
		state.Help = pKeys[SDL_SCANCODE_HELP];
		state.Home = pKeys[SDL_SCANCODE_HOME];
		state.I = pKeys[SDL_SCANCODE_I];
		state.Insert = pKeys[SDL_SCANCODE_INSERT];
		state.J = pKeys[SDL_SCANCODE_J];
		state.K = pKeys[SDL_SCANCODE_K];
		state.L = pKeys[SDL_SCANCODE_L];
		state.Left = pKeys[SDL_SCANCODE_LEFT];
		state.LeftAlt = pKeys[SDL_SCANCODE_LALT];
		state.LeftControl = pKeys[SDL_SCANCODE_LCTRL];
		state.LeftShift = pKeys[SDL_SCANCODE_LSHIFT];
		state.LeftWindows = pKeys[SDL_SCANCODE_LGUI];
		state.M = pKeys[SDL_SCANCODE_M];
		state.Multiply = pKeys[SDL_SCANCODE_KP_MULTIPLY];
		state.N = pKeys[SDL_SCANCODE_N];
		state.NumLock = pKeys[SDL_SCANCODE_NUMLOCKCLEAR];
		state.NumPad0 = pKeys[SDL_SCANCODE_KP_0];
		state.NumPad1 = pKeys[SDL_SCANCODE_KP_1];
		state.NumPad2 = pKeys[SDL_SCANCODE_KP_2];
		state.NumPad3 = pKeys[SDL_SCANCODE_KP_3];
		state.NumPad4 = pKeys[SDL_SCANCODE_KP_4];
		state.NumPad5 = pKeys[SDL_SCANCODE_KP_5];
		state.NumPad6 = pKeys[SDL_SCANCODE_KP_6];
		state.NumPad7 = pKeys[SDL_SCANCODE_KP_7];
		state.NumPad8 = pKeys[SDL_SCANCODE_KP_8];
		state.NumPad9 = pKeys[SDL_SCANCODE_KP_9];
		state.O = pKeys[SDL_SCANCODE_O];
		state.P = pKeys[SDL_SCANCODE_P];
		state.PageDown = pKeys[SDL_SCANCODE_PAGEDOWN];
		state.PageUp = pKeys[SDL_SCANCODE_PAGEUP];
		state.Pause = pKeys[SDL_SCANCODE_PAUSE];
		state.Print = pKeys[SDL_SCANCODE_PRINTSCREEN];
		state.PrintScreen = pKeys[SDL_SCANCODE_PRINTSCREEN];
		state.Q = pKeys[SDL_SCANCODE_Q];
		state.R = pKeys[SDL_SCANCODE_R];
		state.Right = pKeys[SDL_SCANCODE_RIGHT];
		state.RightAlt = pKeys[SDL_SCANCODE_RALT];
		state.RightControl = pKeys[SDL_SCANCODE_RCTRL];
		state.RightShift = pKeys[SDL_SCANCODE_RSHIFT];
		state.RightWindows = pKeys[SDL_SCANCODE_RGUI];
		state.S = pKeys[SDL_SCANCODE_S];
		state.Scroll = pKeys[SDL_SCANCODE_SCROLLLOCK];
		state.Select = pKeys[SDL_SCANCODE_SELECT];
		state.Separator = pKeys[SDL_SCANCODE_KP_COMMA];
		state.Space = pKeys[SDL_SCANCODE_SPACE];
		state.Subtract = pKeys[SDL_SCANCODE_KP_MINUS];
		state.T = pKeys[SDL_SCANCODE_T];
		state.Tab = pKeys[SDL_SCANCODE_TAB];
		state.U = pKeys[SDL_SCANCODE_U];
		state.Up = pKeys[SDL_SCANCODE_UP];
		state.V = pKeys[SDL_SCANCODE_V];
		state.W = pKeys[SDL_SCANCODE_W];
		state.X = pKeys[SDL_SCANCODE_X];
		state.Y = pKeys[SDL_SCANCODE_Y];
		state.Z = pKeys[SDL_SCANCODE_Z];

	return state;
}
