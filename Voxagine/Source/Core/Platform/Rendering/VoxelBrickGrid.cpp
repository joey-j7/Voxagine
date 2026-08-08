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
	const uint32_t uiVoxelCount = v3WorldSize.x * v3WorldSize.y * v3WorldSize.z;
	const uint32_t uiWordCount = CeilDiv(uiVoxelCount, 64);

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

	/* Sized off the voxel count rather than the brick count: a window can keep
	   the same number of bricks while holding a different number of voxels
	   only if it is not brick-aligned, but the two are separate allocations
	   either way and the bitmap is by far the larger of them. */
	if (uiWordCount != m_uiWordCount)
	{
		m_uiWordCount = uiWordCount;

		for (uint32_t i = 0; i < 2; ++i)
			m_pOccupancy[i] = uiWordCount > 0
				? std::unique_ptr<std::atomic<uint64_t>[]>(new std::atomic<uint64_t>[uiWordCount]())
				: nullptr;
	}
	else
	{
		ZeroOccupancy();
	}

	m_uiVoxelCount = uiVoxelCount;

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

void VoxelBrickGrid::ZeroOccupancy()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_pOccupancy[i] == nullptr)
			continue;

		for (uint32_t uiWord = 0; uiWord < m_uiWordCount; ++uiWord)
			m_pOccupancy[i][uiWord].store(0, std::memory_order_relaxed);
	}
}

bool VoxelBrickGrid::IsOccupied(bool bBack, uint32_t uiVoxelID) const
{
	const uint32_t uiIndex = Index(bBack);

	if (uiVoxelID >= m_uiVoxelCount || m_pOccupancy[uiIndex] == nullptr)
		return false;

	const uint64_t uiWord = m_pOccupancy[uiIndex][uiVoxelID >> k_uiWordShift].load(std::memory_order_relaxed);

	return ((uiWord >> (uiVoxelID & k_uiWordMask)) & 1ull) != 0ull;
}

void VoxelBrickGrid::ClearOccupancyRegion(uint32_t uiBuffer, const UVector3& v3Min, const UVector3& v3Size)
{
	std::atomic<uint64_t>* pWords = m_pOccupancy[uiBuffer].get();

	if (pWords == nullptr || m_uiVoxelCount == 0)
		return;

	const uint32_t uiLastX = std::min(v3Min.x + v3Size.x, m_v3WorldSize.x);
	const uint32_t uiLastY = std::min(v3Min.y + v3Size.y, m_v3WorldSize.y);
	const uint32_t uiLastZ = std::min(v3Min.z + v3Size.z, m_v3WorldSize.z);

	if (v3Min.x >= uiLastX)
		return;

	for (uint32_t uiZ = v3Min.z; uiZ < uiLastZ; ++uiZ)
	{
		for (uint32_t uiY = v3Min.y; uiY < uiLastY; ++uiY)
		{
			/* A row of the region is contiguous in the bitmap, so this clears
			   whole words wherever it can and masks only the two ends. */
			const uint32_t uiFirstBit = VoxelID(v3Min.x, uiY, uiZ);
			const uint32_t uiLastBit = uiFirstBit + (uiLastX - v3Min.x);

			uint32_t uiBit = uiFirstBit;

			while (uiBit < uiLastBit)
			{
				const uint32_t uiWord = uiBit >> k_uiWordShift;
				const uint32_t uiOffset = uiBit & k_uiWordMask;
				const uint32_t uiRun = std::min(64u - uiOffset, uiLastBit - uiBit);

				if (uiRun == 64)
				{
					pWords[uiWord].store(0, std::memory_order_relaxed);
				}
				else
				{
					const uint64_t uiMask = ((1ull << uiRun) - 1ull) << uiOffset;
					pWords[uiWord].fetch_and(~uiMask, std::memory_order_relaxed);
				}

				uiBit += uiRun;
			}
		}
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
	ZeroOccupancy();

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_pGPU[i] != nullptr)
			memset(m_pGPU[i], 0, m_uiBrickCount * sizeof(uint32_t));
	}
}

void VoxelBrickGrid::BeginRegion(bool bBack, const UVector3& v3Min, const UVector3& v3Size)
{
	/* Before the early-out below: the bitmap is per voxel and does not depend
	   on there being any bricks, and the caller is about to overwrite this
	   region either way. */
	ClearOccupancyRegion(Index(bBack), v3Min, v3Size);

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

	uint32_t uiBitMismatches = 0;
	uint32_t uiFirstBadBit = UINT32_MAX;

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
				const bool bOccupied = (pVoxelData[uiRowBase + uiX] >> 24) != 0;

				if (bOccupied)
					++expected[uiBrickRowBase + (uiX >> k_uiBrickShift)];

				/* The bitmap is what every write path now believes about this
				   voxel, so it is worth as much as the counts are - a wrong
				   bit produces a wrong count on the *next* write to it, which
				   is much harder to trace back here from. */
				if (IsOccupied(bBack, uiRowBase + uiX) != bOccupied)
				{
					if (uiFirstBadBit == UINT32_MAX)
						uiFirstBadBit = uiRowBase + uiX;

					++uiBitMismatches;
				}
			}
		}
	}

	if (uiBitMismatches > 0)
	{
		fprintf(stderr, "[bricks] occupancy bitmap disagrees with the voxel buffer for %u voxels, first at %u\n",
		        uiBitMismatches, uiFirstBadBit);
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

	fprintf(stderr, "[bricks] validated %u bricks over %u voxels: %u disagree, %u of them would lose geometry; "
	                "%u occupancy bits disagree\n",
	        m_uiBrickCount, m_v3WorldSize.x * m_v3WorldSize.y * m_v3WorldSize.z, uiMismatches, uiLost,
	        uiBitMismatches);

	return uiMismatches + uiBitMismatches;
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
