#pragma once
#include "AI/States/FSMState.h"
#include "Humanoids/Enemies/Monster.h"

class Mon_MeleeAttackState : public FSMState<Monster>
{
private:
	float m_fTimer;
	Vector3 m_velocity;
	bool m_bShouldSeek;

public:
	void Start(Monster* pOwner) override;
	void Tick(Monster* pOwner, float fDeltaTime) override;
	void Exit(Monster* pOwner) override;
};