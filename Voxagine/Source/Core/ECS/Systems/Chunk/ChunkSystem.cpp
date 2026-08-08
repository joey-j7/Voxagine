#include "pch.h"
#include "ChunkSystem.h"
#include "Core/ECS/Systems/Chunk/Chunk.h"
#include "Core/ECS/Components/ChunkViewer.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Rendering/DebugRenderer.h"

#include "Core/Platform/Platform.h"
#include "Core/Platform/Rendering/FrameProfiler.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/ECS/World.h"
#include "Core/Application.h"
#include <chrono>
#include "External/optick/optick.h"

ChunkSystem::ChunkSystem(World* pWorld, std::unordered_map<uint32_t, Chunk*> chunks, UVector2 chunkSize, UVector2 worldSize) :
	ComponentSystem(pWorld)
{
	m_pVoxelGrid = m_pWorld->GetPhysics()->GetVoxelGrid();
	m_Chunks = chunks;
	m_WorldSize = worldSize;
	m_ChunkSize = chunkSize;
	m_uiNumChunkX = worldSize.x / chunkSize.x;
	m_uiNumChunkY = worldSize.y / chunkSize.y;
	m_CameraLoadOffset = Vector3(0, 0, 0);
	Entity entity(pWorld);
	ChunkViewer viewer = ChunkViewer(&entity);

	//Create default chunk
	if (m_Chunks.empty())
	{
		m_Chunks[0] = new Chunk(pWorld->GetApplication(), pWorld);
	}

	m_UpdateGroups.reserve(m_uiNumChunkX * m_uiNumChunkY);
}

ChunkSystem::~ChunkSystem()
{
	for (auto& iter : m_Chunks)
	{
		delete iter.second;
	}
	m_Chunks.clear();

	m_pWorld->Resumed -= this;
}

void ChunkSystem::Start()
{
	m_pWorld->Resumed -= this;
	m_pWorld->Resumed += Event<World*>::Subscriber(std::bind(&ChunkSystem::OnWorldResumed, this, std::placeholders::_1), this);

	Camera* pCamera = m_pWorld->GetMainCamera();
	Vector3 cameraPos = pCamera->GetTransform()->GetPosition() + m_CameraLoadOffset;
	Vector3 worldOffset = CalculateWorldOffset(cameraPos);

	m_pWorld->GetPhysics()->GetVoxelGrid()->SetWorldOffset(worldOffset);
	pCamera->SetCameraOffset(worldOffset);

	int chunkXPos = static_cast<int>(floor((cameraPos.x) / (float)m_ChunkSize.x));
	int chunkYPos = static_cast<int>(floor((cameraPos.z) / (float)m_ChunkSize.y));
	chunkXPos = std::min(std::max(chunkXPos, 0), (int)m_uiNumChunkX);
	chunkYPos = std::min(std::max(chunkYPos, 0), (int)m_uiNumChunkY);

	m_ClampedCameraPosition = UVector2(chunkXPos, chunkYPos);

	IVector2 gridOffset(worldOffset.x / (float)m_ChunkSize.x, worldOffset.z / (float)m_ChunkSize.y);
	ChunkUpdateGroup group(gridOffset.x + gridOffset.y * m_uiNumChunkX, worldOffset);
	UpdateChunks(gridOffset, group, false);

	//Custom code for start only, it needs to load the chunks synchronously the first time
	for (ChunkUpdateGroup::Item& item : group.GetItems())
	{
		switch (item.ItemTarget)
		{
		case ChunkUpdateGroup::Item::Target::T_LOAD:
		{
			item.pChunk->Load(item.GridTargetIndex);
			m_pVoxelGrid->SetChunkVolumeAt(item.GridTargetIndex, item.pChunk->GetVoxelData());
			break;
		}
		case ChunkUpdateGroup::Item::Target::T_MOVE:
		{
			m_pVoxelGrid->SetChunkVolumeAt(item.GridTargetIndex, item.pChunk->GetVoxelData());
			if (!item.pChunk->IsFirstLoad())
			{
				RenderChunk(item, m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelData(), false);
			}
			item.pChunk->SetGridTarget(item.GridTargetIndex);
			break;
		}
		default:
			break;
		}
	}

	SetGroundPlane(m_pWorld->GetGroundTexturePath());

	/* Far-field LOD volume (RENDERING_PLAN.md phase 4). Here rather than in
	   JsonSerializer::DeserializeWorld because it needs every chunk registered
	   with this system and the voxel grid sized, both of which are only true
	   once the system exists - and it is the chunk window's own limit that the
	   far field exists to work around, so this is where it belongs. */
	m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->BuildFarField(m_pWorld);
}

bool ChunkSystem::CanProcessComponent(Component* pComponent)
{
	return false;
}

void ChunkSystem::FixedTick(const GameTimer& fixedTimer)
{
	Vector3 cameraPosition = m_pWorld->GetMainCamera()->GetTransform()->GetPosition() + m_CameraLoadOffset;
	int chunkXPos = static_cast<int>(floor((cameraPosition.x) / (float)m_ChunkSize.x));
	int chunkYPos = static_cast<int>(floor((cameraPosition.z) / (float)m_ChunkSize.y));
	chunkXPos = std::min(std::max(chunkXPos, 0), (int)m_uiNumChunkX);
	chunkYPos = std::min(std::max(chunkYPos, 0), (int)m_uiNumChunkY);

	//Make sure we don't go diagonally as the chunk system does not support this
	if (m_ClampedCameraPosition.x != (uint32_t)chunkXPos && m_ClampedCameraPosition.y != (uint32_t)chunkYPos)
		m_ClampedCameraPosition.x = chunkXPos;
	else
	{
		m_ClampedCameraPosition.x = chunkXPos;
		m_ClampedCameraPosition.y = chunkYPos;
	}

	Vector3 worldOffset = CalculateWorldOffset(cameraPosition);
	Vector3 currWorldOffset = m_pWorld->GetPhysics()->GetVoxelGrid()->GetWorldOffset();
	if (currWorldOffset != worldOffset && 
		(m_ClampedCameraPosition.x >= 0 && m_ClampedCameraPosition.y >= 0) && 
		(m_ClampedCameraPosition.x < m_uiNumChunkX && m_ClampedCameraPosition.y < m_uiNumChunkY))
	{
		IVector2 gridOffset(worldOffset.x / (float)m_ChunkSize.x, worldOffset.z / (float)m_ChunkSize.y);
		ChunkUpdateGroup group(gridOffset.x + gridOffset.y * m_uiNumChunkX, worldOffset);

		//Add the UpdateGroup if its not already in the list
		auto groupIter = std::find_if(m_UpdateGroups.begin(), m_UpdateGroups.end(), group);
		if (groupIter == m_UpdateGroups.end())
		{
			UpdateChunks(gridOffset, group);
			m_UpdateGroups.push_back(group);
		}
		else
		{
			//Check if the requested new group is not the previously requested group
			if (m_UpdateGroups[m_UpdateGroups.size() - 1].GetId() != group.GetId())
			{
				//We have returned to a chunk which we already requested for update, remove all elements above this one 
				//since they came after this chunk request
				for (auto iter = m_UpdateGroups.begin(); iter != m_UpdateGroups.end(); ++iter)
				{
					if (groupIter->GetId() == iter->GetId())
					{
						++iter;
						while (iter != m_UpdateGroups.end())
						{
							iter = RemoveUpdateGroup(iter);
						}
						break;
					}
				}
			}
		}
	}

	if (!m_UpdateGroups.empty())
	{
		UpdateGroup(m_UpdateGroups[0]);
	}
}

void ChunkSystem::PostTick(float fDeltaTime)
{
#if defined(EDITOR) || defined(_DEBUG)

	uint32_t numHorizontalLines = m_uiNumChunkY - 1;
	uint32_t numVerticalLines = m_uiNumChunkX - 1;

	for (uint32_t x = 0; x < numHorizontalLines; ++x)
	{
		m_pWorld->GetDebugRenderer()->AddLine(Vector3(0.f, 5.f, (x + 1) * m_ChunkSize.y), Vector3(m_uiNumChunkX * m_ChunkSize.x, 5.f, (x + 1) * m_ChunkSize.y), VColors::Green);
	}

	for (uint32_t y = 0; y < numVerticalLines; ++y)
	{
		m_pWorld->GetDebugRenderer()->AddLine(Vector3((y + 1) * m_ChunkSize.x, 5.f, 0.f), Vector3((y + 1) * m_ChunkSize.x, 5.f, m_uiNumChunkY * m_ChunkSize.y), VColors::Green);
	}
#endif
}

void ChunkSystem::SetGroundPlane(const std::string& texturePath)
{
	for (auto& chunkIter : m_Chunks)
	{
		chunkIter.second->SetGroundPlane(texturePath);
	}
}

void ChunkSystem::OnComponentAdded(Component* pComponent)
{

}

void ChunkSystem::OnComponentDestroyed(Component* pComponent)
{

}

Vector3 ChunkSystem::CalculateWorldOffset(Vector3 viewPosition)
{
	int chunkXPos = static_cast<int>(floor((viewPosition.x) / (float)m_ChunkSize.x));
	int chunkYPos = static_cast<int>(floor((viewPosition.z) / (float)m_ChunkSize.y));

	Vector3 worldOffset((chunkXPos - 1) * (float)m_ChunkSize.x, 0.f, (chunkYPos - 1) * (float)m_ChunkSize.y);
	worldOffset.x = glm::clamp(worldOffset.x, 0.f, (m_uiNumChunkX - 3) * (float)m_ChunkSize.x);
	worldOffset.z = glm::clamp(worldOffset.z, 0.f, (m_uiNumChunkY - 3) * (float)m_ChunkSize.y);

	return worldOffset;
}

void ChunkSystem::UpdateChunks(IVector2 gridOffset, ChunkUpdateGroup& group, bool bAsync)
{
	OPTICK_CATEGORY("ChunkSystem", Optick::Category::GameLogic);
	OPTICK_EVENT();

	/* RENDERING_PLAN.md Phase 0: the other known main-thread cost, alongside
	   VoxelBaker::Bake. Guarded so a disabled profiler pays nothing but this
	   one branch. */
	const bool bProfiling = FrameProfiler::Get().IsEnabled();
	const std::chrono::steady_clock::time_point start =
		bProfiling ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();

	for (uint32_t x = 0; x < m_uiNumChunkX; ++x)
	{
		for (uint32_t y = 0; y < m_uiNumChunkY; ++y)
		{
			if (x < 0 || x >= m_uiNumChunkX || y < 0 || y >= m_uiNumChunkY) continue;

			Chunk* pChunk = m_Chunks[x + y * m_uiNumChunkX];
			if (!pChunk) continue;

			if (x >= gridOffset.x && x < (gridOffset.x + 3) && y >= gridOffset.y && y < (gridOffset.y + 3))
			{
				UVector2 gridTarget(x - gridOffset.x, y - gridOffset.y);
				if (!pChunk->IsTargetLoaded())
				{
					if (bAsync)
					{
						pChunk->SetTargetLoaded(true);
						group.AddItem(ChunkUpdateGroup::Item(ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD, pChunk, gridTarget, false));
					}
					else
					{
						pChunk->SetTargetLoaded(true);
						group.AddItem(ChunkUpdateGroup::Item(ChunkUpdateGroup::Item::Target::T_LOAD, pChunk, gridTarget));
					}
				}
				else
				{
					group.AddItem(ChunkUpdateGroup::Item(ChunkUpdateGroup::Item::Target::T_MOVE, pChunk, gridTarget));
				}
			}
			else if (pChunk->IsTargetLoaded())
			{
				pChunk->SetTargetLoaded(false);
				group.AddItem(ChunkUpdateGroup::Item(ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD, pChunk, pChunk->GetGridTarget()));
			}
		}
	}

	if (bProfiling)
	{
		const double fMilliseconds =
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

		FrameProfiler::Get().Report("CPU ChunkSystem::UpdateChunks", fMilliseconds);
	}
}

void ChunkSystem::UpdateGroup(ChunkUpdateGroup& group)
{
	switch (group.GetState())
	{
	case ChunkUpdateGroup::UpdateState::US_INIT:
	{
		for (ChunkUpdateGroup::Item& item : group.GetItems())
		{
			if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD)
				item.pChunk->LoadAsync(&item, std::bind(&ChunkSystem::OnChunkLoaded, this, std::placeholders::_1));
		}
		group.SetState(ChunkUpdateGroup::UpdateState::US_WAIT);
		break;
	}
	case ChunkUpdateGroup::UpdateState::US_WAIT:
	{
		bool isReady = true;
		for (ChunkUpdateGroup::Item& item : group.GetItems())
		{
			if (!item.bIsDone)
			{
				isReady = false;
				break;
			}
		}
		if (isReady)
			group.SetState(ChunkUpdateGroup::UpdateState::US_RENDERING);
		break;
	}
	case ChunkUpdateGroup::UpdateState::US_RENDERING:
	{
		if (group.IsRendering()) break;
		group.SetRendering(true);

		uint32_t* viewPortData = m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelBackData();

		JobQueue* pJobQueue = m_pWorld->GetJobQueue();
		if (pJobQueue)
		{
			pJobQueue->Enqueue<bool>([this, &group, viewPortData]()
			{
				for (ChunkUpdateGroup::Item& item : group.GetItems())
				{
					if (item.ItemTarget != ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD)
						RenderChunk(item, viewPortData, true);
				}
				return true;
			}, [this, &group](bool bFinished)
			{
				/* This callback is the *main thread* - JobQueue runs the body
				   above on a worker and the completion here - so everything in
				   it is a frame stall, and a chunk becoming resident for the
				   first time is the expensive case. Measured per phase rather
				   than as a total, because the candidates behave differently:
				   LoadEntities pulls models and textures off disk and each
				   texture costs a GPU round trip (CLAUDE.md), while the physics
				   and entity-update loops are pure CPU.

				   Guarded so a disabled profiler pays one branch, as elsewhere.
				   FrameProfiler has no mutex, which is why nothing reports from
				   the worker body. */
				const bool bProfiling = FrameProfiler::Get().IsEnabled();

				auto now = []() { return std::chrono::steady_clock::now(); };
				auto since = [](const std::chrono::steady_clock::time_point& start)
				{
					return std::chrono::duration<double, std::milli>(
						std::chrono::steady_clock::now() - start).count();
				};

				std::chrono::steady_clock::time_point phase =
					bProfiling ? now() : std::chrono::steady_clock::time_point();

				//Update physics first
				for (ChunkUpdateGroup::Item& item : group.GetItems())
				{
					if (item.ItemTarget != ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD)
						m_pVoxelGrid->SetChunkVolumeAt(item.GridTargetIndex, item.pChunk->GetVoxelData());
				}

				if (bProfiling)
				{
					FrameProfiler::Get().Report("CPU Chunk SetChunkVolumeAt", since(phase));
					phase = now();
				}

				//Load new entities
				uint32_t uiLoaded = 0;

				for (ChunkUpdateGroup::Item& item : group.GetItems())
				{
					if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD)
					{
						item.pChunk->LoadEntities();
						item.pChunk->m_bIsLoaded = true;
						item.pChunk->m_bFirstLoad = false;

						++uiLoaded;
					}
					if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_MOVE)
					{
						item.pChunk->UpdateEntities();
						item.pChunk->SetGridTarget(item.GridTargetIndex);
					}
				}

				if (bProfiling)
				{
					FrameProfiler::Get().Report(
						uiLoaded > 0 ? "CPU Chunk LoadEntities" : "CPU Chunk UpdateEntities", since(phase));
					phase = now();
				}

				// Update offsets
				m_pWorld->GetPhysics()->GetVoxelGrid()->SetWorldOffset(group.GetWorldOffset());
				m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->UpdateWorld();
				m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelMapper()->SwapBuffer();

				// Update camera
				Camera* pMainCamera = m_pWorld->GetMainCamera();
				pMainCamera->SetCameraOffset(group.GetWorldOffset());
				pMainCamera->GetTransform()->SetFromMatrix(pMainCamera->GetTransform()->GetMatrix());
				pMainCamera->Recalculate();
				pMainCamera->ForceUpdate();

				if (bProfiling)
				{
					FrameProfiler::Get().Report("CPU Chunk Offsets", since(phase));
					phase = now();
				}

				// Unload chunk with entities
				for (ChunkUpdateGroup::Item& item : group.GetItems())
				{
					if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD)
					{
						item.bIsDone = false;
						item.pChunk->UnloadAsync(&item, std::bind(&ChunkSystem::OnChunkUnloaded, this, std::placeholders::_1));
					}
				}

				if (bProfiling)
					FrameProfiler::Get().Report("CPU Chunk Unload", since(phase));

				group.SetState(ChunkUpdateGroup::UpdateState::US_UNLOADING);
				group.SetRendering(false);
			});
		}
		break;
	}
	case ChunkUpdateGroup::UpdateState::US_UNLOADING:
	{
		bool isReady = true;
		for (ChunkUpdateGroup::Item& item : group.GetItems())
		{
			if (!item.bIsDone)
			{
				isReady = false;
				break;
			}
		}
		if (isReady)
		{
			RemoveUpdateGroup(m_UpdateGroups.begin());
		}
		break;
	}
	default:
		break;
	}
}

std::vector<ChunkUpdateGroup>::iterator ChunkSystem::RemoveUpdateGroup(const std::vector<ChunkUpdateGroup>::iterator& iter)
{
	for (ChunkUpdateGroup::Item& item : iter->GetItems())
	{
		if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD &&
			(!item.pChunk->IsLoaded() || item.pChunk->IsUnloading() ||
			m_UpdateGroups[0].IsChunkScheduledFor(item.pChunk, ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD)))
		{
			item.pChunk->SetTargetLoaded(false);
		}
		else if (item.ItemTarget == ChunkUpdateGroup::Item::Target::T_ASYNC_UNLOAD &&
			(item.pChunk->IsLoaded() || item.pChunk->IsLoading() ||
				m_UpdateGroups[0].IsChunkScheduledFor(item.pChunk, ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD)))
		{
			item.pChunk->SetTargetLoaded(true);
		}
	}

	return m_UpdateGroups.erase(iter);
}

void ChunkSystem::RenderChunk(ChunkUpdateGroup::Item& updateItem, uint32_t* viewPortData, bool bBackBuffer)
{
	UVector3 gridDimensions = m_pVoxelGrid->GetDimensions();
	UVector2 chunkOffset = updateItem.GridTargetIndex * m_ChunkSize;
	std::vector<Voxel>& voxelData = updateItem.pChunk->GetVoxelData();

	RenderContext* pRenderContext = m_pWorld->GetApplication()->GetPlatform().GetRenderContext();

	if (voxelData.size() > 0 && viewPortData != nullptr && pRenderContext->GetVoxelDataSize() == m_pVoxelGrid->GetNumVoxels())
	{
		/* This overwrites the chunk's slice of the window in full, so its
		   occupancy bricks are rebuilt from the chunk's own CPU-side voxels
		   rather than by diffing against the mapping. Diffing would mean
		   reading eight million voxels back out of uncached host-visible
		   memory, which costs far more than the write itself. */
		VoxelBrickGrid& brickGrid = pRenderContext->GetBrickGrid();

		const UVector3 v3RegionMin(chunkOffset.x, 0, chunkOffset.y);
		const UVector3 v3RegionSize(m_ChunkSize.x, gridDimensions.y, m_ChunkSize.y);

		brickGrid.BeginRegion(bBackBuffer, v3RegionMin, v3RegionSize);

		for (uint32_t z = chunkOffset.y; z < m_ChunkSize.y + chunkOffset.y; ++z)
		{
			for (uint32_t y = 0; y < gridDimensions.y; ++y)
			{
				uint32_t* ptr = viewPortData + chunkOffset.x + y * gridDimensions.x + z * gridDimensions.x * gridDimensions.y;
				Voxel* voxPtr = &voxelData[y * m_ChunkSize.x + (z - chunkOffset.y) * m_ChunkSize.x * gridDimensions.y];
				for (uint32_t x = chunkOffset.x; x < m_ChunkSize.x + chunkOffset.x; ++x)
				{
					const uint32_t uiColor = voxPtr->Color;
					*ptr = uiColor;

					/* Occupancy is alpha > 0 (rule 3) - the byte is a
					   rendererState tag, not an opacity. */
					if ((uiColor >> 24) != 0)
						brickGrid.AddVoxel(bBackBuffer, x, y, z);

					++ptr;
					++voxPtr;
				}
			}
		}

		brickGrid.EndRegion(bBackBuffer, v3RegionMin, v3RegionSize);
	}
}

void ChunkSystem::ClearChunk(UVector2 gridTargetIndex)
{
	OPTICK_EVENT();
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	UVector3 gridDimensions = m_pVoxelGrid->GetDimensions();
	UVector2 chunkOffset = gridTargetIndex * m_ChunkSize;
	uint32_t* viewportColorData = m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelData();

	if (viewportColorData != nullptr && m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelDataSize() == m_pVoxelGrid->GetNumVoxels())
	{
		for (uint32_t z = chunkOffset.y; z < m_ChunkSize.y + chunkOffset.y; ++z)
		{
			for (uint32_t y = 1; y < gridDimensions.y; ++y)
			{
				//Clear all voxels to 0 except the bottom layer which should be untouched
				memset(viewportColorData + chunkOffset.x + y * gridDimensions.x + z * gridDimensions.x * gridDimensions.y, 
					0, sizeof(uint32_t) * m_ChunkSize.x);
			}
		}
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	auto execTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	m_pWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "ChunkSystem", "Chunk clear (ms): " + std::to_string(execTime.count()));

	m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->UpdateWorld();
}

void ChunkSystem::OnChunkLoaded(ChunkUpdateGroup::Item* pUpdateItem)
{
	pUpdateItem->bIsDone = true;

	UVector2 chunkIndex = pUpdateItem->pChunk->GetChunkIndex();
	std::string chunkLoc = "X: " + std::to_string(chunkIndex.x) + " Y: " + std::to_string(chunkIndex.y);
	m_pWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "ChunkSystem", "Chunk loaded at " + chunkLoc);
}

void ChunkSystem::OnChunkUnloaded(ChunkUpdateGroup::Item* pUpdateItem)
{
	pUpdateItem->bIsDone = true;

	UVector2 chunkIndex = pUpdateItem->pChunk->GetChunkIndex();
	std::string chunkLoc = "X: " + std::to_string(chunkIndex.x) + " Y: " + std::to_string(chunkIndex.y);
	m_pWorld->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "ChunkSystem", "Chunk unloaded at " + chunkLoc);
}

void ChunkSystem::OnWorldResumed(World* pWorld)
{
	uint32_t* viewPortData = m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->GetVoxelData();
	for (auto& iter : m_Chunks)
	{
		if (iter.second->IsLoaded())
		{
			ChunkUpdateGroup::Item item(ChunkUpdateGroup::Item::Target::T_ASYNC_LOAD, iter.second, iter.second->GetGridTarget(), false);
			RenderChunk(item, viewPortData, false);
			item.pChunk->UpdateEntities();
		}
	}
}
