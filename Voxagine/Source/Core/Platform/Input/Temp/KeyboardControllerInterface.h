#pragma once

#include "InputKeyIdentifiers.h"

class KeyboardController;

class KeyboardControllerInterface
{
public:
	KeyboardControllerInterface() {} ;
	virtual ~KeyboardControllerInterface() {} ;

	bool IsKeyPressed(InputKey inputKey);
	bool IsKeyHeld(InputKey inputKey);
	bool IsKeyReleased(InputKey inputKey);
	InputKeyStatus GetKeyStatus(InputKey inputKey);

private:
	virtual KeyboardController* GetKeyboard() = 0;
};