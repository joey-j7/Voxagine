#pragma once
#include "Monster.h"

class SpiderMonster : public Monster
{
public:
	SpiderMonster(World* world);
	void Awake() override;
	void Start() override;
	void Tick(float fDeltaTime) override;

	RTTR_ENABLE(Monster)
	RTTR_REGISTRATION_FRIEND;
};