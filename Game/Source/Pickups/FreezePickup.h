#pragma once
#include "GenericPickup.h"

class FreezePickup : public GenericPickup
{
public:
	FreezePickup(World* world);

	void Start() override;

	void OnFreezePicked(Collider* pCollider);
	
	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
};
