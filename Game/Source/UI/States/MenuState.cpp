#include "MenuState.h"

#include "General/Managers/GameStateManager.h"

#include "Humanoids/Players/Player.h"

#include "General/Managers/GameManager.h"
#include "AI/FiniteStateMachine.h"

void MenuState::Start(GameManager* pGameManager) {
	assert(pGameManager); // We need a manager to control
}

void MenuState::Tick(GameManager* pGameManager, float) {
	assert(pGameManager); // We need a manager to control
	pGameManager->GetFSM()->SetCurrentState(GameStateManager::GetInstance());
}

void MenuState::Exit(GameManager* pGameManager) {
	assert(pGameManager); // We need a manager to control
}
