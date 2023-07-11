#include "pch.h"
#include "InputContext.h"

#include "Core/Application.h"

uint64_t InputContext::INPUT_HANDLE_COUNTER = 0;

InputContext::InputContext(Platform* pPlatform)
{
	m_pPlatform = pPlatform;
	m_pLogger = nullptr;
}

void InputContext::Initialize()
{
	m_pLogger = &m_pPlatform->GetApplication()->GetLoggingSystem();
	m_pLogger->CreateCategory("InputContext");

	m_MousePosition = UVector2(0, 0);

	// Create default layer
	CreateLayer();
}

bool InputContext::CreateLayer(bool bActive)
{
	return CreateLayer(DEFAULT_INPUT_LAYER_NAME, bActive);
}

bool InputContext::CreateLayer(const std::string& layerName, bool bActive)
{
	// Check if a binding layer with the specific name already exists
	if (GetBindingLayer(layerName) != nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to create layer due to already existing layer with the specific name: " + layerName);
		return false;
	}

	// Create new layer and set the layer name
	m_Layers.push_back(BindingLayer());
	m_Layers.back().LayerName = layerName;

	if (bActive)
	{
		SetActiveLayer(layerName);
	}

	return true;
}

bool InputContext::SetActiveLayer(const std::string& layerName)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to set active layer due to binding layer not existing: " + layerName);
		return false;
	}

	m_pActiveLayer = pBindingLayer;
	return true;
}

std::string InputContext::GetActiveLayerName() const
{
	return GetActiveLayer()->LayerName;
}

void InputContext::RegisterAction(const std::string& actionName, InputEvent inputEvent, std::vector<InputBinding> bindingMaster)
{
	for (InputBinding& inputbinding : bindingMaster)
	{
		RegisterAction(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, inputbinding);
	}
}

void InputContext::RegisterAction(const std::string& layerName, const std::string & actionName, InputEvent inputEvent, InputBinding bindingMaster)
{
	RegisterAction(layerName, actionName, inputEvent, std::vector<InputBinding>(), bindingMaster);
}

void InputContext::RegisterAction(const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster)
{
	RegisterAction(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, bindingModifiers, bindingMaster);
}

void InputContext::RegisterAction(const std::string & layerName, const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to register action due to binding layer not existing: " + layerName);
		return;
	}

	InputAction action;
	action.Name = actionName;
	action.Type = inputEvent;
	action.CheckEvent = action.Type == IE_RELEASED ? IE_PRESSED : inputEvent;
	
	action.InputModifiers = bindingModifiers;
	action.InputMaster = bindingMaster;

	pBindingLayer->BindingActions.push_back(action);
	ActionAdded(action);
}

void InputContext::RegisterAxis(const std::string& actionName, std::vector<AxisBinding> bindings)
{
	RegisterAxis(DEFAULT_INPUT_LAYER_NAME, actionName, bindings);
}

void InputContext::RegisterAxis(const std::string& layerName, const std::string & actionName, std::vector<AxisBinding> bindings)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to register axis due to binding layer not existing: " + layerName);
		return;
	}

	InputAxis axis;
	axis.Name = actionName;
	axis.Bindings = bindings;
	axis.Value = 0.f;

	pBindingLayer->BindingAxis.push_back(axis);
	AxisAdded(axis);
}

uint64_t InputContext::BindAction(const std::string& actionName, InputEvent inputEvent, std::function<void()> bindFunc)
{
	return BindAction(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, bindFunc);
}

uint64_t InputContext::BindAction(const std::string& layerName, const std::string & actionName, InputEvent inputEvent, std::function<void()> bindFunc)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to bind action due to binding layer not existing: " + layerName);
		return std::numeric_limits<uint64_t>::max();
	}

	for (InputAction& inputAction : pBindingLayer->BindingActions)
	{
		if (inputAction.Type == inputEvent && inputAction.Name == actionName)
		{
			uint64_t handle = INPUT_HANDLE_COUNTER++;
			inputAction.Callbacks[handle] = bindFunc;
			return handle;
		}
	}

	m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to bind action with name: " + actionName);
	return std::numeric_limits<uint64_t>::max();
}

uint64_t InputContext::BindAxis(const std::string& axisName, std::function<void(float)> bindFunc)
{
	return BindAxis(DEFAULT_INPUT_LAYER_NAME, axisName, bindFunc);
}

uint64_t InputContext::BindAxis(const std::string& layerName, const std::string & axisName, std::function<void(float)> bindFunc)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to bind axis due to binding layer not existing: " + layerName);
		return std::numeric_limits<uint64_t>::max();
	}

	for (InputAxis& axis : pBindingLayer->BindingAxis)
	{
		if (axis.Name == axisName)
		{
			uint64_t handle = INPUT_HANDLE_COUNTER++;
			axis.Callbacks[handle] = bindFunc;
			return handle;
		}
	}

	m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to bind axis with name: " + axisName);
	return std::numeric_limits<uint64_t>::max();
}

void InputContext::ReAssignAction(const std::string & actionName, InputEvent inputEvent, InputBinding bindingMaster, InputEvent newInputEvent, InputBinding newBindingMaster)
{
	ReAssignAction(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, bindingMaster, newInputEvent, newBindingMaster);
}

void InputContext::ReAssignAction(const std::string & layerName, const std::string & actionName, InputEvent inputEvent, InputBinding bindingMaster, InputEvent newInputEvent, InputBinding newBindingMaster)
{
	ReAssignAction(layerName, actionName, inputEvent, std::vector<InputBinding>(), bindingMaster, newInputEvent, std::vector<InputBinding>(), newBindingMaster);
}

void InputContext::ReAssignAction(const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster, InputEvent newInputEvent, std::vector<InputBinding> newBindingModifiers, InputBinding newBindingMaster)
{
	ReAssignAction(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, bindingModifiers, bindingMaster, newInputEvent, newBindingModifiers, newBindingMaster);
}

void InputContext::ReAssignAction(const std::string & layerName, const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster, InputEvent newInputEvent, std::vector<InputBinding> newBindingModifiers, InputBinding newBindingMaster)
{
	InputAction* pReAssignInputAction = FindInputActionBinding(layerName, actionName, inputEvent, bindingModifiers, bindingMaster);
	if (pReAssignInputAction == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to reassign binding due binding to reassign not existing!");
		return;
	}

	pReAssignInputAction->Type = newInputEvent;
	pReAssignInputAction->CheckEvent = pReAssignInputAction->Type == IE_RELEASED ? IE_PRESSED : inputEvent;
	pReAssignInputAction->InputModifiers = newBindingModifiers;
	pReAssignInputAction->InputMaster = newBindingMaster;
}

void InputContext::UnBindAction(const uint64_t& uiHandle)
{
	for (BindingLayer& bindlayer : m_Layers)
	{
		for (InputAction& action : bindlayer.BindingActions)
		{
			std::unordered_map<uint64_t, std::function<void()>>::iterator iter = action.Callbacks.find(uiHandle);
			if (iter != action.Callbacks.end())
			{
				action.Callbacks.erase(iter);
				return;
			}
		}
	}

	m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to unbind input action with handle: " + std::to_string(uiHandle));
}

void InputContext::UnBindAxis(const uint64_t & uiHandle)
{
	for (BindingLayer& bindlayer : m_Layers)
	{
		for (InputAxis& axis : bindlayer.BindingAxis)
		{
			std::unordered_map<uint64_t, std::function<void(float)>>::iterator iter = axis.Callbacks.find(uiHandle);
			if (iter != axis.Callbacks.end())
			{
				axis.Callbacks.erase(iter);
				return;
			}
		}
	}

	m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to unbind input axis with handle: " + std::to_string(uiHandle));
}

const InputAxis* InputContext::GetAxis(const std::string& axisName)
{
	return GetAxis(DEFAULT_INPUT_LAYER_NAME, axisName);
}

const InputAxis * InputContext::GetAxis(const std::string & layerName, const std::string & axisName)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to get axis due to binding layer not existing: " + layerName);
		return nullptr;
	}

	for (InputAxis& axis : pBindingLayer->BindingAxis)
	{
		if (axis.Name == axisName)
			return &axis;
	}

	return nullptr;
}

BindingLayer * InputContext::GetActiveLayer() const
{
	return m_pActiveLayer;
}

BindingLayer * InputContext::GetBindingLayer(std::string layerName)
{
	// Convert layer name to upper cases
	std::string layernameupper = layerName;
	for (char& LayerNameIt : layerName)
		LayerNameIt = static_cast<char>(toupper(static_cast<int>(LayerNameIt)));

	for (BindingLayer& bindinglayer : m_Layers)
	{
		// Convert already existing binding layer name to upper cases
		std::string bindinglayernameupper = bindinglayer.LayerName;
		for (char& BindingLayerNameIt : layerName)
			BindingLayerNameIt = static_cast<char>(toupper(static_cast<int>(BindingLayerNameIt)));

		// If binding layer name is equal to layer name to be created, exit with false
		if (bindinglayernameupper == layernameupper)
			return &bindinglayer;
	}

	return nullptr;
}

InputAction * InputContext::FindInputActionBinding(const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster)
{
	return FindInputActionBinding(DEFAULT_INPUT_LAYER_NAME, actionName, inputEvent, bindingModifiers, bindingMaster);
}

InputAction * InputContext::FindInputActionBinding(const std::string & layerName, const std::string & actionName, InputEvent inputEvent, std::vector<InputBinding> bindingModifiers, InputBinding bindingMaster)
{
	BindingLayer* pBindingLayer = GetBindingLayer(layerName);
	if (pBindingLayer == nullptr)
	{
		m_pLogger->Log(LogLevel::LOGLEVEL_WARNING, "InputContext", "Failed to find binding due to binding layer not existing: " + layerName);
		return nullptr;
	}

	for (InputAction& inputAction : pBindingLayer->BindingActions)
	{
		if (inputAction.Name == actionName && inputAction.Type == inputEvent && inputAction.InputModifiers == bindingModifiers && inputAction.InputMaster == bindingMaster)
		{
			return &inputAction;
		}
	}

	return nullptr;
}
