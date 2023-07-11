#pragma once

#include "Core/ECS/Components/BehaviorScript.h"
#include "Core/Objects/TSubclass.h"
#include "Humanoids/Enemies/Monster.h"

namespace pathfinding
{
	class ContinuumCrowdsGroup;
}

/**
 *
 */
enum class SpawnerType
{
	ST_Normal,
	ST_Gauntlet
};

/**
 * @brief Spawner - This class is intended to show the classes inside the editor
 *
 * ------------------------------------------------------------------
 * \code{.cpp}
 * // Create an entity in the scene and add the Spawner component
 * Spawner* spawner = AddComponent<Spawner>();
 * 
 * spawner->fradius = 50.0f // Change the radius
 * spawner->StartWave(); // Start the monster spawning
 *
 * \endcode
 *
 *
 * -------------------------------------------------------------------
 *
 */
class Spawner : public BehaviorScript
{
public:
	Spawner(Entity* pEntity) : BehaviorScript(pEntity) {}
	virtual ~Spawner();

	/**
	 * @brief - To see if the spawner is still alive
	*/
	bool IsAlive() const { return m_bIsAlive; }

	bool HasStarted() const { return m_bIsStarted; }
	bool ShouldSpawning() const;
	void StartWave();

	void Start() override;
	void FixedTick(const GameTimer&) override;

	/**
	 * @brief - This will notify the manager that he is not part
	 * of the group
	 */
	Event<Spawner*> OnSpawnerDisabled;

	Vector3 v3MaxForce = Vector3(10.0f);

#pragma region Wave_Settings
	void Damage(float fDamage, Vector3 v3ImpactNormal);

	bool GetInvisible() const { return m_bIsInvisible; }
	void SetInvisible(bool bVisible);

	float GetRespawnTimeMin() const { return m_fRespawnTimeMin; }
	void SetRespawnTimeMin(float fTime) { m_fRespawnTimeMin = std::max(0.f, std::min(m_fRespawnTimeMax, fTime)); }

	float GetRespawnTimeMax() const { return m_fRespawnTimeMax; }
	void SetRespawnTimeMax(float fTime) { m_fRespawnTimeMax = std::max(m_fRespawnTimeMin, fTime); }

	/**
	 * @brief radius in where the enemies
	 * should spawn.
	*/
	float fRadius = 25.0f;
private:
	void Dead();
	void Shatter() const;

	Monster* SpawnEnemyAtRandomLocation() const;
	// Monster* SpawnEnemyAtLocation(const Vector3& v3Location);

	void SpawnEnemies();

	Vector3 m_InitialScale = Vector3(1.f, 1.f, 1.f);
	float m_fScaleTimer = 0.f;

	bool m_bAutoStart = false;

	bool m_bIsDestroyed = false;
	bool m_bRespawnable = false;

	float m_fRespawnTimeMin = 5.f;
	float m_fRespawnTimeMax = 5.f;

	float m_fRespawnTimer = 0.f;
	float m_fRespawnTime = 0.f;

	/**css
	 * @brief - The type of the monster we need to spawn.
	 */
	TSubclass<Monster> m_MonsterClass;

	VoxRenderer* m_pVoxRenderer = nullptr;

	/**
	 * From what part of the group is the entity part of.
	 */
	pathfinding::ContinuumCrowdsGroup* m_Group = nullptr;

	/**
	 * @brief - Amount of enemies the spawner
	 * should spawn.
	*/
	uint32_t m_iAmountEnemies = 1;

	/**
	 * @brief - Amount of enemies in the scene before
	 * the spawner should spawn new ones.
	*/
	uint32_t m_iAmountEnemiesAlive = 1;

	/**
	 * @brief - The amount of times the spawner should
	 * spawn entities
	 */
	uint32_t m_iAmountWaves = 1;

	/**
	 * @brief - Current counter of times we spawned
	 * something.
	 */
	uint32_t m_iCurrentCounter = 0;

	/**
	 * @brief How many second before we can spawn
	 * the next set enemy
	*/
	float fNextSpawnTimer = 2.5f;
#pragma endregion

#pragma region Spawner_Settings
	/**
	 * @brief - To see if the spawners are alive.
	 */
	bool m_bIsAlive = true;

	/**
	 * @brief - In case the spawner needs to be alive for forever.
	 */
	bool m_bImmortal = false;

	/**
	 * @brief - To see if the spawner has started already.
	 */
	bool m_bIsStarted = false;

	/**
	 * @brief - First state setting the spawners to invisible
	 */
	bool m_bIsInvisible = true;

	/**
	 * @brief - Check if we can place monsters in the world.
	 */
	bool m_bSpawnEnemies = false;

	/**
	 * @brief - All at once
	 */
	bool m_bBurst = true;

	/**
	 * @brief - Next spawn timer
	 * and the reset time
	*/
	float m_fSpawnTimer = 0.0f;

	/**
	 * @brief - Reset spawn timer
	 */
	float m_fResetSpawnTimer = 1.0f;

	/**
	 * @brief - Max health of the spawner
	 */
	float m_fMaxHealth = 1.0f;

	/**
	 * @brief Health of the spawner
	 */
	float m_fHealth = m_fMaxHealth;

	/**
	 * @brief Portal entity
	*/
	Entity* pPortalEntity = nullptr;

	/**
	 * @brief - Component for flashing
	 */
	FlashBehavior* m_pFlashBehavior = nullptr;

	Vector3 m_v3ImpactNormal = Vector3(0.0f);

	/**
	 * @brief type that defines the portal behavior
	 */
	SpawnerType m_eSpawnerType = SpawnerType::ST_Normal;

	/**
	 * @brief - The spawned entities of the wave.
	 */
	std::vector<Monster*> m_vSpawnedEnemies = {};
#pragma endregion 

	RTTR_ENABLE(BehaviorScript);
	RTTR_REGISTRATION_FRIEND;
};
