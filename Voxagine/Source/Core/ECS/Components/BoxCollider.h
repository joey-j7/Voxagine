#pragma once
#include "Core/ECS/Components/Collider.h"

#include "Core/Math.h"
#include <External/rttr/type>

struct VoxFrame;

class VoxRenderer;
class BoxCollider : public Collider
{
public:
	BoxCollider(Entity* pOwner);

	Vector3 GetBoxMin() const;
	Vector3 GetBoxMax() const;
	Vector3 GetBoxSize() const { return m_CollisionBox; }
	Vector3 GetHalfBoxSize() const { return m_CollisionBoxHalf; }
	uint32_t GetNumVoxels() const { return (uint32_t)m_CollisionBox.x * (uint32_t)m_CollisionBox.y * (uint32_t)m_CollisionBox.z; }

	void SetBoxSize(const VoxFrame* pFrame);
	void SetBoxSize(const VoxRenderer* pRenderer);
	void SetBoxSize(Vector3 dimensions);

	void AutoFit(bool bFit = true);
	bool IsAutoFitted() const { return false; };

private:
	Vector3 m_CollisionBox;
	Vector3 m_CollisionBoxHalf;

	RTTR_ENABLE(Collider)
};