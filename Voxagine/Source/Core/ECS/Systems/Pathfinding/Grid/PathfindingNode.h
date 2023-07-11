#pragma once
#include <memory>
#include <array>
#include <unordered_map>
#include <atomic>
#include "Core/Math.h"

namespace pathfinding
{
	class PathfinderGroup;
	class ChunkGrid;
	class Chunk;
	struct Node;

	// A connection between 2 nodes. 
	// Contains information about the connection, shared between all ai agents.
	struct NodeConnection
	{
		// Used to lookup the to node.
		std::atomic<int> m_toNodeWorldPosX;
		std::atomic<int> m_toNodeWorldPosY;
		std::atomic<int> m_toNodeWorldPosZ;

		std::atomic<int> m_bConnected;
		std::atomic<float> m_fHeightGradient;

		NodeConnection();
		NodeConnection(const Node& fromNode, const Node& toNode); // Does not insert itself into the from node
		NodeConnection(const NodeConnection& other);
		NodeConnection& operator=(const NodeConnection& other);

		bool getConnected() const;
		IVector3 getToNodeWorldPos() const;
		Node* getToNode(ChunkGrid& grid) const;
		void connect(const Node& fromNode, const Node& toNode);
		void disconnect();
	};

	// Contains information about a node, specific to a group of ai agents.
	struct GroupNode
	{
		// Used by the fast marging algorithm to determin if a node is evaluated/being evaluated.
		enum ConsiderationId
		{
			UNKNOWN = 0,
			CANDIDATE,
			KNOWN
		};

		ConsiderationId m_considerationId;
		bool m_bIsGoal;
		float m_fSpeed[4];
		float m_fCost[4];
		float m_fPotential;
		std::atomic<float> m_totVelocityX;
		std::atomic<float> m_totVelocityY;

		GroupNode();
		GroupNode(const GroupNode& other);
		GroupNode& operator=(const GroupNode& other);

		void setGoal(bool isGoal, float potential = 0);
		void reset(bool resetGoal = true);
	};

	struct Node
	{
		enum Directions
		{
			NORTH = 0,
			EAST,
			SOUTH,
			WEST
		};

		IVector2 m_parentChunkPos; // Used to lookup the parent chunk.
		std::array<NodeConnection, 4> m_connections;
		std::unordered_map<int, GroupNode> m_groupProperties;

		// Node properties shared by all ai agents
		IVector3 m_worldPos;
		float m_fDensity;
		float m_fDiscomfort;
		Vector2 m_avgVelocity;

		Node(Chunk& parentChunk, IVector3 worldPos);
		void reset();

		void setNeighBourNode(Directions direction, const Node& toNode);
		Node* getNeighbourNode(Directions direction, ChunkGrid& grid) const;
		bool hasNeighbourNode(Directions direction) const;

		Chunk* getParentChunk(ChunkGrid& grid) const;
		
		// Does an arg min on the given axis (x or z).
		// Returns the min node and the direction of this node.
		Node* getPotentialArgMin(bool xAxis, const PathfinderGroup& group, ChunkGrid& grid, Directions& o_direction);
	};
}