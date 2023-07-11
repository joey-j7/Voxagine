#include "pch.h"
#include "Mon_RangeAttackState.h"

#include "Humanoids/Enemies/Monster.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>

void Mon_RangeAttackState::Start(Monster * pOwner)
{
	pathfinding::Pathfinder* pathfinder = pOwner->GetComponent<pathfinding::Pathfinder>();
	pathfinder->m_applyVelocity = false;
	pathfinder->m_applyHeight = true;
	pOwner->GetComponent<PhysicsBody>()->SetVelocity(Vector3(0));

	if (!pOwner->m_rangeAttackAnimation.empty())
	{
		pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iRangeAttackAnimationFPS);
		pOwner->GetComponent<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_rangeAttackAnimation);
	}
	pOwner->m_bIsRangeAttacking = true;
	pOwner->m_fCooldownTimer += pOwner->m_fAttackCooldown;
	m_fTimer = pOwner->m_fRangeAttackDelayTime;
	m_bHasShot = false;
}

void Mon_RangeAttackState::Tick(Monster * pOwner, float fDeltaTime)
{
	m_fTimer -= fDeltaTime;

	// Shoot anticipation
	if (m_fTimer <= 0.f && !m_bHasShot)
	{
		pOwner->RangeAttack();
		m_fTimer = pOwner->m_fRangeAttackTime;
		m_bHasShot = true;
	}

	// Shoot animation
	if (m_fTimer <= 0.f && m_bHasShot)
		pOwner->SetState("Idle");
}

void Mon_RangeAttackState::Exit(Monster * pOwner)
{
	pOwner->m_bIsRangeAttacking = false;
}
