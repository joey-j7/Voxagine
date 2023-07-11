#include "pch.h"
#include "VoxelBaker.h"

#include "RenderSystem.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/Application.h"

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

	const uint32_t* pColors = pFrame->GetColors();
	const uint32_t* pPositions = pFrame->GetPositions();

	Transform* pTransform = pRenderer->GetTransform();

	Vector3 forward = pTransform->GetForward();
	Vector3 right = pTransform->GetRight();
	Vector3 up = pTransform->GetUp();

	Quaternion quat = pTransform->GetRotation();
	Vector3 originOffset(0.f);

	if (pRenderer->IsAxisRounded())
	{
		float fRotationLimit = 45.f;
		Vector3 rotation = pTransform->GetEulerAngles();

		rotation.x = std::fmod(rotation.x + 360.f, 360.f);
		rotation.y = std::fmod(rotation.y + 360.f, 360.f);
		rotation.z = std::fmod(rotation.z + 360.f, 360.f);

		if (abs(rotation.x) > 90 + fRotationLimit / 2.f)
			originOffset.z += 1;

		if (abs(rotation.z) > 90 + fRotationLimit / 2.f)
			originOffset.x += 1;

		rotation.x = round(rotation.x / fRotationLimit) * fRotationLimit * DEG2RAD;
		rotation.y = round(rotation.y / fRotationLimit) * fRotationLimit * DEG2RAD;
		rotation.z = round(rotation.z / fRotationLimit) * fRotationLimit * DEG2RAD;

		quat = glm::quat(glm::vec3(rotation.x, rotation.y, rotation.z));

		forward = glm::round(forward);
		right = glm::round(right);
		up = glm::round(up);
	}
	else if (pRenderer->IsRotationAngleLimited())
	{
		float fRotationLimit = static_cast<float>(pRenderer->GetRotationAngleLimit());
		Vector3 rotation = pTransform->GetEulerAngles();

		rotation.x = std::fmod(rotation.x + 360.f, 360.f);
		rotation.y = std::fmod(rotation.y + 360.f, 360.f);
		rotation.z = std::fmod(rotation.z + 360.f, 360.f);

		if (abs(rotation.x) > 90 + fRotationLimit / 2.f)
			originOffset.z += 1;

		if (abs(rotation.z) > 90 + fRotationLimit / 2.f)
			originOffset.x += 1;

		rotation.x = round(rotation.x / fRotationLimit) * fRotationLimit * DEG2RAD;
		rotation.y = round(rotation.y / fRotationLimit) * fRotationLimit * DEG2RAD;
		rotation.z = round(rotation.z / fRotationLimit) * fRotationLimit * DEG2RAD;

		quat = glm::quat(rotation);

		forward = glm::rotate(quat, Vector3(0, 0, 1));
		right = glm::rotate(quat, Vector3(1, 0, 0));
		up = glm::rotate(quat, Vector3(0, 1, 0));
	}

	Vector3 scale = pTransform->GetScale();
	Vector3 roundedScale = glm::ceil(glm::abs(scale));

	Vector3 size = pFrame->GetFittedSize();

	Vector3 offset = -size * 0.5f;
	Vector3 origin;

	if (pFrame->GetModel()->GetFrameCount() > 1) {

		const VoxFrame* tFrame = pFrame->GetModel()->GetFrame(0);
		Vector3 offsetCen = -(tFrame->GetFitSizeOffset() - pFrame->GetFitSizeOffset()) * 0.5f
			+ ((pFrame->GetFitSizeOffset() + pFrame->GetFittedSize()) - (tFrame->GetFitSizeOffset() + tFrame->GetFittedSize())) * 0.5f;
		offsetCen.y *= -1.f;

		origin = grid.WorldToGrid(pTransform->GetPosition(), true) + scale * glm::rotate(quat, offset - offsetCen);
	}
	else {
		origin = grid.WorldToGrid(pTransform->GetPosition() + scale * glm::floor(glm::rotate(quat, offset)), true);
	}

	origin -= originOffset;
	uint32_t uiSolidVoxelCount = pFrame->GetSolidVoxelCount();

	uint32_t* pBaked = new uint32_t[static_cast<size_t>(uiSolidVoxelCount * roundedScale.x * roundedScale.y * roundedScale.z)];
	uint32_t uiBakedID = 0;

	VColor vColPosition;
	Vector3 modelPosition;

	uint32_t uiWorldID = 0;
	Vector3 worldPosition = Vector3(0.f, 0.f, 0.f);

	UVector3 scaleOffset = UVector3(0, 0, 0);
	Vector3 lastPosition = Vector3(0.f, 0.f, 0.f);

	Voxel* pVoxel = nullptr;
	uint32_t uiColor = 0;

	VColor overrideColor = pRenderer->GetOverrideColor();
	const bool bHasOverrideColor = overrideColor.inst.Colors.a > 0;

	RenderState rendererState = pRenderer->GetState();
	uint64_t uiEntityID = pRenderer->GetOwner()->GetId();

	for (uint32_t i = 0; i < uiSolidVoxelCount; ++i)
	{
		for (scaleOffset.x = 0; scaleOffset.x < roundedScale.x; ++scaleOffset.x)
		{
			for (scaleOffset.y = 0; scaleOffset.y < roundedScale.y; ++scaleOffset.y)
			{
				for (scaleOffset.z = 0; scaleOffset.z < roundedScale.z; ++scaleOffset.z)
				{
					// Translation
					vColPosition = VColor(pPositions[i]);
					modelPosition = Vector3(vColPosition.inst.Colors.r, vColPosition.inst.Colors.g, vColPosition.inst.Colors.b);

					// Scale
					modelPosition *= scale;
					modelPosition += scaleOffset;

					// World space + rotation
					worldPosition = glm::round(origin + glm::rotate(quat, modelPosition));

					// Check if position is different from last time
					if (lastPosition == worldPosition)
					{
						continue;
					}

					lastPosition = worldPosition;

					if (worldPosition.x >= m_pRenderSystem->m_v3WorldSize.x || worldPosition.y >= m_pRenderSystem->m_v3WorldSize.y || worldPosition.z >= m_pRenderSystem->m_v3WorldSize.z \
						|| worldPosition.x < 0 || worldPosition.y < 0 || worldPosition.z < 0)
						continue;

					// World space ID
					uiWorldID = static_cast<uint32_t>(
						static_cast<int32_t>(worldPosition.x) +
						static_cast<int32_t>(worldPosition.y) * m_pRenderSystem->m_v3WorldSize.x +
						static_cast<int32_t>(worldPosition.z) * m_pRenderSystem->m_v3WorldSize.x * m_pRenderSystem->m_v3WorldSize.y
						);

					// Retrieve resulting color
					uiColor = bHasOverrideColor ?
						(overrideColor.inst.Color | static_cast<unsigned char>(rendererState + 1) << 24) :
						(pColors[i] | static_cast<unsigned char>(rendererState + 1) << 24)
						;

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
						if (!pVoxel) continue;

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
				}
			}
		}
	}

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