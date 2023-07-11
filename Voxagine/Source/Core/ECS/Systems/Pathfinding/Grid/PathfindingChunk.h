#pragma once
#include <vector>
#include <unordered_map>
#include "Core/Math.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingNode.h"

class DebugRenderer;
class VoxelGrid;

namespace pathfinding
{
	// A connection leaving the chunk, from node wolrd pos <-> direction leaving the chunk.
	typedef std::pair<IVector3, Node::Directions> ChunkConnection;
	typedef std::vector<ChunkConnection> ChunkConnections;

	// A container of height <-> node id. All nodes in a conatianer are at the same x-z position.
	struct NodeContainer
	{
		std::vector<std::pair<int, int>> m_container;

		NodeContainer() : m_container() {};

		void pushBack(int height, int nodeId) { m_container.emplace_back(height, nodeId); }
		void reset(int reserveCount = 0) { m_container.clear(); m_container.reserve(reserveCount); }
		size_t getSize() const { return m_container.size(); }

		int findNodeId(int height)
		{
			auto nodeIt = std::lower_bound(m_container.begin(), m_container.end(), std::make_pair(height, std::numeric_limits<int>::min()));
			if (nodeIt == m_container.end() || nodeIt->first != height)
				return -1;
			return nodeIt->second;
		}
		std::vector<std::pair<int, int>>::iterator find(int height)
		{
			auto nodeIt = std::lower_bound(m_container.begin(), m_container.end(), std::make_pair(height, std::numeric_limits<int>::min()));
			if (nodeIt == m_container.end() || nodeIt->first != height)
				return m_container.end();
			return nodeIt;
		}
	};

	class Chunk
	{
	public:
		friend class PathfinderGroup;
		friend class ChunkGrid;

		static const unsigned int g_NODESIZE = 16;
		static const unsigned int g_CHUNKSIZE = 12;

		bool m_bGenerateVerticalNodes;
		std::vector<Node> m_nodes;
		std::array<NodeContainer, g_CHUNKSIZE * g_CHUNKSIZE> m_nodeGridLookup;

	private:
		IVector3 m_worldPos;
		ChunkGrid* m_parentGrid;

	public:
		Chunk();
		Chunk(ChunkGrid& grid, IVector3 worldPos, bool generateVerticalNodes, bool rebuildNodes = true);
		void DebugDraw(DebugRenderer& renderer, unsigned int groupId);

		IVector3 getWorldPos() const;
		IVector2 getGridPos() const;
		// Get the position inside the chunk array. Relative to the world 0,0.
		static IVector2 getChunkPos(IVector3 nodeWorldPos);

		// Reconstruct the grid (or a position in the grid). ChunkConnections contain all the possible connections going 
		// outside this class. These can be connected later by connectChunkNeighbours method. This is used by 
		// multithreading to connect a chunk into the grid later.
		void rebuild(const VoxelGrid& voxelGrid, ChunkConnections* chunkConnections = nullptr);
		void rebuild(const VoxelGrid& voxelGrid, IVector2 chunkPos, ChunkConnections* chunkConnections = nullptr);
		void connectChunkNeighbours(const VoxelGrid & voxelGrid, ChunkConnections& chunkConnections);

		NodeContainer* getNodeContainer(int chunkPosX, int chunkPosZ);
		Node* getNodeInChunk(IVector3 nodeWorldPos);

		// This is not called on deconstruction automatically. 
		// Destroy is only needed to remove a chunk from the grid, without destroying the other chunks in the grid.
		void destroyNodeInChunk(Node& node);
		void destroyNodeConnection(IVector3 nodeWorldPos, Node::Directions direction);
		void destroyNodeConnection(Node& node, Node::Directions direction); // Destroys the connection in both ways.
		void destroy();

		// Is the node fully air
		bool isNodePosAir(unsigned int& o_voxelsToSkip, const VoxelGrid& voxelGrid, IVector3 nodePos, unsigned int height = g_NODESIZE) const;
		// Is the space between 2 nodes fully air
		bool canDropBetweenNodes(const VoxelGrid& voxelGrid, const Node& node1, const Node& node2) const;

	private:
		void makeNodeNeighbours(const VoxelGrid& voxelGrid, Node& node, Node::Directions direction, ChunkConnections* chunkConnections = nullptr);
		void connectNodeNeighbours(const VoxelGrid & voxelGrid, Node & node, Node::Directions neighbourDir, const IVector3& neighbourPos, Chunk& neighbourChunk);
	};
}