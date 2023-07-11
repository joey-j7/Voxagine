#pragma once

#include "Core/Math.h"
#include "Core/ECS/Entity.h"

class BoundingWall;
class Spawner;
class BoxCollider;
class SpawnerManager : public Entity
{
public:
	SpawnerManager(World* pWorld) : Entity(pWorld) { }

	Vector3 GetBoxSize() const { return v3InitializeBoxSize; }
	void SetBoxSize(Vector3 v3BoxSize);

	void Awake() override;
	void Start() override;
	void FixedTick(const ::GameTimer&) override;

	void OnCollisionEnter(Collider*, const Manifold&) override;
	void OnCollisionStay(Collider*, const Manifold&) override;
	void OnCollisionExit(Collider*, const Manifold&) override;
	
	Event<> OnSpawningCompleted;

protected:
	/**
	 * @brief - Collider which the player can collide to
	 * to activate all the linked spawners.
	 */
	BoxCollider* m_pCollider = nullptr;
	/**
	 * @brief - Current Players inside the trigger box.
	*/
	Entity* m_pPlayer1 = nullptr, * m_pPlayer2 = nullptr;

	/**
	 * @brief Linked Spawners in the scene
	 */
	std::vector<Spawner*> m_vSpawnerEntities = {};

	/**
	 * @brief - Initial size of the trigger box
	*/
	Vector3 v3InitializeBoxSize = Vector3(50.0f);

	/**
	 * @brief - Bounding wall that needs to block the player
	 * from progressing
	 */
	std::vector<BoundingWall*> m_BoundWalls = {};

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};
