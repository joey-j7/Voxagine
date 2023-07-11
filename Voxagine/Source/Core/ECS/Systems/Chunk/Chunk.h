#pragma once
#include <functional>
#include <vector>
#include "Core/Math.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include <External/rapidjson/document.h>
#include "Core/ECS/Systems/Chunk/ChunkUpdateGroup.h"

#define DEFAULT_CHUNK_SIZE UVector3(256, 128, 256)

using namespace rapidjson;

struct TextureReadData;
class JsonSerializer;
class JobManager;
class Chunk
{
public:
	friend class ChunkSystem;

	Chunk(Application* pApp, World* pWorld, UVector2 chunkIndex = Vector2(0, 0));
	Chunk(Application* pApp, World* pWorld, UVector2 chunkIndex, UVector3 chunkSize, Value& rootEntities);
	~Chunk();

	void Load(UVector2 gridTargetIndex);
	void LoadAsync(ChunkUpdateGroup::Item* pItem, std::function<void(ChunkUpdateGroup::Item*)> callback);
	void UnloadAsync(ChunkUpdateGroup::Item* pItem, std::function<void(ChunkUpdateGroup::Item*)> callback);

	bool IsLoaded() const { return m_bIsLoaded; }
	bool IsLoading() const { return m_bIsLoading; }
	bool IsUnloading() const { return m_bIsUnloading; }
	bool IsFirstLoad() const { return m_bFirstLoad; }
	bool IsTargetLoaded() const { return m_bIsTargetLoaded; }
	void SetTargetLoaded(bool bLoaded) { m_bIsTargetLoaded = bLoaded; }

	void SetGridTarget(UVector2 gridTarget) { m_GridTargetIndex = gridTarget; }
	UVector2 GetGridTarget() const { return m_GridTargetIndex; }

	UVector2 GetChunkIndex() { return m_ChunkIndex; }
	std::vector<Voxel>& GetVoxelData() { return m_VoxelData; }
	UVector3 GetChunkSize() const { return m_ChunkSize; }
	const std::vector<Value>& GetRootEntities() { return m_RootEntities; }

	void FindEntitiesInChunk(const std::vector<Entity*>& allEntities, std::vector<std::pair<Entity*, bool>>& foundEntities) const;
	void SetGroundPlane(const std::string& texturePath);
	void UpdateGroundPlane();

	void LoadEntities();
	void UpdateEntities();

private:
	// Compress voxel chunk data with RLE encoding
	void EncodeVoxels();

	// Decompress data with RLE decoding
	void DecodeVoxels();

	inline bool VoxelEqual(Voxel& a, Voxel& b);

	void UpdateRenderer(Entity* pEntity, bool bFirstLoad);
	
	void SaveAndDeleteEntities(std::vector<std::pair<Entity*, bool>>& entities);
	void DeleteEntity(Entity* pEntity);

	//Start at grid target of 1024, 1024 which is an invalid grid location
	UVector2 m_GridTargetIndex = UVector2(0, 0);
	UVector3 m_ChunkSize = UVector3(0, 0, 0);
	UVector2 m_ChunkIndex = UVector2(0, 0);

	std::vector<Value> m_RootEntities;
	std::vector<unsigned char> m_pEncodedVoxelData;
	std::vector<Voxel> m_VoxelData;

	bool m_bIsLoaded = false;
	bool m_bIsLoading = false;
	bool m_bIsUnloading = false;
	bool m_bFirstLoad = true;
	bool m_bIsTargetLoaded = false;
	bool m_bUpdateGroundPlane = false;

	TextureReadData* m_pTextureReadData;
	Document m_CopyDoc;
	World* m_pWorld;
	JobManager* m_pJobManager;
	JsonSerializer* m_pJsonSerializer;
};