#pragma once
#include "GenericPickup.h"

class TriWeaponPickup : public GenericPickup
{
public:
	TriWeaponPickup(World* world);

	void Start() override;

	void OnTriWeaponPickedUp(Collider* pCollider);

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
};