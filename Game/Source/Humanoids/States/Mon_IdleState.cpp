#include "pch.h"
#include "Mon_IdleState.h"

#include <Core/Math.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/Resources/Formats/VoxModel.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include "Humanoids/Enemies/Monster.h"
#include "Humanoids/Players/Player.h"

void Mon_IdleState::Start(Monster* pOwner)
{
	if (!pOwner->m_idleAnimation.empty())
	{
		pOwner->GetComponentAll<VoxAnimator>()->SetFPS(pOwner->m_iIdleAnimationFPS);
		pOwner->GetComponentAll<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_idleAnimation);
	}

	pathfinding::Pathfinder* pathfinder = pOwner->GetComponentAll<pathfinding::Pathfinder>();
	pathfinder->m_applyVelocity = false;
	pathfinder->m_applyHeight = true;
	pOwner->GetComponentAll<PhysicsBody>()->SetVelocity(Vector3(0.0f));
	pOwner->m_bIsIdle = true;
}

void Mon_IdleState::Tick(Monster* pOwner, float fDeltaTime)
{
	if (!m_bIsAwake)
	{
		// Wake up if the players are in range
		std::vector<Player*> players = pOwner->GetWorld()->FindEntitiesOfType<Player>();
		for (Player* player : players)
		{
			float distance2 = glm::distance(player->GetTransform()->GetPosition(), pOwner->GetTransform()->GetPosition());
			if (distance2 < pOwner->m_fWakeUpRange)
			{
				m_bIsAwake = true;
				m_fWakeUpTimer = pOwner->m_fWakeUpTime; // TODO: replace with animation length

				if (!pOwner->m_wakeUpAnimation.empty())
				{
					pOwner->GetComponentAll<VoxAnimator>()->SetFPS(pOwner->m_iWakeUpAnimationFPS);
					pOwner->GetComponentAll<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_wakeUpAnimation);
				}
			}
		}
	} else
	{
		m_fWakeUpTimer -= fDeltaTime;
		if (m_fWakeUpTimer < 0.f)
		{
			pOwner->m_bIsIdle = false;
			pOwner->SetState("Moving");
		}
	}
}
