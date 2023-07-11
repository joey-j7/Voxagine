#pragma once

#include <functional>
#include "InputKeyIdentifiers.h"

class InputBindingMap;
class InputBindingAxis;
class WorldManager;

class InputBindingMapInterface
{
public:
	InputBindingMapInterface() {};
	virtual ~InputBindingMapInterface() {};

	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster);
	void RegisterAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers);
	void RegisterAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, const InputKey& inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers);

	void RegisterAxis(const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);
	void RegisterAxis(const std::string& bindingMapName, const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);

	uint64_t BindAction(const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager);
	uint64_t BindAction(const std::string& bindingMapName, const std::string& actionName, const InputKeyStatus& inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager);
	bool UnBindAction(uint64_t bindActionHandle);

	uint64_t BindAxis(const std::string& axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager);
	uint64_t BindAxis(const std::string& bindingMapName, const std::string& axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager);
	bool UnBindAxis(uint64_t bindAxisHandle);

	const InputBindingAxis* GetInputBindingAxis(const std::string& axisName);
	const InputBindingAxis* GetInputBindingAxis(const std::string& bindingMapName, const std::string& axisName);

private:
	virtual InputBindingMap* GetInputBindingMap() = 0;
};