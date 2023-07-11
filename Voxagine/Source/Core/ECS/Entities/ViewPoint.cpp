#include "pch.h"
#include "ViewPoint.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<ViewPoint>("ViewPoint")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("IsSet", &ViewPoint::m_bIsSet);
}

void ViewPoint::SetViewTarget(Camera* pCamera)
{
	pCamera->GetTransform()->SetFromMatrix(GetTransform()->GetMatrix());
	pCamera->Recalculate();
	pCamera->ForceUpdate();
	GetWorld()->GetRenderSystem()->ForceUpdate();
}

void ViewPoint::Awake()
{
	Entity::Awake();

	if (!m_bIsSet)
	{
		GetTransform()->SetFromMatrix(GetWorld()->GetMainCamera()->GetTransform()->GetMatrix());
		m_bIsSet = true;
	}
}
