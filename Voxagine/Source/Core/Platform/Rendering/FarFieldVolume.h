#pragma once

#include "Core/Math.h"

#include <cstdint>
#include <memory>

/* A static, downsampled colour volume of the *whole* level, for display only -
 * RENDERING_PLAN.md phase 4.
 *
 * The resident voxel window is a sliding 3x3 chunk view, so the renderer
 * physically cannot draw anything past it: the rest of the level is not on the
 * GPU. This is the rest of the level, at k_uiScale^3 fewer cells: 1536x128x1536
 * downsampled 4x is 384x32x384 = 4.7M cells x 4 B = 18 MiB, against the 512 MiB
 * that widening the window to 4x4 would have cost.
 *
 * Nothing in the game reads it. Physics, entities and destruction all continue
 * to live in the detail window, untouched; this exists so that a ray which
 * leaves the window has something to hit.
 *
 * Coordinates. Three spaces are in play and they are easy to confuse:
 *
 *   - *level* space   - voxels, origin at the level's corner. What this class's
 *                       AddVoxel takes and what the far-field cell grid is a
 *                       scaling of.
 *   - *window* space  - voxels, origin at the resident window's corner. What
 *                       SDFMarcher.hlsl and VoxelGrid work in.
 *   - *cell* space    - this volume's own cells, level space >> k_uiShift.
 *
 * window + camOffset == level, where camOffset is VoxelGrid::GetWorldOffset().
 * The conversion lives in exactly two places: FarField.hlsl on the GPU and
 * here.
 *
 * Linearization is the voxel convention (rule 4) applied to the cell grid:
 *   cell = x + y*G.x + z*G.x*G.y,  G = ceil(levelSize / k_uiScale)
 * FarField.hlsl's PosToFarFieldID must agree.
 */
class FarFieldVolume
{
public:
	static const uint32_t k_uiShift = 2;
	static const uint32_t k_uiScale = 1u << k_uiShift;

	/* Reallocates for a level size in voxels and clears every cell. Touches CPU
	   state only - the caller sizes the far-field Mapper to GetCellCount()
	   elements afterwards and calls Flush with its mapping. */
	void Resize(const UVector3& v3LevelSize);

	const UVector3& GetLevelSize() const { return m_v3LevelSize; }
	const UVector3& GetGridSize() const { return m_v3GridSize; }
	uint32_t GetCellCount() const { return m_uiCellCount; }

	/* True once a build has actually put something in it. The shader reads the
	   cell grid size from the camera buffer and skips the far field entirely
	   when it is zero, so a level whose window already covers everything (a 1x1
	   chunk grid) never pays for one. */
	bool IsBuilt() const { return m_bIsBuilt; }
	void SetBuilt(bool bBuilt) { m_bIsBuilt = bBuilt; }

	void Clear();

	/* Stamps one level-space voxel. Out-of-range coordinates are dropped rather
	   than clamped - a model straddling the level edge is ordinary.

	   The topmost contributor within a cell wins, rather than the first or an
	   average of all 64. Voxel scenes are viewed from above far more often than
	   from below, so the top of a 4-voxel column is the colour a distant viewer
	   would actually have seen, and picking it is order-independent: the result
	   does not depend on which chunk or entity was baked first. */
	inline void AddVoxel(uint32_t uiX, uint32_t uiY, uint32_t uiZ, uint32_t uiColor)
	{
		if (m_uiCellCount == 0 || uiX >= m_v3LevelSize.x || uiY >= m_v3LevelSize.y || uiZ >= m_v3LevelSize.z)
			return;

		const uint32_t uiCell = CellID(uiX >> k_uiShift, uiY >> k_uiShift, uiZ >> k_uiShift);
		const uint8_t uiSubY = static_cast<uint8_t>(uiY & (k_uiScale - 1));

		if (m_pTopY[uiCell] != k_uiEmpty && m_pTopY[uiCell] > uiSubY)
			return;

		m_pTopY[uiCell] = uiSubY;

		/* Occupancy is alpha > 0 (rule 3), but the alpha byte's *value* is a
		   rendererState + 1 tag that means nothing out here - there is no
		   selection outline or grid line to draw on a cell four voxels wide. It
		   is forced opaque so the far field can never accidentally inherit a
		   tag some later phase gives meaning to. */
		m_pColors[uiCell] = (uiColor & 0x00FFFFFFu) | 0xFF000000u;
	}

	/* Writes every cell to the host-visible mapping and rebuilds pBricks from
	   the result. Both are one-shot: the volume is static after a build, so
	   unlike VoxelBrickGrid's window there is no incremental update path and
	   no second buffer.

	   pBricks must already have been Resize'd to GetGridSize() and given its
	   mapping - it describes the *cell* grid, not the voxel one. */
	void Flush(uint32_t* pGPU, class VoxelBrickGrid& bricks);

	/* Occupied cells, for the log line that says whether a build found
	   anything. */
	uint32_t GetOccupiedCellCount() const;

	bool IsCellOccupied(uint32_t uiX, uint32_t uiY, uint32_t uiZ) const
	{
		return (uiX < m_v3GridSize.x && uiY < m_v3GridSize.y && uiZ < m_v3GridSize.z)
			&& (m_pColors[CellID(uiX, uiY, uiZ)] >> 24) != 0;
	}


private:
	static const uint8_t k_uiEmpty = 0xFF;

	inline uint32_t CellID(uint32_t uiX, uint32_t uiY, uint32_t uiZ) const
	{
		return uiX + uiY * m_v3GridSize.x + uiZ * m_v3GridSize.x * m_v3GridSize.y;
	}

	UVector3 m_v3LevelSize = UVector3(0, 0, 0);
	UVector3 m_v3GridSize = UVector3(0, 0, 0);
	uint32_t m_uiCellCount = 0;

	bool m_bIsBuilt = false;

	std::unique_ptr<uint32_t[]> m_pColors;
	std::unique_ptr<uint8_t[]> m_pTopY;
};
