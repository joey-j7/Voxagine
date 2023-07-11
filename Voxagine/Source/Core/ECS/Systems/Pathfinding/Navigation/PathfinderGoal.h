#pragma once
#include "Core/Math.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"

namespace pathfinding
{
	class PathfinderGoal : public Component
	{
	public:
		pathfinding::ContinuumCrowdsGroup* m_group;
		bool m_bProjectPosition;
		float m_fPotential;

	public:
		PathfinderGoal(Entity* pOwner);
		~PathfinderGoal();
		void Start() override;

		IVector3 getGoalWorldPos() const;

		RTTR_ENABLE(Component)
		RTTR_REGISTRATION_FRIEND
	};
}