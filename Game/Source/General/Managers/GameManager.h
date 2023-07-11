#pragma once
#include "Core/ECS/Component.h"
#include "Core/Math.h"
#include "AI/IFiniteManager.h"

enum class EGameState
{
	Start,
	Play,
	End
};

typedef int IMonsterType;

class Player;
class InputHandler;
class LoadOut;
class UIButton;

class HealthUI;
class ComboUI;
class ComboSliderUI;
class ComboIcon;

class GameManager : public Entity, public IFiniteManager<GameManager>
{
public:
	GameManager(World* world);

	bool IsPlaying() const { return m_bIsPlaying; }
	void StopPlaying() { m_bIsPlaying = false; }

	void SetPlayerPosition(const Vector3& vPosition, uint32_t uiIndex);

	void SetEndPlayerPosition(const Vector3& vPosition, uint32_t uiIndex);

	void SetPlayer(Player* pPlayer, uint32_t uiIndex)
	{
		if (uiIndex < m_pPlayers.size()) m_pPlayers[uiIndex] = pPlayer;
	}

	std::array<Player*, 2>& GetPlayers()
	{
		return m_pPlayers;
	}
	
	std::array<Entity*, 2>& GetEndPositions() { return m_vArrEndEntities; }

	EGameState GetPlayState() const { return m_EGameState; }

	void SetPlayState(EGameState state);

	/*!
	 * @brief - Grab the health
	 */
	float GetHealth() const { return sharedPlayerHealth; }

	/*!
	 * @brief - Set the entity health
	 * @param health
	 */
	void SetHealth(float health) { sharedPlayerHealth = health; }

	/*!
	 * @brief - Set max health of the entity
	 * @param maxHealth
	 */
	void SetMaxHealth(float maxHealth) { m_fMaxHealth = maxHealth; }

	/*!
	 * @brief get max health of the players
	 */
	float GetMaxHealth() const { return m_fMaxHealth; }

	bool IsAlive() const { return sharedPlayerHealth <= 0.0f; }

	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;
	void OnDrawGizmos(float) override;

	void StartGame();
	void Reset();

	float currentComboTimer = 0.0f;

	float comboTimerStartTime = 5.0f;

	//The current combo streak of the players
	int comboStreak = 0;

	int comboOnThrow = 0;

	int comboOnCatch = 0;

	int timeBasedBonus1 = 2;

	int timeBasedBonus2 = 1;

	float Bonus1TimeHeld = 1.0f;

	float Bonus2TimeHeld = 2.0f;

	int environmentComboBonus = 1;

	int enemyComboBonus = 2;

	int comboThreshold1 = 2;

	int comboThreshold2 = 4;

	int comboThreshold3 = 6;

	float speedComboMultiplier1 = 1.2f;

	float speedComboMultiplier2 = 1.4f;

	float speedComboMultiplier3 = 1.6f;

	float voxelExplosionRangeMin = 30.f;

	float voxelExplosionRangeMax = 80.f;

	//Increments the current combo streak
	void AddComboStreak(int comboNumber);

	//Increments the current combo streak
	void AddToOnComboOnCatch(int comboNumber);

	//Resets the current combo streak back to the initial value
	void ResetComboStreak();

	void ResetComboTimer();

	int GetComboStreak();

	float GetComboTimer();

	int GetSharedPlayerHealth();

	bool CanBeDamaged();
	void SharedPlayerHealthTakeDamage(int damage);

	int sharedPlayerHealth = 3;

	float m_fInvincibilityTimer = 0.f;
	float m_fInvincibilityTime = 1.f;

	std::vector<UIButton*> vCurrentButtons = {};

	HealthUI* m_pHealthUI = nullptr;

	ComboUI* m_pComboUI = nullptr;

	ComboSliderUI* m_pComboSlider = nullptr;

	ComboIcon* m_pComboIcon = nullptr;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

protected:
	


private:

	uint32_t uiPage = 0;
	float m_fHealth = 5.0f;
	float m_fMaxHealth = 5.0f;
	float m_fBulletReturnSpeed = 320.0f;

	bool m_bIsPlaying = false;

	InputHandler* m_pInputHandler = nullptr;

	// <!----------------------------- NEW GAME ELEMENTS ------------------------->
	LoadOut* m_LoadOut = nullptr;

	/**
	 * @brief - defines the players in the scene
	 */
	std::array<Player*, 2> m_pPlayers = { nullptr, nullptr };

	/**
	 * @brief define the start for the players
	 */
	std::array<Entity*, 2> m_vArrEndEntities = { nullptr, nullptr };

	EGameState m_EGameState = EGameState::Start;
};
