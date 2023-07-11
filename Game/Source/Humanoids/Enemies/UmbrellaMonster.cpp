#include "pch.h"
#include "UmbrellaMonster.h"

#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include "Core/ECS/Components/AudioSource.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include <External/glm/gtx/vector_angle.hpp>

RTTR_REGISTRATION
{
	rttr::registration::class_<UmbrellaMonster>("Umbrella Monster")
	.property("Jump Force", &UmbrellaMonster::m_fJumpForce)(RTTR_PUBLIC)
	.property("Max Jump Force", &UmbrellaMonster::m_fMaxJumpVelocity)(RTTR_PUBLIC)
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

UmbrellaMonster::UmbrellaMonster(World * world) :
	Monster(world),
	m_fJumpForce(1.5f, 1.5f),
	m_fMaxJumpVelocity(100.f, 150.f)
{
	SetName("Umbrella Monster");
}

void UmbrellaMonster::Awake()
{
	Monster::Awake();
}

void UmbrellaMonster::Start()
{
	Monster::Start();
}

void UmbrellaMonster::Tick(float fDeltaTime)
{
	Monster::Tick(fDeltaTime);
}

void UmbrellaMonster::ApplyDefaultValues()
{
	m_fWakeUpTime = 0.f;
	m_bCanMeleeAttack = true;
	m_fAttackCooldown = 2.f;
	m_fMeleeAttackTime = 1.f;
	m_fMeleeRange = 30.f;
	m_fJumpForce = Vector2(1.f, 1.f);
	m_fMaxJumpVelocity = Vector2(150.f, 150.f);

	m_idleAnimation = "Content/Character_Models/Lantern/Lantern_Idle.anim.vox";
	m_movingAnimation = "Content/Character_Models/Lantern/Lantern_Walk.anim.vox";
	m_meleeAttackAnimation = "Content/Character_Models/Lantern/Lantern_Attack.anim.vox";

	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder != nullptr)
	{
		pathfinder->m_bCohesion = true;
		pathfinder->m_fMinVelocity = 150;
		pathfinder->m_fMaxVelocity = 150;
	}
}

void UmbrellaMonster::MeleeAttack(Vector3& velocity)
{
	assert(m_pClosestTarget);

	// Set apply height
	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder != nullptr)
		pathfinder->m_applyHeight = false;

	// Set direction
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

	// Jump attack
	PhysicsBody* physicsBody = GetComponent<PhysicsBody>();
	if (physicsBody != nullptr)
	{
		Vector3 planarPosition = Vector3(GetTransform()->GetPosition().x, 0.f, GetTransform()->GetPosition().z);
		Vector3 planarTarget = Vector3(m_pClosestTarget->GetTransform()->GetPosition().x, 0.f, m_pClosestTarget->GetTransform()->GetPosition().z);

		float distance = glm::distance(planarTarget, planarPosition);
		float height = GetTransform()->GetPosition().y - m_pClosestTarget->GetTransform()->GetPosition().y;
		height -= 10.f;

		float initialVelocity = (1.f / cos(0.7f)) * sqrt(std::max((0.5f * glm::length(PhysicsBody::GRAVITY) * distance * distance) / (distance * tan(0.7f) + height), 1.f));
		Vector3 desiredVelocity = Vector3(0, initialVelocity * sin(0.7f) * m_fJumpForce.y, initialVelocity * cos(0.7f) * m_fJumpForce.x / m_fJumpForce.y);
		desiredVelocity.y = std::min(desiredVelocity.y, m_fMaxJumpVelocity.y);
		desiredVelocity.z = std::min(std::abs(desiredVelocity.z), m_fMaxJumpVelocity.x) * (desiredVelocity.z / std::abs(desiredVelocity.z));
		
		float angleBetween = glm::angle(Vector3(0, 0, 1), glm::normalize(planarTarget - planarPosition));
		if (planarTarget.x < planarPosition.x)
			angleBetween *= -1.f;
		desiredVelocity = glm::angleAxis(angleBetween, Vector3(0, 1, 0)) * desiredVelocity;

		m_fMeleeAttackTime = std::abs(desiredVelocity.y / glm::length(PhysicsBody::GRAVITY)) * 2.f * m_fJumpForce.y;
		physicsBody->SetGravity(true);
		velocity = desiredVelocity;
		desiredVelocity.x = 0;
		desiredVelocity.z = 0;
		physicsBody->SetVelocity(desiredVelocity);
	}
}