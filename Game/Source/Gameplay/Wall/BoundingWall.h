#pragma once

#include "Core/Math.h"
#include "Core/ECS/Entity.h"

class BoxCollider;
class ParticleSystem;
class Player;
class BoundingWall : public Entity
{
public:
	BoundingWall(World* pWorld);

	void Awake() override;
	void Tick(float fDeltaTime) override;

	// Note we could add particle effect on hit
	// void OnCollisionEnter(Collider*, const Manifold&) override;

	void OnEnabled() override;
	void OnDisabled() override;

	bool bEndWall = false;
private:
	BoxCollider* m_pCollider = nullptr;
	ParticleSystem* m_pParticleSystem = nullptr;

	bool m_bActivated = false;

	RTTR_ENABLE(Entity)
};
