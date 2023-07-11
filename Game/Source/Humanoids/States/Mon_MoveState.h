#pragma once
#include "AI/States/FSMState.h"
#include "Humanoids/Enemies/Monster.h"

class Mon_MoveState : public FSMState<Monster>
{
public:
	void Start(Monster* pOwner) override;

	void Tick(Monster* pOwner, float fDeltaTime) override;
	void FixedTick(Monster* pOwner, const GameTimer& gameTimer) override;
};