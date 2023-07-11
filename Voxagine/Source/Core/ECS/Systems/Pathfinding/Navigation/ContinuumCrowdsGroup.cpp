#include "pch.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"

#include <limits>
#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/Threading/JobManager.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.h"
#include "External/optick/optick.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<pathfinding::ContinuumCrowdsGroup>("PathfinderGroupContinuumCrowds")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Max Step Down Height", &pathfinding::ContinuumCrowdsGroup::m_fMinHeightGradient)(RTTR_PUBLIC)
	.property("Max Step Up Height", &pathfinding::ContinuumCrowdsGroup::m_fMaxHeightGradient)(RTTR_PUBLIC)
	.property("Distance Importance", &pathfinding::ContinuumCrowdsGroup::m_fDistanceWeight)(RTTR_PUBLIC)
	.property("Time Importance", &pathfinding::ContinuumCrowdsGroup::m_fTimeWeight)(RTTR_PUBLIC)
	.property("Discomfort Importance", &pathfinding::ContinuumCrowdsGroup::m_fDiscomfortWeight)(RTTR_PUBLIC)
	.property("Separation Strength", &pathfinding::ContinuumCrowdsGroup::m_fMaxSeparationStrength)(RTTR_PUBLIC)
	.property("Separation Distance", &pathfinding::ContinuumCrowdsGroup::m_fSeparationDistance)(RTTR_PUBLIC)
	.property("Separation Cofficient", &pathfinding::ContinuumCrowdsGroup::m_fSeparationCoefficient)(RTTR_PUBLIC)
	.property("Cohesion Strength", &pathfinding::ContinuumCrowdsGroup::m_fCohesionStrength)(RTTR_PUBLIC);
}

namespace pathfinding
{
	const float ContinuumCrowdsGroup::g_INFINTE = 10000;

	ContinuumCrowdsGroup::ContinuumCrowdsGroup(World* pWorld) :
		PathfinderGroup(pWorld),
		m_fMinHeightGradient(-10.f),
		m_fMaxHeightGradient(2.f),
		m_fMinGradientVel(0.f),
		m_fMaxGradientVel(0.f),
		m_fMinVelocity(0.f),
		m_fDistanceWeight(5.f),
		m_fTimeWeight(1.f),
		m_fDiscomfortWeight(5.f),
		m_fMaxSeparationStrength(300.f),
		m_fSeparationDistance(80.f),
		m_fSeparationCoefficient(50.f),
		m_fCohesionStrength(0.f)
	{
		SetPersistent(true);
	}

	void ContinuumCrowdsGroup::Awake()
	{
		PathfinderGroup::Awake();
		SetName("PathfinderGroupContinuumCrowds");
		SetPersistent(true);

		m_fDistanceWeight = 45.f;
		m_fTimeWeight = 3.f;
		m_fDiscomfortWeight = 25.f;
		m_fMaxSeparationStrength = 300.f;
		m_fSeparationDistance = 80.f;
		m_fSeparationCoefficient = 50.f;
	}

	void ContinuumCrowdsGroup::Start()
	{
		PathfinderGroup::Start();

		m_fDistanceWeight = 5.f;
		m_fTimeWeight = 1.f;
		m_fDiscomfortWeight = 5.f;
		m_fMaxSeparationStrength = 5.f;
		m_fSeparationDistance = 15.f;
		m_fSeparationCoefficient = 4.f;
	}

	void ContinuumCrowdsGroup::updatePaths()
	{
		assert(m_pGrid);
		
		// Prepare containers
		std::make_heap(std::begin(m_candidatesHeap), std::end(m_candidatesHeap), std::greater<CandidateNode>());

		// Mark all goal nodes
		for (auto& goal : m_goals)
		{
			GroupNode* node = getGoalNode(*goal);
			if (node != nullptr)
				node->setGoal(true, goal->m_fPotential);
		}

		// For each node
		for (auto& chunk : m_pGrid->m_grid)
		{
			for (auto& node : chunk.m_nodes)
			{
				// Emplace a new node if needed
				int groupId = getId();
				auto groupPropertyIt = node.m_groupProperties.find(groupId);
				if (groupPropertyIt == node.m_groupProperties.end())
					node.m_groupProperties[getId()] = GroupNode();

				// For each neighbour
				for (int i = 0; i < 4; i++)
				{
					// Get node and connection
					const NodeConnection& connection = node.m_connections[(Node::Directions)i];
					Node* neighbour = connection.getToNode(*m_pGrid);

					if (neighbour == nullptr)
						continue;

					// Calculate properties
					float speed = calculateSpeed(node, *neighbour, connection, (Node::Directions)i);
					node.m_groupProperties.at(getId()).m_fSpeed[(Node::Directions)i] = speed;
					float cost = calculateCost(node,  *neighbour, connection, (Node::Directions)i);
					node.m_groupProperties.at(getId()).m_fCost[(Node::Directions)i] = cost;

					if (speed == 0)
						continue;
					CandidateNode candidate = getCandidateNode(node, *neighbour);

					if (candidate.m_pNode != nullptr) 
						pushCandidateToHeap(candidate);
				}
			}
		}

		buildPotentialField();

		// For each node
		for (auto& chunk : m_pGrid->m_grid)
		{
			for (auto& node : chunk.m_nodes)
			{
				Vector4 potentialGradient = calculatePotentialGradient(node);
				Vector2 velocity = calculateTotVelocity(node, potentialGradient);
				assert(!isnan(velocity.x) && !isnan(velocity.y));
				
				node.m_groupProperties[getId()].reset(true);
				node.m_groupProperties.at(getId()).m_totVelocityX.store(velocity.x);
				node.m_groupProperties.at(getId()).m_totVelocityY.store(velocity.y);
			}
		}

		m_fMinVelocity = 0.f;
		m_fMinGradientVel = 0.f;
		m_fMaxGradientVel = 0.f;
	}

	void ContinuumCrowdsGroup::updateAgents(Pathfinder& pathfinder)
	{
		Transform* pathfinderTransform = pathfinder.GetTransform();
		if (pathfinderTransform == nullptr)
			return;

		// Update min/max velocity
		m_fMinVelocity = std::max(std::max(m_fMinVelocity, pathfinder.m_fMinVelocity), 0.f);
		m_fMinGradientVel = std::max(std::max(m_fMinGradientVel, pathfinder.m_fMaxVelocity), 0.f);
		m_fMaxGradientVel = m_fMinVelocity;

		// Seperation
		int agentCount = 0;
		Vector3 sperationVelocity = Vector3(0);
		Vector3 cohesionVelocity = Vector3(0);
		if (m_fMaxSeparationStrength != 0.f)
		{
			for (auto& agent : m_agents)
			{
				if (agent == &pathfinder || agent == nullptr)
					continue;

				Transform* transform = agent->GetTransform();
				if (transform == nullptr)
					continue;

				Vector3 headingVector = pathfinderTransform->GetPosition() - transform->GetPosition();
				float distance = glm::length2(headingVector);

				if (distance < m_fSeparationDistance * m_fSeparationDistance)
				{
					float strength = std::min(m_fMaxSeparationStrength, m_fSeparationCoefficient / (distance * distance));
					if (distance == 0)
						headingVector = Vector3(((float)rand() / (RAND_MAX)) * 2.f - 1.f, 0, ((float)rand() / (RAND_MAX)) * 2.f - 1.f);
					else
						headingVector /= distance;

					sperationVelocity += headingVector * strength;
					cohesionVelocity += transform->GetPosition();
					agentCount++;
				}
			}
		}

		if (agentCount > 0)
		{
			// Set flocking velocities
			cohesionVelocity /= (float)agentCount;
			cohesionVelocity -= pathfinderTransform->GetPosition();
			cohesionVelocity *= m_fCohesionStrength;
			if (!pathfinder.m_bCohesion)
				cohesionVelocity = Vector3(0);


			sperationVelocity /= (float)agentCount;
			pathfinder.m_flockVelocityX.store(sperationVelocity.x + cohesionVelocity.x);
			pathfinder.m_flockVelocityX.store(sperationVelocity.z + cohesionVelocity.z);
		}
	}

	void ContinuumCrowdsGroup::getDesiredVeclocityAndHeight(Vector2 & o_velocity, float & o_height, Node** o_node, const IVector3 & worldPos) const
	{
		assert(m_pGrid);

		// Get the containing chunk
		IVector2 chunkPos = Chunk::getChunkPos(worldPos);
		IVector2 gridPos = ChunkGrid::getGridPos(worldPos);
		Chunk* chunk = m_pGrid->getChunk(gridPos);

		if (chunk == nullptr)
			return;

		// Get the containing container
		chunkPos -= Chunk::getChunkPos(chunk->getWorldPos());
		NodeContainer* container = chunk->getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return;

		// Get the containing nodes
		float lowestDistance = g_INFINTE;
		Node* lowestNode = nullptr;
		for (auto& nodeIdx : container->m_container)
		{
			Node& node = chunk->m_nodes[nodeIdx.second];
			float distance = nodeIdx.first - worldPos.y;

			if (std::abs(distance) < std::abs(lowestDistance))
			{
				lowestDistance = distance;
				lowestNode = &node;
			}
		}

		// Set velcotiy and height
		if (lowestNode != nullptr)
		{
			o_height = (float)lowestNode->m_worldPos.y;
			o_velocity = Vector2(lowestNode->m_groupProperties[getId()].m_totVelocityX.load(),
								 lowestNode->m_groupProperties[getId()].m_totVelocityY.load());
		}

		if (o_node != nullptr)
			*o_node = lowestNode;
	}

	Vector2 ContinuumCrowdsGroup::getDesiredVeclocity(const IVector3 & worldPos) const
	{
		assert(m_pGrid);

		// Get the containing chunk
		IVector2 chunkPos = Chunk::getChunkPos(worldPos);
		IVector2 gridPos = ChunkGrid::getGridPos(worldPos);
		Chunk* chunk = m_pGrid->getChunk(gridPos);

		if (chunk == nullptr)
			return Vector2(0);

		// Get the containing container
		chunkPos -= Chunk::getChunkPos(chunk->getWorldPos());
		NodeContainer* container = chunk->getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return Vector2(0);

		// Get the containing nodes
		float lowestDistance = g_INFINTE;
		Node* lowestNode = nullptr;
		for (auto& nodeIdx : container->m_container)
		{
			Node& node = chunk->m_nodes[nodeIdx.second];
			float distance = nodeIdx.first - worldPos.y;

			if (distance < lowestDistance)
			{
				lowestDistance = distance;
				lowestNode = &node;
			}
		}

		// Set velcotiy and height
		if (lowestNode != nullptr)
		{
			return Vector2(lowestNode->m_groupProperties[getId()].m_totVelocityX.load(),
						   lowestNode->m_groupProperties[getId()].m_totVelocityY.load());
		}

		return Vector2(0);
	}

	float ContinuumCrowdsGroup::getDesiredHeight(const IVector3 & worldPos) const
	{
		assert(m_pGrid);

		// Get the containing chunk
		IVector2 chunkPos = Chunk::getChunkPos(worldPos);
		IVector2 gridPos = ChunkGrid::getGridPos(worldPos);
		Chunk* chunk = m_pGrid->getChunk(gridPos);

		if (chunk == nullptr)
			return 0.f;

		// Get the containing container
		chunkPos -= Chunk::getChunkPos(chunk->getWorldPos());
		NodeContainer* container = chunk->getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return 0.f;

		// Get the containing nodes
		float lowestDistance = g_INFINTE;
		Node* lowestNode = nullptr;
		for (auto& nodeIdx : container->m_container)
		{
			Node& node = chunk->m_nodes[nodeIdx.second];
			float distance = nodeIdx.first - worldPos.y;

			if (distance < lowestDistance)
			{
				lowestDistance = distance;
				lowestNode = &node;
			}
		}

		// Set velcotiy and height
		if (lowestNode != nullptr)
			return (float)lowestNode->m_worldPos.y;

		return -1.f;
	}

	void ContinuumCrowdsGroup::pushCandidateToHeap(CandidateNode candidate)
	{
		assert(candidate.m_pNode);

		candidate.m_pNode->m_groupProperties[getId()].m_considerationId = GroupNode::CANDIDATE;
		m_candidatesHeap.push_back(candidate);
		std::push_heap(m_candidatesHeap.begin(), m_candidatesHeap.end(), std::greater<CandidateNode>());
	}

	ContinuumCrowdsGroup::CandidateNode ContinuumCrowdsGroup::popCandidateFromHeap()
	{
		std::pop_heap(m_candidatesHeap.begin(), m_candidatesHeap.end(), std::greater<CandidateNode>());
		CandidateNode lowestCandidate = m_candidatesHeap.back();
		m_candidatesHeap.pop_back();
		return lowestCandidate;
	}

	GroupNode * ContinuumCrowdsGroup::getGoalNode(const PathfinderGoal & goal)
	{
		assert(m_pGrid);

		// Get the chunk that contains this goal
		IVector2 chunkPos = Chunk::getChunkPos(goal.getGoalWorldPos());
		IVector2 gridPos = ChunkGrid::getGridPos(goal.getGoalWorldPos());
		Chunk* chunk = m_pGrid->getChunk(gridPos);
		if (chunk == nullptr)
			return nullptr;

		// Get the container that contains this goal
		chunkPos -= Chunk::getChunkPos(chunk->getWorldPos());
		NodeContainer* container = chunk->getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return nullptr;

		float lowestDistance = std::numeric_limits<float>::max();
		GroupNode* lowestNode = nullptr;
		if (!goal.m_bProjectPosition)
			lowestDistance = Chunk::g_NODESIZE;

		// Get the containing nodes
		for (auto& nodeIdx : container->m_container)
		{
			float distance = std::abs((nodeIdx.first + Chunk::g_NODESIZE / 2.f) - goal.getGoalWorldPos().y);
			if (distance <= lowestDistance)
			{
				Node& node = chunk->m_nodes[nodeIdx.second];
				lowestNode = &node.m_groupProperties[getId()];
				if (!goal.m_bProjectPosition)
					break;
			}
		}

		// Goal position is not reachable
		return lowestNode;
	}

	float ContinuumCrowdsGroup::calculateSpeed(Node & fromNode, const Node & toNode, const NodeConnection & connection, Node::Directions direction)
	{
		assert(m_pGrid);

		// Calculate speed values
		float topoSpeed = getTopoSpeed(connection.m_fHeightGradient.load());
		float flowSpeed = getFlowSpeed(fromNode, toNode);
		float density = toNode.m_fDensity;

		float speed = 0.0f;
		float minDensity = m_pGrid->m_fMinDensity;
		float maxDensity = m_pGrid->m_fMaxDensity;

		// Interpolate speed values
		if (topoSpeed == 0)
			speed = 0;
		else if (density >= maxDensity)
			speed = flowSpeed;
		else if (density <= minDensity)
			speed = topoSpeed;
		else
			speed = topoSpeed + ((density - minDensity) / (maxDensity - minDensity)) * (flowSpeed - topoSpeed);

		assert(!isnan(speed));
		return speed;
	}

	float ContinuumCrowdsGroup::calculateCost(Node & fromNode, const Node & toNode, const NodeConnection & connection, Node::Directions direction)
	{
		// Assign cost field
		float speed = fromNode.m_groupProperties.at(getId()).m_fSpeed[direction];
		float discomfort = fromNode.m_fDiscomfort;

		// Calculate weighted cost
		if (speed == 0 || discomfort == PathfindingObstacle::g_INFINTE)
			return g_INFINTE;
		else
			return (m_fDistanceWeight * speed + m_fTimeWeight + m_fDiscomfortWeight * discomfort) / speed;
	}

	ContinuumCrowdsGroup::CandidateNode ContinuumCrowdsGroup::getCandidateNode(const Node & fromNode, Node & toNode)
	{
		// Mark goal cell neighbours as candidates
		if (fromNode.m_groupProperties.at(getId()).m_considerationId == GroupNode::KNOWN)
		{
			if (toNode.m_groupProperties[getId()].m_considerationId == GroupNode::UNKNOWN)
			{
				float tempPotential = getPotential(toNode);
				return CandidateNode{ tempPotential, &toNode };
			}
		}
		return CandidateNode();
	}

	void ContinuumCrowdsGroup::buildPotentialField()
	{
		assert(m_pGrid);

		// Fast marging algorithm
		while (m_candidatesHeap.size() > 0)
		{
			// Get lowest potential node
			CandidateNode lowestCandidate = popCandidateFromHeap();
			assert(lowestCandidate.m_pNode);
			Node& lowestNode = *lowestCandidate.m_pNode;
			GroupNode& lowestGroupNode = lowestCandidate.m_pNode->m_groupProperties.at(getId());

			// Heap element already processed
			if (lowestGroupNode.m_considerationId == GroupNode::KNOWN)
				continue;

			// New known
			lowestGroupNode.m_considerationId = GroupNode::KNOWN;
			lowestGroupNode.m_fPotential = lowestCandidate.m_tempPotential;

			// Mark neighbours candidate
			for (int i = 0; i < 4; i++)
			{
				// Not a valid direction
				if (lowestNode.m_groupProperties.at(getId()).m_fCost[(Node::Directions)i] == g_INFINTE)
					continue;

				// Get neighbour
				Node* neighbour = lowestNode.getNeighbourNode((Node::Directions)i, *m_pGrid);
				if (neighbour == nullptr)
					continue;
				GroupNode& neighbourGroupNode = neighbour->m_groupProperties.at(getId());

				if (neighbourGroupNode.m_considerationId != GroupNode::KNOWN)
				{
					float tempPotential = getPotential(*neighbour);
					pushCandidateToHeap(CandidateNode{ tempPotential, neighbour });
				}
			}
		}
	}

	Vector4 ContinuumCrowdsGroup::calculatePotentialGradient(const Node & node)
	{
		assert(m_pGrid);

		Vector4 potentialGradient(0);
		float potential = node.m_groupProperties.at(getId()).m_fPotential;

		// Get neighbours
		const Node* northNode = node.getNeighbourNode(Node::NORTH, *m_pGrid);
		const Node* eastNode =  node.getNeighbourNode(Node::EAST,  *m_pGrid);
		const Node* southNode = node.getNeighbourNode(Node::SOUTH, *m_pGrid);
		const Node* westNode =  node.getNeighbourNode(Node::WEST,  *m_pGrid);

		// Calculate the potential gradient
		if (northNode != nullptr && node.m_groupProperties.at(getId()).m_fSpeed[0] != 0) potentialGradient[Node::NORTH] = northNode->m_groupProperties.at(getId()).m_fPotential - potential;
		if (eastNode  != nullptr && node.m_groupProperties.at(getId()).m_fSpeed[1] != 0) potentialGradient[Node::EAST]  = eastNode->m_groupProperties.at( getId()).m_fPotential - potential;
		if (southNode != nullptr && node.m_groupProperties.at(getId()).m_fSpeed[2] != 0) potentialGradient[Node::SOUTH] = southNode->m_groupProperties.at(getId()).m_fPotential - potential;
		if (westNode  != nullptr && node.m_groupProperties.at(getId()).m_fSpeed[3] != 0) potentialGradient[Node::WEST]  = westNode->m_groupProperties.at( getId()).m_fPotential - potential;
		
		// Normalize the potential gradient
		float xGradient = potentialGradient[Node::EAST]  - potentialGradient[Node::WEST];
		float yGradient = potentialGradient[Node::NORTH] - potentialGradient[Node::SOUTH];
		Vector2 newGradient = glm::normalize(Vector2(xGradient, yGradient));

		// Adjust gradients to normalize
		float xMul = 1.0f;
		if (xGradient != 0.0f)
			xMul = newGradient.x / xGradient;
		float yMul = 1.0f;
		if (yGradient != 0.0f)
			yMul = newGradient.y / yGradient;

		potentialGradient[Node::NORTH] *= yMul;
		potentialGradient[Node::EAST]  *= xMul;
		potentialGradient[Node::SOUTH] *= yMul;
		potentialGradient[Node::WEST]  *= xMul;

		return potentialGradient;
	}

	Vector2 ContinuumCrowdsGroup::calculateTotVelocity(Node & node, const Vector4 & potentialGrad)
	{
		assert(m_pGrid);

		float northVel = 0.0f;
		float eastVel = 0.0f;
		float southVel = 0.0f;
		float westVel = 0.0f;

		if (node.hasNeighbourNode(Node::NORTH)) northVel = potentialGrad[Node::NORTH] * -node.m_groupProperties.at(getId()).m_fSpeed[Node::NORTH];
		if (node.hasNeighbourNode(Node::EAST )) eastVel =  potentialGrad[Node::EAST]  * -node.m_groupProperties.at( getId()).m_fSpeed[Node::EAST];
		if (node.hasNeighbourNode(Node::SOUTH)) southVel = potentialGrad[Node::SOUTH] * -node.m_groupProperties.at(getId()).m_fSpeed[Node::SOUTH];
		if (node.hasNeighbourNode(Node::WEST )) westVel =  potentialGrad[Node::WEST]  * -node.m_groupProperties.at( getId()).m_fSpeed[Node::WEST];

		return Vector2(eastVel - westVel, northVel - southVel);
	}

	float ContinuumCrowdsGroup::getTopoSpeed(float heighGradient) const
	{
		// Impossible to move to
		if (heighGradient > m_fMaxHeightGradient)
			return 0;

		// Flat world
		float heightDif = m_fMaxHeightGradient - m_fMinHeightGradient;
		float velocityDif = m_fMinGradientVel - m_fMaxGradientVel;
		if (heightDif == 0 || velocityDif == 0)
			return m_fMaxGradientVel;

		// Interpolate between the min and max velocity at the min and max gradient.
		float percentage = ((heighGradient - m_fMinHeightGradient) / heightDif);
		return m_fMaxGradientVel + percentage * velocityDif;
	}

	float ContinuumCrowdsGroup::getFlowSpeed(const Node & fromNode, const Node & toNode) const
	{
		// Claculate if the movement aligns with the crowds direction.
		Vector2 offset = Vector2(toNode.m_worldPos.x - fromNode.m_worldPos.x,
								 toNode.m_worldPos.z - fromNode.m_worldPos.z);
		offset = glm::normalize(offset);

		float flowSpeed = glm::dot(toNode.m_avgVelocity, offset);
		return std::max(flowSpeed, m_fMinVelocity);
	}

	float ContinuumCrowdsGroup::getPotential(Node & node)
	{
		assert(m_pGrid);

		// ArgMin on the x-axis
		Node::Directions xMinDir;
		Node::Directions zMinDir;
		Node* xMin = node.getPotentialArgMin(true, *this, *m_pGrid, xMinDir);
		Node* zMin = node.getPotentialArgMin(false, *this, *m_pGrid, zMinDir);

		// Calculate the finite difference
		assert(xMin != nullptr || zMin != nullptr);
		if (xMin == nullptr) return getFiniteDifference(node, *zMin, zMinDir);
		else if (zMin == nullptr) return getFiniteDifference(node, *xMin, xMinDir);
		else return getFiniteDifference(node, *xMin, xMinDir, *zMin, zMinDir);
	}

	float ContinuumCrowdsGroup::getFiniteDifference(const Node & fromNode, const Node & toNode, Node::Directions direction) const
	{
		// Simplified single finite difference for calculating potential
		float potential = toNode.m_groupProperties.at(getId()).m_fPotential;
		float arg1 = potential + fromNode.m_groupProperties.at(getId()).m_fCost[direction];
		float arg2 = potential - fromNode.m_groupProperties.at(getId()).m_fCost[direction];
		return std::max(arg1, arg2);
	}

	float ContinuumCrowdsGroup::getFiniteDifference(const Node & fromNode, const Node & xToNode, Node::Directions xDirection, const Node & zToNode, Node::Directions zDirection) const
	{
		// Get cost and potential
		float costX = fromNode.m_groupProperties.at(getId()).m_fCost[xDirection];
		float costZ = fromNode.m_groupProperties.at(getId()).m_fCost[zDirection];
		float potentialX = xToNode.m_groupProperties.at(getId()).m_fPotential;
		float potentialZ = zToNode.m_groupProperties.at(getId()).m_fPotential;

		// Build equation
		float a = (costX * costX) + (costZ * costZ);
		float b = -2.0f * ((costX * costX * potentialZ) + (costZ * costZ * potentialX));
		float c = (costX * costX * potentialZ * potentialZ) + (costZ * costZ * potentialX * potentialX) - (costX * costX * costZ * costZ);
		float discriminant = (b * b) - (4 * a * c);

		// Discriminant < 0, there are no solutions
		if (discriminant >= 0)
		{
			float discriminantSqrt = std::sqrt(discriminant);
			float arg1 = (-b + discriminantSqrt) / (2.0f * a);
			float arg2 = (-b - discriminantSqrt) / (2.0f * a);

			float potential = std::max(arg1, arg2);
			if (potential >= potentialX && potential >= potentialZ)
				return potential;
		}

		// If there is no solution or the potential is greater than it's neighbours,
		// return the lowest potential direction
		if (potentialX < potentialZ)
			return getFiniteDifference(fromNode, xToNode, xDirection);
		else
			return getFiniteDifference(fromNode, zToNode, zDirection);
	}
}