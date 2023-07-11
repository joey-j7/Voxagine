#include "pch.h"
#include "Mon_MoveState.h"

#include "Humanoids/Enemies/Monster.h"
#include "Humanoids/Players/Player.h"
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>

void Mon_MoveState::Start(Monster* pOwner)
{
	if (!pOwner->m_movingAnimation.empty())
	{
		pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iMovingAnimationFPS);
		pOwner->GetComponent<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_movingAnimation);
	}

	pathfinding::Pathfinder* pathfinder = pOwner->GetComponent<pathfinding::Pathfinder>();
	pathfinder->m_applyVelocity = true;
	pathfinder->m_applyHeight = true;
}

void Mon_MoveState::Tick(Monster* pOwner, float fDeltaTime)
{
	if (pOwner->m_fCooldownTimer <= 0.f)
	{
		std::vector<Player*> players = pOwner->GetWorld()->FindEntitiesOfType<Player>();
		for (Player* player : players)
		{
			float distance2 = glm::length2(player->GetTransform()->GetPosition() - pOwner->GetTransform()->GetPosition());
			if (pOwner->m_bCanMeleeAttack && distance2 < pOwner->m_fMeleeRange *  pOwner->m_fMeleeRange)
			{
				pOwner->m_pClosestTarget = player;
				pOwner->SetState("MeleeAttack");
			} else if (pOwner->m_bCanRangeAttack && distance2 < pOwner->m_fShootingRange *  pOwner->m_fShootingRange)
			{
				pOwner->m_pClosestTarget = player;
				pOwner->SetState("RangeAttack");
			}
		}
	} else
		pOwner->m_fCooldownTimer -= fDeltaTime;


	// Not on grid
	pathfinding::Pathfinder* pathfinder = pOwner->GetComponent<pathfinding::Pathfinder>();
	if (!pathfinder->IsOnGrid())
		pOwner->GetComponent<PhysicsBody>()->SetVelocity(Vector3(0));
}

void Mon_MoveState::FixedTick(Monster* pOwner, const GameTimer& gameTimer)
{

	float angle = 0.f;

	// Rotate towards direction
	Vector3 direction = pOwner->GetDirection();
	Vector3 velocity = pOwner->GetVelocity();
	if (direction != Vector3(0.f, 0.f, 0.f))
	{
		direction.y = 0.f;
		direction = glm::normalize(direction);

		angle = atan2(direction.x, direction.z);
	}
	// Rotate towards velocity's direction if no direction has been specified
	else if (velocity != Vector3(0.f, 0.f, 0.f))
	{
		velocity.y = 0.f;
		velocity = glm::normalize(velocity);

		angle = atan2(velocity.x, velocity.z);
	}

	Quaternion quat;
	quat.x = 0.f;
	quat.y = std::sin(angle * 0.5f);
	quat.z = 0.f;
	quat.w = std::cos(angle * 0.5f);

	pOwner->GetTransform()->SetRotation(quat);
}