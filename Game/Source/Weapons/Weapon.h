#pragma once

#include "Core/ECS/Component.h"
#include "Core/Math.h"

#include "General/Managers/GameManager.h"
#include "General/Managers/WeaponManager.h"

class Player;

class Weapon : public Component {
public:
	Weapon(Entity* pOwner);
	virtual ~Weapon() = default;

	void Start() override;

	uint32_t GetCurrentAmmo() const;
	void SetCurrentAmmo(uint32_t uiAmmo);

	bool HasInfiniteAmmo() const;

	const WeaponManager::Type* GetType() const;
	void ResetAmmo();

	float currentComboTimer = 0.0f;

	// Spawns bullets
	virtual void Fire();

	GameManager* m_pGameManager = nullptr;
	
	RTTR_ENABLE(Component);
	RTTR_REGISTRATION_FRIEND;

protected:
	Player* m_pPlayer = nullptr;

	WeaponManager* m_pManager = nullptr;

	uint32_t m_uiAmmo = 0;
	bool m_bUnlimitedAmmo = false;

	Vector3 m_SpawnOffset = Vector3(0.f, 0.f, 0.f);
};