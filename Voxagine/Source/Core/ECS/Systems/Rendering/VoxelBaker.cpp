#include "pch.h"
#include "VoxelBaker.h"

#include "RenderSystem.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Systems/Rendering/VoxelStamp.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/Application.h"
#include "Core/Platform/Rendering/FrameProfiler.h"

#include <chrono>
#include <cmath>

#define PI 3.14159265359

double degreesToRadians(double angle_in_degrees) {
	return angle_in_degrees * (PI / 180.0);
}

void VoxelBaker::Init(RenderSystem* pRenderSystem, PhysicsSystem* pPhysicsSystem)
{
	m_pRenderSystem = pRenderSystem;
	m_pPhysicsSystem = pPhysicsSystem;

	m_pRenderContext = m_pRenderSystem->m_pRenderContext;
}

void VoxelBaker::Bake()
{
	/* RENDERING_PLAN.md Phase 0: the known main-thread cost this pass
	   measures. Guarded so a disabled profiler pays nothing but this one
	   branch - no chrono call, no Report(). */
	const bool bProfiling = FrameProfiler::Get().IsEnabled();
	const std::chrono::steady_clock::time_point start =
		bProfiling ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();

	VoxelGrid& grid = m_pPhysicsSystem->m_VoxelGrid;

	for (VoxRenderer* pRenderer : m_pRenderSystem->m_VoxRenderers)
	{
		bool bEnabled = pRenderer->IsEnabled();

		bool bIsStaticChunkLoaded = pRenderer->IsChunkInstanceLoaded() && pRenderer->GetOwner()->IsStatic();

		if (!m_pRenderSystem->m_bForcedUpdate && bEnabled && !pRenderer->UpdateRequested() && (!pRenderer->m_BakeData.Updated || bIsStaticChunkLoaded))
			continue;

		/* Remove old voxels */
		Clear(pRenderer);

		if (!bEnabled)
		{
			continue;
		}

		/* Reset */
		Vector3 pos = pRenderer->GetTransform()->GetPosition();

		pos.x = floor(pos.x);
		pos.y = floor(pos.y);
		pos.z = floor(pos.z);

		pRenderer->m_BakeData.LastLocation = pos;
		pRenderer->m_BakeData.LastRotation = pRenderer->GetTransform()->GetRotation();
		pRenderer->m_BakeData.LastScale = pRenderer->GetTransform()->GetScale();
		pRenderer->m_BakeData.WorldOffset = grid.GetWorldOffset();

		pRenderer->m_BakeData.Updated = false;
		pRenderer->m_bUpdateRequested = false;

		/* Occupy new voxels if position is in bounds */
		Occupy(pRenderer, &pRenderer->m_BakeData);
	}

	if (bProfiling)
	{
		const double fMilliseconds =
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

		FrameProfiler::Get().Report("CPU VoxelBaker::Bake", fMilliseconds);
	}
}

uint32_t* VoxelBaker::Occupy(VoxRenderer* pRenderer, VoxRenderer::BakeData* pBakeData)
{
	const VoxFrame* pFrame = pRenderer->GetFrame();

	if (!pFrame)
		return nullptr;

	if (pRenderer->GetWorld()->GetApplication()->IsShuttingDown())
		return nullptr;

	/* Voxel world grid */
	VoxelGrid& grid = m_pPhysicsSystem->m_VoxelGrid;

	bool bIsStatic = pRenderer->GetOwner()->IsStatic();

	/* Placement is shared with the far-field build (RENDERING_PLAN.md phase 4)
	   so that the two grids cannot disagree about where a model is - see
	   VoxelStamp.h. The window's grid origin is the chunk window's world
	   offset, which is exactly what VoxelGrid::WorldToGrid subtracts. */
	VoxelStampTransform stamp;

	if (!ComputeVoxelStampTransform(pRenderer, grid.GetWorldOffset(), 1.f / static_cast<float>(grid.GetVoxelSize()), stamp))
		return nullptr;

	uint32_t uiSolidVoxelCount = pFrame->GetSolidVoxelCount();

	uint32_t* pBaked = new uint32_t[static_cast<size_t>(uiSolidVoxelCount * stamp.RoundedScale.x * stamp.RoundedScale.y * stamp.RoundedScale.z)];
	uint32_t uiBakedID = 0;

	Voxel* pVoxel = nullptr;

	uint64_t uiEntityID = pRenderer->GetOwner()->GetId();

	ForEachStampedVoxel(pRenderer, stamp, [&](const Vector3& worldPosition, uint32_t uiColor)
	{
		/* Written as an in-range test rather than a rejection test so a
		   NaN is discarded too: every comparison against NaN is false,
		   so the old form let it through, and static_cast<int32_t> of a
		   NaN is INT32_MIN - which as a uint32 world ID indexes two
		   billion elements past the voxel array. A renderer whose
		   transform or rotation has gone non-finite is enough to
		   produce one. */
		if (!(worldPosition.x >= 0.f && worldPosition.x < m_pRenderSystem->m_v3WorldSize.x &&
		      worldPosition.y >= 0.f && worldPosition.y < m_pRenderSystem->m_v3WorldSize.y &&
		      worldPosition.z >= 0.f && worldPosition.z < m_pRenderSystem->m_v3WorldSize.z))
		{
			/* Out of bounds is ordinary - a model straddling the world
			   edge. Non-finite is not, and naming it once is the
			   difference between a silent skip and knowing which
			   entity's transform went bad. */
			static bool s_bWarned = false;

			if (!s_bWarned && !std::isfinite(worldPosition.x + worldPosition.y + worldPosition.z))
			{
				s_bWarned = true;
				fprintf(stderr, "[bake] non-finite voxel position from '%s': origin(%.2f %.2f %.2f) voxel(%.2f %.2f %.2f)\n",
				        pRenderer->GetOwner()->GetName().c_str(),
				        stamp.Origin.x, stamp.Origin.y, stamp.Origin.z,
				        worldPosition.x, worldPosition.y, worldPosition.z);
			}

			return;
		}

		// World space ID
		const uint32_t uiWorldID = static_cast<uint32_t>(
			static_cast<int32_t>(worldPosition.x) +
			static_cast<int32_t>(worldPosition.y) * m_pRenderSystem->m_v3WorldSize.x +
			static_cast<int32_t>(worldPosition.z) * m_pRenderSystem->m_v3WorldSize.x * m_pRenderSystem->m_v3WorldSize.y
			);

		bool bForceVoxel = false;

		// Bake as static
		if (bIsStatic)
		{
			// Get grid voxel
			pVoxel = grid.GetVoxel(
				static_cast<uint32_t>(worldPosition.x),
				static_cast<uint32_t>(worldPosition.y),
				static_cast<uint32_t>(worldPosition.z)
			);

			// Check for out-of-bounds
			if (!pVoxel) return;

			bForceVoxel = bIsStatic && ((!pVoxel->UserPointer && !pVoxel->Active) || pVoxel->UserPointer == uiEntityID);

			//TODO: check for layer
			if (bForceVoxel)
			{
				pVoxel->Active = true;
				pVoxel->Color = uiColor;
				pVoxel->UserPointer = uiEntityID;

				m_pRenderSystem->m_pRenderContext->ModifyVoxelFast(uiWorldID, uiColor);
			}
		}

		// Bake color into world
		if (
			bForceVoxel || m_pRenderSystem->ModifyVoxel(
				uiWorldID,
				uiColor, false
			)
			)
		{
			pBaked[uiBakedID] = uiWorldID;
			uiBakedID++;
		}
	});

	pBakeData = pBakeData ? pBakeData : &pRenderer->m_BakeData;

	if (pBakeData->Positions)
		delete[] pBakeData->Positions;

	pBakeData->Positions = pBaked;
	pBakeData->IsStatic = bIsStatic;
	pBakeData->Size = uiBakedID;

	return pBaked;
}

void VoxelBaker::Clear(VoxRenderer* pRenderer, VoxRenderer::BakeData* pBakeData)
{
	if (pRenderer->GetWorld()->GetApplication()->IsShuttingDown())
		return;

	VoxelGrid& grid = m_pPhysicsSystem->m_VoxelGrid;
	UVector3 gridDims = grid.GetDimensions();
	Vector3 worldOffsetDiff = pRenderer->m_BakeData.WorldOffset - grid.GetWorldOffset();

	pBakeData = pBakeData ? pBakeData : &pRenderer->m_BakeData;

	/* Remove old voxels if array is valid */
	if (pBakeData->Positions)
	{
		uint32_t arrSize = pBakeData->Size;
		bool bStatic = pBakeData->IsStatic;
		Vector3 voxelPos(0);

		for (uint32_t i = 0; i < arrSize; ++i)
		{
			//Continue on invalid position
			if (pBakeData->Positions[i] == UINT_MAX)
				continue;

			voxelPos = (Vector3)grid.GetVoxelPosition(pBakeData->Positions[i]);
			if (grid.IsOutOfBounds(voxelPos + worldOffsetDiff))
				continue;

			uint32_t chunkOffsetPosition = (uint32_t)((int)pBakeData->Positions[i] + (int)(worldOffsetDiff.x + worldOffsetDiff.z * gridDims.y * gridDims.x));
			if (chunkOffsetPosition >= grid.GetNumVoxels())
				continue;

			m_pRenderSystem->m_pRenderContext->ModifyVoxelFast(chunkOffsetPosition, 0);

			if (bStatic)
			{
				voxelPos = grid.GetVoxelPosition(chunkOffsetPosition);
				Voxel* pVoxel = grid.GetVoxel((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z);

				pVoxel->Active = false;
				pVoxel->UserPointer = 0;
				pVoxel->Color = 0;
			}
		}

		delete[] pBakeData->Positions;
		pBakeData->Positions = nullptr;
		pBakeData->IsStatic = false;
	}
	else /* Try to remove voxels which could have been placed by the chunk system rendering */
	{
		Box bounds = pRenderer->GetBounds();
		UVector3 size = static_cast<UVector3>(bounds.GetSize());
		uint32_t numVoxels = size.x * size.y * size.z;
		Voxel** voxels = new Voxel*[numVoxels];
		UVector3 chunkStart = grid.WorldToGrid(pBakeData->LastLocation - static_cast<Vector3>(size) * 0.5f, true);
		if (!grid.GetChunk(voxels, chunkStart, size, true))
		{
			delete[] voxels;
			return;
		}

		bool bIsStatic = pRenderer->GetOwner()->IsStatic();
		for (uint32_t i = 0; i < numVoxels; ++i)
		{
			/* Skip if the voxel is invalid */
			if (!voxels[i]) continue;

			if (bIsStatic && voxels[i]->UserPointer == pRenderer->GetOwner()->GetId())
			{
				UVector3 chunkRelVec = VoxelGrid::IndexToVector(i, size);
				uint32_t voxelPos = (chunkRelVec.x + chunkStart.x) + (chunkRelVec.y + chunkStart.y) * gridDims.x + (chunkRelVec.z + chunkStart.z) * gridDims.x * gridDims.y;

				m_pRenderSystem->m_pRenderContext->ModifyVoxelFast(voxelPos, 0);

				if (bIsStatic)
				{
					voxels[i]->Active = false;
					voxels[i]->UserPointer = 0;
					voxels[i]->Color = 0;
				}
			}
		}

		delete[] voxels;
	}
}