#include "pch.h"
#include "Core/ECS/Entities/Camera.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/ECS/Entity.h"
#include "Core/Platform/Window/WindowContext.h"
#include <Core/Platform/Platform.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Camera>("Camera")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("AspectRatio", &Camera::GetAspectRatio, &Camera::SetAspectRatio)
		.property("FarPlane", &Camera::m_fFarPlane) (RTTR_PUBLIC)
		.property("NearPlane", &Camera::m_fNearPlane) (RTTR_PUBLIC)
		.property("FieldOfView", &Camera::m_fFieldofView) (RTTR_PUBLIC)
		.property("Orthographic", &Camera::IsOrthographic, &Camera::SetOrthographic) (RTTR_PUBLIC)
		.property("IsMainCamera", &Camera::IsMainCamera, &Camera::SetMainCamera);
}

Camera::Camera(World* pWorld) :
	Entity(pWorld)
{
	m_cameraOffset = Vector3(0, 0, 0);

	if (pWorld != nullptr)
		GetWorld()->GetApplication()->GetPlatform().GetRenderContext()->SizeChanged += Event<uint32_t, uint32_t, IVector2>::Subscriber(std::bind(&Camera::CalculateProjection, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), this);
}

Camera::~Camera()
{
	if (GetWorld() != nullptr)
		GetWorld()->GetApplication()->GetPlatform().GetRenderContext()->SizeChanged -= this;
}

void Camera::Awake()
{
	Entity::Awake();

	UVector2 windowSize = GetWorld()->GetApplication()->GetPlatform().GetRenderContext()->GetRenderResolution();
	CalculateProjection(uint32_t(windowSize.x), uint32_t(windowSize.y), IVector2(0, 0));
	Recalculate();
}

void Camera::PreTick()
{
	Entity::PreTick();
}

void Camera::PostTick(float fDeltaTime)
{
	Entity::PostTick(fDeltaTime);
}

bool Camera::IsUpdated() const
{
	return m_bIsRecalculated || GetTransform()->IsUpdated();
}

void Camera::ForceUpdate()
{
	m_bIsRecalculated = true;
}

void Camera::Recalculate()
{
	GetTransform()->UpdateMatrix();

	if (IsUpdated()) {
		m_ViewMatrix = glm::lookAt(Vector3(0.f, 0.f, 0.f), Vector3(0.f, 0.f, 1.f), Vector3(0.f, 1.f, 0.f));
		m_ModelView = m_ViewMatrix * GetTransform()->GetMatrix();

		m_ViewMatrix = glm::lookAt(GetTransform()->GetPosition(), GetTransform()->GetPosition() + GetTransform()->GetForward(), GetTransform()->GetUp());
		m_MVP = m_Projection * m_ViewMatrix;
	}
}

void Camera::SetRecalculated(bool bIsUpdated)
{
	m_bIsRecalculated = bIsUpdated;
}

void Camera::SetOrthographic(bool bIsOrthographic)
{
	if (bIsOrthographic == bIsOrthographic)
		return;

	m_bIsOrthographic = bIsOrthographic;

	UVector2 renderResolution = GetWorld()->GetApplication()->GetPlatform().GetRenderContext()->GetRenderResolution();
	CalculateProjection(renderResolution.x, renderResolution.y, IVector2(0, 0));

	m_bIsRecalculated = true;
}

Vector2 Camera::WorldToScreenPoint(const Vector3& worldPos)
{
	Vector4 worldPosition(worldPos.x, worldPos.y, worldPos.z, 1.f);
	Vector4 clipCoords = m_MVP * worldPosition;
	//Vector4 clipCoords = Vector4::Transform(worldPosition, m_MVP);
	//CLEANUP

	/* This position can't be converted to screen because it's behind the camera */
	if (clipCoords.z < 0)
		return Vector2(1.0f, 1.0f) * FLT_MIN;

	const Vector3 deviceCoords = clipCoords / clipCoords.w;
	const UVector2& windowsize = GetWorld()->GetApplication()->GetPlatform().GetWindowContext()->GetSize();

	const int x = (int)round((deviceCoords.x + 1.f) * 0.5f * windowsize.x);
	const int y = (int)round((1.f - deviceCoords.y) * 0.5f * windowsize.y);

	return Vector2((float)x, (float)y);
}

Vector3 Camera::ScreenToWorld(const Vector2& screenCoord)
{
	const UVector2& windowsize = GetWorld()->GetApplication()->GetPlatform().GetWindowContext()->GetSize();

	float devicecoordx = (2.f * screenCoord.x) / windowsize.x - 1;
	float devicecoordy = (2.f * screenCoord.y) / windowsize.y - 1;

	Vector4 clipCoords = Vector4(devicecoordx, -devicecoordy, -1.f, 1.f);

	Vector4 eyecoords = glm::inverse(m_Projection) * clipCoords;
	//Vector4 eyecoords = glm::inverse(m_Projection) Vector4::Transform(clipCoords, m_Projection.Invert());
	eyecoords.z = 1;
	eyecoords.w = 0;

	Vector4 worldcoords = glm::inverse(m_ViewMatrix) * eyecoords;
	//Vector4 worldcoords = Vector4::Transform(eyecoords, m_ViewMatrix.Invert());
	Vector3 norm = Vector3(worldcoords.x, worldcoords.y, worldcoords.z);

	norm = glm::normalize(norm);
	return norm;
}

void Camera::CalculateProjection(uint32_t fWindowSizeX, uint32_t fWindowSizeY, IVector2 resizeDelta)
{
	float fFoV = m_fFieldofView * glm::pi<float>() / 180.f;

	m_fAspectRatio = (float)fWindowSizeX / (float)fWindowSizeY;

	/* Landscape to portait */
	if (m_fAspectRatio < 1.0f)
		fFoV *= 2.0f;

	m_fProjectionValue = 1.0f / tan(fFoV * 0.5f);

	if (m_bIsOrthographic)
	{
		m_Projection = glm::ortho((float)fWindowSizeX * m_fAspectRatio * 0.1f, (float)fWindowSizeY * m_fAspectRatio * 0.1f, m_fNearPlane, m_fFarPlane);
		//m_Projection = Matrix4::CreateOrthographic((float)fWindowSizeX * m_fAspectRatio * 0.1f, (float)fWindowSizeY * m_fAspectRatio * 0.1f, m_fNearPlane, m_fFarPlane);
	}
	else
	{
		m_Projection = glm::perspective(fFoV, m_fAspectRatio, m_fNearPlane, m_fFarPlane);
		//XMStoreFloat4x4(&m_Projection, XMMatrixPerspectiveFovLH(fFoV, m_fAspectRatio, m_fNearPlane, m_fFarPlane));
	}

	m_bIsRecalculated = true;
}