#pragma once

/* SDL3-backed replacement for DirectXTK12's DirectX::Keyboard.
 *
 * State keeps DirectXTK's field names so KeyboardController's 109 key mappings
 * port without being touched, which is exactly where a hand rewrite would
 * silently swap two keys. */
class Keyboard
{
public:
	struct State
	{
		bool A = false;
		bool Add = false;
		bool Apps = false;
		bool B = false;
		bool Back = false;
		bool C = false;
		bool CapsLock = false;
		bool D = false;
		bool D0 = false;
		bool D1 = false;
		bool D2 = false;
		bool D3 = false;
		bool D4 = false;
		bool D5 = false;
		bool D6 = false;
		bool D7 = false;
		bool D8 = false;
		bool D9 = false;
		bool Decimal = false;
		bool Delete = false;
		bool Divide = false;
		bool Down = false;
		bool E = false;
		bool End = false;
		bool Enter = false;
		bool Escape = false;
		bool Execute = false;
		bool F = false;
		bool F1 = false;
		bool F10 = false;
		bool F11 = false;
		bool F12 = false;
		bool F13 = false;
		bool F14 = false;
		bool F15 = false;
		bool F16 = false;
		bool F17 = false;
		bool F18 = false;
		bool F19 = false;
		bool F2 = false;
		bool F20 = false;
		bool F21 = false;
		bool F22 = false;
		bool F23 = false;
		bool F24 = false;
		bool F3 = false;
		bool F4 = false;
		bool F5 = false;
		bool F6 = false;
		bool F7 = false;
		bool F8 = false;
		bool F9 = false;
		bool G = false;
		bool H = false;
		bool Help = false;
		bool Home = false;
		bool I = false;
		bool Insert = false;
		bool J = false;
		bool K = false;
		bool L = false;
		bool Left = false;
		bool LeftAlt = false;
		bool LeftControl = false;
		bool LeftShift = false;
		bool LeftWindows = false;
		bool M = false;
		bool Multiply = false;
		bool N = false;
		bool NumLock = false;
		bool NumPad0 = false;
		bool NumPad1 = false;
		bool NumPad2 = false;
		bool NumPad3 = false;
		bool NumPad4 = false;
		bool NumPad5 = false;
		bool NumPad6 = false;
		bool NumPad7 = false;
		bool NumPad8 = false;
		bool NumPad9 = false;
		bool O = false;
		bool P = false;
		bool PageDown = false;
		bool PageUp = false;
		bool Pause = false;
		bool Print = false;
		bool PrintScreen = false;
		bool Q = false;
		bool R = false;
		bool Right = false;
		bool RightAlt = false;
		bool RightControl = false;
		bool RightShift = false;
		bool RightWindows = false;
		bool S = false;
		bool Scroll = false;
		bool Select = false;
		bool Separator = false;
		bool Space = false;
		bool Subtract = false;
		bool T = false;
		bool Tab = false;
		bool U = false;
		bool Up = false;
		bool V = false;
		bool W = false;
		bool X = false;
		bool Y = false;
		bool Z = false;
	};

	Keyboard();
	~Keyboard();

	Keyboard(const Keyboard&) = delete;
	Keyboard& operator=(const Keyboard&) = delete;

	static Keyboard& Get();

	State GetState() const;
	bool IsConnected() const { return true; }

private:
	static Keyboard* s_pInstance;
};
