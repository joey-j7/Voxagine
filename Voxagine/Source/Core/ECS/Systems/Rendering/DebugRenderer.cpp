#include "pch.h"
#include "DebugRenderer.h"

#include "../../Components/Transform.h"

#if defined(EDITOR) || defined(_DEBUG)
#include <Core/Platform/Rendering/RenderContext.h>
#include <Core/Platform/Platform.h>
#include <Core/Application.h>

#include "../../WorldManager.h"
#include "../../World.h"
#include "../../Entities/Camera.h"
#include "../Physics/PhysicsSystem.h"
#endif

DebugRenderer::DebugRenderer(RenderContext* pRenderContext)
{
	(void)pRenderContext;

#if defined(EDITOR) || defined(_DEBUG)
	m_pRenderContext = pRenderContext;
	m_pWorldManager = &m_pRenderContext->GetPlatform()->GetApplication()->GetWorldManager();
#endif
}

void DebugRenderer::AddLine(const DebugLine& line)
{
	(void)line;

#if defined(EDITOR) || defined(_DEBUG)
	m_pRenderContext->Submit(line);
#endif
}

void DebugRenderer::AddLine(Vector3 start, Vector3 end, VColor color)
{
	(void)start;
	(void)end;
	(void)color;

#if defined(EDITOR) || defined(_DEBUG)
	DebugLine line;
	line.m_Start = start;
	line.m_End = end;
	line.m_Color = color;
	m_pRenderContext->Submit(line);
#endif
}

void DebugRenderer::AddBox(const DebugBox& box)
{
	(void)box;
#if defined(EDITOR) || defined(_DEBUG)
	m_pRenderContext->Submit(box);
#endif
}

void DebugRenderer::AddSphere(const DebugSphere& sphere)
{
	(void)sphere;
#if defined(EDITOR) || defined(_DEBUG)
	m_pRenderContext->Submit(sphere);
#endif
}

void DebugRenderer::AddCenteredSphere(Vector3 position, Vector3 size, VColor color)
{
	(void)position;
	(void)size;
	(void)color;
#if defined(EDITOR) || defined(_DEBUG)
	DebugSphere sphere;

	sphere.m_Center = position;

	sphere.m_Center.x = sphere.m_Center.x;
	sphere.m_Center.y = sphere.m_Center.y;
	sphere.m_Center.z = sphere.m_Center.z;

	sphere.m_fRadius = glm::length(size) * 0.5f;
	sphere.m_Color = color;

	AddSphere(sphere);
#endif
}

void DebugRenderer::AddCenteredBox(Vector3 position, Vector3 size, VColor color)
{
	(void)position;
	(void)size;
	(void)color;

#if defined(EDITOR) || defined(_DEBUG)
	DebugBox debugBox;
	debugBox.m_Extents = size * 0.5f;

	debugBox.m_Center = position;

	debugBox.m_Center.x = debugBox.m_Center.x;
	debugBox.m_Center.y = debugBox.m_Center.y;
	debugBox.m_Center.z = debugBox.m_Center.z;

	debugBox.m_Color = color;

	AddBox(debugBox);
#endif
}

void DebugRenderer::AddCenteredLocalLine(Transform* pTransform, Vector3 size, Vector3 direction, float fDistance, VColor color)
{
	(void)pTransform;
	(void)size;
	(void)direction;
	(void)fDistance;
	(void)color;

#if defined(EDITOR) || defined(_DEBUG)
	DebugLine line;

	size *= 0.5f;

	line.m_Start = pTransform->GetPosition();
	line.m_Start.x = line.m_Start.x;
	line.m_Start.y = line.m_Start.y;
	line.m_Start.z = line.m_Start.z;

	direction = glm::rotate(pTransform->GetRotation(), direction);
	//direction = Vector3::Transform(direction, pTransform->GetRotation());
	direction = glm::normalize(direction);

	line.m_End = line.m_Start + direction * fDistance;
	line.m_End.x = line.m_End.x;
	line.m_End.y = line.m_End.y;
	line.m_End.z = line.m_End.z;

	line.m_Color = color;

	AddLine(line);
#endif
}
