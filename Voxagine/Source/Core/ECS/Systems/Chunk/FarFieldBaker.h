#pragma once

class World;
class FarFieldVolume;

/* Fills a FarFieldVolume with the whole level's static geometry -
 * RENDERING_PLAN.md phase 4.
 *
 * **Where the level's voxels actually come from.** The plan expected to build
 * this from the RLE data in `Chunk::m_pEncodedVoxelData`. There is none to read:
 * a `.wld` stores per chunk only a `RootEntities` array, and a chunk's
 * `m_VoxelData` is a *product* of loading - the ground plane written by
 * `Chunk::UpdateGroundPlane` plus whatever `VoxelBaker` stamps once the chunk's
 * entities are in the world. `EncodeVoxels` runs on *unload*, so a chunk that
 * has never been resident has neither decoded nor encoded voxels. Building from
 * chunk voxel data would therefore have produced a far field showing only the
 * places the player had already been.
 *
 * So the source is the same thing the detail window's source is: the entities.
 * Each chunk's `RootEntities` values are deserialized into throwaway entities
 * that are never added to the world - the pattern `Chunk::LoadEntities` already
 * uses for an entity it decides not to keep - walked for `VoxRenderer`s, and
 * stamped through the shared `VoxelStamp.h` placement so the far field and the
 * window cannot disagree about where a model is.
 *
 * **The ground plane is deliberately not baked in**, and it was tried. The
 * endless ground `GetBackground` draws is not an approximation of the chunk
 * ground plane - it samples the resident window's own y=0 layer, which *is*
 * `Chunk::UpdateGroundPlane`'s output, tiled with `fmod`. So it already
 * reproduces the level's ground exactly, everywhere, and past the level's edge
 * as well, where a baked one would stop.
 *
 * Baking it in on top of that put the same surface in two places with two
 * different lightings - the far field's cells win on distance inside the level
 * and the analytic plane takes over outside it - and the result was a hard
 * brightness edge along the detail window's boundary. It also marked the whole
 * bottom brick layer occupied, so every downward ray descended to the fine
 * walk. Both costs, for a surface that was already being drawn.
 */
namespace FarFieldBaker
{
	/* Rebuilds pVolume from every chunk in pWorld's ChunkSystem. Safe to call
	   with no chunk system or an empty level; both leave the volume unbuilt,
	   which the shader reads as "no far field". Main thread only - it
	   deserializes entities and loads models through the ResourceManager. */
	void Build(World* pWorld, FarFieldVolume& volume);

	/* Cross-checks placement against the chunks that are currently resident,
	   which hold the same geometry at full resolution, and names cells that
	   disagree. Returns the number of *phantom* cells - occupied in the far
	   field, empty in the chunk - which is placement being wrong. The reverse,
	   *missing*, is largely expected: the ground plane and every dynamic entity
	   are in a chunk's voxels and deliberately not in the far field.

	   Reads `Chunk::GetVoxelData`, not the voxel mapper. They hold the same
	   voxels, but the mapper is host-visible and uncached - likely VRAM over
	   PCIe, since it started preferring ReBAR - and sweeping the 75 million
	   voxels of a 768x128x768 window out of it takes long enough to look like
	   the editor has hung. It did; that was the first version of this. A chunk's
	   std::vector<Voxel> is ordinary cached memory. */
	uint32_t Validate(World* pWorld, const FarFieldVolume& volume);
}
