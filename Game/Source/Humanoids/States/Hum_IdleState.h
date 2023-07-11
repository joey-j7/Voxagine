#pragma once

#include "AI/States/FSMState.h"
#include "Humanoids/Players/Player.h"

class Hum_IdleState : public FSMState<Player>
{
public:
	void Start(Player* pOwner) override;
	void Tick(Player* pOwner, float fDeltaTime) override;
};