#include "pch.h"
#include "Core/ECS/Components/BoxCollider.h"

#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/Transform.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "VoxRenderer.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<BoxCollider>("BoxCollider")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Size", &BoxCollider::GetBoxSize, rttr::select_overload<void(Vector3)>(&BoxCollider::SetBoxSize))(RTTR_PUBLIC)
		.property("Auto Fit", &BoxCollider::IsAutoFitted, rttr::select_overload<void(bool)>(&BoxCollider::AutoFit))(RTTR_PUBLIC)
	;
}

BoxCollider::BoxCollider(Entity * pOwner) :
	Collider(pOwner)
{
	m_CollisionBox = Vector3(0.f);
	m_CollisionBoxHalf = Vector3(0.f);
}

Vector3 BoxCollider::GetBoxMax() const
{
	return GetTransform()->GetPosition() + m_CollisionBoxHalf;
}

Vector3 BoxCollider::GetBoxMin() const
{
	return GetTransform()->GetPosition() - m_CollisionBoxHalf;
}

void BoxCollider::SetBoxSize(const VoxFrame* pFrame)
{
	if (pFrame)
	{
		m_CollisionBox = pFrame->GetFittedSize();
		m_CollisionBoxHalf = m_CollisionBox * 0.5f;
	}
}

void BoxCollider::SetBoxSize(Vector3 dimensions)
{
	dimensions.x = abs(dimensions.x);
	dimensions.y = abs(dimensions.y);
	dimensions.z = abs(dimensions.z);

	m_CollisionBox = dimensions;
	m_CollisionBoxHalf = dimensions * 0.5f;
}

void BoxCollider::SetBoxSize(const VoxRenderer* pRenderer)
{
	if (pRenderer)
	{
		m_CollisionBox = pRenderer->GetBounds().GetSize();
		m_CollisionBoxHalf = m_CollisionBox * 0.5f;
	}
}

void BoxCollider::AutoFit(bool bFit)
{
	if (bFit)
	{
		if (VoxRenderer* pRenderer = GetOwner()->GetComponent<VoxRenderer>())
		{
			SetBoxSize(pRenderer);
		}
	}
}
