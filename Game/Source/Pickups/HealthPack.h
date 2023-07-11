#pragma once
#include "GenericPickup.h"

class HealthPack : public GenericPickup {
public:
	HealthPack(World* world);

	void SetAmountHealth(float fHealth) { m_fRestoredHealth = fHealth; }

	void Start() override;

	void OnHealthPickup(Collider* pCollider);

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
private:
	float m_fRestoredHealth = 100.0f;
};
