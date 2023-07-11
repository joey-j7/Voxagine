#include "pch.h"
#include "PathfindingNode.h"

#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"

namespace pathfinding
{
#pragma region NodeConnection
	NodeConnection::NodeConnection() :
		m_toNodeWorldPosX(-1),
		m_toNodeWorldPosY(-1),
		m_toNodeWorldPosZ(-1),
		m_bConnected(0),
		m_fHeightGradient(0.f)
	{}

	NodeConnection::NodeConnection(const Node& fromNode, const Node& toNode) :
		m_toNodeWorldPosX(toNode.m_worldPos.x),
		m_toNodeWorldPosY(toNode.m_worldPos.y),
		m_toNodeWorldPosZ(toNode.m_worldPos.z),
		m_bConnected(1),
		m_fHeightGradient((float)(toNode.m_worldPos.y - fromNode.m_worldPos.y))
	{}

	NodeConnection::NodeConnection(const NodeConnection & other)
	{
		m_toNodeWorldPosX.store(other.m_toNodeWorldPosX.load());
		m_toNodeWorldPosY.store(other.m_toNodeWorldPosY.load());
		m_toNodeWorldPosZ.store(other.m_toNodeWorldPosZ.load());
		m_bConnected.store(other.m_bConnected.load());
		m_fHeightGradient.store(other.m_fHeightGradient.load());
	}

	NodeConnection & NodeConnection::operator=(const NodeConnection & other)
	{
		m_toNodeWorldPosX.store(other.m_toNodeWorldPosX.load());
		m_toNodeWorldPosY.store(other.m_toNodeWorldPosY.load());
		m_toNodeWorldPosZ.store(other.m_toNodeWorldPosZ.load());
		m_bConnected.store(other.m_bConnected.load());
		m_fHeightGradient.store(other.m_fHeightGradient.load());
		return *this;
	}

	bool NodeConnection::getConnected() const
	{
		return m_bConnected.load() != 0;
	}

	IVector3 NodeConnection::getToNodeWorldPos() const
	{
		return IVector3(m_toNodeWorldPosX.load(), m_toNodeWorldPosY.load(), m_toNodeWorldPosZ.load());
	}

	Node * NodeConnection::getToNode(ChunkGrid& grid) const
	{
		if (!getConnected())
			return nullptr;
		return grid.getNode(getToNodeWorldPos());
	}

	void NodeConnection::connect(const Node& fromNode, const Node& toNode)
	{
		m_toNodeWorldPosX.store(toNode.m_worldPos.x);
		m_toNodeWorldPosY.store(toNode.m_worldPos.y);
		m_toNodeWorldPosZ.store(toNode.m_worldPos.z);
		m_bConnected.store(1);
		m_fHeightGradient.store((float)(toNode.m_worldPos.y - fromNode.m_worldPos.y));
	}

	void NodeConnection::disconnect()
	{
		m_bConnected.store(0);
	}
#pragma endregion

#pragma region GroupNode
	GroupNode::GroupNode() :
		m_considerationId(UNKNOWN),
		m_bIsGoal(false),
		m_fSpeed{0, 0, 0, 0},
		m_fCost{0, 0, 0, 0},
		m_fPotential(ContinuumCrowdsGroup::g_INFINTE),
		m_totVelocityX(0),
		m_totVelocityY(0)
	{}

	GroupNode::GroupNode(const GroupNode & other) :
		m_considerationId(other.m_considerationId),
		m_bIsGoal(other.m_bIsGoal),
		m_fPotential(other.m_fPotential)
	{
		std::copy(other.m_fSpeed, other.m_fSpeed + 4, m_fSpeed);
		std::copy(other.m_fCost, other.m_fCost + 4, m_fCost);
	
		m_totVelocityX.store(other.m_totVelocityX.load());
		m_totVelocityY.store(other.m_totVelocityY.load());
	}
	
	GroupNode & GroupNode::operator=(const GroupNode & other)
	{
		m_considerationId = other.m_considerationId;
		m_bIsGoal = other.m_bIsGoal;
		std::copy(other.m_fSpeed, other.m_fSpeed + 4, m_fSpeed);
		std::copy(other.m_fCost, other.m_fCost + 4, m_fCost);
		m_fPotential = other.m_fPotential;
	
		m_totVelocityX.store(other.m_totVelocityX.load());
		m_totVelocityY.store(other.m_totVelocityY.load());

		return *this;
	}

	void GroupNode::setGoal(bool isGoal, float potential)
	{
		if (isGoal)
		{
			m_considerationId = KNOWN;
			m_bIsGoal = true;
			m_fPotential = potential;
		} else
		{
			m_considerationId = UNKNOWN;
			m_bIsGoal = false;
			m_fPotential = ContinuumCrowdsGroup::g_INFINTE;
		}
	}

	void GroupNode::reset(bool resetGoal)
	{
		if (resetGoal)
			m_bIsGoal = false;
		if (!m_bIsGoal)
		{
			m_considerationId = UNKNOWN;
		}
	}
#pragma endregion

#pragma region Node
	Node::Node(Chunk& parentChunk, IVector3 worldPos) :
		m_parentChunkPos(parentChunk.getGridPos()),
		m_worldPos(worldPos),
		m_fDensity(0.f),
		m_fDiscomfort(0.f),
		m_avgVelocity(0.f, 0.f)
	{}

	void Node::reset()
	{
		m_fDensity = 0.f;
		m_avgVelocity = Vector2(0.f);
		m_fDiscomfort = 0.f;
	}

	void Node::setNeighBourNode(Directions direction, const Node & toNode)
	{
		m_connections[direction].connect(*this, toNode);
	}

	Node * Node::getNeighbourNode(Directions direction, ChunkGrid& grid) const
	{
		return m_connections[direction].getToNode(grid);
	}

	bool Node::hasNeighbourNode(Directions direction) const
	{
		return m_connections[direction].getConnected();
	}

	Chunk * Node::getParentChunk(ChunkGrid& grid) const
	{
		return grid.getChunk(m_parentChunkPos);
	}

	Node * Node::getPotentialArgMin(bool xAxis, const PathfinderGroup& group, ChunkGrid & grid, Directions& o_direction)
	{
		// Get nodes along axis
		Node* node1;
		Node* node2;
		float speed1;
		float speed2;
		if (xAxis)
		{
			node1  = getNeighbourNode(Node::EAST, grid);
			node2  = getNeighbourNode(Node::WEST, grid);
			speed1 = m_groupProperties[group.getId()].m_fSpeed[Node::EAST];
			speed2 = m_groupProperties[group.getId()].m_fSpeed[Node::WEST];
		} else
		{
			node1  = getNeighbourNode(Node::NORTH, grid);
			node2  = getNeighbourNode(Node::SOUTH, grid);
			speed1 = m_groupProperties[group.getId()].m_fSpeed[Node::NORTH];
			speed2 = m_groupProperties[group.getId()].m_fSpeed[Node::SOUTH];
		}
		Node* returnNode = nullptr;

		// Check if the nodes are valid. 
		// Operator [] is used for the rare case where no groupproperties have been assigned to the neighbour yet.
		bool node1Valid = (node1 != nullptr && node1->m_groupProperties[group.getId()].m_considerationId == GroupNode::KNOWN && speed1 != ContinuumCrowdsGroup::g_INFINTE);
		bool node2Valid = (node2 != nullptr && node2->m_groupProperties[group.getId()].m_considerationId == GroupNode::KNOWN && speed2 != ContinuumCrowdsGroup::g_INFINTE);

		// Argmin
		if (node1Valid && node2Valid)
		{
			float node1Potential = node1->m_groupProperties.at(group.getId()).m_fPotential;
			float node2Potential = node2->m_groupProperties.at(group.getId()).m_fPotential;

			if (node1Potential < node2Potential) returnNode = node1;
			else returnNode = node2;
		}
		else if (node1Valid) returnNode = node1;
		else if (node2Valid) returnNode = node2;

		// Determin direction
		if (returnNode == node1) o_direction = xAxis ? Node::EAST : Node::NORTH;
		else if (returnNode == node2) o_direction = xAxis ? Node::WEST : Node::SOUTH;
		return returnNode;
	}
#pragma endregion
}