#pragma once
#include "Core/ECS/Entity.h"

class KillTrigger : public Entity
{
public:
	KillTrigger(World* pWorld);

	void Awake() override;
	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;

private:

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};