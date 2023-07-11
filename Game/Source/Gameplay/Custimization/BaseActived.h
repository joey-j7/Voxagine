#pragma once

#include "BaseLoadoutBehavior.h"

class BaseActivated : public BaseLoadOutBehavior
{
public:
	BaseActivated(Entity* pOwner) : BaseLoadOutBehavior(pOwner) {}

	virtual void OnActivated() = 0;

};
