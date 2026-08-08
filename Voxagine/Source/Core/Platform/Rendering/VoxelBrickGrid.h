#pragma once

#include "Core/Math.h"

#include <atomic>
#include <cstdint>
#include <memory>

/* Coarse occupancy over the resident voxel window: one count of occupied
 * voxels per 8^3 brick, so the marcher can skip empty space structurally
 * instead of stepping through it. RENDERING_PLAN.md phase 2.
 *
 * A *count*, not a bit, because destruction has to be able to un-set a brick.
 * A bit can be set from a single voxel write but clearing it correctly needs a
 * rescan of the other 511; a count just decrements. 8^3 = 512 fits a uint16.
 *
 * Linearization is the voxel convention (rule 4) scaled down:
 *   brick = x + y*B.x + z*B.x*B.y,  B = ceil(worldSize / k_uiBrickSize)
 * HLSL's PosToBrickID/GetBrickGridSize in SDFMarcher.hlsl must agree.
 *
 * Two representations are kept deliberately:
 *
 *   - m_pCounts, plain CPU memory, is authoritative and is what every
 *     read-modify-write goes through.
 *   - m_pGPU points into the brick Mapper's host-visible buffers and is
 *     written to, never read from.
 *
 * That split is the point. The mapped buffers are HOST_VISIBLE and uncached
 * (and DEVICE_LOCAL too where ReBAR allows it), so `++count[brick]` performed
 * directly on the mapping would be an uncached read per voxel write. Keeping
 * the counts in ordinary memory makes every update a cached RMW plus one
 * streaming store.
 *
 * Front and back mirror the voxel Mapper's two buffers and must be swapped in
 * lockstep with it - a brick grid built against the pre-swap window describes
 * the wrong voxels. Each buffer's counts describe only that buffer's voxels,
 * so the two never have to agree.
 */
class VoxelBrickGrid
{
public:
	static const uint32_t k_uiBrickShift = 3;
	static const uint32_t k_uiBrickSize = 1u << k_uiBrickShift;
	static const uint32_t k_uiBrickVolume = k_uiBrickSize * k_uiBrickSize * k_uiBrickSize;

	/* Reallocates for a new window size and zeroes every count. Touches CPU
	   state only and drops the mirror pointers - the caller resizes the brick
	   Mapper to GetBrickCount() elements afterwards, hands the new mappings
	   back with SetBuffers, and calls Flush. */
	void Resize(const UVector3& v3WorldSize);

	/* The two host-visible mirrors, in the same front/back sense the voxel
	   Mapper is in right now. Re-supply these after any mapper resize. */
	void SetBuffers(uint32_t* pFront, uint32_t* pBack);

	/* Mirrors Mapper::SwapBuffer. */
	void Swap();

	/* Writes every count to its mirror. Only needed after a resize; the update
	   paths keep the mirrors current themselves. */
	void Flush();

	/* Zeroes both buffers, CPU side and mirrors. */
	void ClearAll();

	const UVector3& GetWorldSize() const { return m_v3WorldSize; }
	const UVector3& GetGridSize() const { return m_v3GridSize; }
	uint32_t GetBrickCount() const { return m_uiBrickCount; }

	uint32_t GetCount(bool bBack, uint32_t uiBrickID) const
	{
		return uiBrickID < m_uiBrickCount
			? m_pCounts[Index(bBack)][uiBrickID].load(std::memory_order_relaxed)
			: 0;
	}

	inline uint32_t VoxelToBrick(uint32_t uiVoxelID) const
	{
		if (m_uiBrickCount == 0)
			return UINT32_MAX;

		const uint32_t uiX = uiVoxelID % m_v3WorldSize.x;
		const uint32_t uiRest = uiVoxelID / m_v3WorldSize.x;
		const uint32_t uiY = uiRest % m_v3WorldSize.y;
		const uint32_t uiZ = uiRest / m_v3WorldSize.y;

		return BrickID(uiX >> k_uiBrickShift, uiY >> k_uiBrickShift, uiZ >> k_uiBrickShift);
	}

	/* Point update for the front buffer, from RenderContext::ModifyVoxel and
	   friends. Only occupancy transitions move the count - overwriting an
	   occupied voxel with a different colour must not increment (rule 3:
	   occupancy is alpha > 0, and alpha carries rendererState + 1, so it is
	   never simply 1). */
	inline void SetVoxel(uint32_t uiVoxelID, bool bWasOccupied, bool bIsOccupied)
	{
		if (bWasOccupied == bIsOccupied)
			return;

		const uint32_t uiBrickID = VoxelToBrick(uiVoxelID);

		if (uiBrickID >= m_uiBrickCount)
			return;

		std::atomic<uint16_t>& count = m_pCounts[m_uiFront][uiBrickID];
		uint16_t uiNew;

		if (bIsOccupied)
		{
			uiNew = static_cast<uint16_t>(count.fetch_add(1, std::memory_order_relaxed) + 1);
		}
		else
		{
			const uint16_t uiOld = count.fetch_sub(1, std::memory_order_relaxed);

			if (uiOld == 0)
			{
				/* Underflow means a decrement arrived for a voxel this grid
				   never counted as occupied. Wrapping to 65535 would only cost
				   traversal, but it would also hide the accounting bug that
				   produced it, so put it back and say so. */
				count.fetch_add(1, std::memory_order_relaxed);
				ReportUnderflow(uiBrickID);
				return;
			}

			uiNew = static_cast<uint16_t>(uiOld - 1);
		}

		if (m_pGPU[m_uiFront] != nullptr)
			m_pGPU[m_uiFront][uiBrickID] = uiNew;
	}

	/* Bulk path, for callers that overwrite a whole rectangular region of the
	   window in one pass (ChunkSystem::RenderChunk). Old occupancy is never
	   consulted - the region is zeroed, re-accumulated from the source data
	   the caller is writing, and flushed:

	       BeginRegion(...);
	       for each voxel written:  if occupied -> AddVoxel(...)
	       EndRegion(...);

	   Regions that are not brick-aligned leave bricks straddling the edge only
	   partly accounted for; EndRegion marks those fully occupied rather than
	   under-counting them, since an over-counted brick costs traversal while an
	   under-counted one loses geometry. */
	void BeginRegion(bool bBack, const UVector3& v3Min, const UVector3& v3Size);
	void EndRegion(bool bBack, const UVector3& v3Min, const UVector3& v3Size);

	inline void AddVoxel(bool bBack, uint32_t uiX, uint32_t uiY, uint32_t uiZ)
	{
		const uint32_t uiBrickID = BrickID(uiX >> k_uiBrickShift, uiY >> k_uiBrickShift, uiZ >> k_uiBrickShift);

		if (uiBrickID >= m_uiBrickCount)
			return;

		/* Non-atomic on purpose: a region is owned by exactly one thread for
		   the length of a Begin/End pair, and the two buffers are disjoint. */
		std::atomic<uint16_t>& count = m_pCounts[Index(bBack)][uiBrickID];
		count.store(static_cast<uint16_t>(count.load(std::memory_order_relaxed) + 1), std::memory_order_relaxed);
	}

	/* Recomputes every count from the voxel buffer and reports disagreements.
	   Reads the whole window out of uncached host-visible memory, so it is a
	   debugging tool, not something to run per frame. Returns the number of
	   bricks that disagreed. */
	uint32_t Validate(bool bBack, const uint32_t* pVoxelData) const;

private:
	uint32_t Index(bool bBack) const { return bBack ? (m_uiFront ^ 1u) : m_uiFront; }

	inline uint32_t BrickID(uint32_t uiX, uint32_t uiY, uint32_t uiZ) const
	{
		return uiX + uiY * m_v3GridSize.x + uiZ * m_v3GridSize.x * m_v3GridSize.y;
	}

	void ZeroCounts();

	static void ReportUnderflow(uint32_t uiBrickID);

	UVector3 m_v3WorldSize = UVector3(0, 0, 0);
	UVector3 m_v3GridSize = UVector3(0, 0, 0);
	uint32_t m_uiBrickCount = 0;

	uint32_t m_uiFront = 0;

	std::unique_ptr<std::atomic<uint16_t>[]> m_pCounts[2];
	uint32_t* m_pGPU[2] = { nullptr, nullptr };
};
