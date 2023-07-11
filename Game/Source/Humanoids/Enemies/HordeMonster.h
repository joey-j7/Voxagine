#pragma once
#include "Monster.h"

class HordeMonster : public Monster
{
public:
	HordeMonster(World* world);
	void Awake() override;

	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};