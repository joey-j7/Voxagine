#pragma once

#include "Core/ECS/Entity.h"

// For debugging purposes, moves a monster with a fixed velocity
class AutoMoveMonster : public Entity
{
public:
	AutoMoveMonster(World* world);
	void Awake() override;

	void Start() override;
	void FixedTick(const GameTimer& gameTimer) override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

private:
	Entity* m_pPlayer = nullptr;
};