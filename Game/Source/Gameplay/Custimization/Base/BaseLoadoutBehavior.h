#pragma once
#include "Core/ECS/Components/BehaviorScript.h"

class BaseLoadOutBehavior : public BehaviorScript
{
public:
	BaseLoadOutBehavior(Entity* pEntity) : BehaviorScript(pEntity) {}

protected:
	float m_fResourceAmount = 10.0f;
	bool m_bUnlocked = false;
	bool m_bActivated = false;
};
