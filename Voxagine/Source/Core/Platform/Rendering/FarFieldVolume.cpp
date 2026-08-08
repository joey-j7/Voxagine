#include "pch.h"

#include "Core/Platform/Rendering/FarFieldVolume.h"
#include "Core/Platform/Rendering/VoxelBrickGrid.h"

#include <cstdio>
#include <cstring>

namespace
{
	uint32_t CeilDiv(uint32_t uiValue, uint32_t uiDivisor)
	{
		return (uiValue + uiDivisor - 1) / uiDivisor;
	}
}

void FarFieldVolume::Resize(const UVector3& v3LevelSize)
{
	m_v3LevelSize = v3LevelSize;

	m_v3GridSize = UVector3(
		CeilDiv(v3LevelSize.x, k_uiScale),
		CeilDiv(v3LevelSize.y, k_uiScale),
		CeilDiv(v3LevelSize.z, k_uiScale)
	);

	const uint32_t uiCellCount = m_v3GridSize.x * m_v3GridSize.y * m_v3GridSize.z;

	if (uiCellCount != m_uiCellCount)
	{
		m_uiCellCount = uiCellCount;

		m_pColors = uiCellCount > 0 ? std::unique_ptr<uint32_t[]>(new uint32_t[uiCellCount]) : nullptr;
		m_pTopY = uiCellCount > 0 ? std::unique_ptr<uint8_t[]>(new uint8_t[uiCellCount]) : nullptr;
	}

	Clear();
}

void FarFieldVolume::Clear()
{
	m_bIsBuilt = false;

	if (m_uiCellCount == 0)
		return;

	memset(m_pColors.get(), 0, m_uiCellCount * sizeof(uint32_t));
	memset(m_pTopY.get(), k_uiEmpty, m_uiCellCount * sizeof(uint8_t));
}

void FarFieldVolume::Flush(uint32_t* pGPU, VoxelBrickGrid& bricks)
{
	if (m_uiCellCount == 0)
		return;

	if (pGPU != nullptr)
		memcpy(pGPU, m_pColors.get(), m_uiCellCount * sizeof(uint32_t));

	/* One region covering the whole cell grid. BeginRegion zeroes it,
	   AddVoxel accumulates, EndRegion pushes the counts to the mirror - the
	   same protocol ChunkSystem::RenderChunk uses per chunk, which is why the
	   brick grid needs no far-field-specific code at all.

	   Reads come from m_pColors rather than from pGPU: the mapping is
	   host-visible and uncached, and a read back out of it costs a PCIe read
	   per cell. */
	const UVector3 v3Zero(0, 0, 0);

	bricks.BeginRegion(false, v3Zero, m_v3GridSize);

	for (uint32_t uiZ = 0; uiZ < m_v3GridSize.z; ++uiZ)
	{
		for (uint32_t uiY = 0; uiY < m_v3GridSize.y; ++uiY)
		{
			const uint32_t uiRowBase = uiY * m_v3GridSize.x + uiZ * m_v3GridSize.x * m_v3GridSize.y;

			for (uint32_t uiX = 0; uiX < m_v3GridSize.x; ++uiX)
			{
				if ((m_pColors[uiRowBase + uiX] >> 24) != 0)
					bricks.AddVoxel(false, uiX, uiY, uiZ);
			}
		}
	}

	bricks.EndRegion(false, v3Zero, m_v3GridSize);
}

uint32_t FarFieldVolume::GetOccupiedCellCount() const
{
	uint32_t uiOccupied = 0;

	for (uint32_t uiCell = 0; uiCell < m_uiCellCount; ++uiCell)
	{
		if ((m_pColors[uiCell] >> 24) != 0)
			++uiOccupied;
	}

	return uiOccupied;
}
