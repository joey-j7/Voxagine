#include "pch.h"
#include "InputBindingAxis.h"

void InputBindingAxis::AddInputKey(const InputKey & inputKey, float fAxisScalar)
{
	m_InputMasters.push_back(InputKeyAxis());
	m_InputMasters.back().MasterKey = inputKey;
	m_InputMasters.back().AxisScalar = fAxisScalar;
}

uint64_t InputBindingAxis::AddCallback(std::function<void(float)>& callBack, WorldManager* pWorldManager)
{
	uint64_t newUniqueHandle = GenerateUniqueHandleID();
	m_Callbacks[newUniqueHandle] = { pWorldManager, callBack };
	return newUniqueHandle;
}

bool InputBindingAxis::RemoveCallback(uint64_t bindHandle)
{
	auto found = m_Callbacks.find(bindHandle);

	if (found != m_Callbacks.end())
	{
		m_Callbacks.erase(found);
		return true;
	}

	return false;
}

bool InputBindingAxis::HasInputKey(const InputKey & inputKey) const
{
	for (const InputKeyAxis& inputKeyAxis : m_InputMasters)
	{
		if (inputKeyAxis.MasterKey == inputKey)
			return true;
	}
	return false;
}

const std::vector<InputKeyAxis>& InputBindingAxis::GetInputMasters() const
{
	return m_InputMasters;
}

const std::unordered_map<uint64_t, InputStateCallback<float>>& InputBindingAxis::GetCallbacks() const
{
	return m_Callbacks;
}
