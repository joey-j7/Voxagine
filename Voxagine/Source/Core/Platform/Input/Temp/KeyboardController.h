#pragma once

#include "InputController.h"

class KeyboardController : public InputController
{
public:
	KeyboardController();
	virtual ~KeyboardController();

	virtual InputBindingAxisValue GetAxisValue(InputKey inputKey) const override;

private:
	void OnInitialize() override;
	void OnUninitialize() override;
	void OnUpdate() override;

	void InitializeButtons() override;

private:
	// Static boolean indicating library dependencies being initialized
	static bool m_bInitLibraries;
};