#include "pch.h"
#include "Mon_MeleeAttackState.h"

#include "Humanoids/Enemies/Monster.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>

void Mon_MeleeAttackState::Start(Monster* pOwner)
{
	pathfinding::Pathfinder* pathfinder = pOwner->GetComponent<pathfinding::Pathfinder>();
	pathfinder->m_applyVelocity = false;
	pathfinder->m_applyHeight = true;
	pOwner->GetComponentAll<PhysicsBody>()->SetVelocity(Vector3(0));

	if (!pOwner->m_meleeAttackAnimation.empty())
	{
		pOwner->GetComponentAll<VoxAnimator>()->SetFPS(pOwner->m_iMeleeAttackAnimationFPS);
		pOwner->GetComponentAll<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_meleeAttackAnimation);
	}
	pOwner->m_bIsMeleeAttacking = true;
	pOwner->m_fCooldownTimer += pOwner->m_fAttackCooldown;
	m_velocity = Vector3(0);
	m_bShouldSeek = false;
	pOwner->MeleeAttack(m_velocity);
	if (m_velocity == Vector3(0))
		m_bShouldSeek = true;
	m_fTimer = pOwner->m_fMeleeAttackTime;
}

void Mon_MeleeAttackState::Tick(Monster * pOwner, float fDeltaTime)
{
	m_fTimer -= fDeltaTime;

	if (pOwner->m_pClosestTarget != nullptr)
	{
		// Homing 
		if (m_bShouldSeek)
		{
			Vector3 desiredVelocity = pOwner->m_pClosestTarget->GetTransform()->GetPosition() - pOwner->GetTransform()->GetPosition();
			float distance = glm::length(desiredVelocity);
			if (distance > pOwner->m_fStopSeekRange)
			{
				desiredVelocity.y = 0;
				if (desiredVelocity != Vector3(0))
					desiredVelocity = glm::normalize(desiredVelocity) * pOwner->GetComponentAll<pathfinding::Pathfinder>()->m_fMaxVelocity;
				desiredVelocity.y = pOwner->GetComponentAll<PhysicsBody>()->GetVelocity().y;
				m_velocity = desiredVelocity;
			} else
				m_velocity = Vector3(0);
		}
		
		m_velocity.y = pOwner->GetComponentAll<PhysicsBody>()->GetVelocity().y;
		pOwner->GetComponentAll<PhysicsBody>()->SetVelocity(m_velocity);

		// Rotation
		Vector3 direction = pOwner->m_pClosestTarget->GetTransform()->GetPosition() - pOwner->GetTransform()->GetPosition();
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

			pOwner->GetTransform()->SetRotation(quat);
		}
	}

	if (m_fTimer <= 0.f)
		pOwner->SetState("Idle");
}

void Mon_MeleeAttackState::Exit(Monster * pOwner)
{
	pOwner->m_bIsMeleeAttacking = false;
}
