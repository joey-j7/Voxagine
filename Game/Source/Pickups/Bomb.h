#pragma once
#include "Core/ECS/Entity.h"

class PhysicsBody;
class Bomb : public Entity
{
public:
	Bomb(World* world);

	void Start() override;
	void Tick(float fDeltaTime) override;
	void OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled) override;

	void SetOwner(Entity* pOwner) { m_pOwner = pOwner; }

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
private:
	Entity* m_pOwner = nullptr;
	PhysicsBody* m_pBody = nullptr;

	float m_fBombRadius = 50.0f;
	float m_fBombForce = 50.0f;

	float m_fLife = 0.0f;
	float m_fDestroyTimer = 2.0f;

	/*!
	* @brief force applied on the bomb
	*/
	float m_fForce = 100.0f;
};