#include <gtest/gtest.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"

namespace pathfinding
{
	// Pathfinding chunk scafholding
	class ChunkTest : public ::testing::Test
	{
	protected:
		IVector3 m_gridSize;
		VoxelGrid m_voxelGrid;

		ChunkGrid m_pathfindingGrid;
		Chunk m_pathfindingChunk;

	protected:
		ChunkTest() :
			m_pathfindingGrid(nullptr),
			m_gridSize(256)
		{}

		void SetUp() override
		{
			assert(ChunkGrid::g_GRIDSIZE == 10 && Chunk::g_CHUNKSIZE == 10 && Chunk::g_NODESIZE == 5);

			// Create voxel grid
			m_gridSize = IVector3(60, 256, 60);
			m_voxelGrid.Create(m_gridSize.x, m_gridSize.y, m_gridSize.z);
			for (int x = 0; x < m_gridSize.x; x++)
			{
				for (int z = 0; z < m_gridSize.z; z++)
				{
					for (int y = 0; y < 2; y++)
					{
						Voxel* voxel = m_voxelGrid.GetVoxel(x, m_gridSize.y - (m_gridSize.y / (y + 1)), z);
						if (voxel != nullptr)
							voxel->Active = true;
					}
				}
			}

			// Create default chunk
			m_pathfindingChunk = Chunk(m_pathfindingGrid, IVector3(0), false);
		}

		void TearDown() override
		{
			m_voxelGrid.Clear();
		}
	};

	// Checks if grid rebuilding generates the expected amount of nodes
	TEST_F(ChunkTest, Rebuild)
	{
		// Chunk inside voxel grid
		m_pathfindingChunk.rebuild(m_voxelGrid);
		int expectedNodeCount = Chunk::g_CHUNKSIZE * Chunk::g_CHUNKSIZE * 2;
		EXPECT_EQ(expectedNodeCount, m_pathfindingChunk.m_nodes.size()) << 
			"Chunk fully inside the grid failed to generate the correct amount of nodes.";

		expectedNodeCount = Chunk::g_CHUNKSIZE * Chunk::g_CHUNKSIZE;
		EXPECT_EQ(expectedNodeCount, m_pathfindingChunk.m_nodeGridLookup.size()) << 
			"Chunk fully inside the grid failed to generate the correct amount of node lookups.";

		// Chunk outside voxel grid
		IVector3 worldPos =  IVector3(-(int)(Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE) / 2);
		m_pathfindingChunk = Chunk(m_pathfindingGrid, worldPos, false);
		m_pathfindingChunk.rebuild(m_voxelGrid);

		expectedNodeCount = Chunk::g_CHUNKSIZE * Chunk::g_CHUNKSIZE / 2;
		EXPECT_EQ(0, m_pathfindingChunk.m_nodes.size()) << 
			"Chunk partially outside the grid failed to generate the correct amount of nodes.";
	}

	// Check the chunk gets correctly destroyed
	TEST_F(ChunkTest, Destroy)
	{
		// Destory all nodes
		m_pathfindingChunk.rebuild(m_voxelGrid);
		m_pathfindingChunk.destroy();
		EXPECT_EQ(0, m_pathfindingChunk.m_nodes.size()) << "Chunk failed to destory all nodes.";

		// Destory a single colum of nodes with a rebuild
		m_pathfindingChunk.rebuild(m_voxelGrid);
		m_voxelGrid.Create(0, 0, 0); // Destory voxel grid to see if nodes get destoyed
		size_t expectedCount = m_pathfindingChunk.m_nodes.size() - 2;
		m_pathfindingChunk.rebuild(m_voxelGrid, IVector2(0, 0));
		EXPECT_EQ(expectedCount, m_pathfindingChunk.m_nodes.size()) << "Rebuild chunk colum failed to destory all nodes.";

		// Destory all nodes with a rebuild
		m_pathfindingChunk.rebuild(m_voxelGrid);
		EXPECT_EQ(0, m_pathfindingChunk.m_nodes.size()) << "Rebuild chunk failed to destory all nodes.";
	}

	// Check if the chunk build the correct amount of connections within the chunk
	TEST_F(ChunkTest, ConnectInnerNeighbours)
	{
		// Count the connections of the chunk which is inside the voxel grid
		m_pathfindingChunk.rebuild(m_voxelGrid);
		int expectedCount = (((Chunk::g_CHUNKSIZE - 2) * (Chunk::g_CHUNKSIZE - 2)) * 4 + 
							(Chunk::g_CHUNKSIZE - 2) * 4 * 3 + 4 * 2) * 2;
		int connectionCount = 0;
		for (auto& node : m_pathfindingChunk.m_nodes)
		{
			for (int i = 0; i < 4; i++)
			{
				if (node.hasNeighbourNode((Node::Directions)i))
					connectionCount++;
			}
		}
		EXPECT_EQ(expectedCount, connectionCount) << "Rebuild failed to connect the right amount of inner neighbour connections";
	}

	// Check if the chunk build the correct amount of connections going outside the chunk
	TEST_F(ChunkTest, ConnectOuterNeighbours)
	{
		// Get all connections leaving this chunk
		ChunkConnections chunkConnections;
		m_pathfindingChunk.rebuild(m_voxelGrid, &chunkConnections);
		EXPECT_EQ(Chunk::g_CHUNKSIZE * 4 * 2, chunkConnections.size()) << 
			"Rebuild failed to identify the right amount of neighbour connections leaving the chunk.";
	}

	TEST_F(ChunkTest, GetChunkPos)
	{
		EXPECT_EQ(IVector2(0), Chunk::getChunkPos(IVector3(0)));
		EXPECT_EQ(IVector2(1), Chunk::getChunkPos(IVector3(Chunk::g_NODESIZE)));
		EXPECT_EQ(IVector2(Chunk::g_CHUNKSIZE), Chunk::getChunkPos(IVector3(Chunk::g_NODESIZE * Chunk::g_CHUNKSIZE)));
		EXPECT_EQ(IVector2(-1), Chunk::getChunkPos(IVector3(-1)));
	}

	TEST_F(ChunkTest, GetNodeContainer)
	{
		// Chunk inside grid
		m_pathfindingChunk.rebuild(m_voxelGrid);

		// Excisting node container
		NodeContainer* nodeContainer = m_pathfindingChunk.getNodeContainer(0, 0);
		EXPECT_NE(nullptr, nodeContainer);

		// Non excisting node container
		nodeContainer = m_pathfindingChunk.getNodeContainer(Chunk::g_CHUNKSIZE, Chunk::g_CHUNKSIZE); 
		EXPECT_EQ(nullptr, nodeContainer);
	}

	TEST_F(ChunkTest, GetNodeInChunk)
	{
		// Chunk inside grid
		m_pathfindingChunk.rebuild(m_voxelGrid);

		EXPECT_NE(nullptr, m_pathfindingChunk.getNodeInChunk(IVector3(0, 1, 0))); // Excisting node
		EXPECT_EQ(nullptr, m_pathfindingChunk.getNodeInChunk(IVector3(0, 0, 0))); // None existing node in chunk
		EXPECT_EQ(nullptr, m_pathfindingChunk.getNodeInChunk(IVector3(-1, -1, -1))); // Node outside chunk
	}

	TEST_F(ChunkTest, GetPosition)
	{
		IVector3 gridPos = IVector3(m_pathfindingChunk.getGridPos().x * Chunk::g_NODESIZE * Chunk::g_CHUNKSIZE,
									0,
									m_pathfindingChunk.getGridPos().y * Chunk::g_NODESIZE * Chunk::g_CHUNKSIZE);
		EXPECT_EQ(gridPos, m_pathfindingChunk.getWorldPos()) << "Chunk world pos has not been set correctly";
	}

	TEST_F(ChunkTest, IsNodePosAir)
	{
		// Chunk inside grid
		m_pathfindingChunk.rebuild(m_voxelGrid);
		unsigned int voxelsToSkip;
		bool isAir;

		// Check solid node pos
		voxelsToSkip = m_gridSize.y;
		isAir = m_pathfindingChunk.isNodePosAir(voxelsToSkip, m_voxelGrid, IVector3(0, 0, 0));
		EXPECT_EQ(false, isAir);
		EXPECT_EQ(1, voxelsToSkip);

		// Check air node pos
		voxelsToSkip = m_gridSize.y;
		isAir = m_pathfindingChunk.isNodePosAir(voxelsToSkip, m_voxelGrid, IVector3(0, 1, 0));
		EXPECT_EQ(true, isAir);
		EXPECT_EQ(Chunk::g_NODESIZE, voxelsToSkip);

		// Check negative numbers
		voxelsToSkip = m_gridSize.y;
		isAir = m_pathfindingChunk.isNodePosAir(voxelsToSkip, m_voxelGrid, IVector3(0, -10, 0));
		EXPECT_EQ(false, isAir);
		EXPECT_EQ(Chunk::g_NODESIZE, voxelsToSkip);

		// Check collum size equal to grid size
		voxelsToSkip = m_gridSize.y;
		isAir = m_pathfindingChunk.isNodePosAir(voxelsToSkip, m_voxelGrid, IVector3(0, 1, 0), m_gridSize.y - 1);
		EXPECT_EQ(false, isAir);
		EXPECT_EQ(m_gridSize.y / 2, voxelsToSkip);

		// Check collum size exceding grid size
		voxelsToSkip = m_gridSize.y;
		isAir = m_pathfindingChunk.isNodePosAir(voxelsToSkip, m_voxelGrid, IVector3(0, 1, 0), m_gridSize.y);
		EXPECT_EQ(false, isAir);
		EXPECT_EQ(m_gridSize.y, voxelsToSkip);
	}

	TEST_F(ChunkTest, CanDropBetweenNodes)
	{
		// Chunk inside grid, but delete the first upper node
		for (int x = 0; x < Chunk::g_NODESIZE; x++)
		{
			for (int z = 0; z < Chunk::g_NODESIZE; z++)
			{
				Voxel* voxel = m_voxelGrid.GetVoxel(x, m_gridSize.y - (m_gridSize.y / 2), z);
				if (voxel != nullptr)
					voxel->Active = false;
			}
		}
		m_pathfindingChunk.rebuild(m_voxelGrid);

		// Get all nodes that need testing
		Node* node1 = m_pathfindingChunk.getNodeInChunk(IVector3(0, 1, 0));
		EXPECT_NE(nullptr, node1) << "Failed to get node 1.";
		Node* node2 = m_pathfindingChunk.getNodeInChunk(IVector3(Chunk::g_NODESIZE, 1, 0));
		EXPECT_NE(nullptr, node2) << "Failed to get node 2.";
		Node* node3 = m_pathfindingChunk.getNodeInChunk(IVector3(Chunk::g_NODESIZE, m_gridSize.y / 2 + 1, 0));
		EXPECT_NE(nullptr, node3) << "Failed to get node 3.";
		Node* node4 = m_pathfindingChunk.getNodeInChunk(IVector3(2 * Chunk::g_NODESIZE, 1, 0));
		EXPECT_NE(nullptr, node4) << "Failed to get node 4.";

		EXPECT_EQ(true, m_pathfindingChunk.canDropBetweenNodes(m_voxelGrid, *node1, *node3)) << 
			"Unexpected fail when nodes are next to each other in the x/z plane.";
		EXPECT_EQ(false, m_pathfindingChunk.canDropBetweenNodes(m_voxelGrid, *node1, *node4)) <<
			"Unexpected succes when nodes are not next to each other in the x/z plane";
		EXPECT_EQ(false, m_pathfindingChunk.canDropBetweenNodes(m_voxelGrid, *node2, *node3)) << 
			"Unxepected succes when nodes are above each other.";
		EXPECT_EQ(false, m_pathfindingChunk.canDropBetweenNodes(m_voxelGrid, *node3, *node4)) << 
			"Unxepected succes when nodes are next to each other in the x/z plane, but a solid is in between";
	}
}