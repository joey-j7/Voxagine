#pragma once
#include "Monster.h"

class RandomMonster : public Monster
{
public:
	RandomMonster(World* world) : Monster(world) {};
	void Start() override;

	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};