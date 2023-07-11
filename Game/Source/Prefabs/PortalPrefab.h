#pragma once

#include "Core/ECS/Entity.h"

class Spawner;
class PortalPrefab : public Entity
{
public:
	PortalPrefab(World* world);

	void Awake() override;

	void OnDrawGizmos(float fDeltaTime) override;

private:
	Spawner* m_pSpawnComponent = nullptr;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
