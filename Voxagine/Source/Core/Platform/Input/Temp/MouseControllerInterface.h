#pragma once

#include "Core/Math.h"

class MouseController;

class MouseControllerInterface
{
public:
	MouseControllerInterface() {};
	virtual ~MouseControllerInterface() {};

	bool IsConnected();

	bool IsMouseButtonDownLeft();
	bool IsMouseButtonDownMiddle();
	bool IsMouseButtonDownRight();

	bool IsMouseButtonDownOptional();
	bool IsMouseButtonDownOptional2();

	Vector2 GetMousePosition();
	Vector2 GetMousePositionDelta();

	float GetMouseAxisX();
	float GetMouseAxisY();
	float GetMouseAxisXDelta();
	float GetMouseAxisYDelta();

	float GetMouseWheel();
	float GetMouseWheelDelta();
private:
	virtual MouseController* GetMouse() = 0;
};