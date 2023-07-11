#include "pch.h"
#include "PathfindingChunkGrid.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/Threading/JobManager.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Systems/Rendering/DebugRenderer.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingNode.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h"
#include "Core/ECS/Systems/Pathfinding/Jobs/ChunkBuilderJob.h"

RTTR_REGISTRATION
{
	rttr::registration::enumeration<pathfinding::ChunkGrid::RenderMode>("PathfindingGridRenderMode")
	(
		rttr::value("None",				 pathfinding::ChunkGrid::RenderMode::NONE),
		rttr::value("Only Center Chunk", pathfinding::ChunkGrid::RenderMode::ONLY_CENTER_CHUNK),
		rttr::value("All Chunks",		 pathfinding::ChunkGrid::RenderMode::ALL_CHUNKS)
	);

	rttr::registration::class_<pathfinding::ChunkGrid>("PathfindingChunkGrid")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Center Entity", &pathfinding::ChunkGrid::m_gridCenterEntity)(RTTR_PUBLIC)
	.property("Enable Vetical Grid Generation", &pathfinding::ChunkGrid::m_bGenerateVerticalNodes)(RTTR_PUBLIC)
	.property("Grid Coarseness", &pathfinding::ChunkGrid::m_iGridCoarseness)(RTTR_PUBLIC)
	.property("Density Exponent", &pathfinding::ChunkGrid::m_fDensityExponent)(RTTR_PUBLIC)
	.property("Max Agents Per Node", &pathfinding::ChunkGrid::m_fMaxAgentsPerNode)(RTTR_PUBLIC)
	.property("Debug Draw Mode", &pathfinding::ChunkGrid::m_renderMode)(RTTR_PUBLIC)
	.property("Rebuild Interval", &pathfinding::ChunkGrid::m_fRebuildInterval)(RTTR_PUBLIC)
	.property("Group To Draw", &pathfinding::ChunkGrid::m_groupToDraw)(RTTR_PUBLIC);
}

namespace pathfinding
{
	std::vector<Job*> activeJobs;

	ChunkGrid::ChunkGrid(World * pWorld) :
		Entity(pWorld),
		m_renderMode(NONE),
		m_groupToDraw(nullptr),
		m_currentGridJob(REBUILD_GRID),
		m_gridCenterEntity(nullptr),
		m_gridCenter(0),
		m_fDensityExponent(3.f),
		m_fMaxAgentsPerNode(3),
		m_fMaxDensity(5.f),
		m_fMinDensity(0.1f),
		m_iGridLocks(0),
		m_chunksNeedingConnecting(),
		m_rebuildGrid(false),
		m_bGenerateVerticalNodes(true),
		m_iGridCoarseness(2)
	{
		SetPersistent(true);
	}

	ChunkGrid::~ChunkGrid()
	{
		if (GetWorld() != nullptr)
		{
			auto entities = GetWorld()->GetEntities();
			for (auto& entity : entities)
			{
				if (entity == this)
					continue;

				PathfinderGroup* pathfinderGroup = dynamic_cast<PathfinderGroup*>(entity);
				if (pathfinderGroup != nullptr)
					pathfinderGroup->m_pGrid = nullptr;

				PathfindingObstacle* pathfindingObstacle = dynamic_cast<PathfindingObstacle*>(entity->GetComponent<PathfindingObstacle>());
				if (pathfindingObstacle != nullptr)
					pathfindingObstacle->m_pGrid = nullptr;
			}
		}
	}

	void ChunkGrid::Awake()
	{
		Entity::Awake();
		SetName("PathfindingGrid");
		SetPersistent(true);
	}

	void ChunkGrid::Start()
	{
		Entity::Start();
		m_gridCenter.x = GetTransform()->GetPosition().x;
		m_gridCenter.y = GetTransform()->GetPosition().z;
		rebuildGrid(m_gridCenter);
	}

	float testTimer = 0;
	void ChunkGrid::Tick(float deltaTime)
	{
		Entity::Tick(deltaTime);
		m_fTimer += deltaTime;

		if (testTimer > 5.f)
		{
			testTimer = 0;
			m_iRebuildCount = 0;
		} else
			testTimer += deltaTime;

#ifdef EDITOR
		// Debug rendering
		DebugRenderer* pRenderer = GetWorld()->GetDebugRenderer();
		assert(pRenderer);

		int groupId = 0;
		if (m_groupToDraw != nullptr)
			groupId = m_groupToDraw->getId();

		if (m_renderMode == RenderMode::ALL_CHUNKS)
		{
			for (auto& chunk : m_grid)
				chunk.DebugDraw(*pRenderer, groupId);

		} else if (m_renderMode == RenderMode::ONLY_CENTER_CHUNK)
		{
			Chunk* chunk = getChunk(getGridPos(IVector3(m_gridCenter.x, 0, m_gridCenter.y)));
			if (chunk != nullptr)
				chunk->DebugDraw(*pRenderer, groupId);
		}
#endif

		JobQueue* pJobQueue = GetWorld()->GetJobQueue();
		if (pJobQueue)
		{
			// Update agents
			pJobQueue->Enqueue<int*>([this]()
			{
				for (auto& group : m_groups)
				{
					for (auto& agent : group->m_agents)
						group->updateAgents(*agent);
				}
				return nullptr;
			}, [this](int*) {});
		}

		// Rebuild paths
		if (m_iGridLocks == 0)
		{
			// Select job
			if (m_currentGridJob == REBUILD_GRID && m_fTimer > m_fRebuildInterval)
			{
				m_iRebuildCount++;
				m_gridCenterTemp = m_gridCenter;
				if (m_gridCenterEntity != nullptr)
				{
					m_gridCenterTemp.x = m_gridCenterEntity->GetTransform()->GetPosition().x;
					m_gridCenterTemp.y = m_gridCenterEntity->GetTransform()->GetPosition().z;
				}
				
				m_chunksNeedingConnecting.clear();
				rebuildGrid(m_gridCenterTemp);
			} else if (m_currentGridJob == RECONNECT_GRID && m_fTimer > m_fRebuildInterval) 
			{
				m_fTimer = 0.f;
				m_gridCenter = m_gridCenterTemp;

				// Connect all chunks together
				for (auto& connection : m_chunksNeedingConnecting)
				{
					if (pJobQueue)
					{
						m_iGridLocks++;
						pJobQueue->Enqueue<int*>([this, connection]()
						{
							PhysicsSystem* physicsSystem = GetWorld()->GetPhysics();
							VoxelGrid* voxelGrid = physicsSystem->GetVoxelGrid();
							assert(voxelGrid);

							if (connection.second.size() > 0)
								m_grid[connection.first].connectChunkNeighbours(*voxelGrid, const_cast<ChunkConnections&>(connection.second));
							return nullptr;
						}, [this](int*) { m_iGridLocks--; });
					}
				}

			} else if (m_currentGridJob == BUILD_SHARED_FIELD)
			{
				if (pJobQueue)
				{
					// Build shared field
					m_iGridLocks++;
					pJobQueue->Enqueue<int*>([this]()
					{
						for (auto& group : m_groups)
						{
							for (auto& agent : group->m_agents)
							{
								if (agent != nullptr)
								{
									Vector2 velocity = agent->getVelocity();
									//assert(!isnan(velocity.x));
									//assert(!isnan(velocity.y));
									if (!isnan(velocity.x) && !isnan(velocity.y))
										splatEnity(agent->getPosition(), agent->getHalfBoxSize(), agent->getVelocity());
								}
							}
						}
						buildDiscomfortField();
						buildAvgVelocityField();
						return nullptr;
					}, [this](int*) { m_iGridLocks--; });
				}
			} else if (m_currentGridJob == BUILD_GROUP_FIELDS)
			{
				// Process groups
				addAndRemoveGroups();

				// Build group fields
				for (auto& group : m_groups)
				{
					if (pJobQueue)
					{
						m_iGridLocks++;
						pJobQueue->Enqueue<int*>([this, &group]()
						{
							group->updatePaths();
							return nullptr;
						}, [this](int*) { m_iGridLocks--; });
					}
				}
			}

			// Increment job
			m_currentGridJob = (GridJob)((m_currentGridJob + 1) % 4);
		}
	}

	void ChunkGrid::addGroup(PathfinderGroup & group)
	{
		// Add group
		m_groupsToAdd.push_back(&group);
	}

	void ChunkGrid::removeGroup(PathfinderGroup & group)
	{
		m_groupsToRemove.push_back(&group);
	}

	void ChunkGrid::addObstacle(PathfindingObstacle & obstacle)
	{
		m_obstacles.push_back(&obstacle);
	}

	void ChunkGrid::removeObstacle(PathfindingObstacle & obstacle)
	{
		if (m_obstacles.size() > 0)
			m_obstacles.erase(std::remove(m_obstacles.begin(), m_obstacles.end(), &obstacle), m_obstacles.end());
	}

	IVector2 ChunkGrid::getGridPos(const IVector3& worldPos)
	{
		return IVector2(std::floor((float)worldPos.x / (float)(Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE)),
						std::floor((float)worldPos.z / (float)(Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE)));
	}

	int ChunkGrid::getChunkIdx(const IVector2 & gridPos)
	{
		IVector3 gridCornerPos = IVector3(m_gridCenter.x, 0, m_gridCenter.y) - (int)((g_GRIDSIZE / 2) * Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE);
		IVector2 gridCorner = getGridPos(gridCornerPos);
		return (gridPos.y - gridCorner.y) * g_GRIDSIZE + (gridPos.x - gridCorner.x);
	}

	Chunk * ChunkGrid::getChunk(const IVector2 & gridPos)
	{
		int chunkIdx = getChunkIdx(gridPos);
		if (chunkIdx < 0 || chunkIdx >= m_grid.size())
			return nullptr;

		Chunk* result = &m_grid[chunkIdx];
		if (result->m_parentGrid == nullptr)
			return nullptr;
		return result;
	}

	Node * ChunkGrid::getNode(const IVector3 & worldPos)
	{
		Chunk* chunk = getChunk(getGridPos(worldPos));
		if (chunk == nullptr)
			return nullptr;

		return chunk->getNodeInChunk(worldPos);
	}

	void ChunkGrid::addAndRemoveGroups()
	{
		// Add new groups
		for (auto& group : m_groupsToAdd)
		{
			// For each node add group
			for (auto& chunk : m_grid)
			{
				for (auto& node : chunk.m_nodes)
					node.m_groupProperties[group->getId()] = GroupNode();
			}

			// Add group
			m_groups.push_back(group);
		}
		m_groupsToAdd.clear();

		// Remove old groups
		for (auto& group : m_groupsToRemove)
		{
			// For each node remove group
			for (auto& chunk : m_grid)
			{
				for (auto& node : chunk.m_nodes)
					node.m_groupProperties.erase(group->getId());
			}

			// Remove group
			m_groups.erase(std::remove_if(m_groups.begin(), m_groups.end(), [&](const PathfinderGroup* other) {
				return other->getId() == group->getId();
			}), m_groups.end());
		}
		m_groupsToRemove.clear();
	}

	void ChunkGrid::rebuildGrid(const IVector2& gridCenter)
	{
		// Get systems
		JobQueue* pJobQueue = GetWorld()->GetJobQueue();
		if (!pJobQueue) return;

		PhysicsSystem* physicsSystem = GetWorld()->GetPhysics();
		VoxelGrid* voxelGrid = physicsSystem->GetVoxelGrid();
		assert(voxelGrid);

		m_iGridLocks++;
		pJobQueue->Enqueue<int*>([this, gridCenter, pJobQueue]()
		{
			// Get grid corner
			IVector3 gridCornerPos = IVector3(gridCenter.x, 0, gridCenter.y) - (int)((g_GRIDSIZE / 2) * Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE);
			IVector2 gridCorner = getGridPos(gridCornerPos);

			std::vector<Job*> jobs;
			jobs.resize(g_GRIDSIZE * g_GRIDSIZE);

			// Build grid on seprate thread
			for (int x = 0; x < g_GRIDSIZE; x++)
			{
				for (int z = 0; z < g_GRIDSIZE; z++)
				{
					IVector3 worldPos = IVector3(gridCorner.x + x, 0, gridCorner.y + z) * (int)(Chunk::g_CHUNKSIZE * Chunk::g_NODESIZE);
					jobs[x + z * g_GRIDSIZE] = new ChunkBuilderJob(*this, worldPos, IVector2(x, z));
				}
			}
			pJobQueue->EnqueueBulk(jobs);

			return nullptr;
		}, [this](int*) { m_iGridLocks--; });
	}

	void ChunkGrid::buildDiscomfortField()
	{
		for (auto& obstacle : m_obstacles)
		{
			Vector3 position = obstacle->GetTransform()->GetPosition();
			IVector3 halfBoxSize = IVector3(glm::ceil(obstacle->m_halfBoxSize / (float)Chunk::g_NODESIZE * 0.5f));

			IVector2 minTilePos = Chunk::getChunkPos(glm::floor(position));
			IVector2 maxTilePos = Chunk::getChunkPos(glm::ceil(position));
			minTilePos -= IVector2(halfBoxSize.x, halfBoxSize.z);
			maxTilePos += IVector2(halfBoxSize.x, halfBoxSize.z);

			// Calculate chunk
			IVector2 gridPos = IVector2(minTilePos.x / Chunk::g_CHUNKSIZE, minTilePos.y / Chunk::g_CHUNKSIZE);
			Chunk* chunk = getChunk(gridPos);

			// Calculate density for each tile in the tile bounds
			for (int x = minTilePos.x; x < maxTilePos.x; x++)
			{
				for (int z = minTilePos.y; z < maxTilePos.y; z++)
				{
					// Get the containing chunk
					IVector2 tilePos = IVector2(x, z);
					IVector2 newGridPos = IVector2(tilePos.x / Chunk::g_CHUNKSIZE, tilePos.y / Chunk::g_CHUNKSIZE);
					if (newGridPos != gridPos)
					{
						gridPos = newGridPos;
						chunk = getChunk(gridPos);
					}

					if (chunk == nullptr)
						continue;

					// Get the containing container
					tilePos -= Chunk::getChunkPos(chunk->getWorldPos());
					NodeContainer* container = chunk->getNodeContainer(tilePos.x, tilePos.y);
					if (container == nullptr)
						continue;

					// Get the containing nodes
					for (auto& nodeIdx : container->m_container)
					{
						// Is the node inside the collision box
						float nodeYPos = nodeIdx.first;
						float minYPos = position.y - halfBoxSize.y * Chunk::g_NODESIZE;
						float maxYPos = position.y + halfBoxSize.y * Chunk::g_NODESIZE;

						if (nodeYPos >= minYPos && nodeYPos <= maxYPos)
						{
							Node& node = chunk->m_nodes[nodeIdx.second];
							node.m_fDiscomfort = obstacle->m_fDiscomfort;
						}
					}
				}
			}
		}
	}

	void ChunkGrid::splatEnity(const Vector3 & position, const Vector3 & halfBoxSize, const Vector3 & velocity)
	{
		// Calculate radius
		float xzRadius = std::max(std::max(halfBoxSize.x, halfBoxSize.z), (float)Chunk::g_NODESIZE);
		float xzRadiusSqr = std::pow(std::ceil(xzRadius), m_fDensityExponent);
		unsigned int tileRadius = (unsigned int)std::ceil(xzRadius) / Chunk::g_NODESIZE;

		// Calculate tile bounds
		IVector2 minTilePos = Chunk::getChunkPos(glm::floor(position));
		IVector2 maxTilePos = Chunk::getChunkPos(glm::ceil(position));
		minTilePos -= IVector2(tileRadius);
		maxTilePos += IVector2(tileRadius);

		// Calculate chunk
		IVector2 gridPos = IVector2(minTilePos.x / Chunk::g_CHUNKSIZE, minTilePos.y / Chunk::g_CHUNKSIZE);
		Chunk* chunk = getChunk(gridPos);

		// Calculate density for each tile in the tile bounds
		for (int x = minTilePos.x; x < maxTilePos.x; x++)
		{
			for (int z = minTilePos.y; z < maxTilePos.y; z++)
			{
				// Get the containing chunk
				IVector2 tilePos = IVector2(x, z);
				IVector2 newGridPos = IVector2(tilePos.x / Chunk::g_CHUNKSIZE, tilePos.y / Chunk::g_CHUNKSIZE);
				if (newGridPos != gridPos)
				{
					gridPos = newGridPos;
					chunk = getChunk(gridPos);
				}

				if (chunk == nullptr)
					continue;

				// Get the containing container
				tilePos -= Chunk::getChunkPos(chunk->getWorldPos());
				NodeContainer* container = chunk->getNodeContainer(tilePos.x, tilePos.y);
				if (container == nullptr)
					continue;

				// Get the containing nodes
				for (auto& nodeIdx : container->m_container)
				{
					float distance = (nodeIdx.first + Chunk::g_NODESIZE / 2.f) - position.y;
					if (std::abs(distance) <= halfBoxSize.y)
					{
						Node& node = chunk->m_nodes[nodeIdx.second];

						// Calculate density
						float deltaX = node.m_worldPos.x - position.x;
						float deltaZ = node.m_worldPos.z - position.z;
						float distanceSqr = deltaX * deltaX + deltaZ * deltaZ;
						float density = std::max(0.f, 1.f - (distanceSqr / xzRadiusSqr));

						// Get the min density
						if (m_fMinDensity < density)
						{
							m_fMinDensity = density;
							m_fMaxDensity = m_fMinDensity * m_fMaxAgentsPerNode;
						}

						// Update density and avg velocity fields
						node.m_fDensity += density;
						node.m_avgVelocity += density * Vector2(velocity.x, velocity.z);
						node.m_fDiscomfort += density * 1.f;
					}
				}
			}
		}
	}

	void ChunkGrid::buildAvgVelocityField()
	{
		// Calculate avg velocity
		for (auto& chunk : m_grid)
		{
			for (auto& node : chunk.m_nodes)
			{
				// Update avg velocity field
				if (node.m_fDensity != 0.f)
					node.m_avgVelocity /= node.m_fDensity;

				// Clamp density field
				node.m_fDensity = std::max(m_fMinDensity, std::min(node.m_fDensity, m_fMaxDensity));
			}
		}
	}
}