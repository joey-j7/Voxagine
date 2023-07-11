#pragma once
#include "Monster.h"

class RangeMonster : public Monster
{
public:
	float m_fRange;

private:
	bool m_hasLineOfSight;
	bool m_prevLineOfSight;
	int m_hasLineOfSightLocks;

public:
	RangeMonster(World* world);
	void Awake() override;
	void Start() override;

	void FixedTick(const GameTimer& gameTimer) override;

	RTTR_ENABLE(Monster)
		RTTR_REGISTRATION_FRIEND;
private:
	bool calculateLineOfSight(IVector3 worldPos, IVector3 goalPos);
};