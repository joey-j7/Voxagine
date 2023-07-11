#pragma once
#include "GenericPickup.h"

class Ammo : public GenericPickup
{
public:
	Ammo(World* world);

	void Start() override;

	void OnAmmoPicked(Collider* pCollider);

	uint32_t uiAmountAmmo = 50;
};
