#include "pch.h"
#include "Transform.h"

#include "Core/Math.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Editor/PropertyRenderer/TransformMetaData.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <External/glm/gtx/matrix_decompose.hpp>

RTTR_REGISTRATION
{
	rttr::registration::class_<Transform>("Transform")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("World Position", &Transform::GetPosition, &Transform::SetGlobalPosition) (RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::GLOBAL, true))
		.property("World Rotation", &Transform::GetEulerAngles, rttr::select_overload<void(Vector3)>(&Transform::SetGlobalEulerAngles)) ( RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::GLOBAL, true))
		.property("World Scale", &Transform::GetScale, &Transform::SetGlobalScale) ( RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::GLOBAL, true))
		.property("Local Position", &Transform::GetLocalPosition, &Transform::SetLocalPosition) ( RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::LOCAL, true))
		.property("Local Rotation", &Transform::GetLocalEulerAngles, rttr::select_overload<void(Vector3)>(&Transform::SetLocalEulerAngles)) ( RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::LOCAL, true))
		.property("Local Scale", &Transform::GetLocalScale, &Transform::SetLocalScale) ( RTTR_PUBLIC, rttr::metadata(MetaData_Transform_Mode::LOCAL, true));
}

Transform::Transform(Entity* pOwner) :
	Component(pOwner)
{
}

void Transform::UpdateMatrix()
{
	if (m_bIsDirty)
	{
		m_TransformationMatrix = glm::translate(Matrix4(1.f), m_Position);
		m_TransformationMatrix *= glm::toMat4(m_Rotation);
		m_TransformationMatrix *= glm::scale(Matrix4(1.f), m_Scale);

		m_LocalTransformationMatrix = glm::translate(Matrix4(1.f), m_LocalPosition);
		m_LocalTransformationMatrix *= glm::toMat4(m_LocalRotation);
		m_LocalTransformationMatrix *= glm::scale(Matrix4(1.f), m_LocalScale);

		m_bIsDirty = false;
		m_bIsUpdated = true;
	}

	for (Entity* pChild : GetOwner()->GetChildren())
	{
		pChild->GetTransform()->UpdateMatrix();
	}
}

const Matrix4& Transform::GetMatrix() const
{
	return m_TransformationMatrix;
}

const Matrix4& Transform::GetLocalMatrix() const
{
	return m_LocalTransformationMatrix;
}

void Transform::SetFromMatrix(const Matrix4& matrix, bool bLocal)
{
	if (!bLocal)
		m_TransformationMatrix = matrix;

	m_LocalTransformationMatrix = matrix;

	if (GetOwner()->GetParent())
	{
		if (bLocal)
		{
			m_TransformationMatrix = GetOwner()->GetParent()->GetTransform()->GetMatrix() * m_LocalTransformationMatrix;
		}
		else
		{
			Matrix4 invMatrix = glm::inverse(GetOwner()->GetParent()->GetTransform()->GetMatrix());
			m_LocalTransformationMatrix = invMatrix * m_TransformationMatrix;
		}
	}

	Vector3 skew;
	Vector4 perspective;
	glm::decompose(m_TransformationMatrix, m_Scale, m_Rotation, m_Position, skew, perspective);
	glm::decompose(m_LocalTransformationMatrix, m_LocalScale, m_LocalRotation, m_LocalPosition, skew, perspective);

	m_EulerAngles = glm::eulerAngles(m_Rotation) * RAD2DEG;
	m_LocalEulerAngles = glm::eulerAngles(m_LocalRotation) * RAD2DEG;

	m_Forward = glm::rotate(m_Rotation, Vector3(0, 0, 1));
	m_Right = glm::rotate(m_Rotation, Vector3(1, 0, 0));
	m_Up = glm::rotate(m_Rotation, Vector3(0, 1, 0));

	SetDirty(false);
	m_bIsUpdated = true;

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetFromLocalMatrix(pEntity->GetTransform()->GetLocalMatrix());
	}
}

Vector3 Transform::GetPosition() const
{
	return m_Position;
}

Vector3 Transform::GetLocalPosition() const
{
	return m_LocalPosition;
}

Vector3 Transform::GetScale() const
{
	return m_Scale;
}

Vector3 Transform::GetLocalScale() const
{
	return m_LocalScale;
}

Vector3 Transform::GetEulerAngles() const
{
	return m_EulerAngles;
}

Vector3 Transform::GetLocalEulerAngles() const
{
	return m_LocalEulerAngles;
}

Quaternion Transform::GetRotation() const
{
	return m_Rotation;
}

Quaternion Transform::GetLocalRotation() const
{
	return m_LocalRotation;
}

const Vector3& Transform::GetForward() const
{
	return m_Forward;
}

const Vector3& Transform::GetRight() const
{
	return m_Right;
}

const Vector3& Transform::GetUp() const
{
	return m_Up;
}

void Transform::Translate(Vector3 translation, bool bLocal)
{
	if (glm::length(translation) == 0)
		return;

	if (bLocal)
	{
		translation = glm::rotate(m_Rotation, translation);
	}

	m_Position.x += translation.x;
	m_Position.y += translation.y;
	m_Position.z += translation.z;

	m_LocalPosition.x += translation.x;
	m_LocalPosition.y += translation.y;
	m_LocalPosition.z += translation.z;

	SetDirty(true);

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetLocalPosition(pEntity->GetTransform()->GetLocalPosition());
	}
}

void Transform::Rotate(Vector3 eulerAngles, bool bLocal)
{
	if (glm::length(eulerAngles) == 0)
		return;

	eulerAngles.x *= DEG2RAD;
	eulerAngles.y *= DEG2RAD;
	eulerAngles.z *= DEG2RAD;

	Quaternion quaternion = Quaternion(Vector3(eulerAngles.x, eulerAngles.y, eulerAngles.z));
	
	if (bLocal)
	{
		m_Rotation = m_Rotation * quaternion;
		m_LocalRotation = m_LocalRotation * quaternion;
	}
	else
	{
		Quaternion invRotation = m_Rotation;
		invRotation = glm::inverse(invRotation);
		if (glm::all(glm::isnan(invRotation)))
		{
			m_Rotation = quaternion;
			m_LocalRotation = quaternion;
		}
		else
		{
			m_Rotation = m_Rotation * invRotation * quaternion * m_Rotation;
			m_LocalRotation = m_LocalRotation * invRotation * quaternion * m_LocalRotation;
		}
	}

	m_Forward = glm::rotate(m_Rotation, Vector3(0, 0, 1));
	m_Right = glm::rotate(m_Rotation, Vector3(1, 0, 0));
	m_Up = glm::rotate(m_Rotation, Vector3(0, 1, 0));

	m_EulerAngles = glm::eulerAngles(m_Rotation) * RAD2DEG;
	m_LocalEulerAngles = glm::eulerAngles(m_LocalRotation) * RAD2DEG;

	SetDirty(true);

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetLocalRotation(pEntity->GetTransform()->GetLocalRotation());
	}
}

void Transform::SetPosition(Vector3 pos, bool bLocal)
{
	bool isParentDirty = GetOwner()->GetParent() && GetOwner()->GetParent()->GetTransform()->IsDirty();
	if (((bLocal && m_LocalPosition == pos) || (!bLocal && m_Position == pos)) && !isParentDirty)
		return;

	m_Position = pos;
	m_LocalPosition = pos;

	if (GetOwner()->GetParent())
	{
		Transform* pParentTransform = GetOwner()->GetParent()->GetTransform();
		if (bLocal)
		{
			m_Position = pParentTransform->GetPosition() + glm::rotate(pParentTransform->GetRotation(), m_LocalPosition);
		}
		else
		{
			Matrix4 invMatrix = glm::inverse(pParentTransform->GetMatrix());
			m_LocalPosition = invMatrix * glm::vec4(m_Position, 1.f);
		}
	}

	SetDirty(true);

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetLocalPosition(pEntity->GetTransform()->GetLocalPosition());
	}
}

void Transform::SetScale(Vector3 scale, bool bLocal)
{
	bool isParentDirty = GetOwner()->GetParent() && GetOwner()->GetParent()->GetTransform()->IsDirty();
	if (((bLocal && m_LocalScale == scale) || (!bLocal && m_Scale == scale)) && !isParentDirty)
		return;

	m_Scale = scale;
	m_LocalScale = scale;

	if (GetOwner()->GetParent())
	{
		Transform* pParentTransform = GetOwner()->GetParent()->GetTransform();
		if (bLocal)
		{
			m_Scale = pParentTransform->GetScale() * m_LocalScale;
		}
		else
		{
			m_LocalScale = scale / pParentTransform->GetScale();
		}
	}

	SetDirty(true);
	UpdateMatrix();

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetFromLocalMatrix(pEntity->GetTransform()->GetLocalMatrix());
	}
}

void Transform::SetRotation(Quaternion quat, bool bLocal)
{
	bool isParentDirty = GetOwner()->GetParent() && GetOwner()->GetParent()->GetTransform()->IsDirty();
	if (m_Rotation == quat && !isParentDirty)
		return;

	if (!bLocal)
		m_Rotation = quat;

	m_LocalRotation = quat;

	m_Forward = glm::rotate(m_Rotation, Vector3(0, 0, 1));
	m_Right = glm::rotate(m_Rotation, Vector3(1, 0, 0));
	m_Up = glm::rotate(m_Rotation, Vector3(0, 1, 0));

	m_EulerAngles = glm::eulerAngles(m_Rotation) * RAD2DEG;
	m_LocalEulerAngles = glm::eulerAngles(m_LocalRotation) * RAD2DEG;

	SetDirty(true);
	UpdateMatrix();

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetFromLocalMatrix(pEntity->GetTransform()->GetLocalMatrix());
	}
}

void Transform::SetEulerAngles(Vector3 eulerAngles, bool bLocal)
{
	bool isParentDirty = GetOwner()->GetParent() && GetOwner()->GetParent()->GetTransform()->IsDirty();
	if (((bLocal && m_LocalEulerAngles == eulerAngles) || (!bLocal && m_EulerAngles == eulerAngles)) && !isParentDirty)
		return;

	if (bLocal)
	{
		m_LocalEulerAngles = eulerAngles;
		m_LocalRotation = Quaternion(Vector3(eulerAngles.x * DEG2RAD, eulerAngles.y * DEG2RAD, eulerAngles.z * DEG2RAD));
	}
	else
	{
		m_EulerAngles = eulerAngles;
		m_Rotation = Quaternion(Vector3(eulerAngles.x * DEG2RAD, eulerAngles.y * DEG2RAD, eulerAngles.z * DEG2RAD));
	}

	m_Forward = glm::rotate(m_Rotation, Vector3(0, 0, 1));
	m_Right = glm::rotate(m_Rotation, Vector3(1, 0, 0));
	m_Up = glm::rotate(m_Rotation, Vector3(0, 1, 0));

	SetDirty(true);
	UpdateMatrix();

	for (Entity* pEntity : GetOwner()->GetChildren())
	{
		pEntity->GetTransform()->SetFromLocalMatrix(pEntity->GetTransform()->GetLocalMatrix());
	}
}

void Transform::SetDirty(bool bDirty)
{
	m_bIsDirty = bDirty;

	if (bDirty)
	{
		for (Entity* pEntity : GetOwner()->GetChildren())
		{
			pEntity->GetTransform()->SetDirty(bDirty);
		}
	}
}

void Transform::SetUpdated(bool bUpdated)
{
	m_bIsUpdated = bUpdated;
}
