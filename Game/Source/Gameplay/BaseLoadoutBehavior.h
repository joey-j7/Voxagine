#pragma once

#include "Core/ECS/Components/BehaviorScript.h"

enum class ELoadOutType
{
	EMovement,
	ERelic,
	EPassive,
	EActivated
};


class Entity;
class BaseLoadOutBehavior : public BehaviorScript
{
public:
	BaseLoadOutBehavior(Entity* pOwner);

protected:
	ELoadOutType EType = ELoadOutType::EMovement;
	bool m_bUnlocked = false;
	bool m_bActivated = false;
	float m_fResourceAmount = 10.0f;
};
