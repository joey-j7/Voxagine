#include "pch.h"
#include "PathfindingChunk.h"

#include "Core/ECS/Systems/Rendering/DebugRenderer.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "External/optick/optick.h"

namespace pathfinding
{
	Chunk::Chunk() :
		m_worldPos(-1, -1, -1),
		m_parentGrid(nullptr),
		m_bGenerateVerticalNodes(true)
	{}

	Chunk::Chunk(ChunkGrid& grid, IVector3 worldPos, bool generateVerticalNodes, bool rebuildNodes) :
		m_parentGrid(&grid),
		m_bGenerateVerticalNodes(generateVerticalNodes)
	{
		m_worldPos.x = ChunkGrid::getGridPos(worldPos).x * (int)(g_CHUNKSIZE * g_NODESIZE);
		m_worldPos.z = ChunkGrid::getGridPos(worldPos).y * (int)(g_CHUNKSIZE * g_NODESIZE);
		m_worldPos.y = 0;

		m_nodeGridLookup.fill(NodeContainer());

		if (rebuildNodes)
		{
			PhysicsSystem* physicsSystem = grid.GetWorld()->GetPhysics();
			VoxelGrid* voxelGrid = physicsSystem->GetVoxelGrid();
			rebuild(*voxelGrid);
		}
	}

	void Chunk::DebugDraw(DebugRenderer& renderer, unsigned int groupId)
	{
		if (m_parentGrid == nullptr)
			return;

		VColor color;
		for (auto& node : m_nodes)
		{
			// Render node connections
			color = VColors::Red;
			Node* northNode = node.getNeighbourNode(Node::NORTH, *m_parentGrid);
			Node* eastNode = node.getNeighbourNode(Node::EAST, *m_parentGrid);
			Node* southNode = node.getNeighbourNode(Node::SOUTH, *m_parentGrid);
			Node* westNode = node.getNeighbourNode(Node::WEST, *m_parentGrid);

			// DO NOT UNCOMMENT, CAUSES MEMORY LEAKS
			//if (northNode != nullptr) renderer.AddLine(node.m_worldPos, northNode->m_worldPos, color);
			//if (eastNode != nullptr ) renderer.AddLine(node.m_worldPos, eastNode->m_worldPos, color);
			//if (southNode != nullptr) renderer.AddLine(node.m_worldPos, southNode->m_worldPos, color);
			//if (westNode != nullptr ) renderer.AddLine(node.m_worldPos, westNode->m_worldPos, color);

			// Render node potential
			float value = node.m_groupProperties[groupId].m_fPotential / 50.f;
			color = VColor(value, value, value, 1);
			renderer.AddCenteredBox(node.m_worldPos + IVector3(g_NODESIZE / 2), Vector3(g_NODESIZE, 1, g_NODESIZE), color);
		}
		renderer.AddCenteredBox(m_worldPos + IVector3(g_CHUNKSIZE * g_NODESIZE / 2), IVector3(g_CHUNKSIZE * g_NODESIZE));
	}

	IVector3 Chunk::getWorldPos() const
	{
		return m_worldPos;
	}

	IVector2 Chunk::getGridPos() const
	{
		return ChunkGrid::getGridPos(m_worldPos);
	}

	IVector2 Chunk::getChunkPos(IVector3 nodeWorldPos)
	{
		return IVector2(std::floor((float)nodeWorldPos.x / (float)Chunk::g_NODESIZE),
						std::floor((float)nodeWorldPos.z / (float)Chunk::g_NODESIZE));
	}

	void Chunk::rebuild(const VoxelGrid& voxelGrid, ChunkConnections* chunkConnections)
	{
		// Break old connections
		for (auto& node : m_nodes)
		{
			for (int i = 0; i < 4; i++)
				destroyNodeConnection(node, (Node::Directions)i);
		}

		// Clear old chunk and reserve new chunk
		for (auto& nodeLookup : m_nodeGridLookup)
			nodeLookup.reset(g_NODESIZE * g_NODESIZE * 2);
		m_nodes.clear();
		m_nodes.reserve(g_NODESIZE * g_NODESIZE * 2);


		// Reserve memory for chunk connections
		if (chunkConnections != nullptr)
			chunkConnections->reserve(g_CHUNKSIZE * 4);

		// Rebuild the grid for each chunk pos
		for (int chunkZ = 0; chunkZ < g_CHUNKSIZE; chunkZ++)
		{
			for (int chunkX = 0; chunkX < g_CHUNKSIZE; chunkX++)
			{
				rebuild(voxelGrid, IVector2(chunkX, chunkZ), chunkConnections);
			}
		}
	}

	void Chunk::rebuild(const VoxelGrid & voxelGrid, IVector2 chunkPos, ChunkConnections* chunkConnections)
	{
		// Delete old nodes
		NodeContainer* container = getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return;

		while (container->getSize() > 0)
			destroyNodeInChunk(m_nodes[container->m_container.begin()->second]);
		container->reset(1);

		// Is this chunk pos inside the voxel grid
		IVector2 chunkPosWorld = IVector2(m_worldPos.x + chunkPos.x * g_NODESIZE, m_worldPos.z + chunkPos.y * g_NODESIZE);
		if (voxelGrid.WorldToGrid(IVector3(chunkPosWorld.x, 1, chunkPosWorld.y)) == Vector3(-1) &&
			voxelGrid.WorldToGrid(IVector3(chunkPosWorld.x + g_NODESIZE, 1, chunkPosWorld.y + g_NODESIZE)) == Vector3(-1))
			return;

		// Find all walkable nodes
		bool isPrevNodeWalkable = true;
		for (int y = 1; y < (int)voxelGrid.GetDimensions().y;) // First voxel is always solid
		{
			unsigned int voxelsToSkip = 1;
			IVector3 nodePos = IVector3(chunkPosWorld.x, y, chunkPosWorld.y);

			// Is this y-level fully air
			bool isWalkable = isNodePosAir(voxelsToSkip, voxelGrid, nodePos);

			if (isPrevNodeWalkable && isWalkable)
			{
				// Create new node
				m_nodes.emplace_back(*this, nodePos);
				m_nodeGridLookup[chunkPos.y  * g_CHUNKSIZE + chunkPos.x].pushBack(nodePos.y, (int)m_nodes.size() - 1);
				makeNodeNeighbours(voxelGrid, m_nodes.back(), Node::NORTH, chunkConnections);
				makeNodeNeighbours(voxelGrid, m_nodes.back(), Node::EAST, chunkConnections);
				makeNodeNeighbours(voxelGrid, m_nodes.back(), Node::SOUTH, chunkConnections);
				makeNodeNeighbours(voxelGrid, m_nodes.back(), Node::WEST, chunkConnections);

				// Should we only generate 1 node per chunkPos?
				if (!m_bGenerateVerticalNodes)
					break;
			}

			// Update walkable and skip to next node
			isPrevNodeWalkable = !isWalkable;
			y += voxelsToSkip;
		}
	}

	void Chunk::connectChunkNeighbours(const VoxelGrid & voxelGrid, ChunkConnections & chunkConnections)
	{
		for (auto& chunkConnection : chunkConnections)
		{
			Node* node = getNodeInChunk(chunkConnection.first);
			if (node != nullptr)
				makeNodeNeighbours(voxelGrid, *node, chunkConnection.second);
		}
	}

	NodeContainer * Chunk::getNodeContainer(int chunkPosX, int chunkPosZ)
	{
		if (chunkPosX < 0 || chunkPosX >= g_CHUNKSIZE ||
			chunkPosZ < 0 || chunkPosZ >= g_CHUNKSIZE)
			return nullptr;
		return &m_nodeGridLookup[chunkPosZ * g_CHUNKSIZE + chunkPosX];
	}

	Node * Chunk::getNodeInChunk(IVector3 nodeWorldPos)
	{
		// Get container
		IVector2 chunkPos = getChunkPos(nodeWorldPos - m_worldPos);
		NodeContainer* container = getNodeContainer(chunkPos.x, chunkPos.y);
		if (container == nullptr)
			return nullptr;

		// Find node in container
		int nodeId = container->findNodeId(nodeWorldPos.y);
		if (nodeId == -1)
			return nullptr;

		return &m_nodes[nodeId];
	}

	void Chunk::destroyNodeInChunk(Node & node)
	{
		// Break connections
		if (m_parentGrid != nullptr)
		{
			for (int i = 0; i < 4; i++)
				destroyNodeConnection(node, (Node::Directions)i);
		}

		// Get container
		IVector2 chunkPos = getChunkPos(node.m_worldPos - m_worldPos);
		NodeContainer* container = getNodeContainer(chunkPos.x, chunkPos.y);
		auto nodeLookup = container->find(node.m_worldPos.y);
		int nodeId = nodeLookup->second;
		assert(nodeId != -1);

		Node& lastNode = m_nodes.back();
		if (m_nodes.size() > 1)
		{
			// Get last node container
			IVector2 lastChunkPos = getChunkPos(lastNode.m_worldPos - m_worldPos);
			NodeContainer* lastContainer = getNodeContainer(lastChunkPos.x, lastChunkPos.y);
			auto lastNodeLookup = lastContainer->find(lastNode.m_worldPos.y);
			int lastNodeId = m_nodes.size() - 1;
			assert(lastContainer);

			// Swap nodes in vector
			std::iter_swap(m_nodes.begin() + nodeId, m_nodes.begin() + lastNodeId);
			lastNodeLookup->second = nodeId;
		}

		// Remove node
		container->m_container.erase(nodeLookup);
		m_nodes.pop_back();
	}

	void Chunk::destroyNodeConnection(Node & node, Node::Directions direction)
	{
		if (node.hasNeighbourNode(direction))
		{
			NodeConnection& oldConnection = node.m_connections[direction];
			Node* oldNeighbour = getNodeInChunk(oldConnection.getToNodeWorldPos());

			// Attempt to get old neighbour
			if (oldNeighbour == nullptr)
				oldNeighbour = node.getNeighbourNode(direction, *m_parentGrid);

			// Connection can't be broken
			assert(oldNeighbour != nullptr);

			// Break connection
			node.m_connections[direction].disconnect();
			oldNeighbour->m_connections[(Node::Directions)((direction + 2) % 4)].disconnect();
		}
	}

	void Chunk::destroy()
	{
		// Break connections
		for (auto& node : m_nodes)
		{
			for (int i = 0; i < 4; i++)
				destroyNodeConnection(node, (Node::Directions)i);
		}

		// Clear old chunk and reserve new chunk
		for (auto& nodeLookup : m_nodeGridLookup)
			nodeLookup.reset(1);
		m_nodes.clear();
	}

	bool Chunk::isNodePosAir(unsigned int& o_voxelsToSkip, const VoxelGrid& voxelGrid, IVector3 nodePos, unsigned int height) const
	{
		// Check each layer of voxels
		for (int y = height - 1; y >= 0; y -= m_parentGrid->m_iGridCoarseness)
		{
			for (int z = 0; z < g_NODESIZE; z += m_parentGrid->m_iGridCoarseness)
			{
				for (int x = 0; x < g_NODESIZE; x += m_parentGrid->m_iGridCoarseness)
				{
					IVector3 voxelPos = nodePos + IVector3(x, y, z);

					// Is the voxel air?
					Vector3 voxelIdx = voxelGrid.WorldToGrid(voxelPos, true);
					const Voxel* voxel = voxelGrid.GetVoxel(voxelIdx.x, voxelIdx.y, voxelIdx.z);
					if (voxel == nullptr || voxel->Active == true)
					{
						o_voxelsToSkip = y + 1;
						return false;
					}
				}
			}
		}
		o_voxelsToSkip = g_NODESIZE;
		return true;
	}

	bool Chunk::canDropBetweenNodes(const VoxelGrid & voxelGrid, const Node & node1, const Node & node2) const
	{
		// Are the nodes cardinal neighbours
		IVector3 distance = node1.m_worldPos - node2.m_worldPos;
		if (glm::length(Vector2(distance.x, distance.z)) != g_NODESIZE)
			return false;

		// Get direction
		Node::Directions direction;
		if (distance.y > 0) direction = Node::NORTH;
		else if (distance.x > 0) direction = Node::EAST;
		else if (distance.y < 0) direction = Node::SOUTH;
		else if (distance.x < 0) direction = Node::WEST;

		// Get top and bottom node
		const Node* topNode = &node1;
		const Node* bottomNode = &node2;
		if (node1.m_worldPos.y < node2.m_worldPos.y)
		{
			const Node* temp = topNode;
			topNode = bottomNode;
			bottomNode = temp;
		}

		// Is air between nodes
		unsigned int temp;
		IVector3 nodePos = bottomNode->m_worldPos + IVector3(0, g_NODESIZE, 0);
		unsigned int height = topNode->m_worldPos.y - bottomNode->m_worldPos.y;
		bool isAir = isNodePosAir(temp, voxelGrid, nodePos, height);

		return isAir;
	}

	void Chunk::makeNodeNeighbours(const VoxelGrid & voxelGrid, Node & node, Node::Directions direction, ChunkConnections* chunkConnections)
	{
		// Needs a neighbour
		if (node.hasNeighbourNode(direction))
			return;

		// Get offset
		IVector3 neighbourPos;
			 if (direction == Node::NORTH) neighbourPos = node.m_worldPos + IVector3(0, 0, (int)g_NODESIZE);
		else if (direction == Node::EAST) neighbourPos = node.m_worldPos  + IVector3((int)g_NODESIZE, 0, 0);
		else if (direction == Node::SOUTH) neighbourPos = node.m_worldPos + IVector3(0, 0, -(int)g_NODESIZE);
		else if (direction == Node::WEST) neighbourPos = node.m_worldPos  + IVector3(-(int)g_NODESIZE, 0, 0);

		// Get chunk
		Chunk* chunk = nullptr;
		IVector2 gridPos = ChunkGrid::getGridPos(neighbourPos);

		if (gridPos == getGridPos())
			chunk = this;
		else
		{
			// Should we attempt to get the chunk from the grid?
			if (chunkConnections == nullptr)
				chunk = m_parentGrid->getChunk(gridPos);
			else
			{
				chunkConnections->push_back(std::make_pair(node.m_worldPos, direction));
				return;
			}
		}

		if (chunk == nullptr)
			return;

		connectNodeNeighbours(voxelGrid, node, direction, neighbourPos, *chunk);
	}

	void Chunk::connectNodeNeighbours(const VoxelGrid & voxelGrid, Node & node, Node::Directions neighbourDir, const IVector3& neighbourPos, Chunk& neighbourChunk)
	{
		// Get potential neighbours container
		IVector2 neighbourChunkPos = getChunkPos(neighbourPos - neighbourChunk.m_worldPos);
		const NodeContainer* container = neighbourChunk.getNodeContainer(neighbourChunkPos.x, neighbourChunkPos.y);
		if (container == nullptr)
			return;

		// Test all potential neighbours
		// Get the closest below and above height nodes
		for (auto& neighbourNodeIdx : container->m_container)
		{
			Node& neighbourNode = neighbourChunk.m_nodes[neighbourNodeIdx.second];
			int height = node.m_worldPos.y - neighbourNode.m_worldPos.y;

			// Can we move between neighbours
			if (canDropBetweenNodes(voxelGrid, node, neighbourNode))
			{
				// Are these closer neighbours?
				NodeConnection& nodeOldConnection = node.m_connections[neighbourDir];
				NodeConnection& neighbourOldConnection = node.m_connections[(Node::Directions)((neighbourDir + 2) % 4)];
				if ((std::abs(nodeOldConnection.m_fHeightGradient.load()) < std::abs(height) && nodeOldConnection.getConnected()) ||
					(std::abs(neighbourOldConnection.m_fHeightGradient.load()) < std::abs(height) && neighbourOldConnection.getConnected()))
					continue;

				// Destroy old neighbours
				if (node.hasNeighbourNode(neighbourDir))
					destroyNodeConnection(node, neighbourDir);

				if (neighbourNode.hasNeighbourNode((Node::Directions)((neighbourDir + 2) % 4)))
					destroyNodeConnection(neighbourNode, (Node::Directions)((neighbourDir + 2) % 4));

				// Connect neighbours
				node.setNeighBourNode(neighbourDir, neighbourNode);
				neighbourNode.setNeighBourNode((Node::Directions)((neighbourDir + 2) % 4), node);

				if (height == 0)
					return;
			}
		}
	}
}