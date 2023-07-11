#pragma once
#include "Core/ECS/ComponentSystem.h"
#include "Core/ECS/Systems/Chunk/ChunkUpdateGroup.h"

#define GRID_SIZE 3
#define GRID_CENTER_OFFSET 1

class Chunk;
class ChunkViewer;
class ChunkSystem : public ComponentSystem
{
public:
	ChunkSystem(World* pWorld, std::unordered_map<uint32_t, Chunk*> chunks = std::unordered_map<uint32_t, Chunk*>(), UVector2 chunkSize = UVector2(256, 256), UVector2 worldSize = UVector2(256, 256));
	~ChunkSystem();

	virtual void Start() override;
	virtual bool CanProcessComponent(Component* pComponent) override;
	virtual void FixedTick(const GameTimer& fixedTimer) override;
	virtual void PostTick(float fDeltaTime) override;

	void SetGroundPlane(const std::string& texturePath);

	UVector2 GetWorldSize() { return m_WorldSize; }
	const std::unordered_map<uint32_t, Chunk*>& GetChunks() { return m_Chunks; }

	void SetCameraLoadOffset(Vector3 offset) { m_CameraLoadOffset = offset; }
	Vector3 GetCameraLoadOffset() const { return m_CameraLoadOffset; }

protected:
	virtual void OnComponentAdded(Component* pComponent) override;
	virtual void OnComponentDestroyed(Component* pComponent) override;

	Vector3 CalculateWorldOffset(Vector3 viewPosition);
	void UpdateChunks(IVector2 gridOffset, ChunkUpdateGroup& group, bool bAsync = true);

	void UpdateGroup(ChunkUpdateGroup& group);
	std::vector<ChunkUpdateGroup>::iterator RemoveUpdateGroup(const std::vector<ChunkUpdateGroup>::iterator& iter);

	void RenderChunk(ChunkUpdateGroup::Item& updateItem, uint32_t* viewPortData);
	void ClearChunk(UVector2 gridTargetIndex);

	void OnChunkLoaded(ChunkUpdateGroup::Item* pUpdateItem);
	void OnChunkUnloaded(ChunkUpdateGroup::Item* pUpdateItem);

	void OnWorldResumed(World* pWorld);

private:
	VoxelGrid* m_pVoxelGrid;
	std::unordered_map<uint32_t, Chunk*> m_Chunks;

	std::vector<ChunkUpdateGroup> m_UpdateGroups;

	UVector2 m_WorldSize;
	UVector2 m_ChunkSize;
	uint32_t m_uiNumChunkY;
	uint32_t m_uiNumChunkX;
	UVector2 m_ClampedCameraPosition;
	Vector3 m_CameraLoadOffset = Vector3(0);
};