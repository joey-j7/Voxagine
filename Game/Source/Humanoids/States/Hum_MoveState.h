#pragma once

#include "AI/States/FSMState.h"
#include "Humanoids/Players/Player.h"

class Hum_MoveState : public FSMState<Player>
{
public:
	void Start(Player* pOwner) override;

	void Tick(Player* pOwner, float fDeltaTime) override;
	void FixedTick(Player* pOwner, const GameTimer& gameTimer) override;
};