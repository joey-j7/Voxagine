#pragma once

#include "InputBindingBase.h"

#include "InputStateCallback.h"

#include "InputKeyIdentifiers.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

class InputBindingAxis : public InputBindingBase
{
public:
	InputBindingAxis(const std::string& axisName) : InputBindingBase(axisName) {};
	~InputBindingAxis() {};
	
	void AddInputKey(const InputKey& inputKey, float fAxisScalar = 1.f);

	uint64_t AddCallback(std::function<void(float)>& callBack, WorldManager* pWorldManager);
	bool RemoveCallback(uint64_t bindHandle);

	bool HasInputKey(const InputKey& inputKey) const;
	const std::vector<InputKeyAxis>& GetInputMasters() const;
	const std::unordered_map<uint64_t, InputStateCallback<float>>& GetCallbacks() const;
private:
	std::vector<InputKeyAxis> m_InputMasters;
	std::unordered_map<uint64_t, InputStateCallback<float>> m_Callbacks;
};