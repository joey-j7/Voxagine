#include "GameStateManager.h"

#include "UI/States/MenuState.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Components/AudioSource.h"
#include "Core/ECS/Components/SpriteRenderer.h"

#include "AI/FiniteStateMachine.h"
// #include "AI/Spawner.h"

#include "General/Managers/GameManager.h"
#include "Humanoids/Players/Player.h"

// #include "Prefabs/DoorPrefab.h"

GameStateManager* GameStateManager::m_sInstance = nullptr;

GameStateManager* GameStateManager::GetInstance() {
	if (m_sInstance == nullptr)
		m_sInstance = new GameStateManager();

	return m_sInstance;
}

void GameStateManager::Start(GameManager* pGameManager)
{
	assert(pGameManager); // We need a manager to control

	pGameManager->StartGame();

	m_Waves = pGameManager->GetWorld()->FindEntities("WaveNumberOne");
	for(auto&& wave : m_Waves)
	{
		wave->Destroyed += Event<Entity*>::Subscriber([&](Entity*) { m_uiWaveCounter++; }, this);
	}

	m_pSpriteRenderer = pGameManager->GetComponent<SpriteRenderer>();
	if(m_pSpriteRenderer) {
		m_pSpriteRenderer->SetFilePath("Content/Sprites/space_to_restart.png");
		m_pSpriteRenderer->SetAlignment(RenderAlignment::RA_CENTERED);
		m_pSpriteRenderer->SetToScreenSpace(true);
		m_pSpriteRenderer->SetEnabled(false);
		// m_pSpriteRenderer->SetOffset(Vector2(i * 85, 10));
	}
}

void GameStateManager::Tick(GameManager* pGameManager, float) 
{
	assert(pGameManager); // We need a manager to control

	if(m_uiWaveCounter == m_Waves.size())
	{
		if (!m_bWon) {
			if (m_pAudioSource) {
				m_pAudioSource->SetFilePath("Content/Audio/BGM/win.ogg");
				m_pAudioSource->SetLoopPoints(342089, 1753286);
				m_pAudioSource->SetAsBGM(1.0f);
			}

			if (m_pSpriteRenderer)
				m_pSpriteRenderer->SetEnabled(true);

			const auto endPos = pGameManager->GetEndPositions()[0] ? pGameManager->GetEndPositions()[0]->GetTransform()->GetPosition() : Vector3(0.0f);

			// DoorPrefab* pDoor = pGameManager->GetWorld()->SpawnEntity<DoorPrefab>(endPos, Vector3(0.f), Vector3(1.f));
			// pDoor->SetWorld("Content/valencio_testworld.wld");

			m_bWon = true;
			m_uiWaveCounter = 0;
		}
	}

	if(!pGameManager->IsAlive())
	{
		if (pGameManager->IsPlaying()) {
			if (m_pAudioSource) {
				m_pAudioSource->SetFilePath("Content/Audio/BGM/lose.ogg");
				m_pAudioSource->SetAsBGM(1.0f);
			}
		}

		// when the player is dead
		// pGameManager->GetFSM()->SetCurrentState(MenuState::GetInstance());
		// pGameManager->Reset();
		pGameManager->StopPlaying();

		if(m_pSpriteRenderer)
			m_pSpriteRenderer->SetEnabled(true);

		m_fGameOverTimer -= pGameManager->GetWorld()->GetDeltaSeconds();
		if (m_fGameOverTimer <= 0.0f) 
		{
			m_fGameOverTimer = m_fGameOverResetTimer;
			// pGameManager->GetWorld()->OpenWorld("Content/valencio_testworld.wld", true);
		}
	}
}

void GameStateManager::Exit(GameManager* pGameManager) {
	assert(pGameManager); // We need a manager to control
}
