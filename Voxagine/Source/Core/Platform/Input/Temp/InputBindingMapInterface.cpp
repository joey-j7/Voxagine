#include "pch.h"
#include "InputBindingMapInterface.h"

#include "InputBindingMap.h"
#include "InputBindingAction.h"

void InputBindingMapInterface::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAction(actionName, inputKeyEvent, inputKeyMaster);
}

void InputBindingMapInterface::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster);
}

void InputBindingMapInterface::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAction(actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
}

void InputBindingMapInterface::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
}

void InputBindingMapInterface::RegisterAxis(const std::string & axisName, const InputKey & masterKey, float fScalar)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAxis(axisName, masterKey, fScalar);
}

void InputBindingMapInterface::RegisterAxis(const std::string & bindingMapName, const std::string & axisName, const InputKey & masterKey, float fScalar)
{
	if (GetInputBindingMap() != nullptr)
		GetInputBindingMap()->RegisterAxis(bindingMapName, axisName, masterKey, fScalar);
}

uint64_t InputBindingMapInterface::BindAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager)
{
	return (GetInputBindingMap() != nullptr) ? GetInputBindingMap()->BindAction(actionName, inputKeyEvent, bindFunc, pWorldManager) : INVALID_UNIQUE_INPUT_HANDLE;
}

uint64_t InputBindingMapInterface::BindAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager)
{
	return (GetInputBindingMap() != nullptr) ? GetInputBindingMap()->BindAction(bindingMapName, actionName, inputKeyEvent, bindFunc, pWorldManager) : INVALID_UNIQUE_INPUT_HANDLE;
}

bool InputBindingMapInterface::UnBindAction(uint64_t bindActionHandle)
{
	return (GetInputBindingMap() != nullptr) ? GetInputBindingMap()->UnBindAction(bindActionHandle) : false;
}

uint64_t InputBindingMapInterface::BindAxis(const std::string & axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager)
{
	return (GetInputBindingMap() != nullptr) ? GetInputBindingMap()->BindAxis(axisName, bindFunc, pWorldManager) : INVALID_UNIQUE_INPUT_HANDLE;
}

uint64_t InputBindingMapInterface::BindAxis(const std::string & bindingMapName, const std::string & axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager)
{
	return (GetInputBindingMap() != nullptr) ? GetInputBindingMap()->BindAxis(bindingMapName, axisName, bindFunc, pWorldManager) : INVALID_UNIQUE_INPUT_HANDLE;
}

bool InputBindingMapInterface::UnBindAxis(uint64_t bindAxisHandle)
{
	if (GetInputBindingMap() != nullptr)
		return GetInputBindingMap()->UnBindAxis(bindAxisHandle);

	return false;
}

const InputBindingAxis * InputBindingMapInterface::GetInputBindingAxis(const std::string & axisName)
{
	return GetInputBindingMap()->GetInputBindingAxis(axisName);
}

const InputBindingAxis * InputBindingMapInterface::GetInputBindingAxis(const std::string & bindingMapName, const std::string & axisName)
{
	return GetInputBindingMap()->GetInputBindingAxis(bindingMapName, axisName);
}
