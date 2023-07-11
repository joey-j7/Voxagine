#include "pch.h"
#include "WINInputContext.h"

#include <wrl/wrappers/corewrappers.h>

#include "Core/Platform/Window/WindowContext.h"
#include "Core/Platform/Input/InputContext.h"
#include "Core/Platform/Platform.h"

#ifdef EDITOR
#include "External/imgui/imgui.h"
#endif

#include <Core/Application.h>

WINInputContext::~WINInputContext()
{
	delete m_pKeyboard;
	delete m_pMouse;
	delete m_pGamepad;
}

void WINInputContext::Initialize()
{
	InputContext::Initialize();

	m_pKeyboard = new Keyboard();
	m_pMouse = new Mouse();
	m_pGamepad = new GamePad();

	m_prevMousePosition = UVector2(0, 0);

	m_pMouse->SetWindow(*(HWND*)m_pPlatform->GetWindowContext()->GetHandle());

	m_KeyTable[InputBinding::IB_None] = Keyboard::None;
	m_KeyTable[InputBinding::IB_Back] = Keyboard::Back;
	m_KeyTable[InputBinding::IB_Tab] = Keyboard::Tab;
	m_KeyTable[InputBinding::IB_Enter] = Keyboard::Enter;
	m_KeyTable[InputBinding::IB_Pause] = Keyboard::Pause;
	m_KeyTable[InputBinding::IB_CapsLock] = Keyboard::CapsLock;
	m_KeyTable[InputBinding::IB_Escape] = Keyboard::Escape;
	m_KeyTable[InputBinding::IB_Space] = Keyboard::Space;
	m_KeyTable[InputBinding::IB_PageUp] = Keyboard::PageUp;
	m_KeyTable[InputBinding::IB_PageDown] = Keyboard::PageDown;
	m_KeyTable[InputBinding::IB_End] = Keyboard::End;
	m_KeyTable[InputBinding::IB_Home] = Keyboard::Home;
	m_KeyTable[InputBinding::IB_Left] = Keyboard::Left;
	m_KeyTable[InputBinding::IB_Up] = Keyboard::Up;
	m_KeyTable[InputBinding::IB_Right] = Keyboard::Right;
	m_KeyTable[InputBinding::IB_Down] = Keyboard::Down;
	m_KeyTable[InputBinding::IB_Select] = Keyboard::Select;
	m_KeyTable[InputBinding::IB_Print] = Keyboard::Print;
	m_KeyTable[InputBinding::IB_Execute] = Keyboard::Execute;
	m_KeyTable[InputBinding::IB_PrintScreen] = Keyboard::PrintScreen;
	m_KeyTable[InputBinding::IB_Insert] = Keyboard::Insert;
	m_KeyTable[InputBinding::IB_Delete] = Keyboard::Delete;
	m_KeyTable[InputBinding::IB_Help] = Keyboard::Help;
	m_KeyTable[InputBinding::IB_D0] = Keyboard::D0;
	m_KeyTable[InputBinding::IB_D1] = Keyboard::D1;
	m_KeyTable[InputBinding::IB_D2] = Keyboard::D2;
	m_KeyTable[InputBinding::IB_D3] = Keyboard::D3;
	m_KeyTable[InputBinding::IB_D4] = Keyboard::D4;
	m_KeyTable[InputBinding::IB_D5] = Keyboard::D5;
	m_KeyTable[InputBinding::IB_D6] = Keyboard::D6;
	m_KeyTable[InputBinding::IB_D7] = Keyboard::D7;
	m_KeyTable[InputBinding::IB_D8] = Keyboard::D8;
	m_KeyTable[InputBinding::IB_D9] = Keyboard::D9;

	m_KeyTable[InputBinding::IB_A] = Keyboard::A;
	m_KeyTable[InputBinding::IB_B] = Keyboard::B;
	m_KeyTable[InputBinding::IB_C] = Keyboard::C;
	m_KeyTable[InputBinding::IB_D] = Keyboard::D;
	m_KeyTable[InputBinding::IB_E] = Keyboard::E;
	m_KeyTable[InputBinding::IB_F] = Keyboard::F;
	m_KeyTable[InputBinding::IB_G] = Keyboard::G;
	m_KeyTable[InputBinding::IB_H] = Keyboard::H;
	m_KeyTable[InputBinding::IB_I] = Keyboard::I;
	m_KeyTable[InputBinding::IB_J] = Keyboard::J;
	m_KeyTable[InputBinding::IB_K] = Keyboard::K;
	m_KeyTable[InputBinding::IB_L] = Keyboard::L;
	m_KeyTable[InputBinding::IB_M] = Keyboard::M;
	m_KeyTable[InputBinding::IB_N] = Keyboard::N;
	m_KeyTable[InputBinding::IB_O] = Keyboard::O;
	m_KeyTable[InputBinding::IB_P] = Keyboard::P;
	m_KeyTable[InputBinding::IB_Q] = Keyboard::Q;
	m_KeyTable[InputBinding::IB_R] = Keyboard::R;
	m_KeyTable[InputBinding::IB_S] = Keyboard::S;
	m_KeyTable[InputBinding::IB_T] = Keyboard::T;
	m_KeyTable[InputBinding::IB_U] = Keyboard::U;
	m_KeyTable[InputBinding::IB_V] = Keyboard::V;
	m_KeyTable[InputBinding::IB_W] = Keyboard::W;
	m_KeyTable[InputBinding::IB_X] = Keyboard::X;
	m_KeyTable[InputBinding::IB_Y] = Keyboard::Y;
	m_KeyTable[InputBinding::IB_Z] = Keyboard::Z;
	m_KeyTable[InputBinding::IB_LeftWindows] = Keyboard::LeftWindows;
	m_KeyTable[InputBinding::IB_RightWindows] = Keyboard::RightWindows;
	m_KeyTable[InputBinding::IB_Apps] = Keyboard::Apps;

	m_KeyTable[InputBinding::IB_NumPad0] = Keyboard::NumPad0;
	m_KeyTable[InputBinding::IB_NumPad1] = Keyboard::NumPad1;
	m_KeyTable[InputBinding::IB_NumPad2] = Keyboard::NumPad2;
	m_KeyTable[InputBinding::IB_NumPad3] = Keyboard::NumPad3;
	m_KeyTable[InputBinding::IB_NumPad4] = Keyboard::NumPad4;
	m_KeyTable[InputBinding::IB_NumPad5] = Keyboard::NumPad5;
	m_KeyTable[InputBinding::IB_NumPad6] = Keyboard::NumPad6;
	m_KeyTable[InputBinding::IB_NumPad7] = Keyboard::NumPad7;
	m_KeyTable[InputBinding::IB_NumPad8] = Keyboard::NumPad8;
	m_KeyTable[InputBinding::IB_NumPad9] = Keyboard::NumPad9;

	m_KeyTable[InputBinding::IB_Multiply] = Keyboard::Multiply;
	m_KeyTable[InputBinding::IB_Add] = Keyboard::Add;
	m_KeyTable[InputBinding::IB_Separator] = Keyboard::Separator;
	m_KeyTable[InputBinding::IB_Subtract] = Keyboard::Subtract;
	m_KeyTable[InputBinding::IB_Decimal] = Keyboard::Decimal;
	m_KeyTable[InputBinding::IB_Divide] = Keyboard::Divide;

	m_KeyTable[InputBinding::IB_F1] = Keyboard::F1;
	m_KeyTable[InputBinding::IB_F2] = Keyboard::F2;
	m_KeyTable[InputBinding::IB_F3] = Keyboard::F3;
	m_KeyTable[InputBinding::IB_F4] = Keyboard::F4;
	m_KeyTable[InputBinding::IB_F5] = Keyboard::F5;
	m_KeyTable[InputBinding::IB_F6] = Keyboard::F6;
	m_KeyTable[InputBinding::IB_F7] = Keyboard::F7;
	m_KeyTable[InputBinding::IB_F8] = Keyboard::F8;
	m_KeyTable[InputBinding::IB_F9] = Keyboard::F9;
	m_KeyTable[InputBinding::IB_F10] = Keyboard::F10;
	m_KeyTable[InputBinding::IB_F11] = Keyboard::F11;
	m_KeyTable[InputBinding::IB_F12] = Keyboard::F12;
	m_KeyTable[InputBinding::IB_F13] = Keyboard::F13;
	m_KeyTable[InputBinding::IB_F14] = Keyboard::F14;
	m_KeyTable[InputBinding::IB_F15] = Keyboard::F15;
	m_KeyTable[InputBinding::IB_F16] = Keyboard::F16;
	m_KeyTable[InputBinding::IB_F17] = Keyboard::F17;
	m_KeyTable[InputBinding::IB_F18] = Keyboard::F18;
	m_KeyTable[InputBinding::IB_F19] = Keyboard::F19;
	m_KeyTable[InputBinding::IB_F20] = Keyboard::F20;
	m_KeyTable[InputBinding::IB_F21] = Keyboard::F21;
	m_KeyTable[InputBinding::IB_F22] = Keyboard::F22;
	m_KeyTable[InputBinding::IB_F23] = Keyboard::F23;
	m_KeyTable[InputBinding::IB_F24] = Keyboard::F24;

	m_KeyTable[InputBinding::IB_NumLock] = Keyboard::NumLock;
	m_KeyTable[InputBinding::IB_Scroll] = Keyboard::Scroll;

	m_KeyTable[InputBinding::IB_LeftShift] = Keyboard::LeftShift;
	m_KeyTable[InputBinding::IB_RightShift] = Keyboard::RightShift;
	m_KeyTable[InputBinding::IB_LeftControl] = Keyboard::LeftControl;
	m_KeyTable[InputBinding::IB_RightControl] = Keyboard::RightControl;
	m_KeyTable[InputBinding::IB_LeftAlt] = Keyboard::LeftAlt;
	m_KeyTable[InputBinding::IB_RightAlt] = Keyboard::RightAlt;
}

void WINInputContext::Update()
{
	/* Update Gamepad */
	auto Pad = m_pGamepad->GetState(0);

	if (Pad.IsConnected()) 
		m_GamePadButtons.Update(Pad);
	else m_GamePadButtons.Reset();

	/* Update Keyboard */
	auto Keyboard = m_pKeyboard->GetState();
	m_KeyboardButtons.Update(Keyboard);

	/* Update Mouse */
	auto mouse = m_pMouse->GetState();

	m_MouseDelta = Vector2((float)mouse.x, (float)mouse.y) - Vector2((float)m_MousePosition.x, (float)m_MousePosition.y);

	m_MousePosition.x = mouse.x;
	m_MousePosition.y = mouse.y;

	m_MouseWheelDelta = mouse.scrollWheelValue - m_MouseWheelValue;
	m_MouseWheelValue = static_cast<float>(mouse.scrollWheelValue);

	/* Update input bindings and run callbacks */
	UpdateGamePadActionTable(Pad);
	UpdateGamePadAxisTable(Pad);
	UpdateMouseTable(mouse);

	UpdateAxis();
	UpdateActions();

	m_Keys[IB_GamepadDPadUp] = m_GamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.dpadUp == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadDPadDown] = m_GamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.dpadDown == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadDPadRight] = m_GamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.dpadRight == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadDPadLeft] = m_GamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.dpadLeft == GamePad::ButtonStateTracker::HELD;

	m_Keys[IB_GamepadButtonUp] = m_GamePadButtons.y == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.y == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadButtonDown] = m_GamePadButtons.a == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.a == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadButtonRight] = m_GamePadButtons.b == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.b == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadButtonLeft] = m_GamePadButtons.x == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.x == GamePad::ButtonStateTracker::HELD;

	m_Keys[IB_GamepadLeftTrigger] = m_GamePadButtons.leftTrigger == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.leftTrigger == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadRightTrigger] = m_GamePadButtons.rightTrigger == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.rightTrigger == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadLeftShoulder] = m_GamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.leftShoulder == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadRightShoulder] = m_GamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.rightShoulder == GamePad::ButtonStateTracker::HELD;

	m_Keys[IB_GamepadMenu] = m_GamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.menu == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadStart] = m_GamePadButtons.start == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.start == GamePad::ButtonStateTracker::HELD;
	m_Keys[IB_GamepadView] = m_GamePadButtons.view == GamePad::ButtonStateTracker::PRESSED || m_GamePadButtons.view == GamePad::ButtonStateTracker::HELD;

	m_Keys[IB_MouseButtonLeft] = mouse.leftButton;
	m_Keys[IB_MouseButtonRight] = mouse.rightButton;
	m_Keys[IB_MouseScrollWheel] = mouse.middleButton;

	m_Keys[IB_None] = false;
	m_Keys[IB_Back] = m_KeyboardButtons.IsKeyPressed(Keyboard::Back);
	m_Keys[IB_Tab] = m_KeyboardButtons.IsKeyPressed(Keyboard::Tab);
	m_Keys[IB_Enter] = m_KeyboardButtons.IsKeyPressed(Keyboard::Enter);
	m_Keys[IB_Pause] = m_KeyboardButtons.IsKeyPressed(Keyboard::Pause);
	m_Keys[IB_CapsLock] = m_KeyboardButtons.IsKeyPressed(Keyboard::CapsLock);
	m_Keys[IB_Escape] = m_KeyboardButtons.IsKeyPressed(Keyboard::Escape);
	m_Keys[IB_Space] = m_KeyboardButtons.IsKeyPressed(Keyboard::Space);
	m_Keys[IB_PageUp] = m_KeyboardButtons.IsKeyPressed(Keyboard::PageUp);
	m_Keys[IB_PageDown] = m_KeyboardButtons.IsKeyPressed(Keyboard::PageDown);
	m_Keys[IB_End] = m_KeyboardButtons.IsKeyPressed(Keyboard::End);
	m_Keys[IB_Home] = m_KeyboardButtons.IsKeyPressed(Keyboard::Home);
	m_Keys[IB_Left] = m_KeyboardButtons.IsKeyPressed(Keyboard::Left);
	m_Keys[IB_Up] = m_KeyboardButtons.IsKeyPressed(Keyboard::Up);
	m_Keys[IB_Right] = m_KeyboardButtons.IsKeyPressed(Keyboard::Right);
	m_Keys[IB_Down] = m_KeyboardButtons.IsKeyPressed(Keyboard::Down);
	m_Keys[IB_Select] = m_KeyboardButtons.IsKeyPressed(Keyboard::Select);
	m_Keys[IB_Print] = m_KeyboardButtons.IsKeyPressed(Keyboard::Print);
	m_Keys[IB_Execute] = m_KeyboardButtons.IsKeyPressed(Keyboard::Execute);
	m_Keys[IB_PrintScreen] = m_KeyboardButtons.IsKeyPressed(Keyboard::PrintScreen);
	m_Keys[IB_Insert] = m_KeyboardButtons.IsKeyPressed(Keyboard::Insert);
	m_Keys[IB_Delete] = m_KeyboardButtons.IsKeyPressed(Keyboard::Delete);
	m_Keys[IB_Help] = m_KeyboardButtons.IsKeyPressed(Keyboard::Help);
	m_Keys[IB_D0] = m_KeyboardButtons.IsKeyPressed(Keyboard::D0);
	m_Keys[IB_D1] = m_KeyboardButtons.IsKeyPressed(Keyboard::D1);
	m_Keys[IB_D2] = m_KeyboardButtons.IsKeyPressed(Keyboard::D2);
	m_Keys[IB_D3] = m_KeyboardButtons.IsKeyPressed(Keyboard::D3);
	m_Keys[IB_D4] = m_KeyboardButtons.IsKeyPressed(Keyboard::D4);
	m_Keys[IB_D5] = m_KeyboardButtons.IsKeyPressed(Keyboard::D5);
	m_Keys[IB_D6] = m_KeyboardButtons.IsKeyPressed(Keyboard::D6);
	m_Keys[IB_D7] = m_KeyboardButtons.IsKeyPressed(Keyboard::D7);
	m_Keys[IB_D8] = m_KeyboardButtons.IsKeyPressed(Keyboard::D8);
	m_Keys[IB_D9] = m_KeyboardButtons.IsKeyPressed(Keyboard::D9);

	m_Keys[IB_A] = m_KeyboardButtons.IsKeyPressed(Keyboard::A);
	m_Keys[IB_B] = m_KeyboardButtons.IsKeyPressed(Keyboard::B);
	m_Keys[IB_C] = m_KeyboardButtons.IsKeyPressed(Keyboard::C);
	m_Keys[IB_D] = m_KeyboardButtons.IsKeyPressed(Keyboard::D);
	m_Keys[IB_E] = m_KeyboardButtons.IsKeyPressed(Keyboard::E);
	m_Keys[IB_F] = m_KeyboardButtons.IsKeyPressed(Keyboard::F);
	m_Keys[IB_G] = m_KeyboardButtons.IsKeyPressed(Keyboard::G);
	m_Keys[IB_H] = m_KeyboardButtons.IsKeyPressed(Keyboard::H);
	m_Keys[IB_I] = m_KeyboardButtons.IsKeyPressed(Keyboard::I);
	m_Keys[IB_J] = m_KeyboardButtons.IsKeyPressed(Keyboard::J);
	m_Keys[IB_K] = m_KeyboardButtons.IsKeyPressed(Keyboard::K);
	m_Keys[IB_L] = m_KeyboardButtons.IsKeyPressed(Keyboard::L);
	m_Keys[IB_M] = m_KeyboardButtons.IsKeyPressed(Keyboard::M);
	m_Keys[IB_N] = m_KeyboardButtons.IsKeyPressed(Keyboard::N);
	m_Keys[IB_O] = m_KeyboardButtons.IsKeyPressed(Keyboard::O);
	m_Keys[IB_P] = m_KeyboardButtons.IsKeyPressed(Keyboard::P);
	m_Keys[IB_Q] = m_KeyboardButtons.IsKeyPressed(Keyboard::Q);
	m_Keys[IB_R] = m_KeyboardButtons.IsKeyPressed(Keyboard::R);
	m_Keys[IB_S] = m_KeyboardButtons.IsKeyPressed(Keyboard::S);
	m_Keys[IB_T] = m_KeyboardButtons.IsKeyPressed(Keyboard::T);
	m_Keys[IB_U] = m_KeyboardButtons.IsKeyPressed(Keyboard::U);
	m_Keys[IB_V] = m_KeyboardButtons.IsKeyPressed(Keyboard::V);
	m_Keys[IB_W] = m_KeyboardButtons.IsKeyPressed(Keyboard::W);
	m_Keys[IB_X] = m_KeyboardButtons.IsKeyPressed(Keyboard::X);
	m_Keys[IB_Y] = m_KeyboardButtons.IsKeyPressed(Keyboard::Y);
	m_Keys[IB_Z] = m_KeyboardButtons.IsKeyPressed(Keyboard::Z);
	m_Keys[IB_LeftWindows] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftWindows);
	m_Keys[IB_RightWindows] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightWindows);
	m_Keys[IB_Apps] = m_KeyboardButtons.IsKeyPressed(Keyboard::Apps);

	m_Keys[IB_NumPad0] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad0);
	m_Keys[IB_NumPad1] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad1);
	m_Keys[IB_NumPad2] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad2);
	m_Keys[IB_NumPad3] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad3);
	m_Keys[IB_NumPad4] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad4);
	m_Keys[IB_NumPad5] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad5);
	m_Keys[IB_NumPad6] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad6);
	m_Keys[IB_NumPad7] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad7);
	m_Keys[IB_NumPad8] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad8);
	m_Keys[IB_NumPad9] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad9);
	m_Keys[IB_Multiply] = m_KeyboardButtons.IsKeyPressed(Keyboard::Multiply);
	m_Keys[IB_Add] = m_KeyboardButtons.IsKeyPressed(Keyboard::Add);
	m_Keys[IB_Separator] = m_KeyboardButtons.IsKeyPressed(Keyboard::Separator);
	m_Keys[IB_Subtract] = m_KeyboardButtons.IsKeyPressed(Keyboard::Subtract);

	m_Keys[IB_Decimal] = m_KeyboardButtons.IsKeyPressed(Keyboard::Decimal);
	m_Keys[IB_Divide] = m_KeyboardButtons.IsKeyPressed(Keyboard::Divide);
	m_Keys[IB_F1] = m_KeyboardButtons.IsKeyPressed(Keyboard::F1);
	m_Keys[IB_F2] = m_KeyboardButtons.IsKeyPressed(Keyboard::F2);
	m_Keys[IB_F3] = m_KeyboardButtons.IsKeyPressed(Keyboard::F3);
	m_Keys[IB_F4] = m_KeyboardButtons.IsKeyPressed(Keyboard::F4);
	m_Keys[IB_F5] = m_KeyboardButtons.IsKeyPressed(Keyboard::F5);
	m_Keys[IB_F6] = m_KeyboardButtons.IsKeyPressed(Keyboard::F6);
	m_Keys[IB_F7] = m_KeyboardButtons.IsKeyPressed(Keyboard::F7);
	m_Keys[IB_F8] = m_KeyboardButtons.IsKeyPressed(Keyboard::F8);
	m_Keys[IB_F9] = m_KeyboardButtons.IsKeyPressed(Keyboard::F9);
	m_Keys[IB_F10] = m_KeyboardButtons.IsKeyPressed(Keyboard::F10);
	m_Keys[IB_F11] = m_KeyboardButtons.IsKeyPressed(Keyboard::F11);
	m_Keys[IB_F12] = m_KeyboardButtons.IsKeyPressed(Keyboard::F12);
	m_Keys[IB_F13] = m_KeyboardButtons.IsKeyPressed(Keyboard::F13);
	m_Keys[IB_F14] = m_KeyboardButtons.IsKeyPressed(Keyboard::F14);
	m_Keys[IB_F15] = m_KeyboardButtons.IsKeyPressed(Keyboard::F15);
	m_Keys[IB_F16] = m_KeyboardButtons.IsKeyPressed(Keyboard::F16);
	m_Keys[IB_F17] = m_KeyboardButtons.IsKeyPressed(Keyboard::F17);
	m_Keys[IB_F18] = m_KeyboardButtons.IsKeyPressed(Keyboard::F18);
	m_Keys[IB_F19] = m_KeyboardButtons.IsKeyPressed(Keyboard::F19);
	m_Keys[IB_F20] = m_KeyboardButtons.IsKeyPressed(Keyboard::F20);
	m_Keys[IB_F21] = m_KeyboardButtons.IsKeyPressed(Keyboard::F21);
	m_Keys[IB_F22] = m_KeyboardButtons.IsKeyPressed(Keyboard::F22);
	m_Keys[IB_F23] = m_KeyboardButtons.IsKeyPressed(Keyboard::F23);
	m_Keys[IB_F24] = m_KeyboardButtons.IsKeyPressed(Keyboard::F24);

	m_Keys[IB_NumLock] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumLock);
	m_Keys[IB_Scroll] = m_KeyboardButtons.IsKeyPressed(Keyboard::Scroll);

	m_Keys[IB_LeftShift] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftShift);
	m_Keys[IB_RightShift] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightShift);
	m_Keys[IB_LeftControl] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftControl);
	m_Keys[IB_RightControl] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightControl);
	m_Keys[IB_LeftAlt] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftAlt);
	m_Keys[IB_RightAlt] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightAlt);

#ifdef EDITOR
	ImGuiIO& io = ImGui::GetIO();

	io.KeysDown[IB_None] = false;
	io.KeysDown[IB_Back] = m_KeyboardButtons.IsKeyPressed(Keyboard::Back);
	io.KeysDown[IB_Tab] = m_KeyboardButtons.IsKeyPressed(Keyboard::Tab);
	io.KeysDown[IB_Enter] = m_KeyboardButtons.IsKeyPressed(Keyboard::Enter);
	io.KeysDown[IB_Pause] = m_KeyboardButtons.IsKeyPressed(Keyboard::Pause);
	io.KeysDown[IB_CapsLock] = m_KeyboardButtons.IsKeyPressed(Keyboard::CapsLock);
	io.KeysDown[IB_Escape] = m_KeyboardButtons.IsKeyPressed(Keyboard::Escape);
	io.KeysDown[IB_Space] = m_KeyboardButtons.IsKeyPressed(Keyboard::Space);
	io.KeysDown[IB_PageUp] = m_KeyboardButtons.IsKeyPressed(Keyboard::PageUp);
	io.KeysDown[IB_PageDown] = m_KeyboardButtons.IsKeyPressed(Keyboard::PageDown);
	io.KeysDown[IB_End] = m_KeyboardButtons.IsKeyPressed(Keyboard::End);
	io.KeysDown[IB_Home] = m_KeyboardButtons.IsKeyPressed(Keyboard::Home);
	io.KeysDown[IB_Left] = m_KeyboardButtons.IsKeyPressed(Keyboard::Left);
	io.KeysDown[IB_Up] = m_KeyboardButtons.IsKeyPressed(Keyboard::Up);
	io.KeysDown[IB_Right] = m_KeyboardButtons.IsKeyPressed(Keyboard::Right);
	io.KeysDown[IB_Down] = m_KeyboardButtons.IsKeyPressed(Keyboard::Down);
	io.KeysDown[IB_Select] = m_KeyboardButtons.IsKeyPressed(Keyboard::Select);
	io.KeysDown[IB_Print] = m_KeyboardButtons.IsKeyPressed(Keyboard::Print);
	io.KeysDown[IB_Execute] = m_KeyboardButtons.IsKeyPressed(Keyboard::Execute);
	io.KeysDown[IB_PrintScreen] = m_KeyboardButtons.IsKeyPressed(Keyboard::PrintScreen);
	io.KeysDown[IB_Insert] = m_KeyboardButtons.IsKeyPressed(Keyboard::Insert);
	io.KeysDown[IB_Delete] = m_KeyboardButtons.IsKeyPressed(Keyboard::Delete);
	io.KeysDown[IB_Help] = m_KeyboardButtons.IsKeyPressed(Keyboard::Help);
	io.KeysDown[IB_D0] = m_KeyboardButtons.IsKeyPressed(Keyboard::D0);
	io.KeysDown[IB_D1] = m_KeyboardButtons.IsKeyPressed(Keyboard::D1);
	io.KeysDown[IB_D2] = m_KeyboardButtons.IsKeyPressed(Keyboard::D2);
	io.KeysDown[IB_D3] = m_KeyboardButtons.IsKeyPressed(Keyboard::D3);
	io.KeysDown[IB_D4] = m_KeyboardButtons.IsKeyPressed(Keyboard::D4);
	io.KeysDown[IB_D5] = m_KeyboardButtons.IsKeyPressed(Keyboard::D5);
	io.KeysDown[IB_D6] = m_KeyboardButtons.IsKeyPressed(Keyboard::D6);
	io.KeysDown[IB_D7] = m_KeyboardButtons.IsKeyPressed(Keyboard::D7);
	io.KeysDown[IB_D8] = m_KeyboardButtons.IsKeyPressed(Keyboard::D8);
	io.KeysDown[IB_D9] = m_KeyboardButtons.IsKeyPressed(Keyboard::D9);

	io.KeysDown[IB_A] = m_KeyboardButtons.IsKeyPressed(Keyboard::A);
	io.KeysDown[IB_B] = m_KeyboardButtons.IsKeyPressed(Keyboard::B);
	io.KeysDown[IB_C] = m_KeyboardButtons.IsKeyPressed(Keyboard::C);
	io.KeysDown[IB_D] = m_KeyboardButtons.IsKeyPressed(Keyboard::D);
	io.KeysDown[IB_E] = m_KeyboardButtons.IsKeyPressed(Keyboard::E);
	io.KeysDown[IB_F] = m_KeyboardButtons.IsKeyPressed(Keyboard::F);
	io.KeysDown[IB_G] = m_KeyboardButtons.IsKeyPressed(Keyboard::G);
	io.KeysDown[IB_H] = m_KeyboardButtons.IsKeyPressed(Keyboard::H);
	io.KeysDown[IB_I] = m_KeyboardButtons.IsKeyPressed(Keyboard::I);
	io.KeysDown[IB_J] = m_KeyboardButtons.IsKeyPressed(Keyboard::J);
	io.KeysDown[IB_K] = m_KeyboardButtons.IsKeyPressed(Keyboard::K);
	io.KeysDown[IB_L] = m_KeyboardButtons.IsKeyPressed(Keyboard::L);
	io.KeysDown[IB_M] = m_KeyboardButtons.IsKeyPressed(Keyboard::M);
	io.KeysDown[IB_N] = m_KeyboardButtons.IsKeyPressed(Keyboard::N);
	io.KeysDown[IB_O] = m_KeyboardButtons.IsKeyPressed(Keyboard::O);
	io.KeysDown[IB_P] = m_KeyboardButtons.IsKeyPressed(Keyboard::P);
	io.KeysDown[IB_Q] = m_KeyboardButtons.IsKeyPressed(Keyboard::Q);
	io.KeysDown[IB_R] = m_KeyboardButtons.IsKeyPressed(Keyboard::R);
	io.KeysDown[IB_S] = m_KeyboardButtons.IsKeyPressed(Keyboard::S);
	io.KeysDown[IB_T] = m_KeyboardButtons.IsKeyPressed(Keyboard::T);
	io.KeysDown[IB_U] = m_KeyboardButtons.IsKeyPressed(Keyboard::U);
	io.KeysDown[IB_V] = m_KeyboardButtons.IsKeyPressed(Keyboard::V);
	io.KeysDown[IB_W] = m_KeyboardButtons.IsKeyPressed(Keyboard::W);
	io.KeysDown[IB_X] = m_KeyboardButtons.IsKeyPressed(Keyboard::X);
	io.KeysDown[IB_Y] = m_KeyboardButtons.IsKeyPressed(Keyboard::Y);
	io.KeysDown[IB_Z] = m_KeyboardButtons.IsKeyPressed(Keyboard::Z);
	io.KeysDown[IB_LeftWindows] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftWindows);
	io.KeysDown[IB_RightWindows] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightWindows);
	io.KeysDown[IB_Apps] = m_KeyboardButtons.IsKeyPressed(Keyboard::Apps);

	io.KeysDown[IB_NumPad0] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad0);
	io.KeysDown[IB_NumPad1] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad1);
	io.KeysDown[IB_NumPad2] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad2);
	io.KeysDown[IB_NumPad3] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad3);
	io.KeysDown[IB_NumPad4] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad4);
	io.KeysDown[IB_NumPad5] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad5);
	io.KeysDown[IB_NumPad6] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad6);
	io.KeysDown[IB_NumPad7] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad7);
	io.KeysDown[IB_NumPad8] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad8);
	io.KeysDown[IB_NumPad9] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumPad9);
	io.KeysDown[IB_Multiply] = m_KeyboardButtons.IsKeyPressed(Keyboard::Multiply);
	io.KeysDown[IB_Add] = m_KeyboardButtons.IsKeyPressed(Keyboard::Add);
	io.KeysDown[IB_Separator] = m_KeyboardButtons.IsKeyPressed(Keyboard::Separator);
	io.KeysDown[IB_Subtract] = m_KeyboardButtons.IsKeyPressed(Keyboard::Subtract);

	io.KeysDown[IB_Decimal] = m_KeyboardButtons.IsKeyPressed(Keyboard::Decimal);
	io.KeysDown[IB_Divide] = m_KeyboardButtons.IsKeyPressed(Keyboard::Divide);
	io.KeysDown[IB_F1] = m_KeyboardButtons.IsKeyPressed(Keyboard::F1);
	io.KeysDown[IB_F2] = m_KeyboardButtons.IsKeyPressed(Keyboard::F2);
	io.KeysDown[IB_F3] = m_KeyboardButtons.IsKeyPressed(Keyboard::F3);
	io.KeysDown[IB_F4] = m_KeyboardButtons.IsKeyPressed(Keyboard::F4);
	io.KeysDown[IB_F5] = m_KeyboardButtons.IsKeyPressed(Keyboard::F5);
	io.KeysDown[IB_F6] = m_KeyboardButtons.IsKeyPressed(Keyboard::F6);
	io.KeysDown[IB_F7] = m_KeyboardButtons.IsKeyPressed(Keyboard::F7);
	io.KeysDown[IB_F8] = m_KeyboardButtons.IsKeyPressed(Keyboard::F8);
	io.KeysDown[IB_F9] = m_KeyboardButtons.IsKeyPressed(Keyboard::F9);
	io.KeysDown[IB_F10] = m_KeyboardButtons.IsKeyPressed(Keyboard::F10);
	io.KeysDown[IB_F11] = m_KeyboardButtons.IsKeyPressed(Keyboard::F11);
	io.KeysDown[IB_F12] = m_KeyboardButtons.IsKeyPressed(Keyboard::F12);
	io.KeysDown[IB_F13] = m_KeyboardButtons.IsKeyPressed(Keyboard::F13);
	io.KeysDown[IB_F14] = m_KeyboardButtons.IsKeyPressed(Keyboard::F14);
	io.KeysDown[IB_F15] = m_KeyboardButtons.IsKeyPressed(Keyboard::F15);
	io.KeysDown[IB_F16] = m_KeyboardButtons.IsKeyPressed(Keyboard::F16);
	io.KeysDown[IB_F17] = m_KeyboardButtons.IsKeyPressed(Keyboard::F17);
	io.KeysDown[IB_F18] = m_KeyboardButtons.IsKeyPressed(Keyboard::F18);
	io.KeysDown[IB_F19] = m_KeyboardButtons.IsKeyPressed(Keyboard::F19);
	io.KeysDown[IB_F20] = m_KeyboardButtons.IsKeyPressed(Keyboard::F20);
	io.KeysDown[IB_F21] = m_KeyboardButtons.IsKeyPressed(Keyboard::F21);
	io.KeysDown[IB_F22] = m_KeyboardButtons.IsKeyPressed(Keyboard::F22);
	io.KeysDown[IB_F23] = m_KeyboardButtons.IsKeyPressed(Keyboard::F23);
	io.KeysDown[IB_F24] = m_KeyboardButtons.IsKeyPressed(Keyboard::F24);

	io.KeysDown[IB_NumLock] = m_KeyboardButtons.IsKeyPressed(Keyboard::NumLock);
	io.KeysDown[IB_Scroll] = m_KeyboardButtons.IsKeyPressed(Keyboard::Scroll);

	io.KeysDown[IB_LeftShift] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftShift);
	io.KeysDown[IB_RightShift] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightShift);
	io.KeysDown[IB_LeftControl] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftControl);
	io.KeysDown[IB_RightControl] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightControl);
	io.KeysDown[IB_LeftAlt] = m_KeyboardButtons.IsKeyPressed(Keyboard::LeftAlt);
	io.KeysDown[IB_RightAlt] = m_KeyboardButtons.IsKeyPressed(Keyboard::RightAlt);

	io.KeyAlt = io.KeysDown[IB_LeftAlt] || io.KeysDown[IB_RightAlt];
	io.KeyCtrl = io.KeysDown[IB_LeftControl] || io.KeysDown[IB_RightControl];
	io.KeyShift = io.KeysDown[IB_LeftShift] || io.KeysDown[IB_RightShift];
	io.KeySuper = io.KeysDown[IB_LeftWindows] || io.KeysDown[IB_RightWindows];
	
	io.MousePos = ImVec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

	io.MouseDown[0] = mouse.leftButton;
	io.MouseDown[1] = mouse.rightButton;
	io.MouseDown[2] = mouse.middleButton;

	io.MouseWheel = m_MouseWheelDelta != 0.0f ? static_cast<float>(m_MouseWheelDelta / abs(m_MouseWheelDelta)) * 0.2f : 0.0f;
#endif
}

void WINInputContext::UpdateGamePadActionTable(DirectX::GamePad::State& gamePad)
{
	/* Set the previous key state to the current state before polling the new key state again */
	for (auto& statePair : m_GamePadActionTable)
		statePair.second.bPreviousState = statePair.second.bCurrentState;

	m_GamePadActionTable[InputBinding::IB_GamepadButtonUp].bCurrentState = gamePad.buttons.y;
	m_GamePadActionTable[InputBinding::IB_GamepadButtonDown].bCurrentState = gamePad.buttons.a;
	m_GamePadActionTable[InputBinding::IB_GamepadButtonRight].bCurrentState = gamePad.buttons.b;
	m_GamePadActionTable[InputBinding::IB_GamepadButtonLeft].bCurrentState = gamePad.buttons.x;

	m_GamePadActionTable[InputBinding::IB_GamepadDPadUp].bCurrentState = gamePad.dpad.up;
	m_GamePadActionTable[InputBinding::IB_GamepadDPadDown].bCurrentState = gamePad.dpad.down;
	m_GamePadActionTable[InputBinding::IB_GamepadDPadRight].bCurrentState = gamePad.dpad.right;
	m_GamePadActionTable[InputBinding::IB_GamepadDPadLeft].bCurrentState = gamePad.dpad.left;

	m_GamePadActionTable[InputBinding::IB_GamepadLeftTrigger].bCurrentState = gamePad.triggers.left > 0.5f;
	m_GamePadActionTable[InputBinding::IB_GamepadRightTrigger].bCurrentState = gamePad.triggers.right > 0.5f;
	m_GamePadActionTable[InputBinding::IB_GamepadLeftShoulder].bCurrentState = gamePad.buttons.leftShoulder;
	m_GamePadActionTable[InputBinding::IB_GamepadRightShoulder].bCurrentState = gamePad.buttons.rightShoulder;
	
	m_GamePadActionTable[InputBinding::IB_GamepadMenu].bCurrentState = gamePad.dpad.up;
	m_GamePadActionTable[InputBinding::IB_GamepadView].bCurrentState = gamePad.dpad.down;
	m_GamePadActionTable[InputBinding::IB_GamepadStart].bCurrentState = gamePad.dpad.right;
}

void WINInputContext::UpdateGamePadAxisTable(DirectX::GamePad::State& gamePad)
{
	m_GamePadAxisTable[InputBinding::IB_GamepadLeftThumbstickXAxis] = gamePad.thumbSticks.leftX;
	m_GamePadAxisTable[InputBinding::IB_GamepadLeftThumbstickYAxis] = gamePad.thumbSticks.leftY;
	m_GamePadAxisTable[InputBinding::IB_GamepadRightThumbstickXAxis] = gamePad.thumbSticks.rightX;
	m_GamePadAxisTable[InputBinding::IB_GamepadRightThumbstickYAxis] = gamePad.thumbSticks.rightY;

	m_GamePadAxisTable[InputBinding::IB_GamepadButtonUp] = gamePad.buttons.y ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadButtonDown] = gamePad.buttons.a ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadButtonRight] = gamePad.buttons.b ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadButtonLeft] = gamePad.buttons.x ? 1.f : 0.f;

	m_GamePadAxisTable[InputBinding::IB_GamepadDPadUp] = gamePad.dpad.up ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadDPadDown] = gamePad.dpad.down ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadDPadRight] = gamePad.dpad.right ? 1.f : 0.f;
	m_GamePadAxisTable[InputBinding::IB_GamepadDPadLeft] = gamePad.dpad.left ? 1.f : 0.f;
}

void WINInputContext::UpdateMouseTable(DirectX::Mouse::State& mouse)
{
	for (auto& statePair : m_MouseActionTable)
		statePair.second.bPreviousState = statePair.second.bCurrentState;

	m_MouseActionTable[InputBinding::IB_MouseButtonLeft].bCurrentState = mouse.leftButton;
	m_MouseActionTable[InputBinding::IB_MouseButtonRight].bCurrentState = mouse.rightButton;
	m_MouseActionTable[InputBinding::IB_MouseScrollWheel].bCurrentState = mouse.scrollWheelValue > 0;
	m_MouseActionTable[InputBinding::IB_MouseXAxis].bCurrentState = abs(mouse.x - (int)m_prevMousePosition.x) > 0;
	m_MouseActionTable[InputBinding::IB_MouseYAxis].bCurrentState = abs((int)m_prevMousePosition.y - mouse.y) > 0;

	m_MouseAxisTable[InputBinding::IB_MouseButtonLeft] = mouse.leftButton ? 1.f : 0.f;
	m_MouseAxisTable[InputBinding::IB_MouseButtonRight] = mouse.rightButton ? 1.f : 0.f;
	m_MouseAxisTable[InputBinding::IB_MouseScrollWheel] = (float)mouse.scrollWheelValue;
	m_MouseAxisTable[InputBinding::IB_MouseXAxis] = (float)(mouse.x - m_prevMousePosition.x);
	m_MouseAxisTable[InputBinding::IB_MouseYAxis] = (float)(m_prevMousePosition.y - mouse.y);

	m_prevMousePosition.x = mouse.x;
	m_prevMousePosition.y = mouse.y;
}

void WINInputContext::UpdateAxis()
{
	DirectX::Mouse::State Mouse = m_pMouse->GetState();
	DirectX::GamePad::State Pad = m_pGamepad->GetState(0);
	DirectX::Keyboard::State Keyboard = m_pKeyboard->GetState();

	BindingLayer* ActiveLayer = GetActiveLayer();

	for (InputAxis& axis : ActiveLayer->BindingAxis)
	{
		axis.Value = 0.f;

		for (AxisBinding& binding : axis.Bindings)
		{
			if (Keyboard.IsKeyDown(static_cast<Keyboard::Keys>(m_KeyTable[binding.Type])))
				axis.Value += 1.f * binding.fScalar;
			else if (m_GamePadAxisTable[binding.Type] != 0.f)
				axis.Value += m_GamePadAxisTable[binding.Type] * binding.fScalar;
			else if (m_MouseAxisTable[binding.Type] != 0.f)
				axis.Value += m_MouseAxisTable[binding.Type] * binding.fScalar;
		}

		//Clamp axis value to -1 and 1
		axis.Value = axis.Value < -1.f ? -1.f : (axis.Value > 1.f ? 1.f : axis.Value);

		for (auto& callback : axis.Callbacks)
			callback.second(axis.Value);
	}
}

void WINInputContext::UpdateActions()
{
	DirectX::Mouse::State Mouse = m_pMouse->GetState();
	DirectX::GamePad::State Pad = m_pGamepad->GetState(0);
	DirectX::Keyboard::State Keyboard = m_pKeyboard->GetState();

	BindingLayer* ActiveLayer = GetActiveLayer();

	for (InputAction& inputAction : ActiveLayer->BindingActions)
	{
		bool callBindFunc = false;

		switch (inputAction.CheckEvent)
		{
		case IE_PRESSED:
		{
				auto iter = m_KeyTable.find(inputAction.InputMaster);
				if (iter != m_KeyTable.end() && Keyboard.IsKeyDown(static_cast<Keyboard::Keys>(iter->second)))
				{
					/* RELEASED key has been pressed so it can go check event RELEASE again */
					if (inputAction.Type == IE_RELEASED)
					{
						inputAction.CheckEvent = IE_RELEASED;
						break;
					}

					callBindFunc = true;

					/* PRESSED key has been pressed so it must check for released state before calling pressed again */
					inputAction.CheckEvent = IE_RELEASED;
					break;
				}

				auto iterActionTable = m_GamePadActionTable.find(inputAction.InputMaster);
				if (iterActionTable != m_GamePadActionTable.end() && 
					iterActionTable->second.bCurrentState && 
					iterActionTable->second.bCurrentState != iterActionTable->second.bPreviousState)
				{
					callBindFunc = true;
					break;
				}

				auto iterMouseActionTable = m_MouseActionTable.find(inputAction.InputMaster);
				if (iterMouseActionTable != m_MouseActionTable.end() &&
					iterMouseActionTable->second.bCurrentState &&
					iterMouseActionTable->second.bCurrentState != iterMouseActionTable->second.bPreviousState)
				{
					callBindFunc = true;
					break;
				}
			break;
		}
		case IE_RELEASED:
		{
				auto iter = m_KeyTable.find(inputAction.InputMaster);
				if (iter != m_KeyTable.end() && Keyboard.IsKeyUp(static_cast<Keyboard::Keys>(iter->second)))
				{
					/* PRESSED key has been released so it can go check for PRESSED event again */
					if (inputAction.Type == IE_PRESSED)
					{
						inputAction.CheckEvent = IE_PRESSED;
						break;
					}

					callBindFunc = true;

					/* RELEASED key has been released so it must check for PRESSED event before calling release again */
					inputAction.CheckEvent = IE_PRESSED;
					break;
				}

				auto iterActionTable = m_GamePadActionTable.find(inputAction.InputMaster);
				if (iterActionTable != m_GamePadActionTable.end() &&
					!iterActionTable->second.bCurrentState &&
					iterActionTable->second.bCurrentState != iterActionTable->second.bPreviousState)
				{
					callBindFunc = true;
					break;
				}

				auto iterMouseActionTable = m_MouseActionTable.find(inputAction.InputMaster);
				if (iterMouseActionTable != m_MouseActionTable.end() &&
					!iterMouseActionTable->second.bCurrentState &&
					iterMouseActionTable->second.bCurrentState != iterMouseActionTable->second.bPreviousState)
				{
					callBindFunc = true;
					break;
				}
			break;
		}
		case IE_REPEAT:
		{
				auto iter = m_KeyTable.find(inputAction.InputMaster);
				if (iter != m_KeyTable.end() && Keyboard.IsKeyDown(static_cast<Keyboard::Keys>(iter->second)))
				{
					callBindFunc = true;
					break;
				}

				auto iterActionTable = m_GamePadActionTable.find(inputAction.InputMaster);
				if (iterActionTable != m_GamePadActionTable.end() && iterActionTable->second.bCurrentState)
				{
					callBindFunc = true;
					break;
				}

				auto iterMouseActionTable = m_MouseActionTable.find(inputAction.InputMaster);
				if (iterMouseActionTable != m_MouseActionTable.end() && iterMouseActionTable->second.bCurrentState)
				{
					callBindFunc = true;
					break;
				}
			break;
		}
		}

		if (callBindFunc)
		{
				for (InputBinding& modifier : inputAction.InputModifiers)
				{
					auto iter = m_KeyTable.find(modifier);
					if (iter != m_KeyTable.end() && !Keyboard.IsKeyDown(static_cast<Keyboard::Keys>(iter->second)))
					{
						callBindFunc = false;
						break;
					}

					auto iterActionTable = m_GamePadActionTable.find(modifier);
					if (iterActionTable != m_GamePadActionTable.end() && !iterActionTable->second.bCurrentState)
					{
						callBindFunc = false;
						break;
					}

					auto iterMouseActionTable = m_MouseActionTable.find(modifier);
					if (iterMouseActionTable != m_MouseActionTable.end() && !iterMouseActionTable->second.bCurrentState)
					{
						callBindFunc = true;
						break;
					}
				}
			}

		/* Call binded functions when needed */
		if (callBindFunc)
		{
			for (auto& callBack : inputAction.Callbacks)
				callBack.second();
		}
	}
}