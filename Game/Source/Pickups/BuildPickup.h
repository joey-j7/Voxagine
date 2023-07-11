#pragma once
#include "GenericPickup.h"

class BuildPickup : public GenericPickup
{
public:
	BuildPickup(World* world);

	void OnBuildPickedUp(Collider* pCollider);

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
};
