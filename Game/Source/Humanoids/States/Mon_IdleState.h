#pragma once
#include "AI/States/FSMState.h"
#include "Humanoids/Enemies/Monster.h"

class Mon_IdleState : public FSMState<Monster>
{
private:
	float m_fWakeUpTimer;
	bool m_bIsAwake;

public:
	void Start(Monster* pOwner) override;
	void Tick(Monster* pOwner, float fDeltaTime) override;
};