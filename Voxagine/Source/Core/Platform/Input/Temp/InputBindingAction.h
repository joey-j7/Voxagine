#pragma once

#include "InputBindingBase.h"

#include "InputStateCallback.h"

#include "InputKeyIdentifiers.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct InputKeyCombination
{
	InputKey InputMaster;
	std::vector<InputKey> InputModifiers;
};

struct InputStateAction
{
	std::vector<InputKeyCombination> KeyCombinations;
	std::unordered_map<uint64_t, InputStateCallback<>> Callbacks;
};

class InputBindingAction : public InputBindingBase
{
public:
	InputBindingAction(std::string actionName) : InputBindingBase(actionName) {};
	~InputBindingAction() {};

	void AddInputKey(const InputKeyStatus& inputMasterKeyEvent, const InputKey& inputMasterKey);
	void AddInputKey(const InputKeyStatus& inputMasterKeyEvent, const InputKey& inputMasterKey, const std::vector<InputKey>& inputModifierKeys);

	uint64_t AddCallback(const InputKeyStatus& inputMasterKeyEvent, std::function<void()>& callBack, WorldManager* pWorldManager);
	bool RemoveCallback(uint64_t bindHandle);

	bool HasInputKey(const InputKeyStatus& inputMasterKeyEvent, const InputKey& inputMasterKey, const std::vector<InputKey>& inputModifierKeys) const;
	const InputStateAction& GetInputStateAction(InputKeyStatus inputKeyStatus) const;

private:
	InputStateAction m_InputStateActions[InputKeyStatus::IKS_COUNT];
};