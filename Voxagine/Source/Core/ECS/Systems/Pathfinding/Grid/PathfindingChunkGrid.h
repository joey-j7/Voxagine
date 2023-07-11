#pragma once
#include <array>
#include <vector>
#include "Core/Math.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"

namespace pathfinding
{
	struct Node;
	class ContinuumCrowdsGroup;
	class PathfindingObstacle;
	class ChunkGrid : public Entity
	{
	public:
		// Rendering
		enum RenderMode
		{
			NONE,
			ONLY_CENTER_CHUNK,
			ALL_CHUNKS
		};
		RenderMode m_renderMode;
		ContinuumCrowdsGroup* m_groupToDraw;

		// Grid
		enum GridJob
		{
			BUILD_SHARED_FIELD,
			BUILD_GROUP_FIELDS,
			REBUILD_GRID,
			RECONNECT_GRID
		};
		GridJob m_currentGridJob;
		static const unsigned int g_GRIDSIZE = 3;
		std::array<Chunk, g_GRIDSIZE * g_GRIDSIZE> m_grid;
		
		// Editor variables
		Entity* m_gridCenterEntity;
		bool m_bGenerateVerticalNodes;
		int m_iGridCoarseness;
		float m_fDensityExponent;
		float m_fMaxAgentsPerNode;
		float m_fMinDensity;
		float m_fMaxDensity;

		int m_iGridLocks; // How many objects are currently locking the grid. (prevents rebuilding while grid locks > 0)
		bool m_rebuildGrid;
		std::vector<PathfinderGroup*> m_groups;
		std::vector<PathfindingObstacle*> m_obstacles;
		std::vector<std::pair<int, ChunkConnections>> m_chunksNeedingConnecting;

	private:
		IVector2 m_gridCenter;
		IVector2 m_gridCenterTemp;
		std::vector<PathfinderGroup*> m_groupsToAdd;
		std::vector<PathfinderGroup*> m_groupsToRemove;
		float m_fTimer = 0;
		float m_fRebuildInterval = 0.f;
		int m_iRebuildCount = 0;

	public:
		ChunkGrid(World* pWorld);
		virtual ~ChunkGrid();

		void Awake() override;
		void Start() override;
		void Tick(float deltaTime) override;

		void addGroup(PathfinderGroup& group);
		void removeGroup(PathfinderGroup& group);
		
		void addObstacle(PathfindingObstacle& obstacle);
		void removeObstacle(PathfindingObstacle& obstacle);

		static IVector2 getGridPos(const IVector3& worldPos);
		int getChunkIdx(const IVector2& gridPos);

		Chunk* getChunk(const IVector2& gridPos);
		Node* getNode(const IVector3& worldPos);

		RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND

	private:
		void addAndRemoveGroups();
		void rebuildGrid(const IVector2& gridCenter);

		// Calculate shared fields
		void buildDiscomfortField();
		void splatEnity(const Vector3& position, const Vector3& halfBoxSize, const Vector3& velocity);
		void buildAvgVelocityField();
	};
}