#include "pch.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<pathfinding::PathfinderGoal>("PathfinderGoal")
	.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Pathfinder Group", &pathfinding::PathfinderGoal::m_group) (RTTR_PUBLIC)
	.property("Project Goal On Grid", &pathfinding::PathfinderGoal::m_bProjectPosition) (RTTR_PUBLIC)
	.property("Potential", &pathfinding::PathfinderGoal::m_fPotential) (RTTR_PUBLIC);
}

namespace pathfinding
{
	PathfinderGoal::PathfinderGoal(Entity * pOwner) :
		Component(pOwner),
		m_fPotential(0),
		m_group(nullptr),
		m_bProjectPosition(true)
	{}

	PathfinderGoal::~PathfinderGoal()
	{
		if (m_group != nullptr)
			m_group->removeGoal(*this);
	}

	void PathfinderGoal::Start()
	{
		Component::Start();

		if (m_group != nullptr)
			m_group->addGoal(*this);
	}

	IVector3 PathfinderGoal::getGoalWorldPos() const
	{
		return ((Component*)this)->GetOwner()->GetTransform()->GetPosition();
	}
}