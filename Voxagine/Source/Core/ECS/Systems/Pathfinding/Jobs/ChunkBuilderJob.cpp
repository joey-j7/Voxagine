#include "pch.h"
#include "ChunkBuilderJob.h"

#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"

namespace pathfinding
{
	ChunkBuilderJob::ChunkBuilderJob(ChunkGrid & grid, const Vector3& worldPos, const Vector2 & gridPos) :
		m_grid(grid),
		m_chunkIdx(gridPos.y * ChunkGrid::g_GRIDSIZE + gridPos.x),
		m_resultChunk(grid, worldPos, grid.m_bGenerateVerticalNodes, false)
	{
		m_grid.m_iGridLocks++;
	}

	void ChunkBuilderJob::Run()
	{
		PhysicsSystem* physicsSystem = m_grid.GetWorld()->GetPhysics();
		VoxelGrid* voxelGrid = physicsSystem->GetVoxelGrid();
		assert(voxelGrid);
		m_resultChunk.rebuild(*voxelGrid, &m_resultChunkConnection);
	}

	void ChunkBuilderJob::Finish()
	{
		PhysicsSystem* physicsSystem = m_grid.GetWorld()->GetPhysics();
		VoxelGrid* voxelGrid = physicsSystem->GetVoxelGrid();
		assert(voxelGrid);

		// Destroy old chunk and place new chunk
		m_grid.m_grid[m_chunkIdx].destroy();
		m_grid.m_grid[m_chunkIdx] = std::move(m_resultChunk);

		// Replace/connect up the new chunk
		m_grid.m_chunksNeedingConnecting.push_back(std::make_pair(m_chunkIdx, m_resultChunkConnection));
		m_grid.m_iGridLocks--;
	}
}