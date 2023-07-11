#pragma once

enum InputKey
{
	// Keyboard input keys
	IK_NONE,
	IK_BACK,
	IK_TAB,
	IK_ENTER,
	IK_PAUSE,
	IK_CAPSLOCK,
	IK_ESCAPE,
	IK_SPACE,
	IK_PAGEUP,
	IK_PAGEDOWN,
	IK_END,
	IK_HOME,
	IK_LEFT,
	IK_UP,
	IK_RIGHT,
	IK_DOWN,
	IK_SELECT,
	IK_PRINT,
	IK_EXECUTE,
	IK_PRINTSCREEN,
	IK_INSERT,
	IK_DELETE,
	IK_HELP,
	IK_D0,
	IK_D1,
	IK_D2,
	IK_D3,
	IK_D4,
	IK_D5,
	IK_D6,
	IK_D7,
	IK_D8,
	IK_D9,

	IK_A,
	IK_B,
	IK_C,
	IK_D,
	IK_E,
	IK_F,
	IK_G,
	IK_H,
	IK_I,
	IK_J,
	IK_K,
	IK_L,
	IK_M,
	IK_N,
	IK_O,
	IK_P,
	IK_Q,
	IK_R,
	IK_S,
	IK_T,
	IK_U,
	IK_V,
	IK_W,
	IK_X,
	IK_Y,
	IK_Z,
	IK_LEFTWINDOWS,
	IK_RIGHTWINDOWS,
	IK_APPS,

	IK_NUMPAD0,
	IK_NUMPAD1,
	IK_NUMPAD2,
	IK_NUMPAD3,
	IK_NUMPAD4,
	IK_NUMPAD5,
	IK_NUMPAD6,
	IK_NUMPAD7,
	IK_NUMPAD8,
	IK_NUMPAD9,
	IK_MULTIPLY,
	IK_ADD,
	IK_SEPARATOR,
	IK_SUBTRACT,

	IK_DECIMAL,
	IK_DIVIDE,
	IK_F1,
	IK_F2,
	IK_F3,
	IK_F4,
	IK_F5,
	IK_F6,
	IK_F7,
	IK_F8,
	IK_F9,
	IK_F10,
	IK_F11,
	IK_F12,
	IK_F13,
	IK_F14,
	IK_F15,
	IK_F16,
	IK_F17,
	IK_F18,
	IK_F19,
	IK_F20,
	IK_F21,
	IK_F22,
	IK_F23,
	IK_F24,

	IK_NUMLOCK,
	IK_SCROLL,

	IK_LEFTSHIFT,
	IK_RIGHTSHIFT,
	IK_LEFTCONTROL,
	IK_RIGHTCONTROL,
	IK_LEFTALT,
	IK_RIGHTALT,

	// Mouse input keys
	IK_MOUSEBUTTONLEFT,
	IK_MOUSEBUTTONMIDDLE,
	IK_MOUSEBUTTONRIGHT,
	IK_MOUSEBUTTONOPTIONAL,
	IK_MOUSEBUTTONOPTIONAL2,

	// Mouse input key for axises values
	IK_MOUSEAXISX,
	IK_MOUSEAXISY,
	IK_MOUSEWHEELAXIS,

	// Mouse input key for axises deltas
	IK_MOUSEAXISXDELTA,
	IK_MOUSEAXISYDELTA,
	IK_MOUSEWHEELAXISDELTA,

	// Gamepad input keys
	IK_GAMEPADRIGHTPADUP,
	IK_GAMEPADRIGHTPADRIGHT,
	IK_GAMEPADRIGHTPADDOWN,
	IK_GAMEPADRIGHTPADLEFT,
	IK_GAMEPADLEFTPADUP,
	IK_GAMEPADLEFTPADRIGHT,
	IK_GAMEPADLEFTPADDOWN,
	IK_GAMEPADLEFTPADLEFT,

	IK_GAMEPADRIGHTSTICK,
	IK_GAMEPADRIGHTSHOULDER,
	IK_GAMEPADRIGHTSHOULDER2,
	IK_GAMEPADLEFTSTICK,
	IK_GAMEPADLEFTSHOULDER,
	IK_GAMEPADLEFTSHOULDER2,

	IK_GAMEPADOPTION,
	IK_GAMEPADHOME,
	IK_GAMEPADSELECT,
	IK_GAMEPADTOUCH,

	// Gamepad input key for axises values
	IK_GAMEPADRIGHTSTICKAXISX,
	IK_GAMEPADRIGHTSTICKAXISY,
	IK_GAMEPADLEFTSTICKAXISX,
	IK_GAMEPADLEFTSTICKAXISY,
	IK_GAMEPADRIGHTSHOULDER2AXIS,
	IK_GAMEPADLEFTSHOULDER2AXIS,

	// Gamepad input key for axises deltas
	IK_GAMEPADRIGHTSTICKAXISXDELTA,
	IK_GAMEPADRIGHTSTICKAXISYDELTA,
	IK_GAMEPADLEFTSTICKAXISXDELTA,
	IK_GAMEPADLEFTSTICKAXISYDELTA,
	IK_GAMEPADRIGHTSHOULDER2AXISDELTA,
	IK_GAMEPADLEFTSHOULDER2AXISDELTA,
};

enum InputKeyStatus
{
	IKS_NONE = -1,
	IKS_PRESSED,
	IKS_HELD,
	IKS_RELEASED,
	IKS_COUNT,
};

struct InputKeyAxis
{
	InputKey MasterKey;
	float AxisScalar;
};