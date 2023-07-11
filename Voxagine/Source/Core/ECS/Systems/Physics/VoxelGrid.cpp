#include "pch.h"
#include "VoxelGrid.h"
#include "External/optick/optick.h"

VoxelGrid::VoxelGrid()
{
	m_uiDimensionX = 0;
	m_uiDimensionY = 0;
	m_uiDimensionZ = 0;
	m_uiNumVoxels = 0;
	m_uiNumChunkX = 0;
	m_uiNumChunkY = 0;

	m_uiVoxelSize = 1;
	m_fInvVoxelSize = 1.f / static_cast<float>(m_uiVoxelSize);
	m_InvChunkSize = Vector3(0.f);
	m_worldOffset = Vector3(0.f, 0.f, 0.f);
	m_chunkSize = UVector3(0, 0, 0);
}

VoxelGrid::VoxelGrid(uint32_t uiVoxelSize)
{
	m_uiDimensionX = 0;
	m_uiDimensionY = 0;
	m_uiDimensionZ = 0;
	m_uiNumVoxels = 0;
	m_uiNumChunkX = 0;
	m_uiNumChunkY = 0;

	m_uiVoxelSize = uiVoxelSize;
	m_fInvVoxelSize = 1.f / static_cast<float>(uiVoxelSize);
	m_InvChunkSize = Vector3(0.f);
	m_worldOffset = Vector3(0.f, 0.f, 0.f);
	m_chunkSize = UVector3(0, 0, 0);
}

VoxelGrid::~VoxelGrid()
{}

void VoxelGrid::Create(uint32_t uiDimX, uint32_t uiDimY, uint32_t uiDimZ, uint32_t uiVoxelSize, UVector3 chunkSize)
{
	m_uiVoxelSize = uiVoxelSize;
	m_chunkSize = chunkSize;
	m_fInvVoxelSize = 1.f / static_cast<float>(uiVoxelSize);
	m_InvChunkSize = 1.f / (Vector3)chunkSize;
	Create(uiDimX, uiDimY, uiDimZ);
}

void VoxelGrid::Create(uint32_t uiDimX, uint32_t uiDimY, uint32_t uiDimZ)
{
	m_uiDimensionX = uiDimX;
	m_uiDimensionY = uiDimY;
	m_uiDimensionZ = uiDimZ;

	m_uiNumVoxels = uiDimX * uiDimY * uiDimZ;
	m_uiNumChunkX = uiDimX / m_chunkSize.x;
	m_uiNumChunkY = uiDimZ / m_chunkSize.z;

	m_ChunkVolumes.resize(m_uiNumChunkX * m_uiNumChunkY);
}

void VoxelGrid::Clear()
{
	m_ChunkVolumes.clear();

	m_uiDimensionX = 0;
	m_uiDimensionY = 0;
	m_uiDimensionZ = 0;
	m_uiNumVoxels = 0;
}

bool VoxelGrid::GetChunk(Voxel** chunk, Vector3 chunkOrigin, Vector3 dimensions, bool bAllowOutBounds /*= false*/)
{
	OPTICK_EVENT();
	uint32_t dimX = static_cast<uint32_t>(dimensions.x);
	uint32_t dimY = static_cast<uint32_t>(dimensions.y);
	uint32_t dimZ = static_cast<uint32_t>(dimensions.z);

	int originX = static_cast<int>(chunkOrigin.x);
	int originY = static_cast<int>(chunkOrigin.y);
	int originZ = static_cast<int>(chunkOrigin.z);

	/* Set voxels to nullptr on default */
	for (uint32_t i = 0; i < dimX * dimY * dimZ; ++i)
		chunk[i] = nullptr;

	/* Return if some of the chunk is out of bounds */
	if (!bAllowOutBounds && (originX + dimX > m_uiDimensionX ||
		originY + dimY > m_uiDimensionY ||
		originZ + dimZ > m_uiDimensionZ ||
		originX < 0 || originY < 0 || originZ < 0))
		return false;

	/* Return if chunk is fully out of bounds */
	if ((originX + (int)dimX < 0 && originY + (int)dimY < 0 && originZ + (int)dimZ < 0) ||
		originX > (int)m_uiDimensionX || originY > (int)m_uiDimensionY || originZ > (int)m_uiDimensionZ)
		return false;

	if (((int)((m_chunkSize.x - originX) % m_chunkSize.x) - (int)dimX) >= 0 && ((int)((m_chunkSize.z - originZ) % m_chunkSize.z) - (int)dimZ) >= 0)
	{
		uint32_t chunkArrPos = ftoi_sse1((float)(originX) * m_InvChunkSize.x) + ftoi_sse1((float)(originZ) * m_InvChunkSize.z) * m_uiNumChunkX;

		for (unsigned z = 0; z < dimZ; ++z)
		{
			/* Continue if Z row lays out of bounds */
			if (originZ + z < 0 || originZ + z >= m_uiDimensionZ)
				continue;

			for (unsigned y = 0; y < dimY; ++y)
			{
				/* Set a minimum valid value for X if out of bounds */
				int startX = 0;
				if (originX > 0)
					startX = originX;

				/* Continue if Y row lays out of bounds */
				if (originY + y < 0 || originY + y >= m_uiDimensionY)
					continue;

				uint32_t chunkXOffset = startX - (chunkArrPos % m_uiNumChunkX * m_chunkSize.x);
				uint32_t chunkZOffset = (originZ + z) - (ftoi_sse1((float)chunkArrPos / m_uiNumChunkX) * m_chunkSize.z);
				uint32_t chunkGridPos = chunkXOffset + (originY + y) * m_chunkSize.x + m_chunkSize.x * m_chunkSize.y * chunkZOffset;

				Voxel* pVoxel = nullptr;
				pVoxel = &m_ChunkVolumes[chunkArrPos]->at(chunkGridPos);

				for (unsigned x = 0; x < dimX; ++x)
				{
					/* Continue until valid X is found */
					if (originX + x < 0 || originX + x >= m_uiDimensionX)
						continue;

					uint32_t chunkPos = x + y * dimX + dimX * dimY * z;

					chunk[chunkPos] = pVoxel;
					++pVoxel;
				}
			}
		}
	}
	else
	{
		for (unsigned z = 0; z < dimZ; ++z)
		{
			/* Continue if Z row lays out of bounds */
			if (originZ + z < 0 || originZ + z >= m_uiDimensionZ)
				continue;

			for (unsigned y = 0; y < dimY; ++y)
			{
				/* Set a minimum valid value for X if out of bounds */
				int startX = 0;
				if (originX > 0)
					startX = originX;

				/* Continue if Y row lays out of bounds */
				if (originY + y < 0 || originY + y >= m_uiDimensionY)
					continue;

				Voxel* pVoxel = nullptr;

				for (unsigned x = 0; x < dimX; ++x)
				{
					/* Continue until valid X is found */
					if (originX + x < 0 || originX + x >= m_uiDimensionX)
						continue;

					uint32_t gridPos = (originX + x) + (originY + y) * m_uiDimensionX + m_uiDimensionX * m_uiDimensionY * (originZ + z);
					uint32_t chunkPos = x + y * dimX + dimX * dimY * z;

					uint32_t chunkArrPos = ftoi_sse1((float)(originX + x) * m_InvChunkSize.x) + ftoi_sse1((float)(originZ + z) * m_InvChunkSize.z) * m_uiNumChunkX;
					uint32_t chunkXOffset = (originX + x) - (chunkArrPos % m_uiNumChunkX * m_chunkSize.x);
					uint32_t chunkZOffset = (originZ + z) - (ftoi_sse1((float)chunkArrPos / m_uiNumChunkX) * m_chunkSize.z);
					uint32_t chunkGridPos = chunkXOffset + (originY + y) * m_chunkSize.x + m_chunkSize.x * m_chunkSize.y * chunkZOffset;
					pVoxel = &m_ChunkVolumes[chunkArrPos]->at(chunkGridPos);

					chunk[chunkPos] = pVoxel;
				}
			}
		}
	}
	return true;
}

Vector3 VoxelGrid::WorldToGridRound(Vector3 worldPos, bool bAllowOutBounds /*= false*/) const
{
	Vector3 gridPos = (worldPos - m_worldOffset) * m_fInvVoxelSize;
	gridPos.x = round(gridPos.x);
	gridPos.y = round(gridPos.y);
	gridPos.z = round(gridPos.z);

	if (bAllowOutBounds ||
		(gridPos.x > 0 && gridPos.x < m_uiDimensionX &&
			gridPos.y > 0 && gridPos.y < m_uiDimensionY &&
			gridPos.z > 0 && gridPos.z < m_uiDimensionZ)
		)
		return gridPos;

	return Vector3(-1, -1, -1);
}

Vector3 VoxelGrid::WorldToGrid(Vector3 worldPos, bool bAllowOutBounds /*= false*/) const
{
	Vector3 gridPos = (worldPos - m_worldOffset) * m_fInvVoxelSize;
	gridPos.x = floor(gridPos.x);
	gridPos.y = floor(gridPos.y);
	gridPos.z = floor(gridPos.z);

	if (bAllowOutBounds || 
		(gridPos.x > 0 && gridPos.x < m_uiDimensionX &&
		gridPos.y > 0 && gridPos.y < m_uiDimensionY &&
		gridPos.z > 0 && gridPos.z < m_uiDimensionZ)
	)
		return gridPos;
		
	return Vector3(-1, -1, -1);
}

Vector3 VoxelGrid::GridToWorld(Vector3 gridPos) const
{
	return gridPos * static_cast<float>(m_uiVoxelSize) + m_worldOffset;
}

Vector3 VoxelGrid::GridToWorld(uint32_t volumeId) const
{
	float fInvXY = 1.0f / (float)(m_uiDimensionX * m_uiDimensionY);
	float fInvX = 1.0f / (float)m_uiDimensionX;

	Vector3 gridPos;
	
	gridPos.z = static_cast<float>((int)(volumeId * fInvXY));
	gridPos.y = static_cast<float>((int)((volumeId - (gridPos.z * m_uiDimensionX * m_uiDimensionY)) * fInvX));
	gridPos.x = static_cast<float>(volumeId - gridPos.z * m_uiDimensionX *m_uiDimensionY - gridPos.y * m_uiDimensionX);

	return gridPos * static_cast<float>(m_uiVoxelSize) + m_worldOffset;
}

void VoxelGrid::GetDimensions(uint32_t& uiX, uint32_t& uiY, uint32_t& uiZ) const
{
	uiX = m_uiDimensionX;
	uiY = m_uiDimensionY;
	uiZ = m_uiDimensionZ;
}

UVector3 VoxelGrid::GetDimensions() const
{
	return UVector3(m_uiDimensionX, m_uiDimensionY, m_uiDimensionZ);
}