#pragma once

#include "InputController.h"

class MouseController : public InputController 
{
public:
	MouseController();
	virtual ~MouseController();

private:
	void OnInitialize() override;
	void OnUninitialize() override;
	void OnUpdate() override;

	void InitializeButtons() override;
	void InitializeAxises() override;
private:
	// Static boolean indicating library dependencies being initialized
	static bool m_bInitLibraries;
};