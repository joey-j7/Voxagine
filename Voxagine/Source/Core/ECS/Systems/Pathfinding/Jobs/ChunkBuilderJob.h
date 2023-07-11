#pragma once
#include "Core/Threading/Job.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"

namespace pathfinding
{
	class ChunkBuilderJob : public Job
	{
	private:
		ChunkGrid& m_grid;
		const int m_chunkIdx;

		// The generated chunk and the potential connections outside this chunk.
		Chunk m_resultChunk;
		ChunkConnections m_resultChunkConnection;

	public: 
		ChunkBuilderJob(ChunkGrid& grid, const Vector3& worldPos, const Vector2& gridPos);
		void Run() override;
		void Finish() override;
		void Canceled() override {};
	};
}