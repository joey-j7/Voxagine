#include "pch.h"
#include "OgreMonster.h"

#include <Core/ECS/Components/PhysicsBody.h>
#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include <External/glm/gtx/vector_angle.hpp>

RTTR_REGISTRATION
{
	rttr::registration::class_<OgreMonster>("Ogre Monster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

OgreMonster::OgreMonster(World * world) :
	Monster(world),
	m_fJumpHeight(20.f)
{
	SetName("Ogre Monster");
	m_fWakeUpTime = 0.f;
	m_fMeleeRange = 100.f;
	m_bCanMeleeAttack = true;

	m_idleAnimation = "Content/Ogre/Ogre.vox";
}

void OgreMonster::Awake()
{
	Monster::Awake();
}

void OgreMonster::Start()
{
	Monster::Start();
}

void OgreMonster::Tick(float fDeltaTime)
{
	Monster::Tick(fDeltaTime);
}

void OgreMonster::MeleeAttack(Vector3& velocity)
{
	assert(m_pClosestTarget);

	Vector3 direction = m_pClosestTarget->GetTransform()->GetPosition() - GetTransform()->GetPosition();
	direction.y = 0.f;
	if (direction != Vector3(0))
	{
		direction = glm::normalize(direction);
		float fAngle = atan2(direction.x, direction.z);

		Quaternion quat;
		quat.x = 0.f;
		quat.y = std::sin(fAngle * 0.5f);
		quat.z = 0.f;
		quat.w = std::cos(fAngle * 0.5f);

		GetTransform()->SetRotation(quat);
	}

	PhysicsBody* physicsBody = GetComponent<PhysicsBody>();
	if (physicsBody != nullptr)
	{
		Vector3 planarPosition = Vector3(GetTransform()->GetPosition().x, 0.f, GetTransform()->GetPosition().z);
		Vector3 planarTarget = Vector3(m_pClosestTarget->GetTransform()->GetPosition().x, 0.f, m_pClosestTarget->GetTransform()->GetPosition().z);

		float distance = glm::distance(planarTarget, planarPosition);
		float height = GetTransform()->GetPosition().y - m_pClosestTarget->GetTransform()->GetPosition().y;

		float initialVelocity = (1.f / cos(0.7f)) * sqrt((0.5f * glm::length(PhysicsBody::GRAVITY) * distance * distance) / (distance * tan(0.7f) + height));
		Vector3 velocity = Vector3(0, initialVelocity * sin(0.7f), initialVelocity * cos(0.7f));

		float angleBetween = glm::angle(Vector3(0, 0, 1), glm::normalize(planarTarget - planarPosition));
		if (planarTarget.x < planarPosition.x)
			angleBetween *= -1.f;
		velocity = glm::angleAxis(angleBetween, Vector3(0, 1, 0)) * velocity;

		m_fMeleeAttackTime = std::abs(velocity.y / glm::length(PhysicsBody::GRAVITY)) * 2.f;
		physicsBody->SetGravity(true);
		physicsBody->ApplyImpulse(velocity);
	}

}