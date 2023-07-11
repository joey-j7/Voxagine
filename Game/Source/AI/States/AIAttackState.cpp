#include "AIAttackState.h"
#include "AIPatrolState.h"

#include "../FiniteStateMachine.h"

#include "../../Player.h"
#include "../../Humanoids/Monster.h"

#include "Core/ECS/World.h"

AIAttackState* AIAttackState::m_sInstance = nullptr;

AIAttackState* AIAttackState::GetInstance() {
	if (m_sInstance == nullptr)
		m_sInstance = new AIAttackState();

	return m_sInstance;
}

void AIAttackState::Start(Monster* pOwner) { }

void AIAttackState::Tick(Monster* pMonster, float fDeltatime) {
	assert(pMonster); // We need a monster to control
	if (const auto player = pMonster->GetPlayerTarget()) {
		Vector3 direction = player->GetTransform()->GetPosition() - pMonster->GetTransform()->GetPosition();
		direction = glm::normalize(direction);

		pMonster->GetTransform()->SetPosition(pMonster->GetTransform()->GetPosition() + direction * pMonster->GetMovementSpeed());

		const float distance = glm::distance(player->GetTransform()->GetPosition(), pMonster->GetTransform()->GetPosition());

		if (distance > m_fMinimalDistance) {
			// TODO needs to keep following
			pMonster->GetFSM()->SetCurrentState(AIPatrolState::GetInstance());
		}
	}
}

void AIAttackState::Exit(Monster* pOwner) {
	
}
