#pragma once

#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Entity.h"
#include "Core/Resources/Formats/VoxModel.h"

#include <cmath>

/* Where a VoxRenderer's model lands in a voxel grid, and how to walk the voxels
 * it puts there.
 *
 * Extracted from VoxelBaker::Occupy so that the far-field volume
 * (RENDERING_PLAN.md phase 4) can stamp the same model at the same place in a
 * *different* grid. The two have to agree exactly or the far field and the
 * detail window disagree about where the level is, and the seam between them
 * moves as the window slides.
 *
 * The grid is parameterized only by its origin and voxel size, which is all
 * VoxelGrid::WorldToGrid ever used: the window passes its world offset, the
 * far field passes zero because level space *is* world space.
 */
struct VoxelStampTransform
{
	Vector3 Origin = Vector3(0.f);
	Quaternion Rotation;
	Vector3 Scale = Vector3(1.f);
	Vector3 RoundedScale = Vector3(1.f);
};

/* False when the renderer has no frame to stamp. */
inline bool ComputeVoxelStampTransform(
	VoxRenderer* pRenderer, const Vector3& v3GridOrigin, float fInvVoxelSize,
	VoxelStampTransform& out)
{
	const VoxFrame* pFrame = pRenderer->GetFrame();

	if (!pFrame)
		return false;

	Transform* pTransform = pRenderer->GetTransform();

	Quaternion quat = pTransform->GetRotation();
	Vector3 originOffset(0.f);

	/* Both quantization modes below round the rotation to a multiple of a
	   limit, and nudge the origin by a voxel when the model has been flipped
	   far enough that the rounding lands it half a voxel off. */
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
	}

	const Vector3 scale = pTransform->GetScale();

	/* VoxelGrid::WorldToGrid, without the VoxelGrid: the grid's origin and
	   voxel size are the whole of what it does. */
	auto worldToGrid = [&v3GridOrigin, fInvVoxelSize](const Vector3& v3World)
	{
		return glm::floor((v3World - v3GridOrigin) * fInvVoxelSize);
	};

	const Vector3 size = pFrame->GetFittedSize();
	const Vector3 offset = -size * 0.5f;

	Vector3 origin;

	if (pFrame->GetModel()->GetFrameCount() > 1)
	{
		const VoxFrame* tFrame = pFrame->GetModel()->GetFrame(0);
		Vector3 offsetCen = -(tFrame->GetFitSizeOffset() - pFrame->GetFitSizeOffset()) * 0.5f
			+ ((pFrame->GetFitSizeOffset() + pFrame->GetFittedSize()) - (tFrame->GetFitSizeOffset() + tFrame->GetFittedSize())) * 0.5f;
		offsetCen.y *= -1.f;

		origin = worldToGrid(pTransform->GetPosition()) + scale * glm::rotate(quat, offset - offsetCen);
	}
	else
	{
		origin = worldToGrid(pTransform->GetPosition() + scale * glm::floor(glm::rotate(quat, offset)));
	}

	out.Origin = origin - originOffset;
	out.Rotation = quat;
	out.Scale = scale;
	out.RoundedScale = glm::ceil(glm::abs(scale));

	return true;
}

/* Calls fn(const Vector3& v3GridPosition, uint32_t uiColor) once per voxel the
 * model puts into the grid. Positions are grid-space and already rounded, but
 * are neither bounds checked nor checked for being finite - the caller knows
 * its own grid's size, and the two callers do genuinely different things with
 * an out-of-range one. VoxelBaker names the entity behind a non-finite
 * position, which is the only diagnostic there is for the open NaN-transform
 * defect; the far-field build just drops it.
 *
 * Every caller must therefore write its range test as an in-range test rather
 * than a rejection test. A NaN compares false against everything, so a
 * rejection test lets it through, and static_cast<int32_t> of a NaN is
 * INT32_MIN.
 */
template <typename Fn>
void ForEachStampedVoxel(VoxRenderer* pRenderer, const VoxelStampTransform& stamp, Fn&& fn)
{
	const VoxFrame* pFrame = pRenderer->GetFrame();

	if (!pFrame)
		return;

	const uint32_t* pColors = pFrame->GetColors();
	const uint32_t* pPositions = pFrame->GetPositions();

	const uint32_t uiSolidVoxelCount = pFrame->GetSolidVoxelCount();

	const VColor overrideColor = pRenderer->GetOverrideColor();
	const bool bHasOverrideColor = overrideColor.inst.Colors.a > 0;

	const RenderState rendererState = pRenderer->GetState();

	Vector3 lastPosition(0.f);
	UVector3 scaleOffset(0, 0, 0);

	for (uint32_t i = 0; i < uiSolidVoxelCount; ++i)
	{
		for (scaleOffset.x = 0; scaleOffset.x < stamp.RoundedScale.x; ++scaleOffset.x)
		{
			for (scaleOffset.y = 0; scaleOffset.y < stamp.RoundedScale.y; ++scaleOffset.y)
			{
				for (scaleOffset.z = 0; scaleOffset.z < stamp.RoundedScale.z; ++scaleOffset.z)
				{
					// Translation
					const VColor vColPosition = VColor(pPositions[i]);
					Vector3 modelPosition(vColPosition.inst.Colors.r, vColPosition.inst.Colors.g, vColPosition.inst.Colors.b);

					// Scale
					modelPosition *= stamp.Scale;
					modelPosition += scaleOffset;

					// Grid space + rotation
					const Vector3 gridPosition = glm::round(stamp.Origin + glm::rotate(stamp.Rotation, modelPosition));

					// Check if position is different from last time
					if (lastPosition == gridPosition)
						continue;

					lastPosition = gridPosition;

					const uint32_t uiColor = bHasOverrideColor
						? (overrideColor.inst.Color | static_cast<unsigned char>(rendererState + 1) << 24)
						: (pColors[i] | static_cast<unsigned char>(rendererState + 1) << 24);

					fn(gridPosition, uiColor);
				}
			}
		}
	}
}
