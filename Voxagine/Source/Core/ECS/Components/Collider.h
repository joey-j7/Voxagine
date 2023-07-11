#pragma once
#include "Core/ECS/Component.h"
#include "Core/ECS/Systems/Physics/Manifold.h"

#include "Core/Math.h"

#include <External/rttr/type>

enum CollisionLayer
{
	CL_ALL = -1,
	CL_DEFAULT = 1 << 1,
	CL_ENTITY = 1 << 2,
	CL_STATIC_ENTITY = 1 << 3,
	CL_PARTICLES = 1 << 4,
	CL_OBSTACLES = 1 << 5
};

struct Voxel;
class PhysicsBody;
class Collider : public Component
{
public:
	Collider(Entity* pOwner);

	Event<Collider*, const Manifold&> CollisionEnter;
	Event<Collider*, const Manifold&> CollisionStay;
	Event<Collider*, const Manifold&> CollisionExit;
	Event<Voxel**, uint32_t, bool&> VoxelCollision;

	virtual void Awake() override;

	bool IgnoreVoxels() const { return m_bIgnoreVoxels; }
	bool ContinuousVoxelCollision() const { return m_bContinuousVoxelCollision; }

	bool IsTrigger() const { return m_bTrigger; }
	CollisionLayer GetLayer() const { return m_Layer; }
	Vector3 GetGridPosition() const { return m_GridPosition; }

	/* Reset all collisions to false for re-evaluation in the current physics update */
	void ResetCollisions();

	/* Evaluate which collision event should be called on this collision */
	void HandleCollision(Manifold& manifold);

	/* Clear all exited collision and call appropriate event */
	void CleanCollisions();

	void OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled);

	void SetTrigger(bool bTrigger) { m_bTrigger = bTrigger; }
	void SetLayer(CollisionLayer layer) { m_Layer = layer; }
	void SetGridPosition(Vector3 pos) { m_GridPosition = pos; }
	void SetIgnoreVoxels(bool bIgnoreVoxels) { m_bIgnoreVoxels = bIgnoreVoxels; }
	void SetContinuousVoxelCollision(bool bVoxelCollision) { m_bContinuousVoxelCollision = bVoxelCollision; }
	
	bool ContinuousCollision() const { return m_bContinuousCollision; }
	void SetContinuousCollision(bool bCollision) { m_bContinuousCollision = bCollision; }

	bool VoxelPreciseCollision() const { return m_bVoxelPresiceCollision; }
	void SetVoxelPreciseCollision(bool bCollision) 
	{ 
		m_bVoxelPresiceCollision = bCollision; 
		if (!m_bVoxelPresiceCollision)
		{
			GetOwner()->SetDestructible(false);
		}
	}

private:
	CollisionLayer m_Layer = CL_DEFAULT;
	bool m_bTrigger = false;
	bool m_bContinuousVoxelCollision = false;
	Vector3 m_GridPosition = Vector3(0.f);
	bool m_bIgnoreVoxels = false;
	bool m_bContinuousCollision = false;
	bool m_bVoxelPresiceCollision = false;

	std::vector<std::pair<bool, Manifold>> m_Collisions;

	RTTR_ENABLE(Component)
};