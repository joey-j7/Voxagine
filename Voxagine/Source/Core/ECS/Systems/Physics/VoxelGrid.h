#pragma once
#include <vector>
#include "Core/Math.h"

struct Voxel
{
	uintptr_t UserPointer = 0;
	bool Active = false;
	uint32_t Color = 0;
};

class VoxelGrid
{
	friend class ChunkSystem;

public:
	VoxelGrid();
	VoxelGrid(uint32_t uiVoxelSize);
	~VoxelGrid();

	void Create(uint32_t uiDimX, uint32_t uiDimY, uint32_t uiDimZ, uint32_t uiVoxelSize, UVector3 chunkSize);
	void Create(uint32_t uiDimX, uint32_t uiDimY, uint32_t uiDimZ);
	void Clear();

	void SetWorldOffset(Vector3 worldOffset) { m_worldOffset = worldOffset; }
	Vector3 GetWorldOffset() const { return m_worldOffset; };

	/*Gets chunk with given origin and dimension inside voxel volume
	Set AllowOutBounds to true if you want the chunk to be padded with empty voxels if they are not in the voxel volume, otherwise an empty chunk will be returned */
	bool GetChunk(Voxel** chunk, Vector3 chunkOrigin, Vector3 dimensions, bool bAllowOutBounds = false);

	inline void ModifyVoxel(int iX, int iY, int iZ, uint32_t uiColor);
	
	inline const Voxel* GetVoxel(uint32_t iX, uint32_t iY, uint32_t iZ) const;
	inline Voxel* GetVoxel(uint32_t iX, uint32_t iY, uint32_t iZ);

	inline uint32_t GetVoxelID(Vector3 gridPos) const;
	inline uint32_t GetVoxelID(UVector3 gridPos) const;
	inline UVector3 GetVoxelPosition(uint32_t voxelID) const;
	inline bool IsOutOfBounds(Vector3 position) const;

	static inline UVector3 IndexToVector(uint32_t index, UVector3 worldSize);

	Vector3 WorldToGridRound(Vector3 worldPos, bool bAllowOutBounds = false) const;
	Vector3 WorldToGrid(Vector3 worldPos, bool bAllowOutBounds = false) const;
	Vector3 GridToWorld(Vector3 gridPos) const;
	Vector3 GridToWorld(uint32_t volumeId) const;

	void GetDimensions(uint32_t& uiX, uint32_t& uiY, uint32_t& uiZ) const;
	UVector3 GetDimensions() const;
	uint64_t GetNumVoxels() const { return m_uiNumVoxels; }
	uint32_t GetVoxelSize() const { return m_uiVoxelSize; }

private:
	std::vector<std::vector<Voxel>*> m_ChunkVolumes;
	uint32_t m_uiDimensionX, m_uiDimensionY, m_uiDimensionZ;
	uint32_t m_uiVoxelSize;
	uint64_t m_uiNumVoxels;
	float m_fInvVoxelSize;
	Vector3 m_InvChunkSize;

	Vector3 m_worldOffset;
	UVector3 m_chunkSize;
	uint32_t m_uiNumChunkY;
	uint32_t m_uiNumChunkX;

	inline void SetChunkVolumeAt(UVector2 loc, std::vector<Voxel>& voxelVolume);
};

inline void VoxelGrid::ModifyVoxel(int iX, int iY, int iZ, uint32_t uiColor)
{
	uint32_t chunkArrPos = ftoi_sse1((float)(iX)* m_InvChunkSize.x) + ftoi_sse1((float)(iZ)* m_InvChunkSize.z) * m_uiNumChunkX;
	uint32_t chunkXOffset = iX - (chunkArrPos % m_uiNumChunkX * m_chunkSize.x);
	uint32_t chunkZOffset = iZ - (ftoi_sse1((float)chunkArrPos / m_uiNumChunkX) * m_chunkSize.z);
	uint32_t chunkGridPos = chunkXOffset + iY * m_chunkSize.x + m_chunkSize.x * m_chunkSize.y * chunkZOffset;

	m_ChunkVolumes[chunkArrPos]->at(chunkGridPos).Color = uiColor;
}

inline const Voxel* VoxelGrid::GetVoxel(uint32_t iX, uint32_t iY, uint32_t iZ) const
{
	if (iX >= m_uiDimensionX || iY >= m_uiDimensionY || iZ >= m_uiDimensionZ)
		return nullptr;

	uint32_t chunkArrPos = ftoi_sse1((float)(iX)* m_InvChunkSize.x) + ftoi_sse1((float)(iZ)* m_InvChunkSize.z) * m_uiNumChunkX;
	uint32_t chunkXOffset = iX - (chunkArrPos % m_uiNumChunkX * m_chunkSize.x);
	uint32_t chunkZOffset = iZ - (ftoi_sse1((float)chunkArrPos / m_uiNumChunkX) * m_chunkSize.z);
	uint32_t chunkGridPos = chunkXOffset + iY * m_chunkSize.x + m_chunkSize.x * m_chunkSize.y * chunkZOffset;
	return &m_ChunkVolumes[chunkArrPos]->at(chunkGridPos);
}

inline Voxel* VoxelGrid::GetVoxel(uint32_t iX, uint32_t iY, uint32_t iZ)
{
	if (iX >= m_uiDimensionX || iY >= m_uiDimensionY || iZ >= m_uiDimensionZ)
		return nullptr;

	uint32_t chunkArrPos = ftoi_sse1((float)(iX) * m_InvChunkSize.x) + ftoi_sse1((float)(iZ) * m_InvChunkSize.z) * m_uiNumChunkX;
	uint32_t chunkXOffset = iX - (chunkArrPos % m_uiNumChunkX * m_chunkSize.x);
	uint32_t chunkZOffset = iZ - (ftoi_sse1((float)chunkArrPos / m_uiNumChunkX) * m_chunkSize.z);
	uint32_t chunkGridPos = chunkXOffset + iY * m_chunkSize.x + m_chunkSize.x * m_chunkSize.y * chunkZOffset;
	return &m_ChunkVolumes[chunkArrPos]->at(chunkGridPos);
}

inline uint32_t VoxelGrid::GetVoxelID(Vector3 gridPos) const
{
	return static_cast<uint32_t>(gridPos.x + gridPos.y * m_uiDimensionX + m_uiDimensionX * m_uiDimensionY * gridPos.z);
}

inline uint32_t VoxelGrid::GetVoxelID(UVector3 gridPos) const 
{
	return static_cast<uint32_t>(gridPos.x + gridPos.y * m_uiDimensionX + m_uiDimensionX * m_uiDimensionY * gridPos.z);
}

inline UVector3 VoxelGrid::GetVoxelPosition(uint32_t voxelID) const
{
	return UVector3(
		voxelID % m_uiDimensionX,
		(voxelID / m_uiDimensionX) % m_uiDimensionY,
		voxelID / (m_uiDimensionX * m_uiDimensionY)
	);
}

inline void VoxelGrid::SetChunkVolumeAt(UVector2 loc, std::vector<Voxel>& voxelVolume)
{
	uint32_t index = loc.x + loc.y * m_uiNumChunkX;
	if (index >= m_ChunkVolumes.size()) return;
	m_ChunkVolumes[index] = &voxelVolume;
}

inline bool VoxelGrid::IsOutOfBounds(Vector3 position) const
{
	return position.x < 0 || position.x >= m_uiDimensionX ||
		position.y < 0 || position.y >= m_uiDimensionY ||
		position.z < 0 || position.z >= m_uiDimensionZ;
}

inline UVector3 VoxelGrid::IndexToVector(uint32_t index, UVector3 worldSize)
{
	return UVector3(
		index % worldSize.x,
		(index / worldSize.x) % worldSize.y,
		index / (worldSize.x * worldSize.y)
	);
}
