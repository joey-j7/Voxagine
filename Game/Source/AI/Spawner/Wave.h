#pragma once

#include <Core/Objects/TSubclass.h>

#include <External/rttr/type>

class Spawner;

/*!
 * Wave - is a struct where you can add wave to the
 * spawner class. The wave is in control of the way enemies are spawned.
 * The spawner class just uses the information of the wave to spawn the enemies.
 * 
 * Specifications
 * - Selectable enemy. Only one per spawn point
 * - Capacity of the enemy.
 * - Spawn trigger position.
 */
class Wave : public VClass
{
	RTTR_ENABLE(VClass);
	RTTR_REGISTRATION_FRIEND;
public:
	// Default constructor
	/* Wave() : Wave(std::vector<SpawnProperties>{}) {} */

	virtual ~Wave() = default;
	Wave& operator=(const Wave& rhWave)
	{
		Wave& Wave = *this;
		Wave.bBurst = rhWave.bBurst;
		Wave.fNextSpawnTimer = rhWave.fNextSpawnTimer;
		Wave.m_fRadius = rhWave.m_fRadius;
		Wave.m_bIstStarted = rhWave.m_bIstStarted;
		Wave.m_bIsAlive = rhWave.m_bIsAlive;
		Wave.m_iSizeOfEnemies = rhWave.m_iSizeOfEnemies;
		// Wave.m_lSpawnProperties = rhWave.m_lSpawnProperties;

		return *this;
	}

	/*!
	 * @brief - Has properties store about how we should spawn the monsters
	 */
	/*struct SpawnProperties
	{
		/*!
		 * @brief - what kind of monster do you want to spawn
		 *
		EMonsterType eMonsterType = static_cast<EMonsterType>(iMonsterType{ 0 });
		std::vector<TSubclass<Monster> m_MonsterClasses;

		/*! 
		 *@brief How fast should the next one spawn 
		*
		float fNextSpawnTimer = 2.5f;

		/*!
		 * @brief - Should we spawn the entities
		 * in a random sphere or at location
		 *
		bool bRandomLocation = true;

		/*!
		 * @brief custom location.
		 * This position is relative to the spawner owner.
		 *
		Vector3 m_vCustomLocation = Vector3();
	};*/

	/*!
	 * @brief Wave constructor
	 *
	 * @param lSpawnProperties - The struct that has information about the spawn entity
	 * @param bBurst - if we should spawn all enemies at once
	 * @param fRadius - How big should the radius be to spawn enemies in. TODO later different kind of things
	 * @param fNextSpawnTimer - When should we instantiate the next wave
	 */
	Wave(/*std::vector<SpawnProperties>&& lSpawnProperties,*/ bool bBurst = true, float fRadius = 5.0f, float fNextSpawnTimer = 0.0f);

	/*!
	 * @brief - Whether the wave is started or not.
	 *
	 * @return bool
	 */
	bool HasStarted() const { return m_bIstStarted; }

	void IsStarted(bool bIstStarted) { m_bIstStarted = bIstStarted; }

	/*!
	 * @brief - Whether the wave is alive.
	 *
	 * @return bool
	 */
	bool IsAlive() const { return m_bIsAlive; }

	/*!
	 * @brief - Returns the size of the list
	 *
	 * @return int
	 */
	uint32_t GetSize() const { return m_iSizeOfEnemies; }

	/*!
	 * @brief - Get the SpawnProperties
	 *
	 * @return SpawnProperties&
	 */
	// SpawnProperties& Get(size_t location) { return m_lSpawnProperties.at(location); }

	/**
	 * @brief - Set the parent
	 */
	void SetParent(Spawner* pSpawnerComp) { m_pSpawnerComp = pSpawnerComp; }

	void Clear();

	/*!
	 * @brief Burst that mean all enemy at ones
	*/
	bool bBurst = true;

	/*!
	 * @brief How many second before we can spawn
	 * the next set enemy
	*/
	float fNextSpawnTimer = 2.5f;

	/*!
	 * @brief radius in where the enemies
	 * should spawn.
	 */
	float m_fRadius = 5.0f;

	/*
	 * @brief the minimal force when the monster dies
	 */
	Vector3 v3MinForce = Vector3(1.0f);

	/*!
	 * @brief the maximal force when the monster dies
	 */
	Vector3 v3MaxForce = Vector3(2.0f);

private:
	/*!
	 * @brief Has started
	*/
	bool m_bIstStarted = false;

	/*!
	 * @brief this is to remove the wave
	 * when the it done with spawning
	 */
	bool m_bIsAlive = true;

	// TODO change to generic name
	uint32_t m_iSizeOfEnemies = 0;

	// std::vector<SpawnProperties> m_lSpawnProperties;
	Spawner* m_pSpawnerComp = nullptr;
};
