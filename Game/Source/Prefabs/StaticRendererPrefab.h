#pragma once

#include "Core/ECS/Entity.h"
#include "Core/Math.h"

class StaticRendererPrefab : public Entity
{
public:
	StaticRendererPrefab(World* world);
	void Awake() override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
