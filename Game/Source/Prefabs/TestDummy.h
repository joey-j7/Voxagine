#pragma once
#include "Core/ECS/Entity.h"

class World;
class TestDummy : public Entity
{
public:
	TestDummy(World* world);

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Tick(float fDeltaTime) override;

	RTTR_ENABLE(Entity)

private:
	float m_fTimer = 0.f;
};