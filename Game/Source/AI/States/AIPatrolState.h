#pragma once
#include "FSMState.h"
#include "../../Humanoids/Monster.h"

/*!
 * @brief - This patrol state is made for monster enemies
 * when they are create they won't go after the player immediatly.
 */
class AIPatrolState : public FSMState<Monster> {
public:
	static AIPatrolState* GetInstance();

	virtual ~AIPatrolState() = default;

	void Start(Monster* pMonster) override;
	void Tick(Monster* pMonster, float fDeltatime) override;
	void Exit(Monster* pMonster) override;
private:
	static AIPatrolState* m_sInstance;

	AIPatrolState() = default;

	float m_fMinimalDistance = 40.0f;
};
