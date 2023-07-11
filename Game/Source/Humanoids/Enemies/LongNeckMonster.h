#pragma once
#include "Monster.h"

class LongNeckMonster : public Monster
{
public:
	LongNeckMonster(World* world);
	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;

	void ApplyDefaultValues() override;
	void MeleeAttack(Vector3& velocity) override;

	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};