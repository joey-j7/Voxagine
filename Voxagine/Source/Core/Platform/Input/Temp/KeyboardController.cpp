#include "pch.h"
#include "KeyboardController.h"

#ifdef _WINDOWS
#include <wrl/wrappers/corewrappers.h>
#include <External/DirectXTK12/Keyboard.h>
#endif

#ifdef _ORBIS
#endif

// Initialization of boolean indicating libraries being initialized
bool KeyboardController::m_bInitLibraries = false;

KeyboardController::KeyboardController()
	: InputController()
{
}

KeyboardController::~KeyboardController()
{
}

InputBindingAxisValue KeyboardController::GetAxisValue(InputKey inputKey) const
{
	InputKeyStatus inputKeyStatus = GetKeyStatus(inputKey);
	InputBindingAxisValue inputBindingAxisValue;
	inputBindingAxisValue.Proccessed = HasKey(inputKey);
	inputBindingAxisValue.Value = (inputKeyStatus == IKS_PRESSED || inputKeyStatus == IKS_HELD) ? 1.f : 0.f;
	return inputBindingAxisValue;
}

#ifdef _WINDOWS
void KeyboardController::OnInitialize()
{
	// Skip if libraries or dependencies have been initialized
	if (!m_bInitLibraries)
	{
		// Create DirectX keyboard singleton
		new DirectX::Keyboard();

		m_bInitLibraries = true;
	}
}

void KeyboardController::OnUninitialize()
{
	// Delete DirectX keyboard singleton
	delete &DirectX::Keyboard::Get();
}

void KeyboardController::OnUpdate()
{
	// Get the keyboard and state
	DirectX::Keyboard::State keyboardState = DirectX::Keyboard::Get().GetState();

	// Update keyboard connected state
	SetConnected(DirectX::Keyboard::Get().IsConnected());

	// Update keyboard button states
	UpdateKeyState(IK_BACK, keyboardState.Back);
	UpdateKeyState(IK_TAB, keyboardState.Tab);
	UpdateKeyState(IK_ENTER, keyboardState.Enter);
	UpdateKeyState(IK_PAUSE, keyboardState.Pause);
	UpdateKeyState(IK_CAPSLOCK, keyboardState.CapsLock);
	UpdateKeyState(IK_ESCAPE, keyboardState.Escape);
	UpdateKeyState(IK_SPACE, keyboardState.Space);
	UpdateKeyState(IK_PAGEUP, keyboardState.PageUp);
	UpdateKeyState(IK_PAGEDOWN, keyboardState.PageDown);
	UpdateKeyState(IK_END, keyboardState.End);
	UpdateKeyState(IK_HOME, keyboardState.Home);
	UpdateKeyState(IK_LEFT, keyboardState.Left);
	UpdateKeyState(IK_UP, keyboardState.Up);
	UpdateKeyState(IK_RIGHT, keyboardState.Right);
	UpdateKeyState(IK_DOWN, keyboardState.Down);
	UpdateKeyState(IK_SELECT, keyboardState.Select);
	UpdateKeyState(IK_PRINT, keyboardState.Print);
	UpdateKeyState(IK_EXECUTE, keyboardState.Execute);
	UpdateKeyState(IK_PRINTSCREEN, keyboardState.PrintScreen);
	UpdateKeyState(IK_INSERT, keyboardState.Insert);
	UpdateKeyState(IK_DELETE, keyboardState.Delete);
	UpdateKeyState(IK_HELP, keyboardState.Help);
	UpdateKeyState(IK_D0, keyboardState.D0);
	UpdateKeyState(IK_D1, keyboardState.D1);
	UpdateKeyState(IK_D2, keyboardState.D2);
	UpdateKeyState(IK_D3, keyboardState.D3);
	UpdateKeyState(IK_D4, keyboardState.D4);
	UpdateKeyState(IK_D5, keyboardState.D5);
	UpdateKeyState(IK_D6, keyboardState.D6);
	UpdateKeyState(IK_D7, keyboardState.D7);
	UpdateKeyState(IK_D8, keyboardState.D8);
	UpdateKeyState(IK_D9, keyboardState.D9);

	UpdateKeyState(IK_A, keyboardState.A);
	UpdateKeyState(IK_B, keyboardState.B);
	UpdateKeyState(IK_C, keyboardState.C);
	UpdateKeyState(IK_D, keyboardState.D);
	UpdateKeyState(IK_E, keyboardState.E);
	UpdateKeyState(IK_F, keyboardState.F);
	UpdateKeyState(IK_G, keyboardState.G);
	UpdateKeyState(IK_H, keyboardState.H);
	UpdateKeyState(IK_I, keyboardState.I);
	UpdateKeyState(IK_J, keyboardState.J);
	UpdateKeyState(IK_K, keyboardState.K);
	UpdateKeyState(IK_L, keyboardState.L);
	UpdateKeyState(IK_M, keyboardState.M);
	UpdateKeyState(IK_N, keyboardState.N);
	UpdateKeyState(IK_O, keyboardState.O);
	UpdateKeyState(IK_P, keyboardState.P);
	UpdateKeyState(IK_Q, keyboardState.Q);
	UpdateKeyState(IK_R, keyboardState.R);
	UpdateKeyState(IK_S, keyboardState.S);
	UpdateKeyState(IK_T, keyboardState.T);
	UpdateKeyState(IK_U, keyboardState.U);
	UpdateKeyState(IK_V, keyboardState.V);
	UpdateKeyState(IK_W, keyboardState.W);
	UpdateKeyState(IK_X, keyboardState.X);
	UpdateKeyState(IK_Y, keyboardState.Y);
	UpdateKeyState(IK_Z, keyboardState.Z);
	UpdateKeyState(IK_LEFTWINDOWS, keyboardState.LeftWindows);
	UpdateKeyState(IK_RIGHTWINDOWS, keyboardState.RightWindows);
	UpdateKeyState(IK_APPS, keyboardState.Apps);

	UpdateKeyState(IK_NUMPAD0, keyboardState.NumPad0);
	UpdateKeyState(IK_NUMPAD1, keyboardState.NumPad1);
	UpdateKeyState(IK_NUMPAD2, keyboardState.NumPad2);
	UpdateKeyState(IK_NUMPAD3, keyboardState.NumPad3);
	UpdateKeyState(IK_NUMPAD4, keyboardState.NumPad4);
	UpdateKeyState(IK_NUMPAD5, keyboardState.NumPad5);
	UpdateKeyState(IK_NUMPAD6, keyboardState.NumPad6);
	UpdateKeyState(IK_NUMPAD7, keyboardState.NumPad7);
	UpdateKeyState(IK_NUMPAD8, keyboardState.NumPad8);
	UpdateKeyState(IK_NUMPAD9, keyboardState.NumPad9);
	UpdateKeyState(IK_MULTIPLY, keyboardState.Multiply);
	UpdateKeyState(IK_ADD, keyboardState.Add);
	UpdateKeyState(IK_SEPARATOR, keyboardState.Separator);
	UpdateKeyState(IK_SUBTRACT, keyboardState.Subtract);

	UpdateKeyState(IK_DECIMAL, keyboardState.Decimal);
	UpdateKeyState(IK_DIVIDE, keyboardState.Divide);
	UpdateKeyState(IK_F1, keyboardState.F1);
	UpdateKeyState(IK_F2, keyboardState.F2);
	UpdateKeyState(IK_F3, keyboardState.F3);
	UpdateKeyState(IK_F4, keyboardState.F4);
	UpdateKeyState(IK_F5, keyboardState.F5);
	UpdateKeyState(IK_F6, keyboardState.F6);
	UpdateKeyState(IK_F7, keyboardState.F7);
	UpdateKeyState(IK_F8, keyboardState.F8);
	UpdateKeyState(IK_F9, keyboardState.F9);
	UpdateKeyState(IK_F10, keyboardState.F10);
	UpdateKeyState(IK_F11, keyboardState.F11);
	UpdateKeyState(IK_F12, keyboardState.F12);
	UpdateKeyState(IK_F13, keyboardState.F13);
	UpdateKeyState(IK_F14, keyboardState.F14);
	UpdateKeyState(IK_F15, keyboardState.F15);
	UpdateKeyState(IK_F16, keyboardState.F16);
	UpdateKeyState(IK_F17, keyboardState.F17);
	UpdateKeyState(IK_F18, keyboardState.F18);
	UpdateKeyState(IK_F19, keyboardState.F19);
	UpdateKeyState(IK_F20, keyboardState.F20);
	UpdateKeyState(IK_F21, keyboardState.F21);
	UpdateKeyState(IK_F22, keyboardState.F22);
	UpdateKeyState(IK_F23, keyboardState.F23);
	UpdateKeyState(IK_F24, keyboardState.F24);

	UpdateKeyState(IK_NUMLOCK, keyboardState.NumLock);
	UpdateKeyState(IK_SCROLL, keyboardState.Scroll);

	UpdateKeyState(IK_LEFTSHIFT, keyboardState.LeftShift);
	UpdateKeyState(IK_RIGHTSHIFT, keyboardState.RightShift);
	UpdateKeyState(IK_LEFTCONTROL, keyboardState.LeftControl);
	UpdateKeyState(IK_RIGHTCONTROL, keyboardState.RightControl);
	UpdateKeyState(IK_LEFTALT, keyboardState.LeftAlt);
	UpdateKeyState(IK_RIGHTALT, keyboardState.RightAlt);
}
#endif

#ifdef _ORBIS
void KeyboardController::OnInitialize()
{
}

void KeyboardController::OnUninitialize()
{
}

void KeyboardController::OnUpdate()
{
}
#endif

void KeyboardController::InitializeButtons()
{
	AddInputKeyStateMap(IK_BACK);
	AddInputKeyStateMap(IK_TAB);
	AddInputKeyStateMap(IK_ENTER);
	AddInputKeyStateMap(IK_PAUSE);
	AddInputKeyStateMap(IK_CAPSLOCK);
	AddInputKeyStateMap(IK_ESCAPE);
	AddInputKeyStateMap(IK_SPACE);
	AddInputKeyStateMap(IK_PAGEUP);
	AddInputKeyStateMap(IK_PAGEDOWN);
	AddInputKeyStateMap(IK_END);
	AddInputKeyStateMap(IK_HOME);
	AddInputKeyStateMap(IK_LEFT);
	AddInputKeyStateMap(IK_UP);
	AddInputKeyStateMap(IK_RIGHT);
	AddInputKeyStateMap(IK_DOWN);
	AddInputKeyStateMap(IK_SELECT);
	AddInputKeyStateMap(IK_PRINT);
	AddInputKeyStateMap(IK_EXECUTE);
	AddInputKeyStateMap(IK_PRINTSCREEN);
	AddInputKeyStateMap(IK_INSERT);
	AddInputKeyStateMap(IK_DELETE);
	AddInputKeyStateMap(IK_HELP);
	AddInputKeyStateMap(IK_D0);
	AddInputKeyStateMap(IK_D1);
	AddInputKeyStateMap(IK_D2);
	AddInputKeyStateMap(IK_D3);
	AddInputKeyStateMap(IK_D4);
	AddInputKeyStateMap(IK_D5);
	AddInputKeyStateMap(IK_D6);
	AddInputKeyStateMap(IK_D7);
	AddInputKeyStateMap(IK_D8);
	AddInputKeyStateMap(IK_D9);

	AddInputKeyStateMap(IK_A);
	AddInputKeyStateMap(IK_B);
	AddInputKeyStateMap(IK_C);
	AddInputKeyStateMap(IK_D);
	AddInputKeyStateMap(IK_E);
	AddInputKeyStateMap(IK_F);
	AddInputKeyStateMap(IK_G);
	AddInputKeyStateMap(IK_H);
	AddInputKeyStateMap(IK_I);
	AddInputKeyStateMap(IK_J);
	AddInputKeyStateMap(IK_K);
	AddInputKeyStateMap(IK_L);
	AddInputKeyStateMap(IK_M);
	AddInputKeyStateMap(IK_N);
	AddInputKeyStateMap(IK_O);
	AddInputKeyStateMap(IK_P);
	AddInputKeyStateMap(IK_Q);
	AddInputKeyStateMap(IK_R);
	AddInputKeyStateMap(IK_S);
	AddInputKeyStateMap(IK_T);
	AddInputKeyStateMap(IK_U);
	AddInputKeyStateMap(IK_V);
	AddInputKeyStateMap(IK_W);
	AddInputKeyStateMap(IK_X);
	AddInputKeyStateMap(IK_Y);
	AddInputKeyStateMap(IK_Z);
	AddInputKeyStateMap(IK_LEFTWINDOWS);
	AddInputKeyStateMap(IK_RIGHTWINDOWS);
	AddInputKeyStateMap(IK_APPS);

	AddInputKeyStateMap(IK_NUMPAD0);
	AddInputKeyStateMap(IK_NUMPAD1);
	AddInputKeyStateMap(IK_NUMPAD2);
	AddInputKeyStateMap(IK_NUMPAD3);
	AddInputKeyStateMap(IK_NUMPAD4);
	AddInputKeyStateMap(IK_NUMPAD5);
	AddInputKeyStateMap(IK_NUMPAD6);
	AddInputKeyStateMap(IK_NUMPAD7);
	AddInputKeyStateMap(IK_NUMPAD8);
	AddInputKeyStateMap(IK_NUMPAD9);
	AddInputKeyStateMap(IK_MULTIPLY);
	AddInputKeyStateMap(IK_ADD);
	AddInputKeyStateMap(IK_SEPARATOR);
	AddInputKeyStateMap(IK_SUBTRACT);

	AddInputKeyStateMap(IK_DECIMAL);
	AddInputKeyStateMap(IK_DIVIDE);
	AddInputKeyStateMap(IK_F1);
	AddInputKeyStateMap(IK_F2);
	AddInputKeyStateMap(IK_F3);
	AddInputKeyStateMap(IK_F4);
	AddInputKeyStateMap(IK_F5);
	AddInputKeyStateMap(IK_F6);
	AddInputKeyStateMap(IK_F7);
	AddInputKeyStateMap(IK_F8);
	AddInputKeyStateMap(IK_F9);
	AddInputKeyStateMap(IK_F10);
	AddInputKeyStateMap(IK_F11);
	AddInputKeyStateMap(IK_F12);
	AddInputKeyStateMap(IK_F13);
	AddInputKeyStateMap(IK_F14);
	AddInputKeyStateMap(IK_F15);
	AddInputKeyStateMap(IK_F16);
	AddInputKeyStateMap(IK_F17);
	AddInputKeyStateMap(IK_F18);
	AddInputKeyStateMap(IK_F19);
	AddInputKeyStateMap(IK_F20);
	AddInputKeyStateMap(IK_F21);
	AddInputKeyStateMap(IK_F22);
	AddInputKeyStateMap(IK_F23);
	AddInputKeyStateMap(IK_F24);

	AddInputKeyStateMap(IK_NUMLOCK);
	AddInputKeyStateMap(IK_SCROLL);

	AddInputKeyStateMap(IK_LEFTSHIFT);
	AddInputKeyStateMap(IK_RIGHTSHIFT);
	AddInputKeyStateMap(IK_LEFTCONTROL);
	AddInputKeyStateMap(IK_RIGHTCONTROL);
	AddInputKeyStateMap(IK_LEFTALT);
	AddInputKeyStateMap(IK_RIGHTALT);
}
