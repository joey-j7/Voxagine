#pragma once

#include "FSMState.h"
#include "../../Humanoids/Monster.h"

class AIAttackState : public FSMState<Monster> {
public:
	static AIAttackState* GetInstance();

	virtual ~AIAttackState() = default;

	void Start(Monster* pOwner) override;
	void Tick(Monster* pOwner, float fDeltatime) override;
	void Exit(Monster* pOwner) override;

private:
	static AIAttackState* m_sInstance;

	AIAttackState() = default;

	float m_fMinimalDistance = 80.0f;
};

