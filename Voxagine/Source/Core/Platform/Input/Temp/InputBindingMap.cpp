#include "pch.h"
#include "InputBindingMap.h"

bool InputBindingMap::CreateBindingMap(const std::string & bindingMapName, bool bSetActiveMap)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);

	if (inputBindingMapInformation == nullptr)
	{
		m_InputBindingMap[bindingMapName] = InputBindingMapInformation();
		m_InputBindingMap[bindingMapName].Name = bindingMapName;
		inputBindingMapInformation = &m_InputBindingMap[bindingMapName];
	}

	if (bSetActiveMap)
		SetActiveBindingMap(bindingMapName);

	return (inputBindingMapInformation != nullptr);
}

bool InputBindingMap::DestroyBindingMap(const std::string & bindingMapName)
{
	std::unordered_map<std::string, InputBindingMapInformation>::iterator found = m_InputBindingMap.find(bindingMapName);
	if (found == m_InputBindingMap.end())
		return false;

	m_InputBindingMap.erase(found);

	return true;
}

const InputBindingMapInformation * InputBindingMap::GetBindingMap(const std::string & bindingMapName)
{
	return FindInputBindingMap(bindingMapName);
}

bool InputBindingMap::SetActiveBindingMap(const std::string & bindingMapName)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation == nullptr)
		return false;

	if (m_pActiveBindingLayer != nullptr)
	{
		if (m_pActiveBindingLayer->Name == bindingMapName)
			return false;
	}

	m_pActiveBindingLayer = inputBindingMapInformation;
	return true;
}

const InputBindingMapInformation * InputBindingMap::GetActiveBindingMap() const
{
	return m_pActiveBindingLayer;
}

void InputBindingMap::SetDefaultMapName(const std::string & defaultMapName)
{
	m_DefaultMapName = defaultMapName;
}

std::string InputBindingMap::GetDefaultMapName() const
{
	return m_DefaultMapName;
}

void InputBindingMap::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster)
{
	RegisterAction(GetDefaultMapName(), actionName, inputKeyEvent, inputKeyMaster);
}

void InputBindingMap::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster)
{
	RegisterAction(bindingMapName, actionName, inputKeyEvent, inputKeyMaster, {});
}

void InputBindingMap::RegisterAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers)
{
	RegisterAction(GetDefaultMapName(), actionName, inputKeyEvent, inputKeyMaster, inputKeyModifiers);
}

void InputBindingMap::RegisterAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, const InputKey & inputKeyMaster, const std::vector<InputKey>& inputKeyModifiers)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation != nullptr)
	{
		InputBindingAction* inputBindingAction = FindInputBindingAction(inputBindingMapInformation, actionName);
		if (inputBindingAction == nullptr)
		{
			inputBindingMapInformation->Actions.push_back(InputBindingAction(actionName));
			inputBindingAction = &inputBindingMapInformation->Actions.back();
		}
		
		if (!inputBindingAction->HasInputKey(inputKeyEvent, inputKeyMaster, inputKeyModifiers))
		{
			inputBindingAction->AddInputKey(inputKeyEvent, inputKeyMaster, inputKeyModifiers);
		}
	}
}

uint64_t InputBindingMap::BindAction(const std::string & actionName, const InputKeyStatus & inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager)
{
	return BindAction(GetDefaultMapName(), actionName, inputKeyEvent, bindFunc, pWorldManager);
}

uint64_t InputBindingMap::BindAction(const std::string & bindingMapName, const std::string & actionName, const InputKeyStatus & inputKeyEvent, std::function<void()> bindFunc, WorldManager* pWorldManager)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation != nullptr)
	{
		InputBindingAction* inputBindingAction = FindInputBindingAction(inputBindingMapInformation, actionName);
		if (inputBindingAction != nullptr)
		{
			return inputBindingAction->AddCallback(inputKeyEvent, bindFunc, pWorldManager);
		}
	}

	return INVALID_UNIQUE_INPUT_HANDLE;
}

bool InputBindingMap::UnBindAction(uint64_t bindActionHandle)
{
	for (auto&& inputBindingMapInformation : m_InputBindingMap)
	{
		for (InputBindingAction& inputBindingAction : inputBindingMapInformation.second.Actions)
		{
			if (inputBindingAction.RemoveCallback(bindActionHandle))
				return true;
		}
	}

	return false;
}

void InputBindingMap::RegisterAxis(const std::string & axisName, const InputKey & masterKey, float fScalar)
{
	RegisterAxis(GetDefaultMapName(), axisName, masterKey, fScalar);
}

void InputBindingMap::RegisterAxis(const std::string & bindingMapName, const std::string & axisName, const InputKey & masterKey, float fScalar)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation != nullptr)
	{
		InputBindingAxis* inputBindingAxis = FindInputBindingAxis(inputBindingMapInformation, axisName);
		if (inputBindingAxis == nullptr)
		{
			inputBindingMapInformation->Axises.push_back(InputBindingAxis(axisName));
			inputBindingAxis = &inputBindingMapInformation->Axises.back();
		}
		
		if (!inputBindingAxis->HasInputKey(masterKey))
		{
			inputBindingAxis->AddInputKey(masterKey, fScalar);
		}
	}
}

uint64_t InputBindingMap::BindAxis(const std::string & axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager)
{
	return BindAxis(GetDefaultMapName(), axisName, bindFunc, pWorldManager);
}

uint64_t InputBindingMap::BindAxis(const std::string & bindingMapName, const std::string & axisName, std::function<void(float)> bindFunc, WorldManager* pWorldManager)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation != nullptr)
	{
		InputBindingAxis* inputBindingAxis = FindInputBindingAxis(inputBindingMapInformation, axisName);
		if (inputBindingAxis != nullptr)
		{
			return inputBindingAxis->AddCallback(bindFunc, pWorldManager);
		}
	}

	return INVALID_UNIQUE_INPUT_HANDLE;
}

bool InputBindingMap::UnBindAxis(uint64_t bindAxisHandle)
{
	for (auto&& inputBindingMapInformation : m_InputBindingMap)
	{
		for (InputBindingAxis& inputBindingAxis : inputBindingMapInformation.second.Axises)
		{
			if (inputBindingAxis.RemoveCallback(bindAxisHandle))
				return true;
		}
	}

	return false;
}

const InputBindingAxis * InputBindingMap::GetInputBindingAxis(const std::string & axisName)
{
	return GetInputBindingAxis(GetDefaultMapName(), axisName);
}

const InputBindingAxis * InputBindingMap::GetInputBindingAxis(const std::string & bindingMapName, const std::string & axisName)
{
	InputBindingMapInformation* inputBindingMapInformation = FindInputBindingMap(bindingMapName);
	if (inputBindingMapInformation != nullptr && inputBindingMapInformation == m_pActiveBindingLayer)
		return FindInputBindingAxis(inputBindingMapInformation, axisName);

	return nullptr;
}


InputBindingMapInformation * InputBindingMap::FindInputBindingMap(const std::string & bindingMapName)
{
	std::unordered_map<std::string, InputBindingMapInformation>::iterator found = m_InputBindingMap.find(bindingMapName);
	return (found == m_InputBindingMap.end()) ? nullptr : &found->second;
}

InputBindingAction * InputBindingMap::FindInputBindingAction(InputBindingMapInformation * inputBindingMapInformation, const std::string & actionName)
{
	for (InputBindingAction& inputBindingAction : inputBindingMapInformation->Actions)
	{
		if (inputBindingAction.GetName() == actionName)
			return &inputBindingAction;
	}

	return nullptr;
}

InputBindingAxis * InputBindingMap::FindInputBindingAxis(InputBindingMapInformation * inputBindingMapInformation, const std::string & axisName)
{
	for (InputBindingAxis& inputBindingAxis : inputBindingMapInformation->Axises)
	{
		if (inputBindingAxis.GetName() == axisName)
			return &inputBindingAxis;
	}

	return nullptr;
}