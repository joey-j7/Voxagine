#include "pch.h"
#include "Core/ECS/Components/Collider.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <External/rttr/registration>
#include "External/rttr/policy.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Collider>("Collider")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Ignore Voxels", &Collider::IgnoreVoxels, &Collider::SetIgnoreVoxels) (RTTR_PUBLIC)
		.property("Trigger", &Collider::IsTrigger, &Collider::SetTrigger) (RTTR_PUBLIC)
		.property("Continuous Voxel Collision", &Collider::ContinuousVoxelCollision, &Collider::SetContinuousVoxelCollision) (RTTR_PUBLIC)
		.property("Continuous Collision", &Collider::ContinuousCollision, &Collider::SetContinuousCollision) (RTTR_PUBLIC)
		.property("Voxel Precise Collision", &Collider::VoxelPreciseCollision, &Collider::SetVoxelPreciseCollision) (RTTR_PUBLIC);
}

Collider::Collider(Entity* pOwner) :
	Component(pOwner)
{}

void Collider::Awake()
{
	Component::Awake();
	m_GridPosition = GetWorld()->GetVoxelGrid()->WorldToGrid(m_pTransform->GetPosition());
}

void Collider::ResetCollisions()
{
	for (std::pair<bool, Manifold>& collisionPair : m_Collisions)
		collisionPair.first = false;
}

void Collider::HandleCollision(Manifold& manifold)
{
	for (std::pair<bool, Manifold>& collisionPair : m_Collisions)
	{
		if (collisionPair.second.Collider1 == manifold.Collider1 && collisionPair.second.Collider2 == manifold.Collider2)
		{
			GetOwner()->OnCollisionStay(manifold.Collider2, manifold);
			CollisionStay(manifold.Collider2, manifold);
			collisionPair.first = true;
			return;
		}
	}

	GetOwner()->OnCollisionEnter(manifold.Collider2, manifold);
	CollisionEnter(manifold.Collider2, manifold);
	m_Collisions.push_back(std::make_pair(true, manifold));
}

void Collider::CleanCollisions()
{
	std::vector<std::pair<bool, Manifold>>::iterator iter = m_Collisions.begin();
	for (; iter != m_Collisions.end(); )
	{
		if (!iter->first)
		{
			GetOwner()->OnCollisionExit(iter->second.Collider2, iter->second);
			CollisionExit(iter->second.Collider2, iter->second);
			iter = m_Collisions.erase(iter);
			continue;
		}
		++iter;
	}
		
}

void Collider::OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled)
{
	VoxelCollision(voxels, uiSize, isHandled);
	bool oldIsHandled = isHandled;

	GetOwner()->OnVoxelCollision(voxels, uiSize, isHandled);
	if (oldIsHandled)
		isHandled = oldIsHandled;
}
