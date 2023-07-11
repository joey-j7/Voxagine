#pragma once
#include "Core/ECS/Entity.h"
#include "Core/Math.h"

class PhysicsBody;
class Player;
class AimPrefab;

class Projectile :
	public Entity
{
public:
	Projectile(World* world); 

	void Start() override;
	void Tick(float fDeltaTime) override;


	RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND

private:
	Entity* m_pOwner = nullptr;
	PhysicsBody* m_pBody = nullptr;
	Player* m_pPlayer = nullptr;

	// How fast the bullet travels
	float m_fSpeed = 200.0f;

	float m_fDetonationRadius = 50.0f;
	float m_fDetonationForce = 50.0f;

	float m_fLife = 0.0f;
	float m_fDestroyTimer = 1.f;

	Vector3 m_Direction = Vector3(0.f, 0.f, 1.f);

	/*!
	* @brief force applied on the bomb
	*/
	float m_fForce = 100.0f;
};

