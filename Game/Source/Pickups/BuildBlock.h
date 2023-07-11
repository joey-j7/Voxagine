#pragma once
#include "Weapons/Bullet.h"

class PhysicsBody;
class VoxModel;
class BuildBlock : public Bullet
{
public:
	BuildBlock(World* world);

	void SetOwner(Entity* pOwner) { m_pOwner = pOwner; };

	void Start() override;
	void OnCollisionEnter(Collider* pCollider, const Manifold&) override;
	void OnVoxelCollision(Voxel**, uint32_t, bool&) override {}

	RTTR_ENABLE(Bullet)
	RTTR_REGISTRATION_FRIEND
private:
	Entity* m_pOwner = nullptr;
	VoxModel* m_pModel = nullptr;
	PhysicsBody* m_pBody = nullptr;
};
