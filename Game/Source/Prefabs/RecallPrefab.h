#pragma once

#include "Core/ECS/Entity.h"
#include "Core/Math.h"

class SpriteRenderer;
class RecallPrefab : public Entity
{
public:
	RecallPrefab(World* world);

	void Start() override;

	SpriteRenderer* pArrowRenderer = nullptr;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
