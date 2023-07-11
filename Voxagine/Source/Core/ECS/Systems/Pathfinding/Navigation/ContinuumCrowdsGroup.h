#pragma once
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "Core/ECS/SYstems/Pathfinding/Grid/PathfindingNode.h"

namespace pathfinding
{
	class Pathfinder;
	class ContinuumCrowdsGroup : public PathfinderGroup
	{
	private:
		// Used to calculate the potential field
		struct CandidateNode
		{
			float m_tempPotential = ContinuumCrowdsGroup::g_INFINTE;
			Node* m_pNode = nullptr;
			bool operator>(const CandidateNode& other) const { return (m_tempPotential > other.m_tempPotential); }
		};
		std::vector<CandidateNode> m_candidatesHeap;

	public:
		static const float g_INFINTE;

		// Group properties
		float m_fMinHeightGradient, m_fMaxHeightGradient;				// Min and max slope the agent can walk down/up.
		float m_fMinGradientVel, m_fMaxGradientVel;						// The velocity at the min and max slope.
		float m_fMinVelocity;											// The velocity to maintain at all times.
		float m_fDistanceWeight, m_fTimeWeight, m_fDiscomfortWeight;	// How much does each metric weight to the pathfinding.
		float m_fMaxSeparationStrength;
		float m_fSeparationDistance;
		float m_fSeparationCoefficient;
		float m_fCohesionStrength;

	public:
		ContinuumCrowdsGroup(World* pWorld);
		void Awake() override;
		void Start() override;
		void updatePaths() override;
		void updateAgents(Pathfinder& pathfinder) override;

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

	private:
		// Used to calculate pathfinding fields
		// Particularly in the fast marging algorithm as a shorthand push and pop
		void pushCandidateToHeap(CandidateNode candidate);
		CandidateNode popCandidateFromHeap();

		// Calculte group properties
		GroupNode* getGoalNode(const PathfinderGoal& goal);
		float calculateSpeed(Node& fromNode, const Node& toNode, const NodeConnection& connection, Node::Directions direction);
		float calculateCost(Node& fromNode, const Node& toNode, const NodeConnection& connection, Node::Directions direction);
		CandidateNode getCandidateNode(const Node& fromNode, Node& toNode);
		void buildPotentialField();
		Vector4 calculatePotentialGradient(const Node& node);
		Vector2 calculateTotVelocity(Node& node, const Vector4& potentialGrad);

		// Field calculation helper functions
		float getTopoSpeed(float heighGradient) const;
		float getFlowSpeed(const Node& fromNode, const Node& toNode) const;
		float getPotential(Node& node);
		float getFiniteDifference(const Node& fromNode, const Node& toNode, Node::Directions direction) const;
		float getFiniteDifference(const Node& fromNode, const Node& xToNode, Node::Directions xDirection, const Node& zToNode, Node::Directions zDirection) const;
	};
}