#pragma once

#include "BaseMovement.h"

class BasePassive : public BaseMovement
{
public:
	BasePassive(Entity* pOwner) : BaseMovement(pOwner) { }

	void OnSpawn() override;
};
