#pragma once
#include "GenericPickup.h"

class Duplicator : public GenericPickup
{
public:
	Duplicator(World* world);

	void Start() override;

	void OnDuplicatorPicked(Collider* pCollider);

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
private:
	float m_fRadius = 35.0f;
};
