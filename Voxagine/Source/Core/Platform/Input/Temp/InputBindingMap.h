#pragma once

#include "InputBindingAction.h"
#include "InputBindingAxis.h"

#include <unordered_map>
#include <string>

struct InputBindingMapInformation
{
	std::string Name;
	std::vector<InputBindingAction> Actions;
	std::vector<InputBindingAxis> Axises;
};

class InputBindingMap 
{
public:
	InputBindingMap() {};
	~InputBindingMap() {};

	bool CreateBindingMap(const std::string& bindingMapName, bool bSetActiveMap = true);
	bool DestroyBindingMap(const std::string& bindingMapName);
	const InputBindingMapInformation* GetBindingMap(const std::string& bindingMapName);

	bool SetActiveBindingMap(const std::string& bindingMapName);
	const InputBindingMapInformation* GetActiveBindingMap() const;

	void SetDefaultMapName(const std::string& defaultMapName);
	std::string GetDefaultMapName() const;

	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster);
	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers);

	uint64_t BindAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager);
	uint64_t BindAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager);
	bool UnBindAction(uint64_t bindActionHandle); 

	void RegisterAxis(const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);
	void RegisterAxis(const std::string& bindingMapName, const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);

	uint64_t BindAxis(const std::string& axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager);
	uint64_t BindAxis(const std::string& bindingMapName, const std::string& axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager);
	bool UnBindAxis(uint64_t bindAxisHandle);

	const InputBindingAxis* GetInputBindingAxis(const std::string& axisName);
	const InputBindingAxis* GetInputBindingAxis(const std::string& bindingMapName, const std::string& axisName);

protected:
	InputBindingMapInformation* FindInputBindingMap(const std::string& bindingMapName);
	InputBindingAction* FindInputBindingAction(InputBindingMapInformation* inputBindingMapInformation, const std::string& actionName);
	InputBindingAxis* FindInputBindingAxis(InputBindingMapInformation* inputBindingMapInformation, const std::string& axisName);
private:
	std::unordered_map<std::string, InputBindingMapInformation> m_InputBindingMap;
	InputBindingMapInformation* m_pActiveBindingLayer = nullptr;
	std::string m_DefaultMapName;
};