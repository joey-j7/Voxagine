#pragma once
#include "GenericPickup.h"

class BombPickup : public GenericPickup
{
public:
	BombPickup(World* world);

	void OnBombPickedUp(Collider* pCollider);

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
};
