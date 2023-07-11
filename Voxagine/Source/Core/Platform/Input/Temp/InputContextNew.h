#pragma once

#include "InputBindingMap.h"
#include "PlayerController.h"
#include "MouseController.h"
#include "KeyboardController.h"
#include "GamePadController.h"

#include "MouseControllerInterface.h"
#include "KeyboardControllerInterface.h"
#include "InputBindingHandlerInterface.h"

#include <vector>

#define DEFAULT_INPUT_MAP_NAME "Default"

class WindowContext;

enum BindingMapType
{
	BMT_GLOBAL = 0,
	BMT_PLAYERCONTROLLER_1,
	BMT_PLAYERCONTROLLER_2,
	BMT_PLAYERCONTROLLER_3,
	BMT_PLAYERCONTROLLER_4,
	BMT_PLAYERCONTROLLER_5,
	BMT_PLAYERCONTROLLER_6,
	BMT_PLAYERCONTROLLER_7,
	BMT_PLAYERCONTROLLER_8,
	BMT_PLAYERCONTROLLERS,
};

class InputContextNew : 
	public MouseControllerInterface, public KeyboardControllerInterface,
	public InputBindingHandlerInterface
{
public:
	InputContextNew() {};
	~InputContextNew() {};

	void Initialize(WindowContext* pWindowContext, bool bInitDefaultMap = true);
	void Update();
	void Uninitialize();

	int GetMaxPlayerCount() const;
	PlayerController* ReceivePlayerController(int iPlayerID);

	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);

	void BindAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::vector<uint64_t>& bindActionHandles, BindingMapType inputBindingMapType, std::function<void()> bindFunc);
	void BindAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::vector<uint64_t>& bindActionHandles, BindingMapType inputBindingMapType, std::function<void()> bindFunc);
	
	bool UnBindAction(const std::vector<uint64_t>& bindActionHandle, std::vector<uint64_t>* unsolvedBindActionHandle = nullptr);
	bool UnBindAction(uint64_t bindActionHandle);

	void RegisterAxis(const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	void RegisterAxis(const std::string& bindingMapName, const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);

	uint64_t BindAxis(const std::string& axisName, std::function<void(float)> bindFunc);
	uint64_t BindAxis(const std::string& bindingMapName, const std::string& axisName, std::function<void(float)> bindFunc);
	bool UnBindAxis(uint64_t bindHandle);

	InputBindingAxisValue GetAxisValue(const std::string& axisName, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	InputBindingAxisValue GetAxisValue(const std::string& bindingMapName, const std::string& axisName, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);

	bool CreateBindingMap(const std::string& bindingMapName, bool bSetActiveMap = true, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	bool DestroyBindingMap(const std::string& bindingMapName, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	const InputBindingMapInformation* GetBindingMap(const std::string& bindingMapName);

	bool SetActiveBindingMap(const std::string& bindingMapName, BindingMapType inputBindingMapType = BindingMapType::BMT_PLAYERCONTROLLERS);
	const InputBindingMapInformation* GetActiveBindingMap(BindingMapType inputBindingMapType = BindingMapType::BMT_GLOBAL) const;

	void SetDefaultMapName(const std::string& defaultMapName);
	std::string GetDefaultMapName() const;

private:
	void UpdateHardwareControllers();
	void ProcessInputBindings();
	void ProcessInputBindingsPlayerController();

#ifdef EDITOR
	void UpdateIMGUI();
#endif

	virtual MouseController* GetMouse() override;
	virtual KeyboardController* GetKeyboard() override;

	MouseController* GetMouseController() override;
	KeyboardController* GetKeyBoardController() override;
	GamePadController* GetGamePadController() override;

private:
	WindowContext* m_WindowContext = nullptr;
	InputBindingMap m_InputBindingMap;
	std::string m_DefaultMapName = DEFAULT_INPUT_MAP_NAME;
	int m_iMaxPlayerCount = 1;

	std::vector<PlayerController> m_PlayerControllers;
	mutable std::vector<GamePadController> m_GamePadControllers;
	mutable MouseController m_MouseController;
	mutable KeyboardController m_KeyboardController;
};