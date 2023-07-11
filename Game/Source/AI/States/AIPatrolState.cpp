#include "AIPatrolState.h"
#include "AIAttackState.h"

// #include "../FiniteStateMachine.h"

#include "../Flocking/Flock.h"
// #include "../../Utils.h"
#include "../../Player.h"
#include "../../Humanoids/Monster.h"

#include "Core/ECS/World.h"

AIPatrolState* AIPatrolState::m_sInstance = nullptr;

AIPatrolState* AIPatrolState::GetInstance() {
	if (m_sInstance == nullptr)
		m_sInstance = new AIPatrolState();

	return m_sInstance;
}

void AIPatrolState::Start(Monster* pMonster) { }

void AIPatrolState::Tick(Monster* pMonster, float fDeltatime) {
	assert(pMonster); // We need a monster to control
	if (const auto player = pMonster->GetPlayerTarget()) {
		const float distance = glm::distance(player->GetTransform()->GetPosition(), pMonster->GetTransform()->GetPosition());

		if (distance < m_fMinimalDistance) {
			// TODO change to other state
			// pMonster->GetFSM()->SetCurrentState(AIAttackState::GetInstance());
		}
	}
}

void AIPatrolState::Exit(Monster* pMonster) { }