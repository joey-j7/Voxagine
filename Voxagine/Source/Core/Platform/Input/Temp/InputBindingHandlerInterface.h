#pragma once

#include <vector>

#include "InputBindingAxisValue.h"
#include "InputKeyIdentifiers.h"

struct InputBindingMapInformation;
class InputBindingAction;
struct InputStateAction;
class InputBindingAxis;

class InputController;
class MouseController;
class KeyboardController;
class GamePadController;

class InputBindingHandlerInterface
{
public:
	InputBindingHandlerInterface() {};
	virtual ~InputBindingHandlerInterface() {};

protected:
	void ProcessBindingMapAction(const InputBindingMapInformation& inputBindingMapInformation);
	void ProcessBindingAction(const InputBindingAction& inputBindingAction);
	void ProcessBindingActionState(const InputStateAction& inputStateAction, const InputKeyStatus& inputKeyStatus);

	InputBindingAxisValue ProcessBindingAxis(const InputBindingAxis& inputBindingAxis);
	InputBindingAxisValue ProcessBindingAxisKey(const InputKeyAxis& inputKey);

private:
	bool ProcessInputKey(InputController* inputController, const InputKey& inputKey, const InputKeyStatus& inputKeyStatus);
	InputKeyStatus GetInputKeyStatusFromController(InputController* inputController, const InputKey& inputKey);
	void GetInputControllers(std::vector<InputController*>& inputControllers);

private:
	virtual MouseController* GetMouseController() = 0;
	virtual KeyboardController* GetKeyBoardController() = 0;
	virtual GamePadController* GetGamePadController() = 0;
};