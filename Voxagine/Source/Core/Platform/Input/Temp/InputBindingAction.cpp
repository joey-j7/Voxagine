#include "pch.h"
#include "InputBindingAction.h"

void InputBindingAction::AddInputKey(const InputKeyStatus & inputMasterKeyEvent, const InputKey & inputMasterKey)
{
	AddInputKey(inputMasterKeyEvent, inputMasterKey, {} );
}

void InputBindingAction::AddInputKey(const InputKeyStatus & inputMasterKeyEvent, const InputKey & inputMasterKey, const std::vector<InputKey>& inputModifierKeys)
{
	m_InputStateActions[inputMasterKeyEvent].KeyCombinations.push_back(InputKeyCombination());
	m_InputStateActions[inputMasterKeyEvent].KeyCombinations.back().InputMaster = inputMasterKey;
	m_InputStateActions[inputMasterKeyEvent].KeyCombinations.back().InputModifiers = inputModifierKeys;
}

uint64_t InputBindingAction::AddCallback(const InputKeyStatus & inputMasterKeyEvent, std::function<void()>& callBack, WorldManager* pWorldManager)
{
	uint64_t newUniqueHandle = GenerateUniqueHandleID();
	m_InputStateActions[inputMasterKeyEvent].Callbacks[newUniqueHandle] = { pWorldManager, callBack };
	return newUniqueHandle;
}

bool InputBindingAction::RemoveCallback(uint64_t bindHandle)
{
	for (unsigned inputBindingStateIt = 0; inputBindingStateIt != 3; ++inputBindingStateIt)
	{
		auto findHandle = m_InputStateActions[inputBindingStateIt].Callbacks.find(bindHandle);

		if (findHandle != m_InputStateActions[inputBindingStateIt].Callbacks.end())
		{
			m_InputStateActions[inputBindingStateIt].Callbacks.erase(findHandle);
			return true;
		}
	}

	return false;
}

bool InputBindingAction::HasInputKey(const InputKeyStatus & inputMasterKeyEvent, const InputKey & inputMasterKey, const std::vector<InputKey>& inputModifierKeys) const
{
	for (const InputKeyCombination& inputKeyCombinationIt : m_InputStateActions[inputMasterKeyEvent].KeyCombinations)
	{
		if (inputKeyCombinationIt.InputMaster == inputMasterKey)
		{
			bool findKeyCombination = true;

			for (const InputKey& inputKeyModifierIt : inputModifierKeys)
			{
				auto found = std::find(inputKeyCombinationIt.InputModifiers.begin(), inputKeyCombinationIt.InputModifiers.end(), inputKeyModifierIt);

				if (found == inputKeyCombinationIt.InputModifiers.end())
				{
					findKeyCombination = false;
					break;
				}
			}

			if (findKeyCombination)
				return true;
		}
	}

	return false;
}

const InputStateAction & InputBindingAction::GetInputStateAction(InputKeyStatus inputKeyStatus) const
{
	return m_InputStateActions[inputKeyStatus];
}
