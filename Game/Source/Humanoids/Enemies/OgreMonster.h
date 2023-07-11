#pragma once
#include "Monster.h"

class OgreMonster : public Monster
{
public:
	float m_fJumpHeight;

public:
	OgreMonster(World* world);
	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;
	void MeleeAttack(Vector3& velocity) override;

	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};