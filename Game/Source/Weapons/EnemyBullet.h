#pragma once
#include "General/Managers/GameManager.h"
#include "Core/ECS/Entity.h"

class EnemyBullet : public Entity {
public:
	float m_fDamage = 1.f;
	float m_fSpeed = 200.f;
	float m_fLifeTime = 4.f;
	float m_fBulletExplosionRange = 10.0f;
	std::string m_modelFile = "";
	Vector3 m_direction = Vector3(0);

public:
	EnemyBullet(World* world);
	void Start() override;
	void FixedTick(const GameTimer& gameTimer) override;
	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;
	void OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled) override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;
};
