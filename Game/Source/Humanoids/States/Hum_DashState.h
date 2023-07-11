#pragma once

#include "AI/States/FSMState.h"
#include "Humanoids/Players/Player.h"

class Hum_DashState : public FSMState<Player>
{
public:
	void Start(Player* pOwner) override;

	void Tick(Player* pOwner, float fDeltaTime) override;
	void FixedTick(Player* pOwner, const GameTimer& gameTimer) override;
	void Exit(Player* pOwner) override;

private:
	Vector3 m_v3Forward = Vector3(0.f, 0.f, 0.f);
	float m_fTimer = 0.f;
};