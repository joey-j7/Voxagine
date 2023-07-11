#include "pch.h"
#include "KeyboardControllerInterface.h"

#include "KeyboardController.h"

bool KeyboardControllerInterface::IsKeyPressed(InputKey inputKey)
{
	return (GetKeyboard() != nullptr) ? GetKeyboard()->IsKeyPressed(inputKey) : false;
}

bool KeyboardControllerInterface::IsKeyHeld(InputKey inputKey) 
{
	if (GetKeyboard() == nullptr)
		return false;

	InputKeyStatus inputKeyStatus = GetKeyboard()->GetKeyStatus(inputKey);
	return (inputKeyStatus == InputKeyStatus::IKS_PRESSED || inputKeyStatus == InputKeyStatus::IKS_HELD);
}

bool KeyboardControllerInterface::IsKeyReleased(InputKey inputKey)
{
	return (GetKeyboard() != nullptr) ? GetKeyboard()->IsKeyReleased(inputKey) : false;
}

InputKeyStatus KeyboardControllerInterface::GetKeyStatus(InputKey inputKey) 
{
	return (GetKeyboard() != nullptr) ? GetKeyboard()->GetKeyStatus(inputKey) : InputKeyStatus::IKS_NONE;
}
