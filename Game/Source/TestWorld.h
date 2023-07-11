#pragma once
#include "Core/ECS/World.h"

class TestWorld : public World
{
public:
	TestWorld(Application* pApp);

	void Initialize() override;
};