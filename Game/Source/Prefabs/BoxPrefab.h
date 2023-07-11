#pragma once
#include "Core/ECS/Entity.h"

class BoxPrefab : public Entity
{
public:
	BoxPrefab(World* world);

	void Start() override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
