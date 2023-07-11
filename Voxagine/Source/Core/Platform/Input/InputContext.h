#pragma once
#include <vector>
#include <unordered_map>
#include "Core/Event.h"
#include <functional>

#include "Core/Math.h"

#define DEFAULT_INPUT_LAYER_NAME "Default"

enum InputBinding
{
	IB_GamepadLeftThumbstickYAxis,
	IB_GamepadLeftThumbstickXAxis,
	IB_GamepadRightThumbstickYAxis,
	IB_GamepadRightThumbstickXAxis,

	IB_GamepadDPadUp,
	IB_GamepadDPadDown,
	IB_GamepadDPadRight,
	IB_GamepadDPadLeft,

	IB_GamepadButtonUp,
	IB_GamepadButtonDown,
	IB_GamepadButtonRight,
	IB_GamepadButtonLeft,

	IB_GamepadLeftTrigger,
	IB_GamepadRightTrigger,
	IB_GamepadLeftShoulder,
	IB_GamepadRightShoulder,

	IB_GamepadMenu,
	IB_GamepadStart,
	IB_GamepadView,

	IB_MouseXAxis,
	IB_MouseYAxis,
	IB_MouseButtonLeft,
	IB_MouseButtonRight,
	IB_MouseScrollWheel,

	IB_None,
	IB_Back,
	IB_Tab,
	IB_Enter,
	IB_Pause,
	IB_CapsLock,
	IB_Escape,
	IB_Space,
	IB_PageUp,
	IB_PageDown,
	IB_End,
	IB_Home,
	IB_Left,
	IB_Up,
	IB_Right,
	IB_Down,
	IB_Select,
	IB_Print,
	IB_Execute,
	IB_PrintScreen,
	IB_Insert,
	IB_Delete,
	IB_Help,
	IB_D0,
	IB_D1,
	IB_D2,
	IB_D3,
	IB_D4,
	IB_D5,
	IB_D6,
	IB_D7,
	IB_D8,
	IB_D9,

	IB_A,
	IB_B,
	IB_C,
	IB_D,
	IB_E,
	IB_F,
	IB_G,
	IB_H,
	IB_I,
	IB_J,
	IB_K,
	IB_L,
	IB_M,
	IB_N,
	IB_O,
	IB_P,
	IB_Q,
	IB_R,
	IB_S,
	IB_T,
	IB_U,
	IB_V,
	IB_W,
	IB_X,
	IB_Y,
	IB_Z,
	IB_LeftWindows,
	IB_RightWindows,
	IB_Apps,

	IB_NumPad0,
	IB_NumPad1,
	IB_NumPad2,
	IB_NumPad3,
	IB_NumPad4,
	IB_NumPad5,
	IB_NumPad6,
	IB_NumPad7,
	IB_NumPad8,
	IB_NumPad9,
	IB_Multiply,
	IB_Add,
	IB_Separator,
	IB_Subtract,

	IB_Decimal,
	IB_Divide,
	IB_F1,
	IB_F2,
	IB_F3,
	IB_F4,
	IB_F5,
	IB_F6,
	IB_F7,
	IB_F8,
	IB_F9,
	IB_F10,
	IB_F11,
	IB_F12,
	IB_F13,
	IB_F14,
	IB_F15,
	IB_F16,
	IB_F17,
	IB_F18,
	IB_F19,
	IB_F20,
	IB_F21,
	IB_F22,
	IB_F23,
	IB_F24,

	IB_NumLock,
	IB_Scroll,

	IB_LeftShift,
	IB_RightShift,
	IB_LeftControl,
	IB_RightControl,
	IB_LeftAlt,
	IB_RightAlt,

	IB_BindCount
};

enum InputEvent
{
	IE_PRESSED,
	IE_RELEASED,
	IE_REPEAT
};

struct AxisBinding
{
	AxisBinding(InputBinding type, float scalar) : Type(type), fScalar(scalar) {}
	InputBinding Type;
	float fScalar;
};

struct InputAction
{
	std::vector<InputBinding> InputModifiers;
	InputBinding InputMaster;
	std::string Name;
	InputEvent Type;
	InputEvent CheckEvent;
	std::unordered_map<uint64_t, std::function<void()>> Callbacks;
};

struct InputAxis
{
	std::vector<AxisBinding> Bindings;
	std::string Name;
	float Value;
	std::unordered_map<uint64_t, std::function<void(float)>> Callbacks;
};

struct BindingLayer
{
	std::string LayerName;
	std::vector<InputAction> BindingActions;
	std::vector<InputAxis> BindingAxis;
};

class Platform;
class LoggingSystem;
class InputContext
{
public:
	InputContext(Platform* pPlatform);
	virtual ~InputContext() {}

	Event<InputAction&> ActionAdded;
	Event<InputAxis&> AxisAdded;

	virtual void Initialize();
	virtual void Update() = 0;

	bool CreateLayer(bool bActive = true);
	bool CreateLayer(const std::string& layerName, bool bActive = false);
	bool SetActiveLayer(const std::string& layerName);
	std::string GetActiveLayerName() const;

	void RegisterAction(const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingMaster);
	void RegisterAction(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, InputBinding bindingMaster);
	void RegisterAction(const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster);
	void RegisterAction(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster);

	void RegisterAxis(const std::string& actionName, std::vector<AxisBinding> bindings);
	void RegisterAxis(const std::string& layerName, const std::string& actionName, std::vector<AxisBinding> bindings);

	uint64_t BindAction(const std::string& actionName, InputEvent inputEvent, std::function<void()> bindFunc);
	uint64_t BindAction(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, std::function<void()> bindFunc);
	uint64_t BindAxis(const std::string& axisName, std::function<void(float)> bindFunc);
	uint64_t BindAxis(const std::string& layerName, const std::string& axisName, std::function<void(float)> bindFunc);

	void ReAssignAction(const std::string& actionName, InputEvent inputEvent, InputBinding bindingMaster, InputEvent newInputEvent, InputBinding newBindingMaster);
	void ReAssignAction(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, InputBinding bindingMaster, InputEvent newInputEvent, InputBinding newBindingMaster);
	void ReAssignAction(const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster, InputEvent newInputEvent, std::vector<InputBinding> newBindingModifiers, InputBinding newBindingMaster);
	void ReAssignAction(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster, InputEvent newInputEvent, std::vector<InputBinding> newBindingModifiers, InputBinding newBindingMaster);

	void UnBindAction(const uint64_t& uiHandle);
	void UnBindAxis(const uint64_t& uiHandle);

	const InputAxis* GetAxis(const std::string& axisName);
	const InputAxis* GetAxis(const std::string& layerName, const std::string& axisName);

	UVector2 GetMousePosition() const { return m_MousePosition; };
	Vector2 GetMouseDelta() const { return m_MouseDelta; };

	float GetMouseWheelValue() const { return m_MouseWheelValue; };
	float GetMouseWheelDelta() const { return m_MouseWheelDelta; };

	bool IsDown(InputBinding binding) const { return m_Keys[binding]; };


protected:
	BindingLayer* GetActiveLayer() const;
	BindingLayer* GetBindingLayer(std::string layerName);

	InputAction* FindInputActionBinding(const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster);
	InputAction* FindInputActionBinding(const std::string& layerName, const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster);

protected:
	Platform* m_pPlatform = nullptr;
	std::vector<BindingLayer> m_Layers;
	BindingLayer* m_pActiveLayer;

	UVector2 m_MousePosition = { 0, 0 };
	Vector2 m_MouseDelta = Vector2(0.f);

	float m_MouseWheelValue = 0.0f;
	float m_MouseWheelDelta = 0.0f;

	bool m_Keys[IB_BindCount];

private:
	static uint64_t INPUT_HANDLE_COUNTER;
	LoggingSystem* m_pLogger;
};