#include "pch.h"

#include "Core/ECS/Systems/Chunk/FarFieldBaker.h"
#include "Core/ECS/Systems/Chunk/Chunk.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Rendering/VoxelStamp.h"

#include "Core/Application.h"
#include "Core/JsonSerializer.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/Platform/Rendering/FarFieldVolume.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/Platform/Platform.h"

#include <chrono>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace
{
	/* Every VoxRenderer "File" under a value, without constructing anything.
	   Matches ValueToComponent's own key names. */
	void CollectModelPaths(const Value& entity, std::set<std::string>& paths)
	{
		if (entity.HasMember("Components"))
		{
			const Value& components = entity["Components"];

			for (SizeType i = 0; i < components.Size(); ++i)
			{
				if (std::string(components[i]["ComponentType"].GetString()) != "VoxRenderer")
					continue;

				if (components[i].HasMember("File") && components[i]["File"].GetStringLength() > 0)
					paths.insert(components[i]["File"].GetString());
			}
		}

		if (entity.HasMember("Children"))
		{
			const Value& children = entity["Children"];

			for (SizeType i = 0; i < children.Size(); ++i)
				CollectModelPaths(children[i], paths);
		}
	}

	void StampEntity(Entity* pEntity, FarFieldVolume& volume, const UVector3& v3LevelSize, uint32_t& uiRenderers)
	{
		if (!pEntity->IsEnabled())
			return;

		/* GetComponentAll, not GetComponent: a component that has been added but
		   not yet processed by the world sits in m_AddedComponents, and these
		   entities are never added to a world at all - so everything
		   ValueToEntity built is still there and nowhere else. */
		VoxRenderer* pRenderer = pEntity->GetComponentAll<VoxRenderer>();

		if (pRenderer != nullptr && pRenderer->IsEnabled() && pRenderer->GetFrame() != nullptr)
		{
			/* Level space is world space, so the grid origin is zero - the
			   window's own bake passes VoxelGrid::GetWorldOffset() here. The
			   voxel size is the physics grid's, and a level is authored at
			   one world unit per voxel. */
			VoxelStampTransform stamp;

			if (ComputeVoxelStampTransform(pRenderer, Vector3(0.f), 1.f, stamp))
			{
				++uiRenderers;

				ForEachStampedVoxel(pRenderer, stamp, [&](const Vector3& v3Position, uint32_t uiColor)
				{
					/* In-range rather than a rejection test, so a non-finite
					   position is dropped rather than cast to INT32_MIN - see
					   VoxelStamp.h. VoxelBaker is where one gets reported; a
					   build that runs before the world has ticked has no
					   business naming entities as broken. */
					if (v3Position.x >= 0.f && v3Position.x < v3LevelSize.x &&
					    v3Position.y >= 0.f && v3Position.y < v3LevelSize.y &&
					    v3Position.z >= 0.f && v3Position.z < v3LevelSize.z)
					{
						volume.AddVoxel(
							static_cast<uint32_t>(v3Position.x),
							static_cast<uint32_t>(v3Position.y),
							static_cast<uint32_t>(v3Position.z),
							uiColor
						);
					}
				});
			}
		}

		for (Entity* pChild : pEntity->GetChildren())
			StampEntity(pChild, volume, v3LevelSize, uiRenderers);
	}

	/* ~Entity deletes its components, not its children - the same recursion
	   Chunk::DeleteEntity does for an entity it built and decided not to
	   keep. */
	void DeleteEntity(Entity* pEntity)
	{
		for (Entity* pChild : pEntity->GetChildren())
			DeleteEntity(pChild);

		delete pEntity;
	}

}

void FarFieldBaker::Build(World* pWorld, FarFieldVolume& volume)
{
	volume.Clear();

	ChunkSystem* pChunkSystem = pWorld != nullptr ? pWorld->GetChunkSystem() : nullptr;

	if (pChunkSystem == nullptr || volume.GetCellCount() == 0)
		return;

	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	const UVector3 v3LevelSize = volume.GetLevelSize();

	JsonSerializer& serializer = pWorld->GetApplication()->GetSerializer();

	uint32_t uiRenderers = 0;
	uint32_t uiEntities = 0;

	/* Pin every model the level names before building anything.
	   ResourceManager caches by path and frees at zero references, and each
	   throwaway entity releases its model when it is deleted - so without this,
	   a model shared by fifty entities is read off disk fifty times. Released
	   at the end, leaving the resident chunks' own references to decide what
	   stays loaded. */
	std::vector<VoxModel*> pinned;

	{
		std::set<std::string> paths;

		for (const std::pair<const uint32_t, Chunk*>& chunkEntry : pChunkSystem->GetChunks())
		{
			if (chunkEntry.second == nullptr)
				continue;

			for (const Value& rootEntity : chunkEntry.second->GetRootEntities())
				CollectModelPaths(rootEntity, paths);
		}

		ResourceManager& resources = pWorld->GetApplication()->GetResourceManager();

		for (const std::string& path : paths)
			pinned.push_back(resources.LoadVox(path));
	}

	for (const std::pair<const uint32_t, Chunk*>& chunkEntry : pChunkSystem->GetChunks())
	{
		Chunk* pChunk = chunkEntry.second;

		if (pChunk == nullptr)
			continue;

		for (const Value& rootEntity : pChunk->GetRootEntities())
		{
			/* Only static geometry. A monster or a projectile baked into a
			   volume that is never rebuilt would leave a ghost of wherever it
			   happened to be when the level loaded. Read off the value rather
			   than the constructed entity so that a Player or a spawner is
			   never constructed at all - some of those have side effects that
			   a throwaway copy has no business triggering. */
			if (!rootEntity.HasMember("Static") || !rootEntity["Static"].GetBool())
				continue;

			/* ValueToEntity needs a mutable Value but does not modify it for
			   the default bGenerateNewId of false; Chunk::GetRootEntities
			   hands out const because nothing else has reason to write. */
			Entity* pEntity = serializer.ValueToEntity(const_cast<Value&>(rootEntity), *pWorld);

			if (pEntity == nullptr)
				continue;

			++uiEntities;

			StampEntity(pEntity, volume, v3LevelSize, uiRenderers);

			/* Never added to the world, so this is the only owner. */
			DeleteEntity(pEntity);
		}
	}

	for (VoxModel* pModel : pinned)
	{
		if (pModel != nullptr)
			pModel->Release();
	}

	volume.SetBuilt(uiRenderers > 0);

	const double fMilliseconds =
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

	fprintf(stderr, "[farfield] built %u x %u x %u cells from %u renderers on %u static entities in %.1f ms "
	                "(%u cells occupied)\n",
	        volume.GetGridSize().x, volume.GetGridSize().y, volume.GetGridSize().z,
	        uiRenderers, uiEntities, fMilliseconds, volume.GetOccupiedCellCount());
}

uint32_t FarFieldBaker::Validate(World* pWorld, const FarFieldVolume& volume)
{
	ChunkSystem* pChunkSystem = pWorld != nullptr ? pWorld->GetChunkSystem() : nullptr;

	if (pChunkSystem == nullptr || volume.GetCellCount() == 0)
		return 0;

	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	const UVector3 v3GridSize = volume.GetGridSize();

	/* Two bits per cell rather than a pass per chunk: covered says a resident
	   chunk had something to say about this cell at all, occupied says what. */
	std::vector<uint8_t> covered(volume.GetCellCount(), 0);
	std::vector<uint8_t> occupied(volume.GetCellCount(), 0);

	uint32_t uiChunks = 0;

	for (const std::pair<const uint32_t, Chunk*>& chunkEntry : pChunkSystem->GetChunks())
	{
		Chunk* pChunk = chunkEntry.second;

		if (pChunk == nullptr || !pChunk->IsLoaded())
			continue;

		const std::vector<Voxel>& voxels = pChunk->GetVoxelData();
		const UVector3 v3ChunkSize = pChunk->GetChunkSize();

		if (voxels.size() < static_cast<size_t>(v3ChunkSize.x) * v3ChunkSize.y * v3ChunkSize.z)
			continue;

		++uiChunks;

		/* A chunk's level-space corner. Chunk sizes are multiples of the cell
		   size, so every cell is either wholly inside this chunk or wholly
		   outside it - there is no partial coverage to reason about. */
		const UVector3 v3Origin(
			pChunk->GetChunkIndex().x * v3ChunkSize.x,
			0,
			pChunk->GetChunkIndex().y * v3ChunkSize.z
		);

		for (uint32_t uiZ = 0; uiZ < v3ChunkSize.z; ++uiZ)
		{
			const uint32_t uiCellZ = (uiZ + v3Origin.z) >> FarFieldVolume::k_uiShift;

			if (uiCellZ >= v3GridSize.z)
				continue;

			for (uint32_t uiY = 0; uiY < v3ChunkSize.y; ++uiY)
			{
				const uint32_t uiCellY = (uiY + v3Origin.y) >> FarFieldVolume::k_uiShift;

				if (uiCellY >= v3GridSize.y)
					continue;

				/* ChunkSystem::RenderChunk's indexing: x + y*CX + z*CX*CY. */
				const size_t uiRowBase = static_cast<size_t>(uiY) * v3ChunkSize.x
					+ static_cast<size_t>(uiZ) * v3ChunkSize.x * v3ChunkSize.y;

				const uint32_t uiCellRowBase = uiCellY * v3GridSize.x + uiCellZ * v3GridSize.x * v3GridSize.y;

				for (uint32_t uiX = 0; uiX < v3ChunkSize.x; ++uiX)
				{
					const uint32_t uiCellX = (uiX + v3Origin.x) >> FarFieldVolume::k_uiShift;

					if (uiCellX >= v3GridSize.x)
						continue;

					const uint32_t uiCell = uiCellRowBase + uiCellX;

					covered[uiCell] = 1;

					/* Occupancy is alpha > 0 (rule 3), read off the colour, the
					   same predicate RenderChunk uses when it writes these very
					   voxels into the window. */
					if ((voxels[uiRowBase + uiX].Color >> 24) != 0)
						occupied[uiCell] = 1;
				}
			}
		}
	}

	uint32_t uiChecked = 0;
	uint32_t uiPhantom = 0;
	uint32_t uiMissing = 0;
	uint32_t uiReported = 0;

	for (uint32_t uiZ = 0; uiZ < v3GridSize.z; ++uiZ)
	{
		for (uint32_t uiY = 0; uiY < v3GridSize.y; ++uiY)
		{
			for (uint32_t uiX = 0; uiX < v3GridSize.x; ++uiX)
			{
				const uint32_t uiCell = uiX + uiY * v3GridSize.x + uiZ * v3GridSize.x * v3GridSize.y;

				if (!covered[uiCell])
					continue;

				++uiChecked;

				const bool bCellOccupied = volume.IsCellOccupied(uiX, uiY, uiZ);

				if (bCellOccupied == (occupied[uiCell] != 0))
					continue;

				if (bCellOccupied)
				{
					++uiPhantom;

					if (uiReported < 8)
					{
						++uiReported;
						fprintf(stderr, "[farfield] cell (%u %u %u) is occupied but the level at (%u %u %u) is empty\n",
						        uiX, uiY, uiZ,
						        uiX << FarFieldVolume::k_uiShift,
						        uiY << FarFieldVolume::k_uiShift,
						        uiZ << FarFieldVolume::k_uiShift);
					}
				}
				else
				{
					++uiMissing;
				}
			}
		}
	}

	const double fMilliseconds =
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

	fprintf(stderr, "[farfield] validated %u cells against %u resident chunks in %.1f ms: "
	                "%u phantom (placement is wrong), %u missing (the ground plane and dynamic "
	                "entities are not baked, so most of this is expected)\n",
	        uiChecked, uiChunks, fMilliseconds, uiPhantom, uiMissing);

	return uiPhantom;
}
