#pragma once

#include "GenericPickup.h"

class WeaponManager;

class WeaponPickup : public GenericPickup
{
public:
	WeaponPickup(World* world);

	void Start() override;

	void OnPickUp(Collider* pCollider);

	size_t GetWeaponTypeID() const { return m_WeaponID; };
	void SetWeaponTypeID(size_t index);

private:
	WeaponManager* m_pWeaponManager = nullptr;
	size_t m_WeaponID = 0;

	RTTR_ENABLE(GenericPickup)
	RTTR_REGISTRATION_FRIEND
};