#pragma once
#include "AI/States/FSMState.h"
#include "Humanoids/Enemies/Monster.h"

class Mon_RangeAttackState : public FSMState<Monster>
{
private:
	float m_fTimer;
	bool m_bHasShot;

public:
	void Start(Monster* pOwner) override;
	void Tick(Monster* pOwner, float fDeltaTime) override;
	void Exit(Monster* pOwner) override;
};