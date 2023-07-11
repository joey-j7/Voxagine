#pragma once
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "Core/ECS/SYstems/Pathfinding/Grid/PathfindingNode.h"

namespace pathfinding
{
	class Pathfinder;
	class FlowFieldGroup : public PathfinderGroup
	{
	public:
		static const float g_INFINTE;

	public:
		FlowFieldGroup(World* pWorld);
		void Awake() override;
		void updatePaths() override;

		// Get the desired velocity and height given an agent.
		using PathfinderGroup::getDesiredVeclocityAndHeight;
		using PathfinderGroup::getDesiredVeclocity;
		using PathfinderGroup::getDesiredHeight;

		// Get the desired velocity and height given an position.
		void getDesiredVeclocityAndHeight(Vector2& o_velocity, float& o_height, Node** o_node, const IVector3& worldPos) const override;
		Vector2 getDesiredVeclocity(const IVector3& worldPos) const override;
		float getDesiredHeight(const IVector3& worldPos) const override;

		RTTR_ENABLE(PathfinderGroup)
		RTTR_REGISTRATION_FRIEND
	};
}