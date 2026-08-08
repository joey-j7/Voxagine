#include "pch.h"

#include "Core/Platform/Rendering/VoxelBrickGrid.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
	uint32_t CeilDiv(uint32_t uiValue, uint32_t uiDivisor)
	{
		return (uiValue + uiDivisor - 1) / uiDivisor;
	}
}

void VoxelBrickGrid::Resize(const UVector3& v3WorldSize)
{
	/* Deliberately touches only CPU state: the mirrors belong to a Mapper the
	   caller has not resized yet, so the pointers in m_pGPU are about to be
	   freed. The caller resizes the mapper, calls SetBuffers, then Flush. */
	m_v3WorldSize = v3WorldSize;

	m_v3GridSize = UVector3(
		CeilDiv(v3WorldSize.x, k_uiBrickSize),
		CeilDiv(v3WorldSize.y, k_uiBrickSize),
		CeilDiv(v3WorldSize.z, k_uiBrickSize)
	);

	const uint32_t uiBrickCount = m_v3GridSize.x * m_v3GridSize.y * m_v3GridSize.z;

	if (uiBrickCount != m_uiBrickCount)
	{
		m_uiBrickCount = uiBrickCount;

		for (uint32_t i = 0; i < 2; ++i)
			m_pCounts[i] = uiBrickCount > 0
				? std::unique_ptr<std::atomic<uint16_t>[]>(new std::atomic<uint16_t>[uiBrickCount]())
				: nullptr;
	}
	else
	{
		ZeroCounts();
	}

	m_pGPU[0] = nullptr;
	m_pGPU[1] = nullptr;
}

void VoxelBrickGrid::SetBuffers(uint32_t* pFront, uint32_t* pBack)
{
	m_pGPU[m_uiFront] = pFront;
	m_pGPU[m_uiFront ^ 1u] = pBack;
}

void VoxelBrickGrid::Swap()
{
	m_uiFront ^= 1u;
}

void VoxelBrickGrid::ZeroCounts()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_pCounts[i] == nullptr)
			continue;

		for (uint32_t uiBrick = 0; uiBrick < m_uiBrickCount; ++uiBrick)
			m_pCounts[i][uiBrick].store(0, std::memory_order_relaxed);
	}
}

void VoxelBrickGrid::Flush()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_pCounts[i] == nullptr || m_pGPU[i] == nullptr)
			continue;

		for (uint32_t uiBrick = 0; uiBrick < m_uiBrickCount; ++uiBrick)
			m_pGPU[i][uiBrick] = m_pCounts[i][uiBrick].load(std::memory_order_relaxed);
	}
}

void VoxelBrickGrid::ClearAll()
{
	ZeroCounts();

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_pGPU[i] != nullptr)
			memset(m_pGPU[i], 0, m_uiBrickCount * sizeof(uint32_t));
	}
}

void VoxelBrickGrid::BeginRegion(bool bBack, const UVector3& v3Min, const UVector3& v3Size)
{
	if (m_uiBrickCount == 0)
		return;

	std::atomic<uint16_t>* pCounts = m_pCounts[Index(bBack)].get();

	if (pCounts == nullptr)
		return;

	/* Outward-rounded: every brick the region touches, including ones it only
	   partly covers. EndRegion is what decides what to do with those. */
	const UVector3 v3First(
		v3Min.x >> k_uiBrickShift,
		v3Min.y >> k_uiBrickShift,
		v3Min.z >> k_uiBrickShift
	);

	const UVector3 v3Last(
		std::min((v3Min.x + v3Size.x - 1) >> k_uiBrickShift, m_v3GridSize.x - 1),
		std::min((v3Min.y + v3Size.y - 1) >> k_uiBrickShift, m_v3GridSize.y - 1),
		std::min((v3Min.z + v3Size.z - 1) >> k_uiBrickShift, m_v3GridSize.z - 1)
	);

	for (uint32_t uiZ = v3First.z; uiZ <= v3Last.z; ++uiZ)
	{
		for (uint32_t uiY = v3First.y; uiY <= v3Last.y; ++uiY)
		{
			for (uint32_t uiX = v3First.x; uiX <= v3Last.x; ++uiX)
				pCounts[BrickID(uiX, uiY, uiZ)].store(0, std::memory_order_relaxed);
		}
	}
}

void VoxelBrickGrid::EndRegion(bool bBack, const UVector3& v3Min, const UVector3& v3Size)
{
	if (m_uiBrickCount == 0)
		return;

	const uint32_t uiIndex = Index(bBack);
	std::atomic<uint16_t>* pCounts = m_pCounts[uiIndex].get();
	uint32_t* pGPU = m_pGPU[uiIndex];

	if (pCounts == nullptr)
		return;

	const UVector3 v3First(
		v3Min.x >> k_uiBrickShift,
		v3Min.y >> k_uiBrickShift,
		v3Min.z >> k_uiBrickShift
	);

	const UVector3 v3Last(
		std::min((v3Min.x + v3Size.x - 1) >> k_uiBrickShift, m_v3GridSize.x - 1),
		std::min((v3Min.y + v3Size.y - 1) >> k_uiBrickShift, m_v3GridSize.y - 1),
		std::min((v3Min.z + v3Size.z - 1) >> k_uiBrickShift, m_v3GridSize.z - 1)
	);

	/* A brick straddling the region edge was only counted for the part of it
	   the caller rewrote, so its count is an undercount of what is actually
	   there - and an undercounted brick that reaches zero deletes visible
	   geometry. Chunk regions are brick-aligned in practice (chunk sizes and
	   the window height are multiples of 8), so this is a guard against
	   content that is not, and it fires per region rather than per frame. */
	const bool bAligned =
		(v3Min.x % k_uiBrickSize) == 0 && (v3Size.x % k_uiBrickSize) == 0 &&
		(v3Min.y % k_uiBrickSize) == 0 && (v3Size.y % k_uiBrickSize) == 0 &&
		(v3Min.z % k_uiBrickSize) == 0 && (v3Size.z % k_uiBrickSize) == 0;

	if (!bAligned)
	{
		static bool s_bWarned = false;

		if (!s_bWarned)
		{
			s_bWarned = true;
			fprintf(stderr, "[bricks] region (%u %u %u)+(%u %u %u) is not a whole number of %u-voxel bricks; "
			                "edge bricks are marked fully occupied\n",
			        v3Min.x, v3Min.y, v3Min.z, v3Size.x, v3Size.y, v3Size.z, k_uiBrickSize);
		}
	}

	for (uint32_t uiZ = v3First.z; uiZ <= v3Last.z; ++uiZ)
	{
		for (uint32_t uiY = v3First.y; uiY <= v3Last.y; ++uiY)
		{
			for (uint32_t uiX = v3First.x; uiX <= v3Last.x; ++uiX)
			{
				const uint32_t uiBrickID = BrickID(uiX, uiY, uiZ);

				if (!bAligned)
				{
					const bool bInside =
						uiX * k_uiBrickSize >= v3Min.x && (uiX + 1) * k_uiBrickSize <= v3Min.x + v3Size.x &&
						uiY * k_uiBrickSize >= v3Min.y && (uiY + 1) * k_uiBrickSize <= v3Min.y + v3Size.y &&
						uiZ * k_uiBrickSize >= v3Min.z && (uiZ + 1) * k_uiBrickSize <= v3Min.z + v3Size.z;

					if (!bInside)
						pCounts[uiBrickID].store(static_cast<uint16_t>(k_uiBrickVolume), std::memory_order_relaxed);
				}

				if (pGPU != nullptr)
					pGPU[uiBrickID] = pCounts[uiBrickID].load(std::memory_order_relaxed);
			}
		}
	}
}

uint32_t VoxelBrickGrid::Validate(bool bBack, const uint32_t* pVoxelData) const
{
	if (m_uiBrickCount == 0 || pVoxelData == nullptr)
		return 0;

	const std::atomic<uint16_t>* pCounts = m_pCounts[Index(bBack)].get();

	if (pCounts == nullptr)
		return 0;

	std::vector<uint16_t> expected(m_uiBrickCount, 0);

	for (uint32_t uiZ = 0; uiZ < m_v3WorldSize.z; ++uiZ)
	{
		for (uint32_t uiY = 0; uiY < m_v3WorldSize.y; ++uiY)
		{
			const uint32_t uiRowBase = uiY * m_v3WorldSize.x + uiZ * m_v3WorldSize.x * m_v3WorldSize.y;
			const uint32_t uiBrickRowBase =
				(uiY >> k_uiBrickShift) * m_v3GridSize.x +
				(uiZ >> k_uiBrickShift) * m_v3GridSize.x * m_v3GridSize.y;

			for (uint32_t uiX = 0; uiX < m_v3WorldSize.x; ++uiX)
			{
				if ((pVoxelData[uiRowBase + uiX] >> 24) != 0)
					++expected[uiBrickRowBase + (uiX >> k_uiBrickShift)];
			}
		}
	}

	uint32_t uiMismatches = 0;
	uint32_t uiLost = 0;

	for (uint32_t uiBrick = 0; uiBrick < m_uiBrickCount; ++uiBrick)
	{
		const uint16_t uiActual = pCounts[uiBrick].load(std::memory_order_relaxed);

		if (uiActual == expected[uiBrick])
			continue;

		++uiMismatches;

		/* The two failure modes are not equally bad: a brick counted higher
		   than it is only costs traversal, one counted at zero when it holds
		   something deletes geometry from the image. */
		if (uiActual == 0 && expected[uiBrick] != 0)
			++uiLost;

		if (uiMismatches <= 8)
		{
			fprintf(stderr, "[bricks] brick %u: counted %u, actually %u\n",
			        uiBrick, uiActual, expected[uiBrick]);
		}
	}

	fprintf(stderr, "[bricks] validated %u bricks over %u voxels: %u disagree, %u of them would lose geometry\n",
	        m_uiBrickCount, m_v3WorldSize.x * m_v3WorldSize.y * m_v3WorldSize.z, uiMismatches, uiLost);

	return uiMismatches;
}

void VoxelBrickGrid::ReportUnderflow(uint32_t uiBrickID)
{
	static bool s_bWarned = false;

	if (s_bWarned)
		return;

	s_bWarned = true;

	fprintf(stderr, "[bricks] occupancy count for brick %u went below zero - a voxel was cleared that the "
	                "grid never counted as occupied\n", uiBrickID);
}
