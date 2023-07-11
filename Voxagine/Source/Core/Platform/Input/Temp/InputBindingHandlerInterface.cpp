#include "pch.h"
#include "InputBindingHandlerInterface.h"

#include "InputBindingMap.h"
#include "InputBindingAxis.h"
#include "InputBindingAction.h"

#include "InputController.h"
#include "MouseController.h"
#include "KeyboardController.h"
#include "GamePadController.h"

void InputBindingHandlerInterface::ProcessBindingMapAction(const InputBindingMapInformation & inputBindingMapInformation)
{
	for (const InputBindingAction& inputBindingActionIt : inputBindingMapInformation.Actions)
		ProcessBindingAction(inputBindingActionIt);
}

void InputBindingHandlerInterface::ProcessBindingAction(const InputBindingAction & inputBindingAction)
{
	ProcessBindingActionState(inputBindingAction.GetInputStateAction(InputKeyStatus::IKS_PRESSED), InputKeyStatus::IKS_PRESSED);
	ProcessBindingActionState(inputBindingAction.GetInputStateAction(InputKeyStatus::IKS_HELD), InputKeyStatus::IKS_HELD);
	ProcessBindingActionState(inputBindingAction.GetInputStateAction(InputKeyStatus::IKS_RELEASED), InputKeyStatus::IKS_RELEASED);
}

void InputBindingHandlerInterface::ProcessBindingActionState(const InputStateAction & inputStateAction, const InputKeyStatus & inputKeyStatus)
{
	std::vector<InputController*> inputControllers;
	GetInputControllers(inputControllers);

	for (const InputKeyCombination& inputKeyCombinationIt : inputStateAction.KeyCombinations)
	{
		for (InputController* controllerMasterKeyIt : inputControllers)
		{
			bool isKeysCombinationHit = false;

			if (ProcessInputKey(controllerMasterKeyIt, inputKeyCombinationIt.InputMaster, inputKeyStatus))
			{
				bool isKeyModifierHit = inputKeyCombinationIt.InputModifiers.size() == 0;

				for (const InputKey& inputKeyModifierIt : inputKeyCombinationIt.InputModifiers)
				{
					InputKeyStatus inputKeyStatusModifier = InputKeyStatus::IKS_NONE;

					for (InputController* controllerModifierKeyIt : inputControllers)
					{
						inputKeyStatusModifier = GetInputKeyStatusFromController(controllerModifierKeyIt, inputKeyModifierIt);

						if (inputKeyStatusModifier == InputKeyStatus::IKS_RELEASED)
						{
							isKeyModifierHit = true;
							break;
						}
					}

					if (isKeyModifierHit)
						break;
				}

				if (isKeyModifierHit)
					isKeysCombinationHit = true;
			}

			if (isKeysCombinationHit)
			{
				for (auto&& actionCallbackIt : inputStateAction.Callbacks)
					actionCallbackIt.second();
			}
		}
	}
}

InputBindingAxisValue InputBindingHandlerInterface::ProcessBindingAxis(const InputBindingAxis & inputBindingAxis)
{
	InputBindingAxisValue inputKeyAxisValue = InputBindingAxisValue({ false, 0.f });

	for (const InputKeyAxis& inputKeyIt : inputBindingAxis.GetInputMasters())
	{
		InputBindingAxisValue inputKeyAxisValueIt = ProcessBindingAxisKey(inputKeyIt);

		if (inputKeyAxisValueIt.Proccessed)
		{
			if (!inputKeyAxisValue.Value)
			{
				inputKeyAxisValue.Value = inputKeyAxisValueIt.Value;
				inputKeyAxisValue.Proccessed = true;
			}
			else
			{
				inputKeyAxisValue.Value += inputKeyAxisValueIt.Value;
			}
		}
	}

	return inputKeyAxisValue;
}

InputBindingAxisValue InputBindingHandlerInterface::ProcessBindingAxisKey(const InputKeyAxis & inputKey)
{
	std::vector<InputController*> inputControllers;
	GetInputControllers(inputControllers);

	for (InputController* inputControllerIt : inputControllers)
	{
		InputBindingAxisValue inputKeyAxisValue = inputControllerIt->GetAxisValue(inputKey.MasterKey);

		if (inputKeyAxisValue.Proccessed)
		{
			if (inputKey.AxisScalar != 0.f)
				inputKeyAxisValue.Value *= inputKey.AxisScalar;

			return inputKeyAxisValue;
		}
	}

	return InputBindingAxisValue({ false, 0.f });
}

bool InputBindingHandlerInterface::ProcessInputKey(InputController * inputController, const InputKey & inputKey, const InputKeyStatus & inputKeyStatus)
{
	return (GetInputKeyStatusFromController(inputController, inputKey) == inputKeyStatus);
}

InputKeyStatus InputBindingHandlerInterface::GetInputKeyStatusFromController(InputController * inputController, const InputKey & inputKey)
{
	return inputController->GetKeyStatus(inputKey);
}

void InputBindingHandlerInterface::GetInputControllers(std::vector<InputController*>& inputControllers)
{
	if (GetMouseController() != nullptr)
		inputControllers.push_back(GetMouseController());

	if (GetKeyBoardController() != nullptr)
		inputControllers.push_back(GetKeyBoardController());

	if (GetGamePadController() != nullptr)
		inputControllers.push_back(GetGamePadController());
}
