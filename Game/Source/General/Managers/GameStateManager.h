#pragma once
#include <cstdint>
#include <vector>

#include "AI/States/FSMState.h"

class GameManager;
class Entity;
class AudioSource;
class SpriteRenderer;

class GameStateManager : public FSMState<GameManager> {
public:
	static GameStateManager* GetInstance();

	virtual ~GameStateManager() = default;

	void Start(GameManager* pOwner) override;
	void Tick(GameManager* pOwner, float fDeltatime) override;
	void Exit(GameManager* pOwner) override;

private:
	static GameStateManager* m_sInstance;

	GameStateManager() = default;

	uint32_t m_uiWaveCounter = 0;
	std::vector<Entity*> m_Waves;
	SpriteRenderer* m_pSpriteRenderer = nullptr;
	AudioSource* m_pAudioSource = nullptr;
	bool m_bWon = false;
	float m_fGameOverResetTimer = 8.0f;
	float m_fGameOverTimer = m_fGameOverResetTimer;
};

