#include "pch.h"
#include "MouseControllerInterface.h"

#include "MouseController.h"

bool MouseControllerInterface::IsConnected() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsConnected() : false;
}

bool MouseControllerInterface::IsMouseButtonDownLeft() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsKeyDown(InputKey::IK_MOUSEBUTTONLEFT) : false;
}

bool MouseControllerInterface::IsMouseButtonDownMiddle() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsKeyDown(InputKey::IK_MOUSEBUTTONMIDDLE) : false;
}

bool MouseControllerInterface::IsMouseButtonDownRight() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsKeyDown(InputKey::IK_MOUSEBUTTONRIGHT) : false;
}

bool MouseControllerInterface::IsMouseButtonDownOptional() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsKeyDown(InputKey::IK_MOUSEBUTTONOPTIONAL) : false;
}

bool MouseControllerInterface::IsMouseButtonDownOptional2() 
{
	return (GetMouse() != nullptr) ? GetMouse()->IsKeyDown(InputKey::IK_MOUSEBUTTONOPTIONAL2) : false;
}

Vector2 MouseControllerInterface::GetMousePosition() 
{
	return (GetMouse() != nullptr) ? Vector2(GetMouseAxisX(), GetMouseAxisY()) : Vector2();
}

Vector2 MouseControllerInterface::GetMousePositionDelta() 
{
	return (GetMouse() != nullptr) ? Vector2(GetMouseAxisXDelta(), GetMouseAxisYDelta()) : Vector2();
}

float MouseControllerInterface::GetMouseAxisX() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEAXISX).Value : 0.f;
}

float MouseControllerInterface::GetMouseAxisY() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEAXISY).Value : 0.f;
}

float MouseControllerInterface::GetMouseAxisXDelta() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEAXISXDELTA).Value : 0.f;
}

float MouseControllerInterface::GetMouseAxisYDelta() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEAXISYDELTA).Value : 0.f;
}

float MouseControllerInterface::GetMouseWheel() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEWHEELAXIS).Value : 0.f;
}

float MouseControllerInterface::GetMouseWheelDelta() 
{
	return (GetMouse() != nullptr) ? GetMouse()->GetAxisValue(IK_MOUSEWHEELAXISDELTA).Value : 0.f;
}
