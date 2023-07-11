#pragma once

#include "AI/States/FSMState.h"
#include "Humanoids/Players/Player.h"

class Hum_ThrowState : public FSMState<Player>
{
public:
	Hum_ThrowState();
	void Start(Player* pOwner) override;
	void Tick(Player* pOwner, float fDeltaTime) override;
	void FixedTick(Player* pOwner, const GameTimer& gameTimer) override;
	float m_fTimer;
};
