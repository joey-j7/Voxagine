#include "pch.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include "Core/Resources/Formats/VoxModel.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/Resources/ReferenceObject.h"
#include "VoxAnimator.h"
#include <Core/Application.h>
#include <Core/LoggingSystem/LoggingSystem.h>
#include "../World.h"
#include "../Component.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<VoxRenderer>("VoxRenderer")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("File", &VoxRenderer::GetModelFilePath, &VoxRenderer::SetModelFilePath) (RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Round on Axis", &VoxRenderer::IsAxisRounded, &VoxRenderer::SetAxisRounded) (RTTR_PUBLIC)
		.property("Limit Rotation Angle", &VoxRenderer::GetRotationAngleLimit, &VoxRenderer::SetRotationAngleLimit) (RTTR_PUBLIC);
}

VoxRenderer::VoxRenderer(Entity * pOwner) :
		Component(pOwner)
{

}

VoxRenderer::~VoxRenderer()
{
	SetFrame(nullptr);
}

std::string VoxRenderer::GetModelFilePath() const
{
	return m_modelFilePath;
}

void VoxRenderer::SetModelFilePath(std::string filePath)
{
	// Do not set the same model twice
	if (m_modelFilePath == filePath) return;

	SetFrame(nullptr);
	m_modelFilePath = filePath;

	if (!filePath.empty())
	{
		VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox(filePath);
		SetModel(pModel, false);
	}
}

void VoxRenderer::SetFrame(const VoxFrame* pFrame, bool bIncrementRef)
{
	if (pFrame == m_pFrame)
		return;

	m_bIsFrameChanged = true;

	bool bFrameInitialized = m_pFrame == nullptr && pFrame != nullptr;
	bool bFrameUninitialized = pFrame == nullptr && m_pFrame != nullptr;

	bool bModelChanged = m_pFrame && pFrame && m_pFrame->GetModel() != pFrame->GetModel();

	if (bModelChanged || bFrameInitialized || bFrameUninitialized)
	{
		if (bIncrementRef && pFrame)
		{
			pFrame->GetModel()->IncrementRef();
		}

		if (m_pFrame)
		{
			m_pFrame->GetModel()->Release();
		}
	}

	m_pFrame = pFrame;
	m_modelFilePath = m_pFrame ? m_pFrame->GetModel()->GetRefPath() : "";

	FrameChanged(this);
}

void VoxRenderer::SetModel(const VoxModel* pModel, bool bIncrementRef)
{
	VoxAnimator* pAnimator = GetOwner()->GetComponent<VoxAnimator>();

	if (pAnimator && pAnimator->IsEnabled())
	{
		GetWorld()->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_WARNING, "VoxRenderer", "Model not set because the entity has an animator component");
		return;
	}

	SetFrame(pModel->GetFrame(0), bIncrementRef);
}

void VoxRenderer::SetOverrideColor(VColor overrideColor)
{
	m_OverrideColor = overrideColor;

	// Set frame change so that it forces the update
	m_bIsFrameChanged = true;
}

Box VoxRenderer::GetBounds() const
{
	Box bounds;
	const VoxFrame* pFrame = GetFrame();
	if (!pFrame)
		return bounds;

	Vector3 extents = pFrame->GetFittedSize() * 0.5f;

	bounds.Max = extents;
	bounds.Min = -extents;

	std::vector<Vector3> corners;
	corners.push_back(bounds.Min);
	corners.push_back(Vector3(bounds.Min.x, bounds.Min.y, bounds.Max.z));
	corners.push_back(Vector3(bounds.Min.x, bounds.Max.y, bounds.Min.z));
	corners.push_back(Vector3(bounds.Max.x, bounds.Min.y, bounds.Min.z));
	corners.push_back(Vector3(bounds.Min.x, bounds.Max.y, bounds.Max.z));
	corners.push_back(Vector3(bounds.Max.x, bounds.Min.y, bounds.Max.z));
	corners.push_back(Vector3(bounds.Max.x, bounds.Max.y, bounds.Min.z));
	corners.push_back(bounds.Max);

	bounds.Min = Vector3(1, 1, 1) * FLT_MAX;
	bounds.Max = Vector3(1, 1, 1) * FLT_MIN;

	for (uint32_t i = 0; i < corners.size(); i++)
	{
		Vector3 transformed = GetTransform()->GetMatrix() * Vector4(corners[i], 1.f);
		bounds.Min = glm::min(bounds.Min, transformed);
		bounds.Max = glm::max(bounds.Max, transformed);
	}

	return bounds;
}

void VoxRenderer::ResetModel()
{
	SetFrame(nullptr);
}
