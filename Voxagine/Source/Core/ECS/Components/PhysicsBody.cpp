#include "pch.h"
#include "Core/ECS/Components/PhysicsBody.h"

#include "Core/ECS/Components/Transform.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/World.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<PhysicsBody>("PhysicsBody")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Gravity Enabled", &PhysicsBody::HasGravity, &PhysicsBody::SetGravity) (RTTR_PUBLIC)
		.property("Mass", &PhysicsBody::GetMass, &PhysicsBody::SetMass) (RTTR_PUBLIC)
		.property("Velocity", &PhysicsBody::GetVelocity, &PhysicsBody::SetVelocity) (RTTR_PROTECTED)
		.property("Step Height", &PhysicsBody::GetStepHeight, &PhysicsBody::SetStepHeight) (RTTR_PUBLIC);
}

const Vector3 PhysicsBody::GRAVITY = Vector3(0.f, -147.15f * 2.0f, 0.f);

PhysicsBody::PhysicsBody(Entity* pOwner) :
	Component(pOwner)
{
	m_Velocity = Vector3(0.f);
	m_Force = Vector3(0.f);

	m_bIsResting = false;
	m_bGravity = true;
	m_fMass = 1.f;
	m_fInvMass = 1.f / m_fMass;
	m_fDrag = 0.999;
	m_uiStepHeight = 0;

	Requires<BoxCollider>();
}

void PhysicsBody::ApplyForce(Vector3 force)
{
	m_Force += force;
}

void PhysicsBody::ApplyImpulse(Vector3 impulse)
{
	m_Velocity += impulse * m_fInvMass;
}

void PhysicsBody::OnEnabled()
{
	m_pCollider = GetOwner()->GetComponent<BoxCollider>();
}

void PhysicsBody::OnDisabled()
{
	m_pCollider = nullptr;
}

void PhysicsBody::Awake()
{
	Component::Awake();

	m_pCollider = GetOwner()->GetComponent<BoxCollider>();
}

void PhysicsBody::Tick(float fDeltaTime)
{
	if (GetOwner()->IsStatic()) return;

	Vector3 acceleration = m_Force * m_fInvMass * fDeltaTime;

	if (m_bGravity && !m_bIsResting)
		acceleration += GRAVITY * fDeltaTime;

	m_Velocity += acceleration;
	//m_Velocity *= m_fDrag;

	m_Velocity *= Vector3{ 0.5f, 1.f, 0.5f }; // Fake friction

	if (glm::length2(m_Velocity) > 0.f)
	{
		m_pTransform->Translate(m_Velocity * fDeltaTime);
	}

	if (m_pTransform->IsUpdated())
		m_bIsResting = false;

	m_Force *= 0;
}

void PhysicsBody::SetMass(float fMass)
{
	if (fMass <= 0) return;

	m_fMass = fMass;
	m_fInvMass = 1.f / m_fMass;
}
