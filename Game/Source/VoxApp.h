#pragma once
#include "Core/Application.h"

class VoxApp : public Application
{
protected:
	virtual void OnCreate() override;
	virtual void OnUpdate() override;
	virtual void OnDraw() override;
	virtual void OnExit() override;
};