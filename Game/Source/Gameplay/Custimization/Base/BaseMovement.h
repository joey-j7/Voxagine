#pragma once

#include "BaseLoadoutBehavior.h"

class BaseMovement : public BaseLoadOutBehavior
{
public:
	BaseMovement(Entity* pOwner) : BaseLoadOutBehavior(pOwner) { }

	virtual void OnSpawn() = 0;
};
