#pragma once

#include "Humanoids/Enemies/Monster.h"

// Gameplay
#include "Wave.h"

class BoxCollider;
class AudioSource;
class BoundingWall;
class SpawnerEntity;
class PortalPrefab;

class SpawnerEntity : public Entity
{
public:
	SpawnerEntity(World* world);
	~SpawnerEntity();

	Spawner* GetSpawnerComponent() const { return m_pSpawnerComponent; }

	Vector3 GetSize() const { return v3InitializeBoxSize; }
	void SetSize(Vector3 v3Size);

	void SetManager(bool bIsManager);
	bool IsManager() const { return m_bIsManager; }

	bool ShouldWait() const { return m_bShouldWait; }
	bool HasMaster() const { return m_pSpawnerMaster != nullptr; }

	void AddSpawnerEntity(SpawnerEntity* pEntity);

	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;
	void PostTick(float fDeltaTime) override;
	void OnDrawGizmos(float fDeltaTime) override;
	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;

	// Remind the spawner to start again with the new wave
	Event<SpawnerEntity*> WaveEnded;
	// The other spawners should call this functions to tell the master that it is done spawning
	Event<> SpawningWavesFinished;

	/*!
	 * @brief Portal entity
	 */
	PortalPrefab* pPortalEntity = nullptr;

private:

	void OnWaveEnded(SpawnerEntity* pSpawner);
	void OnSpawningWavesFinished();

	/**
	 * @brief - Initial size of the box
	 */
	Vector3 v3InitializeBoxSize = Vector3(50.0f);

	Entity* m_pPlayer1 = nullptr, * m_pPlayer2 = nullptr;
	BoxCollider* m_pCollider = nullptr;
	Spawner* m_pSpawnerComponent = nullptr;
	AudioSource* m_pAudio = nullptr;

	// We make one spawner entity / component the manager of all spawners
	// we make events for when all waves are done. so then we can check if those are equal to the amount of spawners including the master.
	// So then the bounding is only needed for master spawner.
	SpawnerEntity* m_pSpawnerMaster = nullptr;

	uint32_t uiWaveCounter = 0;
	uint32_t uiCurrentCounter = 0;

	std::vector<SpawnerEntity*> m_pSpawners = {};
	std::vector<SpawnerEntity*> m_pFinishedWaves = {};
	std::vector<BoundingWall*> m_BoundWalls = {};

	bool m_bTriggerMusic = false;
	bool m_bIsManager = false;
	bool m_bShouldWait = true;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};

