#pragma once

#include "Core/ECS/Entity.h"

class World;
class BasePickup : public Entity
{
public:
	BasePickup(World* pWorld) : Entity(pWorld) { }
	virtual ~BasePickup() = default;

	virtual void OnPickup() = 0;
	virtual void OnCollide(Collider* pCollider) = 0;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
