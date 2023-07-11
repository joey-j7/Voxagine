#include "pch.h"

#include "InputContextNew.h"
#include "InputKeyIdentifiers.h"
#include "Core/Platform/Window/WindowContext.h"
#include "External/optick/optick.h"

#include "Core/Platform/Platform.h"
#include "Core/Application.h"

#include <algorithm>

#ifdef EDITOR
#include <External/imgui/imgui.h>
#endif

#define MAX_PLAYERS 2

void InputContextNew::Initialize(WindowContext* pWindowContext, bool bInitDefaultMap)
{
	m_WindowContext = pWindowContext;
	m_DefaultMapName = DEFAULT_INPUT_MAP_NAME;
	m_iMaxPlayerCount = MAX_PLAYERS;

	if (bInitDefaultMap)
	{
		m_InputBindingMap.SetDefaultMapName(GetDefaultMapName());
		m_InputBindingMap.CreateBindingMap(GetDefaultMapName(), true);
	}

	m_MouseController.Initialize(pWindowContext);
	m_KeyboardController.Initialize(pWindowContext);

	m_GamePadControllers.resize(m_iMaxPlayerCount);
	for (GamePadController& gamePadIt : m_GamePadControllers)
		gamePadIt.Initialize(pWindowContext);

	m_PlayerControllers.resize(m_iMaxPlayerCount);

	m_PlayerControllers[0].SetPlayerMouse(&m_MouseController);
	m_PlayerControllers[0].SetPlayerKeyboard(&m_KeyboardController);

	// If we want player controller 1 to be connected to player 1
	//m_PlayerControllers[0].SetPlayerGamepad(&m_GamePadControllers[0]);
	// else if we want the last controller to be player 1 (Player1 = Keyboard and Player2 = GameController)
	m_PlayerControllers[0].SetPlayerGamepad(&m_GamePadControllers[m_iMaxPlayerCount - 1]);

	m_PlayerControllers[0].SetDefaultMapName(GetDefaultMapName());

	if (bInitDefaultMap)
		m_PlayerControllers[0].CreateBindingMap(GetDefaultMapName(), true);

	for (unsigned playerControllerIndex = 1; playerControllerIndex != m_iMaxPlayerCount; ++playerControllerIndex)
	{
		m_PlayerControllers[playerControllerIndex].SetPlayerMouse(nullptr);
		m_PlayerControllers[playerControllerIndex].SetPlayerKeyboard(nullptr);

		// If we want player controller 1 to be connected to player 1
		//m_PlayerControllers[playerControllerIndex].SetPlayerGamepad(&m_GamePadControllers[playerControllerIndex]);
		// else if we want the last controller to be player 1 (Player1 = Keyboard and Player2 = GameController)
		m_PlayerControllers[playerControllerIndex].SetPlayerGamepad(&m_GamePadControllers[playerControllerIndex - 1]);

		m_PlayerControllers[playerControllerIndex].SetDefaultMapName(GetDefaultMapName());

		if (bInitDefaultMap)
			m_PlayerControllers[playerControllerIndex].CreateBindingMap(GetDefaultMapName(), true);
	}
}

void InputContextNew::Update()
{
	OPTICK_CATEGORY("Input", Optick::Category::Input);
	OPTICK_EVENT();
	UpdateHardwareControllers();
	ProcessInputBindings();
	ProcessInputBindingsPlayerController();

#ifdef EDITOR
	UpdateIMGUI();
#endif

}

void InputContextNew::Uninitialize()
{
	m_MouseController.UnInitialize();
	m_KeyboardController.UnInitialize();

	for (GamePadController& gamePadIt : m_GamePadControllers)
		gamePadIt.UnInitialize();
}

int InputContextNew::GetMaxPlayerCount() const
{
	return m_iMaxPlayerCount;
}

PlayerController * InputContextNew::ReceivePlayerController(int iPlayerID)
{
	if (iPlayerID >= 1 && iPlayerID <= MAX_PLAYERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			if (playerControllerIt.GetPlayerID() == iPlayerID)
				return &playerControllerIt;
		}
	}

	return nullptr;
}

void InputContextNew::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, BindingMapType inputBindingMapType)
{
	RegisterAction(GetDefaultMapName(), actionName, inputKeyEvent, inputKeyMaster, inputBindingMapType);
}

void InputContextNew::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, BindingMapType inputBindingMapType)
{
	RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, {} , inputBindingMapType);
}

void InputContextNew::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers, BindingMapType inputBindingMapType)
{
	RegisterAction(GetDefaultMapName(), actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers, inputBindingMapType);
}

void InputContextNew::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_GLOBAL)
	{
		m_InputBindingMap.RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
		return;
	}
	
	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			playerControllerIt.RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
		}
		return;
	}

	unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
	m_PlayerControllers[playerControllerIndex].RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
}

void InputContextNew::BindAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::vector<uint64_t>& bindActionHandles, BindingMapType inputBindingMapType, std::function<void()> bindFunc)
{
	return BindAction(GetDefaultMapName(), actionName, inputKeyEvent, bindActionHandles, inputBindingMapType, bindFunc);
}

void InputContextNew::BindAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::vector<uint64_t>& bindActionHandles, BindingMapType inputBindingMapType, std::function<void()> bindFunc)
{
	WorldManager* pWorldManager = &m_WindowContext->GetPlatform()->GetApplication()->GetWorldManager();
	if (inputBindingMapType == BMT_GLOBAL)
	{
		bindActionHandles.push_back(m_InputBindingMap.BindAction(bindingMapName, actionName, inputKeyEvent, bindFunc, pWorldManager));
		return;
	}

	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
			bindActionHandles.push_back(playerControllerIt.BindAction(bindingMapName, actionName, inputKeyEvent, bindFunc, pWorldManager));

		return;
	}

	if (inputBindingMapType != BMT_PLAYERCONTROLLERS && inputBindingMapType != BMT_GLOBAL)
	{
		unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
		bindActionHandles.push_back(m_PlayerControllers[playerControllerIndex].BindAction(bindingMapName, actionName, inputKeyEvent, bindFunc, pWorldManager));
		return;
	}
}

bool InputContextNew::UnBindAction(const std::vector<uint64_t>& bindActionHandle, std::vector<uint64_t>* unsolvedBindActionHandle)
{
	bool succeed = true;

	for (uint64_t bindhandle : bindActionHandle)
	{
		if (!UnBindAction(bindhandle))
		{
			if (unsolvedBindActionHandle != nullptr)
				unsolvedBindActionHandle->push_back(bindhandle);

			succeed = false;
		}
	}

	return succeed;
}

bool InputContextNew::UnBindAction(uint64_t bindActionHandle)
{
	if (m_InputBindingMap.UnBindAction(bindActionHandle))
		return true;

	for (PlayerController& playerControllerIt : m_PlayerControllers)
	{
		if (playerControllerIt.UnBindAction(bindActionHandle))
			return true;
	}

	return false;
}

void InputContextNew::UpdateHardwareControllers()
{
	m_MouseController.Update();
	m_KeyboardController.Update();

	for (GamePadController& gamePadControllerIt : m_GamePadControllers)
	{
		gamePadControllerIt.Update();
	}
}

void InputContextNew::ProcessInputBindings()
{
	const InputBindingMapInformation* inputBindingMapInformation = m_InputBindingMap.GetActiveBindingMap();
	if (inputBindingMapInformation == nullptr)
		return;

	ProcessBindingMapAction(*inputBindingMapInformation);
}

void InputContextNew::ProcessInputBindingsPlayerController()
{
	for (PlayerController& playerControllerIt : m_PlayerControllers)
	{
		playerControllerIt.Update();
	}
}

#ifdef EDITOR
void InputContextNew::UpdateIMGUI()
{
	ImGuiIO& io = ImGui::GetIO();

	io.KeyMap[13] = IK_ENTER;

	io.KeysDown[IK_NONE] = m_KeyboardController.IsKeyDown(IK_NONE);
	io.KeysDown[IK_BACK] = m_KeyboardController.IsKeyDown(IK_BACK);
	io.KeysDown[IK_TAB] = m_KeyboardController.IsKeyDown(IK_TAB);
	io.KeysDown[IK_ENTER] = m_KeyboardController.IsKeyDown(IK_ENTER);
	io.KeysDown[IK_PAUSE] = m_KeyboardController.IsKeyDown(IK_PAUSE);
	io.KeysDown[IK_CAPSLOCK] = m_KeyboardController.IsKeyDown(IK_CAPSLOCK);
	io.KeysDown[IK_ESCAPE] = m_KeyboardController.IsKeyDown(IK_ESCAPE);
	io.KeysDown[IK_SPACE] = m_KeyboardController.IsKeyDown(IK_SPACE);
	io.KeysDown[IK_PAGEUP] = m_KeyboardController.IsKeyDown(IK_PAGEUP);
	io.KeysDown[IK_PAGEDOWN] = m_KeyboardController.IsKeyDown(IK_PAGEDOWN);
	io.KeysDown[IK_END] = m_KeyboardController.IsKeyDown(IK_END);
	io.KeysDown[IK_HOME] = m_KeyboardController.IsKeyDown(IK_HOME);
	io.KeysDown[IK_LEFT] = m_KeyboardController.IsKeyDown(IK_LEFT);
	io.KeysDown[IK_UP] = m_KeyboardController.IsKeyDown(IK_UP);
	io.KeysDown[IK_RIGHT] = m_KeyboardController.IsKeyDown(IK_RIGHT);
	io.KeysDown[IK_DOWN] = m_KeyboardController.IsKeyDown(IK_DOWN);
	io.KeysDown[IK_SELECT] = m_KeyboardController.IsKeyDown(IK_SELECT);
	io.KeysDown[IK_PRINT] = m_KeyboardController.IsKeyDown(IK_PRINT);
	io.KeysDown[IK_EXECUTE] = m_KeyboardController.IsKeyDown(IK_EXECUTE);
	io.KeysDown[IK_PRINTSCREEN] = m_KeyboardController.IsKeyDown(IK_PRINTSCREEN);
	io.KeysDown[IK_INSERT] = m_KeyboardController.IsKeyDown(IK_INSERT);
	io.KeysDown[IK_DELETE] = m_KeyboardController.IsKeyDown(IK_DELETE);
	io.KeysDown[IK_HELP] = m_KeyboardController.IsKeyDown(IK_HELP);
	io.KeysDown[IK_D0] = m_KeyboardController.IsKeyDown(IK_D0);
	io.KeysDown[IK_D1] = m_KeyboardController.IsKeyDown(IK_D1);
	io.KeysDown[IK_D2] = m_KeyboardController.IsKeyDown(IK_D2);
	io.KeysDown[IK_D3] = m_KeyboardController.IsKeyDown(IK_D3);
	io.KeysDown[IK_D4] = m_KeyboardController.IsKeyDown(IK_D4);
	io.KeysDown[IK_D5] = m_KeyboardController.IsKeyDown(IK_D5);
	io.KeysDown[IK_D6] = m_KeyboardController.IsKeyDown(IK_D6);
	io.KeysDown[IK_D7] = m_KeyboardController.IsKeyDown(IK_D7);
	io.KeysDown[IK_D8] = m_KeyboardController.IsKeyDown(IK_D8);
	io.KeysDown[IK_D9] = m_KeyboardController.IsKeyDown(IK_D9);

	io.KeysDown[IK_A] = m_KeyboardController.IsKeyDown(IK_A);
	io.KeysDown[IK_B] = m_KeyboardController.IsKeyDown(IK_B);
	io.KeysDown[IK_C] = m_KeyboardController.IsKeyDown(IK_C);
	io.KeysDown[IK_D] = m_KeyboardController.IsKeyDown(IK_D);
	io.KeysDown[IK_E] = m_KeyboardController.IsKeyDown(IK_E);
	io.KeysDown[IK_F] = m_KeyboardController.IsKeyDown(IK_F);
	io.KeysDown[IK_G] = m_KeyboardController.IsKeyDown(IK_G);
	io.KeysDown[IK_H] = m_KeyboardController.IsKeyDown(IK_H);
	io.KeysDown[IK_I] = m_KeyboardController.IsKeyDown(IK_I);
	io.KeysDown[IK_J] = m_KeyboardController.IsKeyDown(IK_J);
	io.KeysDown[IK_K] = m_KeyboardController.IsKeyDown(IK_K);
	io.KeysDown[IK_L] = m_KeyboardController.IsKeyDown(IK_L);
	io.KeysDown[IK_M] = m_KeyboardController.IsKeyDown(IK_M);
	io.KeysDown[IK_N] = m_KeyboardController.IsKeyDown(IK_N);
	io.KeysDown[IK_O] = m_KeyboardController.IsKeyDown(IK_O);
	io.KeysDown[IK_P] = m_KeyboardController.IsKeyDown(IK_P);
	io.KeysDown[IK_Q] = m_KeyboardController.IsKeyDown(IK_Q);
	io.KeysDown[IK_R] = m_KeyboardController.IsKeyDown(IK_R);
	io.KeysDown[IK_S] = m_KeyboardController.IsKeyDown(IK_S);
	io.KeysDown[IK_T] = m_KeyboardController.IsKeyDown(IK_T);
	io.KeysDown[IK_U] = m_KeyboardController.IsKeyDown(IK_U);
	io.KeysDown[IK_V] = m_KeyboardController.IsKeyDown(IK_V);
	io.KeysDown[IK_W] = m_KeyboardController.IsKeyDown(IK_W);
	io.KeysDown[IK_X] = m_KeyboardController.IsKeyDown(IK_X);
	io.KeysDown[IK_Y] = m_KeyboardController.IsKeyDown(IK_Y);
	io.KeysDown[IK_Z] = m_KeyboardController.IsKeyDown(IK_Z);
	io.KeysDown[IK_LEFTWINDOWS] = m_KeyboardController.IsKeyDown(IK_LEFTWINDOWS);
	io.KeysDown[IK_RIGHTWINDOWS] = m_KeyboardController.IsKeyDown(IK_RIGHTWINDOWS);
	io.KeysDown[IK_APPS] = m_KeyboardController.IsKeyDown(IK_APPS);

	io.KeysDown[IK_NUMPAD0] = m_KeyboardController.IsKeyDown(IK_NUMPAD0);
	io.KeysDown[IK_NUMPAD1] = m_KeyboardController.IsKeyDown(IK_NUMPAD1);
	io.KeysDown[IK_NUMPAD2] = m_KeyboardController.IsKeyDown(IK_NUMPAD2);
	io.KeysDown[IK_NUMPAD3] = m_KeyboardController.IsKeyDown(IK_NUMPAD3);
	io.KeysDown[IK_NUMPAD4] = m_KeyboardController.IsKeyDown(IK_NUMPAD4);
	io.KeysDown[IK_NUMPAD5] = m_KeyboardController.IsKeyDown(IK_NUMPAD5);
	io.KeysDown[IK_NUMPAD6] = m_KeyboardController.IsKeyDown(IK_NUMPAD6);
	io.KeysDown[IK_NUMPAD7] = m_KeyboardController.IsKeyDown(IK_NUMPAD7);
	io.KeysDown[IK_NUMPAD8] = m_KeyboardController.IsKeyDown(IK_NUMPAD8);
	io.KeysDown[IK_NUMPAD9] = m_KeyboardController.IsKeyDown(IK_NUMPAD9);
	io.KeysDown[IK_MULTIPLY] = m_KeyboardController.IsKeyDown(IK_MULTIPLY);
	io.KeysDown[IK_ADD] = m_KeyboardController.IsKeyDown(IK_ADD);
	io.KeysDown[IK_SEPARATOR] = m_KeyboardController.IsKeyDown(IK_SEPARATOR);
	io.KeysDown[IK_SUBTRACT] = m_KeyboardController.IsKeyDown(IK_SUBTRACT);

	io.KeysDown[IK_DECIMAL] = m_KeyboardController.IsKeyDown(IK_DECIMAL);
	io.KeysDown[IK_DIVIDE] = m_KeyboardController.IsKeyDown(IK_DIVIDE);
	io.KeysDown[IK_F1] = m_KeyboardController.IsKeyDown(IK_F1);
	io.KeysDown[IK_F2] = m_KeyboardController.IsKeyDown(IK_F2);
	io.KeysDown[IK_F3] = m_KeyboardController.IsKeyDown(IK_F3);
	io.KeysDown[IK_F4] = m_KeyboardController.IsKeyDown(IK_F4);
	io.KeysDown[IK_F5] = m_KeyboardController.IsKeyDown(IK_F5);
	io.KeysDown[IK_F6] = m_KeyboardController.IsKeyDown(IK_F6);
	io.KeysDown[IK_F7] = m_KeyboardController.IsKeyDown(IK_F7);
	io.KeysDown[IK_F8] = m_KeyboardController.IsKeyDown(IK_F8);
	io.KeysDown[IK_F9] = m_KeyboardController.IsKeyDown(IK_F9);
	io.KeysDown[IK_F10] = m_KeyboardController.IsKeyDown(IK_F10);
	io.KeysDown[IK_F11] = m_KeyboardController.IsKeyDown(IK_F11);
	io.KeysDown[IK_F12] = m_KeyboardController.IsKeyDown(IK_F12);
	io.KeysDown[IK_F13] = m_KeyboardController.IsKeyDown(IK_F13);
	io.KeysDown[IK_F14] = m_KeyboardController.IsKeyDown(IK_F14);
	io.KeysDown[IK_F15] = m_KeyboardController.IsKeyDown(IK_F15);
	io.KeysDown[IK_F16] = m_KeyboardController.IsKeyDown(IK_F16);
	io.KeysDown[IK_F17] = m_KeyboardController.IsKeyDown(IK_F17);
	io.KeysDown[IK_F18] = m_KeyboardController.IsKeyDown(IK_F18);
	io.KeysDown[IK_F19] = m_KeyboardController.IsKeyDown(IK_F19);
	io.KeysDown[IK_F20] = m_KeyboardController.IsKeyDown(IK_F20);
	io.KeysDown[IK_F21] = m_KeyboardController.IsKeyDown(IK_F21);
	io.KeysDown[IK_F22] = m_KeyboardController.IsKeyDown(IK_F22);
	io.KeysDown[IK_F23] = m_KeyboardController.IsKeyDown(IK_F23);
	io.KeysDown[IK_F24] = m_KeyboardController.IsKeyDown(IK_F24);

	io.KeysDown[IK_NUMLOCK] = m_KeyboardController.IsKeyDown(IK_NUMLOCK);
	io.KeysDown[IK_SCROLL] = m_KeyboardController.IsKeyDown(IK_SCROLL);

	io.KeysDown[IK_LEFTSHIFT] = m_KeyboardController.IsKeyDown(IK_LEFTSHIFT);
	io.KeysDown[IK_RIGHTSHIFT] = m_KeyboardController.IsKeyDown(IK_RIGHTSHIFT);
	io.KeysDown[IK_LEFTCONTROL] = m_KeyboardController.IsKeyDown(IK_LEFTCONTROL);
	io.KeysDown[IK_RIGHTCONTROL] = m_KeyboardController.IsKeyDown(IK_RIGHTCONTROL);
	io.KeysDown[IK_LEFTALT] = m_KeyboardController.IsKeyDown(IK_LEFTALT);
	io.KeysDown[IK_RIGHTALT] = m_KeyboardController.IsKeyDown(IK_RIGHTALT);

	io.KeyAlt = io.KeysDown[IK_LEFTALT] || io.KeysDown[IK_RIGHTALT];
	io.KeyCtrl = io.KeysDown[IK_LEFTCONTROL] || io.KeysDown[IK_RIGHTCONTROL];
	io.KeyShift = io.KeysDown[IK_LEFTSHIFT] || io.KeysDown[IK_RIGHTSHIFT];
	io.KeySuper = io.KeysDown[IK_LEFTWINDOWS] || io.KeysDown[IK_RIGHTWINDOWS];

	io.MousePos = ImVec2(static_cast<float>(m_MouseController.GetAxisValue(IK_MOUSEAXISX).Value), m_MouseController.GetAxisValue(IK_MOUSEAXISY).Value);

	io.MouseDown[0] = m_MouseController.IsKeyDown(IK_MOUSEBUTTONLEFT);
	io.MouseDown[1] = m_MouseController.IsKeyDown(IK_MOUSEBUTTONRIGHT);
	io.MouseDown[2] = m_MouseController.IsKeyDown(IK_MOUSEBUTTONMIDDLE);

	float fMouseWheelDelta = m_MouseController.GetAxisValue(IK_MOUSEWHEELAXISDELTA).Value;
	io.MouseWheel = (fMouseWheelDelta != 0.0f) ? static_cast<float>(fMouseWheelDelta / abs(fMouseWheelDelta)) * 0.2f : 0.0f;
}
#endif

MouseController * InputContextNew::GetMouse()
{
	return GetMouseController();
}
KeyboardController * InputContextNew::GetKeyboard()
{
	return GetKeyBoardController();
}

MouseController * InputContextNew::GetMouseController()
{
	return &m_MouseController;
}

KeyboardController * InputContextNew::GetKeyBoardController()
{
	return &m_KeyboardController;
}

GamePadController * InputContextNew::GetGamePadController()
{
	return &m_GamePadControllers[0];
}

void InputContextNew::RegisterAxis(const std::string & axisName, const InputKey & masterKey, float fScalar, BindingMapType inputBindingMapType)
{
	RegisterAxis(GetDefaultMapName(), axisName, masterKey, fScalar, inputBindingMapType);
}

void InputContextNew::RegisterAxis(const std::string & bindingMapName, const std::string & axisName, const InputKey & masterKey, float fScalar, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_GLOBAL)
	{
		m_InputBindingMap.RegisterAxis(bindingMapName, axisName, masterKey, fScalar);
		return;
	}

	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			playerControllerIt.RegisterAxis(bindingMapName, axisName, masterKey, fScalar);
		}
		return;
	}

	unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
	m_PlayerControllers[playerControllerIndex].RegisterAxis(bindingMapName, axisName, masterKey, fScalar);
}

uint64_t InputContextNew::BindAxis(const std::string & axisName, std::function<void(float)> bindFunc)
{
	return BindAxis(GetDefaultMapName(), axisName, bindFunc);
}

uint64_t InputContextNew::BindAxis(const std::string & bindingMapName, const std::string & axisName, std::function<void(float)> bindFunc)
{
	WorldManager* pWorldManager = &m_WindowContext->GetPlatform()->GetApplication()->GetWorldManager();
	return m_InputBindingMap.BindAxis(bindingMapName, axisName, bindFunc, pWorldManager);
}

bool InputContextNew::UnBindAxis(uint64_t bindHandle)
{
	return m_InputBindingMap.UnBindAxis(bindHandle);
}

InputBindingAxisValue InputContextNew::GetAxisValue(const std::string & axisName, BindingMapType inputBindingMapType)
{
	return GetAxisValue(GetActiveBindingMap()->Name, axisName, inputBindingMapType);
}

InputBindingAxisValue InputContextNew::GetAxisValue(const std::string & bindingMapName, const std::string & axisName, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_GLOBAL)
	{
		const InputBindingAxis* pCurrentInputBindingAxis = m_InputBindingMap.GetInputBindingAxis(bindingMapName, axisName);
		return (pCurrentInputBindingAxis != nullptr) ? ProcessBindingAxis(*pCurrentInputBindingAxis) : InputBindingAxisValue({ false, 0.f });
	}

	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		InputBindingAxisValue innputBindingAxisValues = InputBindingAxisValue({ false, 0.f });

		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			const InputBindingAxis* pCurrentInputBindingAxis = playerControllerIt.GetInputBindingAxis(bindingMapName, axisName);

			if (pCurrentInputBindingAxis != nullptr)
			{
				InputBindingAxisValue tempInputBindingAxisValue = playerControllerIt.ProcessBindingAxis(*pCurrentInputBindingAxis);
				if (tempInputBindingAxisValue.Proccessed == true)
				{
					innputBindingAxisValues.Proccessed = true;
					innputBindingAxisValues.Value += tempInputBindingAxisValue.Value;
				}
			}
		}

		if (innputBindingAxisValues.Value > 1.f)
			innputBindingAxisValues.Value = 1.f;
		else if (innputBindingAxisValues.Value < -1.f)
			innputBindingAxisValues.Value = -1.f;

		return (innputBindingAxisValues.Proccessed) ? innputBindingAxisValues : InputBindingAxisValue({ false, 0.f });
	}

	if (inputBindingMapType != BMT_PLAYERCONTROLLERS && inputBindingMapType != BMT_GLOBAL)
	{
		unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
		const InputBindingAxis* pCurrentInputBindingAxis = m_PlayerControllers[playerControllerIndex].GetInputBindingAxis(bindingMapName, axisName);

		return (pCurrentInputBindingAxis != nullptr) ? ProcessBindingAxis(*pCurrentInputBindingAxis) : InputBindingAxisValue({ false, 0.f });
	}
}

bool InputContextNew::CreateBindingMap(const std::string & bindingMapName, bool bSetActiveMap, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_GLOBAL)
		return m_InputBindingMap.CreateBindingMap(bindingMapName, bSetActiveMap);

	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			playerControllerIt.CreateBindingMap(bindingMapName, bSetActiveMap);
		}
		return true;
	}

	unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
	return m_PlayerControllers[playerControllerIndex].CreateBindingMap(bindingMapName, bSetActiveMap);
}

bool InputContextNew::DestroyBindingMap(const std::string & bindingMapName, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_GLOBAL)
		return m_InputBindingMap.DestroyBindingMap(bindingMapName);

	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			playerControllerIt.DestroyBindingMap(bindingMapName);
		}
		return true;
	}

	unsigned playerControllerIndex = BMT_PLAYERCONTROLLERS;
	return m_PlayerControllers[playerControllerIndex].DestroyBindingMap(bindingMapName);
}

const InputBindingMapInformation * InputContextNew::GetBindingMap(const std::string & bindingMapName)
{
	return m_InputBindingMap.GetBindingMap(bindingMapName);
}

bool InputContextNew::SetActiveBindingMap(const std::string & bindingMapName, BindingMapType inputBindingMapType)
{
	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		bool success = true;
		for (PlayerController& playerControllerIt : m_PlayerControllers)
		{
			if (!playerControllerIt.SetActiveBindingMap(bindingMapName))
				success = false;
		}
		return success;
	}

	return m_InputBindingMap.SetActiveBindingMap(bindingMapName);
}

const InputBindingMapInformation * InputContextNew::GetActiveBindingMap(BindingMapType inputBindingMapType) const
{
	if (inputBindingMapType == BMT_PLAYERCONTROLLERS)
	{
		if (m_PlayerControllers.size() > 0)
			return m_PlayerControllers[0].GetActiveBindingMap();
	}

	return m_InputBindingMap.GetActiveBindingMap();
}

void InputContextNew::SetDefaultMapName(const std::string & defaultMapName)
{
	m_InputBindingMap.SetDefaultMapName(defaultMapName);
}

std::string InputContextNew::GetDefaultMapName() const
{
	return m_DefaultMapName;
}
