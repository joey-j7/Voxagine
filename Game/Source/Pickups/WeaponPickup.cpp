#include "WeaponPickup.h"
#include "Weapons/Weapon.h"

#include "Humanoids/Players/Player.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"

#include <Core/ECS/Components/Collider.h>
#include "Core/ECS/Components/PhysicsBody.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include <Core/MetaData/PropertyTypeMetaData.h>
#include <External/rttr/registration.h>
#include "General/Managers/WeaponManager.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<WeaponPickup>("WeaponPickup")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)

		.property("Weapon Manager", &WeaponPickup::m_pWeaponManager)(RTTR_PUBLIC)
		.property("Weapon Type ID", &WeaponPickup::GetWeaponTypeID, &WeaponPickup::SetWeaponTypeID)(RTTR_PUBLIC)
	;
}

WeaponPickup::WeaponPickup(World* world) : GenericPickup(world)
{
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();

	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();

	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Projectiles/Main_Character_Weapon_Male.vox");
	pRenderer->SetModel(pModel);

	pBody->SetGravity(true);

	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&WeaponPickup::OnPickUp, this, std::placeholders::_1);

	SetName("Weapon Pickup");
}

void WeaponPickup::Start()
{
	GenericPickup::Start();

	if (!m_pWeaponManager)
	{
		const std::vector<WeaponManager*>& pWeaponManagers = GetWorld()->FindEntitiesOfType<WeaponManager>();

		if (!pWeaponManagers.empty())
		{
			m_pWeaponManager = pWeaponManagers.front();
		}
	}
}


void WeaponPickup::OnPickUp(Collider* pCollider)
{
	if (pCollider->GetOwner()->HasTag("Player"))
	{
		if (Player* pPlayer = dynamic_cast<Player*>(pCollider->GetOwner()))
		{
			Weapon* pWeapon = pPlayer->GetCurrentWeapon();
			pWeapon->SetCurrentAmmo(pWeapon->GetCurrentAmmo() + 1);

			m_pWeaponManager->SetTypeID(m_WeaponID);

			Destroy();
		}
	}
}

void WeaponPickup::SetWeaponTypeID(size_t index)
{
	if (!m_pWeaponManager)
		return;

	if(m_pWeaponManager->GetType(index))
		m_WeaponID = index;
}
