#pragma once
#include "Core/ECS/Entity.h"

class GenericPickup : public Entity
{
public:
	GenericPickup(World* pWorld);

	void OnCollisionEnter(Collider* pCollider, const Manifold&) override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;
protected:
	std::function<void(Collider* pCollider)> m_OnCollideCallback = nullptr;
};
