#pragma once
#include "Monster.h"

class UmbrellaMonster : public Monster
{
private:
	Vector2 m_fJumpForce;
	Vector3 m_velocity;
	Vector2 m_fMaxJumpVelocity;

public:
	UmbrellaMonster(World* world);
	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;

	void ApplyDefaultValues() override;
	void MeleeAttack(Vector3& velocity) override;
	
	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};