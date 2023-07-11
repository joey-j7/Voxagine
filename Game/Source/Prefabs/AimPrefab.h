#pragma once

#include "Core/ECS/Entity.h"
#include "Core/Math.h"

class VoxRenderer;
class AimPrefab : public Entity
{
public:
	AimPrefab(World* world);

	void Start() override;

	VoxRenderer* pAimRenderer = nullptr;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
